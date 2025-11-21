#ifndef DB_ADAPTER_H
#define DB_ADAPTER_H

#include <libpq-fe.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    PGconn* conn;
} Database;

// Core
bool db_init(const char* conninfo);
void db_cleanup(void);

// User operations
int32_t db_register_user(const char* username, const char* password_hash, const char* email);
int32_t db_login_user(const char* username, const char* password_hash, int32_t* user_id, int64_t* balance_vnd);
bool db_update_balance(int32_t user_id, int64_t vnd_change);           
bool db_get_user_balance(int32_t user_id, int64_t* balance_vnd);

// Room operations
int32_t db_create_room(const char* name, const char* desc, int32_t creator_id,
                       uint64_t start_time, uint64_t end_time);
bool db_get_active_rooms(PGresult** res);        // Caller must PQclear()
bool db_get_room_items(int32_t room_id, PGresult** res);

// Item operations
int32_t db_create_item(int32_t room_id, int32_t seller_id, const char* name, const char* desc,
                       int64_t start_price_vnd, int64_t buy_now_price_vnd, uint32_t duration_sec);

bool db_place_bid(int32_t item_id, int32_t bidder_id, int64_t bid_amount_vnd, int64_t* new_current_price_vnd);
bool db_buy_now(int32_t item_id, int32_t buyer_id, int64_t buy_now_price_vnd);
bool db_get_item_details(int32_t item_id, PGresult** res);
bool db_update_item_winner(int32_t item_id, int32_t winner_id, int64_t final_price_vnd, const char* win_type);

// Transaction & History
bool db_add_transaction(int32_t user_id, int64_t amount_vnd, const char* type, int32_t related_item_id, const char* status);
bool db_get_user_history(int32_t user_id, PGresult** res);

#endif