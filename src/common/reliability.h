#ifndef RELIABILITY_H
#define RELIABILITY_H

#include <stdint.h>
#include <time.h>
#include <string.h>
#include "protocol_header.h"
#include "protocol_helpers.h"

// ACK timeout in milliseconds
#define ACK_TIMEOUT_MS 5000

// Maximum retransmission attempts
#define MAX_RETRIES 3

// Structure to track pending messages awaiting ACK
typedef struct {
    uint32_t request_id;        // Original request ID
    uint8_t message_type;       // Original message type
    uint64_t send_time;         // Time sent (milliseconds since epoch)
    uint8_t retry_count;        // Number of retries so far
    char payload[2048];         // Serialized message to retransmit
    uint32_t payload_len;       // Length of payload
    int is_active;              // 1 if awaiting ACK, 0 if completed
} PendingMessage;

// Pending message queue (simple array-based implementation)
#define MAX_PENDING_MESSAGES 100

typedef struct {
    PendingMessage messages[MAX_PENDING_MESSAGES];
    int count;
} PendingQueue;

// ========== Reliability Helper Functions ==========

// Initialize pending message queue
static inline void pending_queue_init(PendingQueue* queue) {
    queue->count = 0;
    for (int i = 0; i < MAX_PENDING_MESSAGES; i++) {
        queue->messages[i].is_active = 0;
    }
}

// Add a message to the pending queue (returns index or -1 if full)
static inline int pending_queue_add(PendingQueue* queue, uint32_t request_id,
                                    uint8_t msg_type, const char* payload, uint32_t len) {
    if (queue->count >= MAX_PENDING_MESSAGES) {
        return -1;  // Queue full
    }
    
    PendingMessage* pm = &queue->messages[queue->count];
    pm->request_id = request_id;
    pm->message_type = msg_type;
    pm->send_time = (uint64_t)time(NULL) * 1000;  // Convert to milliseconds
    pm->retry_count = 0;
    pm->payload_len = len > 2048 ? 2048 : len;
    if (payload && len > 0) {
        memcpy(pm->payload, payload, pm->payload_len);
    }
    pm->is_active = 1;
    
    return queue->count++;
}

// Find a pending message by request_id
static inline int pending_queue_find(PendingQueue* queue, uint32_t request_id) {
    for (int i = 0; i < queue->count; i++) {
        if (queue->messages[i].is_active && queue->messages[i].request_id == request_id) {
            return i;
        }
    }
    return -1;
}

// Mark a message as received (ACK received)
static inline void pending_queue_ack(PendingQueue* queue, uint32_t request_id) {
    int idx = pending_queue_find(queue, request_id);
    if (idx >= 0) {
        queue->messages[idx].is_active = 0;
    }
}

// Get all messages that need retransmission (based on timeout)
static inline int pending_queue_get_expired(PendingQueue* queue, uint32_t request_ids[], 
                                           int max_count) {
    uint64_t now = (uint64_t)time(NULL) * 1000;
    int count = 0;
    
    for (int i = 0; i < queue->count && count < max_count; i++) {
        if (queue->messages[i].is_active) {
            uint64_t elapsed = now - queue->messages[i].send_time;
            if (elapsed > ACK_TIMEOUT_MS && queue->messages[i].retry_count < MAX_RETRIES) {
                request_ids[count++] = queue->messages[i].request_id;
            }
        }
    }
    
    return count;
}

// Increment retry count for a message
static inline void pending_queue_increment_retry(PendingQueue* queue, uint32_t request_id) {
    int idx = pending_queue_find(queue, request_id);
    if (idx >= 0) {
        queue->messages[idx].retry_count++;
    }
}

// Get retry count for a message
static inline uint8_t pending_queue_get_retry_count(PendingQueue* queue, uint32_t request_id) {
    int idx = pending_queue_find(queue, request_id);
    return (idx >= 0) ? queue->messages[idx].retry_count : 0;
}

// Get payload of a pending message
static inline const char* pending_queue_get_payload(PendingQueue* queue, uint32_t request_id, 
                                                   uint32_t* out_len) {
    int idx = pending_queue_find(queue, request_id);
    if (idx >= 0) {
        if (out_len) *out_len = queue->messages[idx].payload_len;
        return queue->messages[idx].payload;
    }
    if (out_len) *out_len = 0;
    return NULL;
}

// Clear expired messages (that exceeded max retries)
static inline void pending_queue_cleanup(PendingQueue* queue) {
    for (int i = 0; i < queue->count; i++) {
        if (queue->messages[i].is_active && queue->messages[i].retry_count >= MAX_RETRIES) {
            queue->messages[i].is_active = 0;
        }
    }
}

// ========== ACK Message Helper ==========

// Create an ACK message in response to a received message
static inline void create_ack_message(Message* ack_msg, uint32_t request_id, uint8_t original_type) {
    ack_msg->header.version = 1;
    ack_msg->header.type = original_type;  // Keep original type for correlation
    ack_msg->header.request_id = request_id;
    ack_msg->header.timestamp = (uint64_t)time(NULL);
    ack_msg->header.payload_length = 0;  // ACK has no payload
    
    set_flag(&ack_msg->header.flags, FLAG_IS_ACK);
}

#endif
