#ifndef PROTOCOL_HEADER_H
#define PROTOCOL_HEADER_H

#include <stdint.h>
#include "protocol_types.h"

#define BUFF_SIZE 2048
#define PORT 5500

// Flag definitions for reliability and control
#define FLAG_REQUIRES_ACK      0x0001  // Bit 0: Message requires ACK
#define FLAG_IS_ACK            0x0002  // Bit 1: This is an ACK message
#define FLAG_RETRANSMISSION    0x0004  // Bit 2: This is a retransmitted message
#define FLAG_COMPRESSED        0x0008  // Bit 3: Payload is compressed
#define FLAG_FRAGMENTED        0x0010  // Bit 4: Message is fragmented (sequence in request_id)
#define FLAG_BROADCAST         0x0020  // Bit 5: Broadcast message (no ACK needed)
#define FLAG_PRIORITY_HIGH     0x0040  // Bit 6: High priority message
#define FLAG_ENCRYPTED         0x0080  // Bit 7: Payload is encrypted

// Standard Header (packed to avoid padding)
typedef struct __attribute__((packed)) {
    uint8_t version;        // Protocol version (start with 1)
    uint8_t type;           // Message type (cast from MessageType enum)
    uint16_t flags;         // Flags for message properties (see FLAG_* definitions)
    uint32_t request_id;    // Unique ID for req/res correlation
    uint64_t timestamp;     // Unix timestamp for ordering/security
    uint32_t payload_length;// Payload length
} MessageHeader;

// Generic Message Container
typedef struct {
    MessageHeader header;
    char payload[BUFF_SIZE];
} Message;

#endif