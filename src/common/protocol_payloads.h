#ifndef PROTOCOL_PAYLOADS_H
#define PROTOCOL_PAYLOADS_H

#include <stdint.h>


// Auth
typedef struct __attribute__((packed)) {
    char username[50];
    char password[50];  // Send hashed in practice!
} LoginReq;  // Also usable for RegisterReq

typedef struct __attribute__((packed)) {
    int32_t status;
    char message[100];
    uint32_t user_id;
    char session_token[64];  // For session persistence and reconnects
} LoginRes;  // Also for RegisterRes

typedef struct __attribute__((packed)) {
    char session_token[64];  // To validate logout
} LogoutReq;

typedef struct __attribute__((packed)) {
    int32_t status;
    char message[100];
} LogoutRes;

// Account Management
typedef struct __attribute__((packed)) {
    int64_t amount;  // in VND
} DepositReq;  // Also for RedeemReq

typedef struct __attribute__((packed)) {
    int32_t status;
    char message[100];
    int64_t new_balance;  // Updated balance after transaction
} DepositRes;  // Also for RedeemRes

typedef struct __attribute__((packed)) {
    // Empty, or add filters like date range if needed
} ViewHistoryReq;

typedef struct __attribute__((packed)) {
    int32_t status;
    char message[100];
    uint16_t count;  // Number of history entries following
    // Followed by count * HistoryEntry in payload
} ViewHistoryRes;

// Outside-Room Actions
typedef struct __attribute__((packed)) {
    char name[100];
    char description[256];
} CreateRoomReq;

typedef struct __attribute__((packed)) {
    int32_t status;
    char message[100];
    uint32_t room_id;
} CreateRoomRes;

typedef struct __attribute__((packed)) {
    char query[100];  // Optional search filter, empty for all
} ListRoomsReq;

typedef struct __attribute__((packed)) {
    int32_t status;
    char message[100];
    uint16_t count;  // Number of rooms following
    // Followed by count * RoomInfo
} ListRoomsRes;

typedef struct __attribute__((packed)) {
    char query[100];
} SearchItemReq;

typedef struct __attribute__((packed)) {
    int32_t status;
    char message[100];
    uint16_t count;  // Number of items
    // Followed by count * ItemInfo
} SearchItemRes;

typedef struct __attribute__((packed)) {
    uint32_t room_id;
} JoinRoomReq;  // Also for LeaveRoomReq

typedef struct __attribute__((packed)) {
    int32_t status;
    char message[100];
} JoinRoomRes;  // Also for LeaveRoomRes

// In-Room Actions
typedef struct __attribute__((packed)) {
    // Empty if room implicit, or uint32_t room_id
} ViewItemsReq;

typedef struct __attribute__((packed)) {
    int32_t status;
    char message[100];
    uint16_t count;
    // Followed by count * ItemInfo
} ViewItemsRes;

typedef struct __attribute__((packed)) {
    uint32_t item_id;
    int64_t bid_amount; // in VND
} BidReq;

typedef struct __attribute__((packed)) {
    int32_t status;
    char message[100];
} BidRes;

typedef struct __attribute__((packed)) {
    uint32_t item_id;
    int64_t new_price;  // In VND
    uint32_t winner_id;
    char winner_name[50];
} BidNotify;

typedef struct __attribute__((packed)) {
    uint32_t item_id;
} BuyNowReq;

typedef struct __attribute__((packed)) {
    int32_t status;
    char message[100];
} BuyNowRes;

typedef struct __attribute__((packed)) {
    char text[256];
} ChatReq;

typedef struct __attribute__((packed)) {
    uint32_t sender_id;
    char sender_name[50];
    char text[256];
} ChatNotify;

typedef struct __attribute__((packed)) {
    char name[100];
    char description[256];
    int64_t start_price;
    int64_t buy_now_price;  // 0 if no buy now
    uint32_t duration_sec;  // Auction duration
} CreateItemReq;

typedef struct __attribute__((packed)) {
    int32_t status;
    char message[100];
    uint32_t item_id;
} CreateItemRes;

typedef struct __attribute__((packed)) {
    uint32_t item_id;
} DeleteItemReq;

typedef struct __attribute__((packed)) {
    int32_t status;
    char message[100];
} DeleteItemRes;

// Auction Events
typedef struct __attribute__((packed)) {
    uint32_t item_id;
    uint32_t remaining_sec;
} TimerUpdate;

typedef struct __attribute__((packed)) {
    uint32_t item_id;
    uint32_t winner_id;
    char winner_name[50];
    int64_t final_price;
} ItemSold;

#endif