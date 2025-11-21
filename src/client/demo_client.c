// src/client/demo_client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <inttypes.h>
#include "../common/protocol.h"
#include "../common/reliability.h"

#define PORT 5500
#define SERVER_IP "127.0.0.1"

int sockfd = -1;
uint32_t user_id = 0;
int32_t current_room_id = -1;
uint32_t request_id = 1;
PendingQueue pending_queue = {0};  // Track messages awaiting ACK

void connect_to_server() {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        exit(1);
    }
    printf("Connected to server at %s:%d\n\n", SERVER_IP, PORT);
}

void send_message(MessageType type, const void* payload, uint16_t payload_len) {
    Message msg = {0};
    msg.header.version = 1;
    msg.header.type = (uint8_t)type;
    msg.header.request_id = request_id++;
    msg.header.timestamp = (uint64_t)time(NULL);
    msg.header.payload_length = payload_len;
    msg.header.flags = 0;  // No special flags by default

    if (payload && payload_len > 0) {
        memcpy(msg.payload, payload, payload_len);
    }

    ssize_t n = send(sockfd, &msg, sizeof(MessageHeader) + payload_len, 0);
    if (n < 0) {
        perror("send");
    }
}

// Enhanced send with reliability flags
void send_message_reliable(MessageType type, const void* payload, uint16_t payload_len, 
                          uint16_t flags) {
    Message msg = {0};
    uint32_t current_req_id = request_id++;
    
    msg.header.version = 1;
    msg.header.type = (uint8_t)type;
    msg.header.request_id = current_req_id;
    msg.header.timestamp = (uint64_t)time(NULL);
    msg.header.payload_length = payload_len;
    msg.header.flags = flags;

    if (payload && payload_len > 0) {
        memcpy(msg.payload, payload, payload_len);
    }

    ssize_t n = send(sockfd, &msg, sizeof(MessageHeader) + payload_len, 0);
    if (n < 0) {
        perror("send");
        return;
    }

    // If this message requires ACK, add it to pending queue
    if (is_flag_set(flags, FLAG_REQUIRES_ACK)) {
        char buffer[BUFF_SIZE] = {0};
        memcpy(buffer, &msg.header, sizeof(MessageHeader));
        if (payload && payload_len > 0) {
            memcpy(buffer + sizeof(MessageHeader), payload, payload_len);
        }
        pending_queue_add(&pending_queue, current_req_id, type, buffer, 
                         sizeof(MessageHeader) + payload_len);
    }
}

Message* receive_message() {
    static Message msg;
    memset(&msg, 0, sizeof(msg));

    ssize_t n = recv(sockfd, &msg, sizeof(Message), 0);
    if (n < 0) {
        perror("recv");
        return NULL;
    }
    if (n == 0) {
        printf("Connection closed by server\n");
        return NULL;
    }
    
    // If this message requires an ACK, send one back
    if (is_flag_set(msg.header.flags, FLAG_REQUIRES_ACK)) {
        Message ack_msg = {0};
        create_ack_message(&ack_msg, msg.header.request_id, msg.header.type);
        send(sockfd, &ack_msg, sizeof(MessageHeader), 0);
    }
    
    // If this is an ACK, mark the request as received
    if (is_flag_set(msg.header.flags, FLAG_IS_ACK)) {
        pending_queue_ack(&pending_queue, msg.header.request_id);
        return NULL;  // ACK messages don't get processed further
    }
    
    return &msg;
}

void demo_register() {
    printf("\n=== REGISTER ===\n");
    LoginReq req = {0};
    strncpy(req.username, "testuser", sizeof(req.username) - 1);
    strncpy(req.password, "password123", sizeof(req.password) - 1);

    send_message(MSG_REGISTER_REQ, &req, sizeof(req));

    Message* res_msg = receive_message();
    if (res_msg) {
        LoginRes* res = (LoginRes*)res_msg->payload;
        printf("Status: %d\nMessage: %s\nUser ID: %d\n", res->status, res->message, res->user_id);
    }
}

void demo_login() {
    printf("\n=== LOGIN ===\n");
    LoginReq req = {0};
    strncpy(req.username, "alice", sizeof(req.username) - 1);
    strncpy(req.password, "pass123", sizeof(req.password) - 1);

    send_message(MSG_LOGIN_REQ, &req, sizeof(req));

    Message* res_msg = receive_message();
    if (res_msg) {
        LoginRes* res = (LoginRes*)res_msg->payload;
        printf("Status: %d\nMessage: %s\nUser ID: %d\n", res->status, res->message, res->user_id);
        if (res->status == 1) {
            user_id = res->user_id;
        }
    }
}

