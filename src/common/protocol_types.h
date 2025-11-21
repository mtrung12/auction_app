#ifndef PROTOCOL_TYPES_H
#define PROTOCOL_TYPES_H

// Enum for all Message Types (grouped for clarity)
typedef enum {
    // Auth (1-10)
    MSG_LOGIN_REQ = 1,
    MSG_LOGIN_RES,
    MSG_REGISTER_REQ,
    MSG_REGISTER_RES,
    MSG_LOGOUT_REQ,
    MSG_LOGOUT_RES,
    
    // Account Management (11-20)
    MSG_DEPOSIT_REQ = 11,
    MSG_DEPOSIT_RES,
    MSG_REDEEM_REQ,
    MSG_REDEEM_RES,
    MSG_VIEW_HISTORY_REQ,
    MSG_VIEW_HISTORY_RES,
    
    // Outside-Room Actions (21-30)
    MSG_JOIN_ROOM_REQ = 21,
    MSG_JOIN_ROOM_RES,
    MSG_LEAVE_ROOM_REQ,
    MSG_LEAVE_ROOM_RES,
    MSG_LIST_ROOMS_REQ,
    MSG_LIST_ROOMS_RES,
    MSG_SEARCH_ITEM_REQ,
    MSG_SEARCH_ITEM_RES,
    MSG_CREATE_ROOM_REQ,
    MSG_CREATE_ROOM_RES,
    
    // In-Room Actions (31-50)
    MSG_VIEW_ITEMS_REQ = 31,
    MSG_VIEW_ITEMS_RES,
    MSG_BID_REQ,
    MSG_BID_RES,
    MSG_BID_NOTIFY,     // Broadcast
    MSG_BUY_NOW_REQ,
    MSG_BUY_NOW_RES,
    MSG_CHAT_REQ,
    MSG_CHAT_NOTIFY,    // Broadcast
    MSG_CREATE_ITEM_REQ,
    MSG_CREATE_ITEM_RES,
    MSG_DELETE_ITEM_REQ,
    MSG_DELETE_ITEM_RES,
    MSG_TIMER_UPDATE = 51,  // Broadcast
    MSG_ITEM_SOLD           // Broadcast
} MessageType;

#endif