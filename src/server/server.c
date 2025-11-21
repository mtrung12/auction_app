#include "server.h"
#include "handlers/logic_handler.h"
#include "session_mgr.h"
#include "../common/protocol.h"
#include "../common/reliability.h"
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <stddef.h>

#define BACKLOG 10

static int server_fd;

void* client_thread(void* arg) {
    int client_fd = (intptr_t)arg;
    ClientSession* client = session_add_client(client_fd);
    if (!client) {
        close(client_fd);
        return NULL;
    }

    Message msg;
    while (1) {
        memset(&msg, 0, sizeof(msg));
        ssize_t n = recv(client_fd, &msg, sizeof(MessageHeader), 0);
        if (n <= 0) {
            fprintf(stdout, "Client disconnected (fd=%d)\n", client_fd);
            break;
        }

        // Handle payload if exists
        uint32_t payload_len = ntohl(msg.header.payload_length);
        if (payload_len > 0 && payload_len <= BUFF_SIZE) {
            ssize_t pn = recv(client_fd, msg.payload, payload_len, MSG_WAITALL);
            if (pn != payload_len) {
                fprintf(stderr, "Incomplete payload received\n");
                break;
            }
        }

        // If client requires ACK, send one back
        if (is_flag_set(msg.header.flags, FLAG_REQUIRES_ACK)) {
            Message ack_msg = {0};
            create_ack_message(&ack_msg, msg.header.request_id, msg.header.type);
            send(client_fd, &ack_msg, sizeof(MessageHeader), 0);
        }

        // Process the message unless it's an ACK
        if (!is_flag_set(msg.header.flags, FLAG_IS_ACK)) {
            handle_client_message(client, &msg);
        }
    }

    session_remove_client(client_fd);
    close(client_fd);
    return NULL;
}

bool server_start(uint16_t port) {
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = INADDR_ANY
    };

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) return false;

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(server_fd);
        return false;
    }

    if (listen(server_fd, BACKLOG) < 0) {
        close(server_fd);
        return false;
    }

    printf("Server listening on port %d\n", port);

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) continue;

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, (void*)(intptr_t)client_fd);
        pthread_detach(tid);
    }
}