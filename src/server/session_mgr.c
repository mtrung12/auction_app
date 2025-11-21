// src/server/session_mgr.c
#include "session_mgr.h"
#include "../common/protocol.h"
#include "../common/utils.h"  // for logging if you have it
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/uio.h>    // for writev / struct iovec
#include <time.h>       // for time()
#include <stdint.h>     // for uint64_t

#define MAX_CLIENTS 1024
#define MAX_ROOMS   1000   // Reasonable upper limit for room_id

typedef struct {
    ClientSession* clients[MAX_CLIENTS];
    int client_count;
    pthread_mutex_t lock;
} SessionManager;

static SessionManager g_sessions = {0};

static void send_to_client(ClientSession* client, const Message* msg_host_order);

/* portable 64-bit host->network helper */
static inline uint64_t host_to_net64(uint64_t v)
{
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    return __builtin_bswap64(v);
#elif defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    return v;
#else
    // fallback: try endian macros from endian.h if available
    #ifdef __BYTE_ORDER
        #if __BYTE_ORDER == __LITTLE_ENDIAN
            return __builtin_bswap64(v);
        #else
            return v;
        #endif
    #else
        return v; // best effort
    #endif
#endif
}

/* nolock variant to avoid deadlocks when caller already holds g_sessions.lock */
static bool session_leave_room_nolock(ClientSession* client)
{
    if (!client || client->current_room_id == -1) return false;
    client->current_room_id = -1;
    client->state = STATE_IN_LOBBY;
    return true;
}

/* -------------------------------------------------------------------------- */
/*                          Public API Implementation                         */
/* -------------------------------------------------------------------------- */

bool session_init(void)
{
    memset(g_sessions.clients, 0, sizeof(g_sessions.clients));
    g_sessions.client_count = 0;
    if (pthread_mutex_init(&g_sessions.lock, NULL) != 0) {
        return false;
    }
    return true;
}

void session_cleanup(void)
{
    pthread_mutex_lock(&g_sessions.lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_sessions.clients[i]) {
            close(g_sessions.clients[i]->sockfd);
            free(g_sessions.clients[i]);
            g_sessions.clients[i] = NULL;
        }
    }
    g_sessions.client_count = 0;
    pthread_mutex_unlock(&g_sessions.lock);
    pthread_mutex_destroy(&g_sessions.lock);
}

ClientSession* session_add_client(int sockfd)
{
    ClientSession* client = calloc(1, sizeof(ClientSession));
    if (!client) return NULL;

    client->sockfd = sockfd;
    client->user_id = 0;
    client->current_room_id = -1;
    client->state = STATE_DISCONNECTED;
    client->last_heartbeat = time(NULL);

    pthread_mutex_lock(&g_sessions.lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_sessions.clients[i] == NULL) {
            g_sessions.clients[i] = client;
            g_sessions.client_count++;
            pthread_mutex_unlock(&g_sessions.lock);
            return client;
        }
    }
    pthread_mutex_unlock(&g_sessions.lock);

    free(client);
    return NULL;  // full
}

void session_remove_client(int sockfd)
{
    pthread_mutex_lock(&g_sessions.lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        ClientSession* c = g_sessions.clients[i];
        if (c && c->sockfd == sockfd) {
            /* already hold lock -> call nolock variant to avoid double-lock */
            session_leave_room_nolock(c);  // clean room state without re-locking
            close(c->sockfd);
            free(c);
            g_sessions.clients[i] = NULL;
            g_sessions.client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&g_sessions.lock);
}

ClientSession* session_get_by_fd(int sockfd)
{
    pthread_mutex_lock(&g_sessions.lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_sessions.clients[i] && g_sessions.clients[i]->sockfd == sockfd) {
            ClientSession* c = g_sessions.clients[i];
            pthread_mutex_unlock(&g_sessions.lock);
            return c;
        }
    }
    pthread_mutex_unlock(&g_sessions.lock);
    return NULL;
}

ClientSession* session_get_by_user_id(uint32_t user_id)
{
    if (user_id == 0) return NULL;

    pthread_mutex_lock(&g_sessions.lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        ClientSession* c = g_sessions.clients[i];
        if (c && c->user_id == user_id) {
            pthread_mutex_unlock(&g_sessions.lock);
            return c;
        }
    }
    pthread_mutex_unlock(&g_sessions.lock);
    return NULL;
}

bool session_join_room(ClientSession* client, int32_t room_id)
{
    if (!client || room_id <= 0) return false;

    pthread_mutex_lock(&g_sessions.lock);
    // Leave current room first if any (use nolock variant since we hold lock)
    if (client->current_room_id != -1) {
        session_leave_room_nolock(client);
    }

    client->current_room_id = room_id;
    client->state = STATE_IN_ROOM;
    pthread_mutex_unlock(&g_sessions.lock);
    return true;
}

bool session_leave_room(ClientSession* client)
{
    if (!client || client->current_room_id == -1) return false;

    pthread_mutex_lock(&g_sessions.lock);
    session_leave_room_nolock(client);
    pthread_mutex_unlock(&g_sessions.lock);
    return true;
}

//Broadcast Helper

static void send_to_client(ClientSession* client, const Message* msg_host)
{
    if (!client || client->sockfd < 0 || !msg_host) return;

    Message net_msg = *msg_host;

    net_msg.header.version    = 1;
    net_msg.header.type       = msg_host->header.type;  // type is uint8_t, no conversion needed
    net_msg.header.flags      = htons(msg_host->header.flags);
    net_msg.header.request_id = htonl(msg_host->header.request_id);
    net_msg.header.timestamp  = host_to_net64((uint64_t)msg_host->header.timestamp);

    /* payload length: guard and convert to network order */
    uint32_t payload_len = msg_host->header.payload_length;
    if (payload_len > sizeof(net_msg.payload)) payload_len = sizeof(net_msg.payload);
    net_msg.header.payload_length = htonl(payload_len);

    if (payload_len > 0) {
        memcpy(net_msg.payload, msg_host->payload, payload_len);
    }

    // Send header + payload in one go if possible
    struct iovec iov[2];
    int iovcnt = 0;

    iov[iovcnt].iov_base = &net_msg.header;
    iov[iovcnt].iov_len  = sizeof(MessageHeader);
    iovcnt++;

    if (payload_len > 0) {
        iov[iovcnt].iov_base = net_msg.payload;
        iov[iovcnt].iov_len  = payload_len;
        iovcnt++;
    }

    // best-effort writev; consider handling partial writes in production
    writev(client->sockfd, iov, iovcnt);
}

void session_broadcast_room(int32_t room_id, const Message* msg_host, ClientSession* exclude)
{
    if (room_id <= 0) return;

    pthread_mutex_lock(&g_sessions.lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        ClientSession* c = g_sessions.clients[i];
        if (c && c->current_room_id == room_id && c != exclude) {
            send_to_client(c, msg_host);
        }
    }
    pthread_mutex_unlock(&g_sessions.lock);
}

// broadcast to everyone (e.g., server announcements) 
void session_broadcast_all(const Message* msg_host, ClientSession* exclude)
{
    pthread_mutex_lock(&g_sessions.lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        ClientSession* c = g_sessions.clients[i];
        if (c && c != exclude) {
            send_to_client(c, msg_host);
        }
    }
    pthread_mutex_unlock(&g_sessions.lock);
}