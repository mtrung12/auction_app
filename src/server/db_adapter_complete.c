#include "db_adapter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>

static Database db = {0};

bool db_init(const char* conninfo)
{
    db.conn = PQconnectdb(conninfo);
    if (PQstatus(db.conn) != CONNECTION_OK) {
        fprintf(stderr, "DB Connection failed: %s\n", PQerrorMessage(db.conn));
        PQfinish(db.conn);
        db.conn = NULL;
        return false;
    }
    fprintf(stdout, "Database connected successfully\n");
    return true;
}

void db_cleanup(void)
{
    if (db.conn) {
        PQfinish(db.conn);
        db.conn = NULL;
    }
}

// === USER OPERATIONS ===
int32_t db_register_user(const char* username, const char* password_hash, const char* email)
{
    if (!db.conn || !username || !password_hash || !email) return -1;

    const char* paramValues[3] = { username, password_hash, email };
    PGresult* res = PQexecParams(db.conn,
        "INSERT INTO \"User\" (username, password, email, balance) "
        "VALUES ($1, $2, $3, 0) RETURNING user_id",
        3, NULL, paramValues, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        fprintf(stderr, "Register failed: %s\n", PQerrorMessage(db.conn));
        PQclear(res);
        return -1;
    }
    int32_t user_id = atoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    return user_id;
}

int32_t db_login_user(const char* username, const char* password_hash, int32_t* user_id, int64_t* balance)
{
    if (!db.conn || !username || !password_hash) return 0;

    const char* paramValues[2] = { username, password_hash };
    PGresult* res = PQexecParams(db.conn,
        "SELECT user_id, balance FROM \"User\" WHERE username=$1 AND password=$2",
        2, NULL, paramValues, NULL, NULL, 0);

    if (PQntuples(res) == 0) {
        PQclear(res);
        return 0; // fail
    }
    *user_id = atoi(PQgetvalue(res, 0, 0));
    *balance = atoll(PQgetvalue(res, 0, 1));
    PQclear(res);

    // Update last_login
    char uid_str[32];
    snprintf(uid_str, sizeof(uid_str), "%d", *user_id);
    const char* uid_param[1] = { uid_str };
    PQexecParams(db.conn, "UPDATE \"User\" SET last_login=CURRENT_TIMESTAMP WHERE user_id=$1",
                 1, NULL, uid_param, NULL, NULL, 0);
    return 1; // success
}

bool db_update_balance(int32_t user_id, int64_t amount_change)
{
    if (!db.conn || user_id <= 0) return false;

    char uid_str[32], amt_str[64];
    snprintf(uid_str, sizeof(uid_str), "%d", user_id);
    snprintf(amt_str, sizeof(amt_str), "%" PRId64, amount_change);

    const char* paramValues[2] = { amt_str, uid_str };
    PGresult* res = PQexecParams(db.conn,
        "UPDATE \"User\" SET balance = balance + $1 WHERE user_id = $2 AND balance + $1 >= 0 "
        "RETURNING user_id",
        2, NULL, paramValues, NULL, NULL, 0);

    bool success = (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0);
    PQclear(res);
    return success;
}

bool db_get_user_balance(int32_t user_id, int64_t* balance)
{
    if (!db.conn || user_id <= 0) return false;

    char query[128];
    snprintf(query, sizeof(query), "SELECT balance FROM \"User\" WHERE user_id = %d", user_id);
    PGresult* res = PQexec(db.conn, query);
    if (PQntuples(res) == 0) { PQclear(res); return false; }
    *balance = atoll(PQgetvalue(res, 0, 0));
    PQclear(res);
    return true;
}

// === ROOM OPERATIONS ===
int32_t db_create_room(const char* name, const char* desc, int32_t creator_id,
                       uint64_t start_time, uint64_t end_time)
{
    if (!db.conn || !name || creator_id <= 0) return -1;

    char start_str[32], end_str[32], creator_str[32];
    snprintf(start_str, sizeof(start_str), "%" PRIu64, start_time);
    snprintf(end_str, sizeof(end_str), "%" PRIu64, end_time);
    snprintf(creator_str, sizeof(creator_str), "%d", creator_id);

    const char* paramValues[5] = { name, desc ? desc : "", start_str, end_str, creator_str };
    PGresult* res = PQexecParams(db.conn,
        "INSERT INTO \"AuctionRoom\" (name, description, start_time, end_time, creator_id, status) "
        "VALUES ($1, $2, to_timestamp($3), to_timestamp($4), $5, 'active') RETURNING room_id",
        5, NULL, paramValues, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        fprintf(stderr, "Create room failed: %s\n", PQerrorMessage(db.conn));
        PQclear(res);
        return -1;
    }
    int32_t id = atoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    return id;
}

bool db_get_active_rooms(PGresult** res)
{
    if (!db.conn) return false;
    *res = PQexec(db.conn,
        "SELECT room_id, name, description FROM \"AuctionRoom\" WHERE status='active' "
        "ORDER BY room_id");
    return (PQresultStatus(*res) == PGRES_TUPLES_OK);
}

