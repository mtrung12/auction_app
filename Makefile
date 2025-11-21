CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I. -Isrc -Isrc/common -I/usr/include/postgresql -g
LDFLAGS = -lpq -pthread
DB_USER ?= $(USER)

# Directories
BUILD_DIR = build
SRC_DIR = src
SERVER_DIR = $(SRC_DIR)/server
CLIENT_DIR = $(SRC_DIR)/client
HANDLERS_DIR = $(SERVER_DIR)/handlers

# Output files
SERVER = $(BUILD_DIR)/auction_server
CLIENT = $(BUILD_DIR)/demo_client

# Server source files
SERVER_SRCS = $(SERVER_DIR)/main.c \
              $(SERVER_DIR)/server.c \
              $(SERVER_DIR)/session_mgr.c \
              $(SERVER_DIR)/db_adapter_complete.c \
              $(HANDLERS_DIR)/handle_auth.c \
              $(HANDLERS_DIR)/handle_account.c \
              $(HANDLERS_DIR)/handle_room.c \
              $(HANDLERS_DIR)/handle_bidding.c \
              $(HANDLERS_DIR)/logic_handler.c \
              src/common/utils.c

SERVER_OBJS = $(SERVER_SRCS:.c=.o)

# Client source files
CLIENT_SRCS = $(CLIENT_DIR)/demo_client.c
CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)

# Default target
all: $(BUILD_DIR) $(SERVER) $(CLIENT)

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	@echo "Created $(BUILD_DIR) directory"

# Compile server
$(SERVER): $(SERVER_OBJS)
	@echo "Linking server..."
	$(CC) -o $@ $^ $(LDFLAGS)
	@echo "Server compiled: $@"

# Compile client
$(CLIENT): $(CLIENT_OBJS)
	@echo "Linking client..."
	$(CC) -o $@ $^ $(LDFLAGS)
	@echo "Client compiled: $@"

# Compile object files
%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)
	rm -f $(SRC_DIR)/**/*.o
	rm -f $(SRC_DIR)/*.o
	@echo "Clean complete"

# Rebuild from scratch
rebuild: clean all

# Run server
run-server: $(SERVER)
	@echo "Starting auction server..."
	./$(SERVER)

# Run client
run-client: $(CLIENT)
	@echo "Starting demo client..."
	./$(CLIENT)

# Run server in background
run-server-bg: $(SERVER)
	@echo "Starting auction server in background..."
	./$(SERVER) &
	@echo "Server PID: $$!"

# Database setup
db-setup:
	@echo "Setting up database..."
	psql -U $(DB_USER) -d auction -f data/schema.sql
	psql -U $(DB_USER) -d auction -f data/data.sql
	@echo "Database setup complete"

# Database reset
db-reset:
	@echo "Resetting database..."
	# Create DB_USER role if it doesn't exist
	# Correct
	@sudo -u postgres psql -tc "SELECT 1 FROM pg_roles WHERE rolname='$(DB_USER)';" | grep -q 1 || \
    sudo -u postgres psql -c "CREATE ROLE $(DB_USER) WITH LOGIN SUPERUSER;"

	# Drop the database if it exists
	@dropdb -U $(DB_USER) auction 2>/dev/null || true
	# Create the database owned by DB_USER
	@createdb -O $(DB_USER) auction
	# Load schema and data
	@psql -U $(DB_USER) -d auction -f data/schema.sql
	@psql -U $(DB_USER) -d auction -f data/data.sql
	@echo "Database reset complete"

# Database cleanup
db-clean:
	@echo "Dropping database..."
	dropdb -U $(DB_USER) auction 2>/dev/null || true
	@echo "Database dropped"

# Demo: Full workflow
demo: all db-reset
	@echo "=== Auction App Demo Ready ==="
	@echo "Run in separate terminals:"
	@echo "  Terminal 1: make run-server"
	@echo "  Terminal 2: make run-client"

# Show available targets
help:
	@echo "Available targets:"
	@echo "  make              - Build both server and client"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make rebuild      - Clean and rebuild"
	@echo "  make run-server   - Run auction server"
	@echo "  make run-client   - Run demo client"
	@echo "  make run-server-bg - Run server in background"
	@echo "  make db-setup     - Initialize database"
	@echo "  make db-reset     - Reset database (drop and recreate)"
	@echo "  make db-clean     - Drop database only"
	@echo "  make demo         - Full setup (build + db-reset)"
	@echo "  make help         - Show this help message"

# Phony targets (not actual files)
.PHONY: all clean rebuild run-server run-client run-server-bg db-setup db-reset db-clean demo help