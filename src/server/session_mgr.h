#ifndef SESSION_MGR_H
#define SESSION_MGR_H

#include "../common/protocol.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    STATE_DISCONNECTED,
    STATE_AUTHENTICATED,
    STATE_IN_LOBBY,
    STATE_IN_ROOM
} ClientState;

typedef struct {
    int sockfd;
    uint32_t user_id;
    char username[50];
    char session_token[64];
    int32_t current_room_id;  // -1 if not in room
    ClientState state;
    uint64_t last_heartbeat;
} ClientSession;

bool session_init(void);
void session_cleanup(void);

ClientSession* session_add_client(int sockfd);
void session_remove_client(int sockfd);
ClientSession* session_get_by_fd(int sockfd);
ClientSession* session_get_by_user_id(uint32_t user_id);

bool session_join_room(ClientSession* client, int32_t room_id);
bool session_leave_room(ClientSession* client);

// Broadcast to all in room (except sender)
void session_broadcast_room(int32_t room_id, const Message* msg, ClientSession* exclude);

// Broadcast to all connected clients (except sender)
void session_broadcast_all(const Message* msg, ClientSession* exclude);

#endif