bool db_get_room_items(int32_t room_id, PGresult** res)
{
    if (!db.conn || room_id <= 0) return false;

    char query[256];
    snprintf(query, sizeof(query),
        "SELECT item_id, name, starting_price, current_price, buy_now_price, status, "
        "seller_id, queue_position FROM \"Item\" WHERE room_id=%d ORDER BY queue_position",
        room_id);
    *res = PQexec(db.conn, query);
    return (PQresultStatus(*res) == PGRES_TUPLES_OK);
}

// === ITEM OPERATIONS ===
int32_t db_create_item(int32_t room_id, int32_t seller_id, const char* name, const char* desc,
                       int64_t start_price_vnd, int64_t buy_now_price_vnd, uint32_t duration_sec)
{
    if (!db.conn || room_id <= 0 || seller_id <= 0 || !name || start_price_vnd < 0) return -1;

    char room_str[32], seller_str[32], start_str[64], buy_now_str[64], dur_str[32];
    snprintf(room_str, sizeof(room_str), "%d", room_id);
    snprintf(seller_str, sizeof(seller_str), "%d", seller_id);
    snprintf(start_str, sizeof(start_str), "%" PRId64, start_price_vnd);
    snprintf(buy_now_str, sizeof(buy_now_str), "%" PRId64, buy_now_price_vnd);
    snprintf(dur_str, sizeof(dur_str), "%u", duration_sec);

    // Get next queue position
    char queue_query[256];
    snprintf(queue_query, sizeof(queue_query),
        "SELECT COALESCE(MAX(queue_position), 0) + 1 FROM \"Item\" WHERE room_id=%d", room_id);
    PGresult* queue_res = PQexec(db.conn, queue_query);
    int queue_pos = 0;
    if (PQntuples(queue_res) > 0) {
        queue_pos = atoi(PQgetvalue(queue_res, 0, 0));
    }
    PQclear(queue_res);

    char queue_str[32];
    snprintf(queue_str, sizeof(queue_str), "%d", queue_pos);

    const char* paramValues[9] = {
        name, desc ? desc : "", start_str, start_str, buy_now_str, 
        "pending", room_str, seller_str, queue_str
    };

    PGresult* res = PQexecParams(db.conn,
        "INSERT INTO \"Item\" (name, description, starting_price, current_price, buy_now_price, "
        "status, room_id, seller_id, queue_position) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9) RETURNING item_id",
        9, NULL, paramValues, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        fprintf(stderr, "Create item failed: %s\n", PQerrorMessage(db.conn));
        PQclear(res);
        return -1;
    }
    int32_t item_id = atoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    return item_id;
}

bool db_delete_item(int32_t item_id)
{
    if (!db.conn || item_id <= 0) return false;

    char query[128];
    snprintf(query, sizeof(query), "UPDATE \"Item\" SET status='deleted' WHERE item_id=%d", item_id);
    PGresult* res = PQexec(db.conn, query);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    return success;
}

bool db_get_item_details(int32_t item_id, PGresult** res)
{
    if (!db.conn || item_id <= 0) return false;

    char query[256];
    snprintf(query, sizeof(query),
        "SELECT item_id, name, description, starting_price, current_price, buy_now_price, "
        "status, seller_id, room_id FROM \"Item\" WHERE item_id=%d", item_id);
    *res = PQexec(db.conn, query);
    return (PQresultStatus(*res) == PGRES_TUPLES_OK && PQntuples(*res) > 0);
}

// === BIDDING OPERATIONS ===
bool db_place_bid(int32_t item_id, int32_t bidder_id, int64_t bid_amount_vnd, int64_t* new_current_price_vnd)
{
    if (!db.conn || item_id <= 0 || bidder_id <= 0 || bid_amount_vnd <= 0) return false;

    // Start transaction
    PQexec(db.conn, "BEGIN");

    // Get current price with lock
    char item_str[32];
    snprintf(item_str, sizeof(item_str), "%d", item_id);
    const char* param[1] = { item_str };

    PGresult* res = PQexecParams(db.conn,
        "SELECT current_price FROM \"Item\" WHERE item_id=$1 FOR UPDATE",
        1, NULL, param, NULL, NULL, 0);

    if (PQntuples(res) == 0) {
        PQclear(res);
        PQexec(db.conn, "ROLLBACK");
        return false;
    }

    int64_t current_price = atoll(PQgetvalue(res, 0, 0));
    PQclear(res);

    // Check if bid is valid (at least 10000 VND higher)
    if (bid_amount_vnd <= current_price || bid_amount_vnd - current_price < 10000) {
        PQexec(db.conn, "ROLLBACK");
        return false;
    }

    char bidder_str[32], bid_str[64];
    snprintf(bidder_str, sizeof(bidder_str), "%d", bidder_id);
    snprintf(bid_str, sizeof(bid_str), "%" PRId64, bid_amount_vnd);

    const char* params[3] = { bid_str, item_str, bidder_str };

    res = PQexecParams(db.conn,
        "UPDATE \"Item\" SET current_price = $1 WHERE item_id = $2; "
        "INSERT INTO \"Bid\" (item_id, bidder_id, bid_amount) VALUES ($2, $3, $1)",
        3, NULL, params, NULL, NULL, 0);

    if (PQresultStatus(res) == PGRES_COMMAND_OK) {
        PQclear(res);
        PQexec(db.conn, "COMMIT");
        *new_current_price_vnd = bid_amount_vnd;
        return true;
    } else {
        PQclear(res);
        PQexec(db.conn, "ROLLBACK");
        return false;
    }
}

