#include "db_adapter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../common/utils.h"  // for logging

static Database db = {0};

bool db_init(const char* conninfo)
{
    db.conn = PQconnectdb(conninfo);
    if (PQstatus(db.conn) != CONNECTION_OK) {
        log_error("DB Connection failed: %s", PQerrorMessage(db.conn));
        PQfinish(db.conn);
        db.conn = NULL;
        return false;
    }
    log_info("Database connected successfully");
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
    const char* paramValues[3] = { username, password_hash, email };
    PGresult* res = PQexecParams(db.conn,
        "INSERT INTO \"User\" (username, password, email) "
        "VALUES ($1, $2, $3) RETURNING user_id",
        3, NULL, paramValues, NULL, NULL, 0);

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0) {
        log_error("Register failed: %s", PQerrorMessage(db.conn));
        PQclear(res);
        return -1;
    }
    int32_t user_id = atoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    return user_id;
}

int32_t db_login_user(const char* username, const char* password_hash, int32_t* user_id, int64_t* balance)
{
    const char* paramValues[2] = { username, password_hash };
    PGresult* res = PQexecParams(db.conn,
        "SELECT user_id, balance FROM \"User\" WHERE username=$1 AND password=$2",
        2, NULL, paramValues, NULL, NULL, 0);

    if (PQntuples(res) == 0) {
        PQclear(res);
        return 0; // fail
    }
    *user_id = atoi(PQgetvalue(res, 0, 0));
    *balance = atof(PQgetvalue(res, 0, 1));
    PQclear(res);

    // Update last_login
    PQexecParams(db.conn, "UPDATE \"User\" SET last_login=CURRENT_TIMESTAMP WHERE user_id=$1",
                 1, NULL, (const char*[]){ PQgetvalue(res, 0, 0) }, NULL, NULL, 0);
    return 1; // success
}

bool db_update_balance(int32_t user_id, int64_t amount_change)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", user_id);

    const char* paramValues[2] = { buf, amount_change > 0 ? "+" : "" };
    char query[256];
    snprintf(query, sizeof(query),
             "UPDATE \"User\" SET balance = balance + %.2f WHERE user_id = %d AND balance + %.2f >= 0",
             amount_change, user_id, amount_change);

    PGresult* res = PQexec(db.conn, query);
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK && atoi(PQcmdTuples(res)) > 0);
    PQclear(res);
    return success;
}

bool db_get_user_balance(int32_t user_id, int64_t* balance)
{
    char query[128];
    snprintf(query, sizeof(query), "SELECT balance FROM \"User\" WHERE user_id = %d", user_id);
    PGresult* res = PQexec(db.conn, query);
    if (PQntuples(res) == 0) { PQclear(res); return false; }
    *balance = atof(PQgetvalue(res, 0, 0));
    PQclear(res);
    return true;
}

// === ROOM & ITEM ===
int32_t db_create_room(const char* name, const char* desc, int32_t creator_id,
                       uint64_t start_time, uint64_t end_time)
{
    char start_str[32], end_str[32];
    snprintf(start_str, sizeof(start_str), "%" PRIu64, start_time);
    snprintf(end_str, sizeof(end_str), "%" PRIu64, end_time);

    const char* paramValues[5] = { name, desc ? desc : "", start_str, end_str, NULL };
    snprintf((char*)paramValues[4], 16, "%d", creator_id);

    PGresult* res = PQexecParams(db.conn,
        "INSERT INTO \"AuctionRoom\" (name, description, start_time, end_time, creator_id) "
        "VALUES ($1, $2, to_timestamp($3), to_timestamp($4), $5) RETURNING room_id",
        5, NULL, paramValues, NULL, NULL, 0);

    if (PQntuples(res) == 0) { PQclear(res); return -1; }
    int32_t id = atoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    return id;
}

bool db_get_active_rooms(PGresult** res)
{
    *res = PQexec(db.conn, "SELECT room_id, name, description, user_count FROM \"AuctionRoom\" WHERE status='active'");
    return (PQresultStatus(*res) == PGRES_TUPLES_OK);
}

bool db_get_room_items(int32_t room_id, PGresult** res)
{
    char query[128];
    snprintf(query, sizeof(query),
             "SELECT item_id, name, current_price, buy_now_price, status FROM \"Item\" WHERE room_id=%d", room_id);
    *res = PQexec(db.conn, query);
    return (PQresultStatus(*res) == PGRES_TUPLES_OK);
}

// === BIDDING ===
bool db_place_bid(int32_t item_id, int32_t bidder_id, int64_t bid_amount, int64_t* new_current_price)
{
    PGresult* res;
    ExecStatusType status;

    // Start transaction
    PQexec(db.conn, "BEGIN");

    // Lock item row
    res = PQexec(db.conn, "SELECT current_price FROM \"Item\" WHERE item_id=$1 FOR UPDATE", 1, NULL,
                 (const char*[]){ &item_id }, NULL, NULL, 0);
    if (PQntuples(res) == 0) { PQexec(db.conn, "ROLLBACK"); PQclear(res); return false; }
    int64_t current = atof(PQgetvalue(res, 0, 0));
    PQclear(res);

    if (bid_amount <= current) { PQexec(db.conn, "ROLLBACK"); return false; }

    char buf[3][32];
    snprintf(buf[0], sizeof(buf[0]), "%d", item_id);
    snprintf(buf[1], sizeof(buf[1]), "%d", bidder_id);
    snprintf(buf[2], sizeof(buf[2]), "%.2f", bid_amount);

    const char* params[3] = { buf[0], buf[1], buf[2] };
    res = PQexecParams(db.conn,
        "UPDATE \"Item\" SET current_price = $3 WHERE item_id = $1; "
        "INSERT INTO \"Bid\" (item_id, bidder_id, bid_amount) VALUES ($1, $2, $3)",
        3, NULL, params, NULL, NULL, 0);

    status = PQresultStatus(res);
    PQclear(res);

    if (status == PGRES_COMMAND_OK) {
        *new_current_price = bid_amount;
        PQexec(db.conn, "COMMIT");
        return true;
    } else {
        PQexec(db.conn, "ROLLBACK");
        return false;
    }
}

bool db_buy_now(int32_t item_id, int32_t buyer_id, int64_t buy_now_price)
{
    // Similar to bid, but also mark as sold
    // Left as exercise or extend later
    return false; // placeholder
}

// === TRANSACTION LOG ===
bool db_add_transaction(int32_t user_id, int64_t amount, const char* type, int32_t related_item_id, const char* status)
{
    char query[512];
    char item_str[16] = "NULL";
    if (related_item_id > 0) snprintf(item_str, sizeof(item_str), "%d", related_item_id);

    snprintf(query, sizeof(query),
             "INSERT INTO \"Transaction\" (user_id, amount, type, related_item_id, status) "
             "VALUES (%d, %.2f, '%s', %s, '%s')",
             user_id, amount, type, item_str, status);
    PGresult* res = PQexec(db.conn, query);
    bool ok = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    return ok;
}

bool db_get_user_history(int32_t user_id, PGresult** res)
{
    char query[256];
    snprintf(query, sizeof(query),
             "SELECT t.timestamp, t.type, t.amount, i.name FROM \"Transaction\" t "
             "LEFT JOIN \"Item\" i ON t.related_item_id = i.item_id "
             "WHERE t.user_id = %d ORDER BY t.timestamp DESC", user_id);
    *res = PQexec(db.conn, query);
    return (PQresultStatus(*res) == PGRES_TUPLES_OK);
}