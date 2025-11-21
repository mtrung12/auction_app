#include "server.h"
#include "db_adapter.h"
#include "session_mgr.h"
#include <stdlib.h> // getenv
#include <stdio.h>

int main() {
    const char *db_user = getenv("DB_USER");
    const char *db_password = getenv("DB_PASSWORD");
    
    if (!db_user) db_user = "trung"; // fallback
    if (!db_password) db_password = "123"; // fallback

    char conninfo[256];
    snprintf(conninfo, sizeof(conninfo),
             "host=localhost dbname=auction user=%s password=%s", db_user, db_password);

    if (!db_init(conninfo)) {
        fprintf(stderr, "DB Connection failed\n");
        return 1;
    }

    session_init();
    server_start(PORT);

    db_cleanup();
    session_cleanup();
    return 0;
}