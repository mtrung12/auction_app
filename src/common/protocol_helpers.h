#ifndef PROTOCOL_HELPERS_H
#define PROTOCOL_HELPERS_H

#include <stdint.h>
#include "protocol_header.h"

// Common response fields (status and message can be included in specific res structs)
typedef struct __attribute__((packed)) {
    int32_t status;     // 1 = Success, 0 = Fail, negative for specific errors (e.g., -1 invalid, -2 insufficient funds)
    char message[100];  // Optional error or info message, null-terminated
} BaseResponse;

// Room info for lists
typedef struct __attribute__((packed)) {
    uint32_t room_id;
    char name[100];
    char description[256];
    uint16_t user_count;
    uint8_t is_active;  // 1 if auction ongoing
} RoomInfo;

// Item info for lists and views
typedef struct __attribute__((packed)) {
    uint32_t item_id;
    uint32_t room_id;
    char name[100];
    char description[256];
    int64_t start_price;   // In cents
    int64_t current_price; // In cents
    int64_t buy_now_price; // In cents, 0 if not applicable
    uint32_t seller_id;
    char seller_name[50];
    uint64_t end_timestamp; // Unix time when auction ends
    uint8_t status;         // 0: pending, 1: active, 2: sold
} ItemInfo;

// History entry
typedef struct __attribute__((packed)) {
    uint32_t auction_id;
    uint32_t item_id;
    char item_name[100];
    int64_t bid_amount; // Your bid, in cents
    uint8_t won;        // 1 if you won
    uint64_t timestamp;
} HistoryEntry;

// ========== Flag Helper Functions ==========

// Set a flag bit in the flags field
static inline void set_flag(uint16_t* flags, uint16_t flag) {
    *flags |= flag;
}

// Clear a flag bit in the flags field
static inline void clear_flag(uint16_t* flags, uint16_t flag) {
    *flags &= ~flag;
}

// Check if a flag bit is set
static inline int is_flag_set(uint16_t flags, uint16_t flag) {
    return (flags & flag) != 0;
}

// Check if message requires ACK
static inline int requires_ack(uint16_t flags) {
    return is_flag_set(flags, FLAG_REQUIRES_ACK);
}

// Check if message is an ACK
static inline int is_ack(uint16_t flags) {
    return is_flag_set(flags, FLAG_IS_ACK);
}

// Check if message is a retransmission
static inline int is_retransmission(uint16_t flags) {
    return is_flag_set(flags, FLAG_RETRANSMISSION);
}

// Check if message is broadcast
static inline int is_broadcast(uint16_t flags) {
    return is_flag_set(flags, FLAG_BROADCAST);
}

// Check if message has high priority
static inline int is_high_priority(uint16_t flags) {
    return is_flag_set(flags, FLAG_PRIORITY_HIGH);
}

#endif