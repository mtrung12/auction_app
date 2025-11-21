-- Drop existing tables (with CASCADE to handle foreign keys)
DROP TABLE IF EXISTS chat_messages CASCADE;
DROP TABLE IF EXISTS activity_logs CASCADE;
DROP TABLE IF EXISTS bids CASCADE;
DROP TABLE IF EXISTS user_rooms CASCADE;
DROP TABLE IF EXISTS auction_items CASCADE;
DROP TABLE IF EXISTS auction_rooms CASCADE;
DROP TABLE IF EXISTS users CASCADE;

-- ============================
-- Create Users Table
-- ============================
CREATE TABLE users (
    user_id SERIAL PRIMARY KEY,
    username VARCHAR(255) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    email VARCHAR(255) UNIQUE NOT NULL,
    bank_account VARCHAR(50),
    bank_name VARCHAR(100),
    balance DECIMAL(15, 2) DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- ============================
-- Create Auction Rooms Table
-- ============================
CREATE TABLE auction_rooms (
    room_id SERIAL PRIMARY KEY,
    room_name VARCHAR(255) NOT NULL,
    created_by INT NOT NULL,
    description TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (created_by) REFERENCES users(user_id) ON DELETE CASCADE
);

-- ============================
-- Create Auction Items Table
-- ============================
CREATE TABLE auction_items (
    item_id SERIAL PRIMARY KEY,
    room_id INT NOT NULL,
    item_name VARCHAR(255) NOT NULL,
    description TEXT,
    starting_price DECIMAL(15, 2) NOT NULL,
    current_price DECIMAL(15, 2),
    buy_now_price DECIMAL(15, 2),
    auction_duration INT DEFAULT 3600,
    status VARCHAR(50) DEFAULT 'scheduled' CHECK (status IN ('scheduled', 'active', 'available', 'sold', 'cancelled')),
    created_by INT NOT NULL,
    queue_position INT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (room_id) REFERENCES auction_rooms(room_id) ON DELETE CASCADE,
    FOREIGN KEY (created_by) REFERENCES users(user_id) ON DELETE CASCADE
);

-- ============================
-- Create Bids Table
-- ============================
CREATE TABLE bids (
    bid_id SERIAL PRIMARY KEY,
    item_id INT NOT NULL,
    user_id INT NOT NULL,
    bid_amount DECIMAL(15, 2) NOT NULL,
    bid_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (item_id) REFERENCES auction_items(item_id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE
);

-- ============================
-- Create User Rooms Table (One room per user constraint)
-- ============================
CREATE TABLE user_rooms (
    user_id INT NOT NULL,
    room_id INT NOT NULL,
    joined_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (user_id, room_id),
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
    FOREIGN KEY (room_id) REFERENCES auction_rooms(room_id) ON DELETE CASCADE,
    UNIQUE(user_id)
);

-- ============================
-- Create Activity Logs Table
-- ============================
CREATE TABLE activity_logs (
    log_id SERIAL PRIMARY KEY,
    user_id INT,
    action VARCHAR(255) NOT NULL,
    details TEXT,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE SET NULL
);

-- ============================
-- Create Chat Messages Table
-- ============================
CREATE TABLE chat_messages (
    message_id SERIAL PRIMARY KEY,
    room_id INT NOT NULL,
    user_id INT NOT NULL,
    message TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (room_id) REFERENCES auction_rooms(room_id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE
);