void demo_deposit() {
    printf("\n=== DEPOSIT ===\n");
    DepositReq req = {0};
    req.amount = 5000000;  // 5 million VND

    send_message(MSG_DEPOSIT_REQ, &req, sizeof(req));

    Message* res_msg = receive_message();
    if (res_msg) {
        DepositRes* res = (DepositRes*)res_msg->payload;
        printf("Status: %d\nMessage: %s\nNew Balance: %" PRId64 "\n", res->status, res->message, res->new_balance);
    }
}

void demo_create_room() {
    printf("\n=== CREATE ROOM ===\n");
    CreateRoomReq req = {0};
    strncpy(req.name, "Tech Auction #1", sizeof(req.name) - 1);
    strncpy(req.description, "Auction for electronics and gadgets", sizeof(req.description) - 1);

    send_message(MSG_CREATE_ROOM_REQ, &req, sizeof(req));

    Message* res_msg = receive_message();
    if (res_msg) {
        CreateRoomRes* res = (CreateRoomRes*)res_msg->payload;
        printf("Status: %d\nMessage: %s\nRoom ID: %d\n", res->status, res->message, res->room_id);
        if (res->status == 1) {
            current_room_id = res->room_id;
        }
    }
}

void demo_list_rooms() {
    printf("\n=== LIST ROOMS ===\n");
    ListRoomsReq req = {0};
    strncpy(req.query, "", sizeof(req.query) - 1);

    send_message(MSG_LIST_ROOMS_REQ, &req, sizeof(req));

    Message* res_msg = receive_message();
    if (res_msg) {
        ListRoomsRes* header = (ListRoomsRes*)res_msg->payload;
        printf("Status: %d\nMessage: %s\nCount: %d\n", header->status, header->message, header->count);

        if (header->count > 0) {
            RoomInfo* rooms = (RoomInfo*)(res_msg->payload + sizeof(ListRoomsRes));
            for (int i = 0; i < header->count; i++) {
                printf("  Room %d: %s (%s) - %d users\n", rooms[i].room_id, rooms[i].name,
                       rooms[i].description, rooms[i].user_count);
            }
        }
    }
}

void demo_join_room() {
    if (current_room_id <= 0) {
        printf("No room to join. Create or list rooms first.\n");
        return;
    }

    printf("\n=== JOIN ROOM %d ===\n", current_room_id);
    JoinRoomReq req = {0};
    req.room_id = current_room_id;

    send_message(MSG_JOIN_ROOM_REQ, &req, sizeof(req));

    Message* res_msg = receive_message();
    if (res_msg) {
        JoinRoomRes* res = (JoinRoomRes*)res_msg->payload;
        printf("Status: %d\nMessage: %s\n", res->status, res->message);
    }
}

void demo_create_item() {
    if (current_room_id <= 0) {
        printf("Join a room first.\n");
        return;
    }

    printf("\n=== CREATE ITEM ===\n");
    CreateItemReq req = {0};
    strncpy(req.name, "iPhone 14 Pro", sizeof(req.name) - 1);
    strncpy(req.description, "Barely used, all accessories included", sizeof(req.description) - 1);
    req.start_price = 15000000;    // 15 million VND
    req.buy_now_price = 20000000;  // 20 million VND
    req.duration_sec = 300;         // 5 minutes

    send_message(MSG_CREATE_ITEM_REQ, &req, sizeof(req));

    Message* res_msg = receive_message();
    if (res_msg) {
        CreateItemRes* res = (CreateItemRes*)res_msg->payload;
        printf("Status: %d\nMessage: %s\nItem ID: %d\n", res->status, res->message, res->item_id);
    }
}

void demo_view_items() {
    if (current_room_id <= 0) {
        printf("Join a room first.\n");
        return;
    }

    printf("\n=== VIEW ITEMS ===\n");
    ViewItemsReq req = {0};

    send_message(MSG_VIEW_ITEMS_REQ, &req, sizeof(req));

    Message* res_msg = receive_message();
    if (res_msg) {
        ViewItemsRes* header = (ViewItemsRes*)res_msg->payload;
        printf("Status: %d\nMessage: %s\nCount: %d\n", header->status, header->message, header->count);

        if (header->count > 0) {
            ItemInfo* items = (ItemInfo*)(res_msg->payload + sizeof(ViewItemsRes));
            for (int i = 0; i < header->count; i++) {
                printf("  Item %d: %s\n", items[i].item_id, items[i].name);
                printf("    Start: %" PRId64 " | Current: %" PRId64 " | Buy Now: %" PRId64 "\n",
                       items[i].start_price, items[i].current_price, items[i].buy_now_price);
            }
        }
    }
}