bool db_buy_now(int32_t item_id, int32_t buyer_id, int64_t buy_now_price_vnd)
{
    if (!db.conn || item_id <= 0 || buyer_id <= 0) return false;

    char item_str[32], buyer_str[32], price_str[64];
    snprintf(item_str, sizeof(item_str), "%d", item_id);
    snprintf(buyer_str, sizeof(buyer_str), "%d", buyer_id);
    snprintf(price_str, sizeof(price_str), "%" PRId64, buy_now_price_vnd);

    const char* params[3] = { item_str, buyer_str, price_str };

    PGresult* res = PQexecParams(db.conn,
        "UPDATE \"Item\" SET status='sold', winner_id=$2, win_amount=$3, win_type='buy_now' "
        "WHERE item_id=$1 AND status IN ('pending', 'bidding')",
        3, NULL, params, NULL, NULL, 0);

    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK && atoi(PQcmdTuples(res)) > 0);
    PQclear(res);
    return success;
}

bool db_update_item_winner(int32_t item_id, int32_t winner_id, int64_t final_price_vnd, const char* win_type)
{
    if (!db.conn || item_id <= 0 || winner_id <= 0 || !win_type) return false;

    char item_str[32], winner_str[32], price_str[64];
    snprintf(item_str, sizeof(item_str), "%d", item_id);
    snprintf(winner_str, sizeof(winner_str), "%d", winner_id);
    snprintf(price_str, sizeof(price_str), "%" PRId64, final_price_vnd);

    const char* params[4] = { winner_str, price_str, win_type, item_str };

    PGresult* res = PQexecParams(db.conn,
        "UPDATE \"Item\" SET status='sold', winner_id=$1, win_amount=$2, win_type=$3 "
        "WHERE item_id=$4",
        4, NULL, params, NULL, NULL, 0);

    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    return success;
}

// === TRANSACTION OPERATIONS ===
bool db_add_transaction(int32_t user_id, int64_t amount_vnd, const char* type, int32_t related_item_id, const char* status)
{
    if (!db.conn || user_id <= 0 || !type || !status) return false;

    char uid_str[32], amt_str[64], item_str[32];
    snprintf(uid_str, sizeof(uid_str), "%d", user_id);
    snprintf(amt_str, sizeof(amt_str), "%" PRId64, amount_vnd);
    snprintf(item_str, sizeof(item_str), "%d", related_item_id);

    const char* params[5] = {
        uid_str,
        amt_str,
        type,
        (related_item_id > 0) ? item_str : NULL,
        status
    };

    const char* query_str = (related_item_id > 0) ?
        "INSERT INTO \"Transaction\" (user_id, amount, type, related_item_id, status) "
        "VALUES ($1, $2, $3, $4, $5)" :
        "INSERT INTO \"Transaction\" (user_id, amount, type, status) "
        "VALUES ($1, $2, $3, $5)";

    int nparams = (related_item_id > 0) ? 5 : 4;
    PGresult* res = PQexecParams(db.conn, query_str, nparams, NULL, params, NULL, NULL, 0);

    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    return success;
}

bool db_get_user_history(int32_t user_id, PGresult** res)
{
    if (!db.conn || user_id <= 0) return false;

    char query[512];
    snprintf(query, sizeof(query),
        "SELECT t.transaction_id, t.timestamp, t.type, t.amount, "
        "COALESCE(i.name, 'N/A') as item_name, t.status "
        "FROM \"Transaction\" t "
        "LEFT JOIN \"Item\" i ON t.related_item_id = i.item_id "
        "WHERE t.user_id = %d ORDER BY t.timestamp DESC LIMIT 50", user_id);
    *res = PQexec(db.conn, query);
    return (PQresultStatus(*res) == PGRES_TUPLES_OK);
}

// === SEARCH OPERATIONS ===
bool db_search_items(const char* search_term, PGresult** res)
{
    if (!db.conn) return false;

    char query[512];
    if (search_term && strlen(search_term) > 0) {
        // Escape for SQL injection protection
        snprintf(query, sizeof(query),
            "SELECT item_id, name, description, starting_price, current_price, "
            "buy_now_price, status, room_id FROM \"Item\" "
            "WHERE (name ILIKE '%%' || '%s' || '%%' OR description ILIKE '%%' || '%s' || '%%') "
            "AND status IN ('pending', 'bidding') ORDER BY item_id DESC",
            search_term, search_term);
    } else {
        snprintf(query, sizeof(query),
            "SELECT item_id, name, description, starting_price, current_price, "
            "buy_now_price, status, room_id FROM \"Item\" "
            "WHERE status IN ('pending', 'bidding') ORDER BY item_id DESC");
    }
    *res = PQexec(db.conn, query);
    return (PQresultStatus(*res) == PGRES_TUPLES_OK);
}