void demo_bid() {
    if (current_room_id <= 0) {
        printf("Join a room first.\n");
        return;
    }

    printf("\n=== PLACE BID ===\n");
    printf("Enter item ID: ");
    uint32_t item_id;
    scanf("%u", &item_id);

    printf("Enter bid amount: ");
    int64_t amount;
    scanf("%" PRId64, &amount);

    BidReq req = {0};
    req.item_id = item_id;
    req.bid_amount = amount;

    send_message(MSG_BID_REQ, &req, sizeof(req));

    Message* res_msg = receive_message();
    if (res_msg) {
        BidRes* res = (BidRes*)res_msg->payload;
        printf("Status: %d\nMessage: %s\n", res->status, res->message);
    }
}

void demo_buy_now() {
    if (current_room_id <= 0) {
        printf("Join a room first.\n");
        return;
    }

    printf("\n=== BUY NOW ===\n");
    printf("Enter item ID: ");
    uint32_t item_id;
    scanf("%u", &item_id);

    BuyNowReq req = {0};
    req.item_id = item_id;

    send_message(MSG_BUY_NOW_REQ, &req, sizeof(req));

    Message* res_msg = receive_message();
    if (res_msg) {
        BuyNowRes* res = (BuyNowRes*)res_msg->payload;
        printf("Status: %d\nMessage: %s\n", res->status, res->message);
    }
}

void demo_chat() {
    if (current_room_id <= 0) {
        printf("Join a room first.\n");
        return;
    }

    printf("\n=== SEND CHAT ===\n");
    printf("Enter message: ");
    char text[256];
    fgets(text, sizeof(text), stdin);

    ChatReq req = {0};
    strncpy(req.text, text, sizeof(req.text) - 1);

    send_message(MSG_CHAT_REQ, &req, sizeof(req));
    printf("Message sent\n");
}

void demo_view_history() {
    printf("\n=== VIEW HISTORY ===\n");
    ViewHistoryReq req = {0};

    send_message(MSG_VIEW_HISTORY_REQ, &req, sizeof(req));

    Message* res_msg = receive_message();
    if (res_msg) {
        ViewHistoryRes* header = (ViewHistoryRes*)res_msg->payload;
        printf("Status: %d\nMessage: %s\nCount: %d\n", header->status, header->message, header->count);

        if (header->count > 0) {
            HistoryEntry* entries = (HistoryEntry*)(res_msg->payload + sizeof(ViewHistoryRes));
            for (int i = 0; i < header->count; i++) {
                printf("  %s: %" PRId64 " VND (Won: %s)\n", entries[i].item_name, entries[i].bid_amount,
                       entries[i].won ? "Yes" : "No");
            }
        }
    }
}

void print_menu() {
    printf("\n========== AUCTION DEMO CLIENT ==========\n");
    printf("1. Register\n");
    printf("2. Login (alice/pass123)\n");
    printf("3. Deposit\n");
    printf("4. Create Room\n");
    printf("5. List Rooms\n");
    printf("6. Join Room\n");
    printf("7. Create Item\n");
    printf("8. View Items\n");
    printf("9. Place Bid\n");
    printf("10. Buy Now\n");
    printf("11. Send Chat\n");
    printf("12. View History\n");
    printf("0. Exit\n");
    printf("========================================\n");
    printf("Select option: ");
}

int main() {
    connect_to_server();

    int choice;
    while (1) {
        print_menu();
        scanf("%d", &choice);
        getchar();  // consume newline

        switch (choice) {
            case 1: demo_register(); break;
            case 2: demo_login(); break;
            case 3: demo_deposit(); break;
            case 4: demo_create_room(); break;
            case 5: demo_list_rooms(); break;
            case 6: demo_join_room(); break;
            case 7: demo_create_item(); break;
            case 8: demo_view_items(); break;
            case 9: demo_bid(); break;
            case 10: demo_buy_now(); break;
            case 11: demo_chat(); break;
            case 12: demo_view_history(); break;
            case 0:
                close(sockfd);
                printf("Goodbye!\n");
                return 0;
            default:
                printf("Invalid choice\n");
        }
    }

    return 0;
}
