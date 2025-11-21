-- ============================
-- Insert Users
-- ============================
INSERT INTO users (username, password_hash, email, bank_account, bank_name, balance)
VALUES
('alice', 'pass123', 'alice@example.com', '001', 'Vietcombank', 50000000),
('bob', 'pass123', 'bob@example.com', '002', 'Techcombank', 30000000),
('charlie', 'pass123', 'charlie@example.com', '003', 'MB Bank', 15000000),
('david', 'pass123', 'david@example.com', '004', 'Vietinbank', 8000000),
('emma', 'pass123', 'emma@example.com', '005', 'VPBank', 12000000),
('frank', 'pass123', 'frank@example.com', '006', 'BIDV', 9500000),
('hannah', 'pass123', 'hannah@example.com', '007', 'ACB', 3000000),
('ian', 'pass123', 'ian@example.com', '008', 'BIDV', 6000000),
('julia', 'pass123', 'julia@example.com', '009', 'Techcombank', 42000000),
('kevin', 'pass123', 'kevin@example.com', '010', 'Vietcombank', 20000000);

-- ============================
-- Insert Auction Rooms
-- ============================
INSERT INTO auction_rooms (room_name, created_by, description)
VALUES
('Electronics Auction', 1, 'Phones, laptops, gadgets'),
('Luxury Items', 2, 'Jewelry and watches'),
('Vintage Collection', 3, 'Antiques and collectibles');

-- ============================
-- Insert Items (corrected status values)
-- ============================
INSERT INTO auction_items (room_id, item_name, description, starting_price, current_price, buy_now_price, auction_duration, status, created_by, queue_position)
VALUES
-- Room 1: Electronics
(1, 'iPhone 15', 'Brand new phone', 8000000, 9500000, 12000000, 1800, 'sold', 1, 1),
(1, 'MacBook Air M2', 'Lightweight laptop', 9000000, 10000000, 15000000, 1800, 'active', 1, 2),
(1, 'Samsung Galaxy S23', 'High-end Android phone', 7000000, 7800000, NULL, 1800, 'available', 2, 3),

-- Room 2: Luxury Items
(2, 'Rolex Submariner', 'Luxury watch', 50000000, 80000000, 80000000, 3600, 'sold', 3, 1),
(2, 'Diamond Ring', '18K gold diamond ring', 20000000, 25000000, 35000000, 3600, 'sold', 4, 2),
(2, 'Cartier Bracelet', 'Gold bracelet', 15000000, 16000000, NULL, 3600, 'available', 5, 3),

-- Room 3: Vintage Collection
(3, 'Vintage Camera', 'Film camera from 1970s', 3000000, 4000000, 6000000, 3600, 'sold', 6, 1),
(3, 'Old Painting', '19th century art piece', 5000000, 6500000, NULL, 3600, 'sold', 7, 2),
(3, 'Antique Vase', 'Porcelain vase', 2000000, 3200000, NULL, 3600, 'sold', 8, 3);

-- ============================
-- Insert User Rooms (One room per user constraint)
-- ============================
INSERT INTO user_rooms (user_id, room_id, joined_at)
VALUES
(1, 1, NOW() - INTERVAL '30 MINUTES'),
(2, 1, NOW() - INTERVAL '25 MINUTES'),
(3, 1, NOW() - INTERVAL '20 MINUTES'),
(4, 1, NOW() - INTERVAL '15 MINUTES'),
(5, 2, NOW() - INTERVAL '20 HOURS'),
(6, 1, NOW() - INTERVAL '5 MINUTES'),
(7, 2, NOW() - INTERVAL '18 HOURS'),
(8, 2, NOW() - INTERVAL '16 HOURS'),
(9, 2, NOW() - INTERVAL '14 HOURS'),
(10, 2, NOW() - INTERVAL '12 HOURS');

-- ============================
-- Insert Bids
-- ============================

-- Room 1 bids
INSERT INTO bids (bid_amount, item_id, user_id, bid_time)
VALUES
(8500000, 1, 2, NOW() - INTERVAL '25 MINUTES'),
(9000000, 1, 3, NOW() - INTERVAL '20 MINUTES'),
(9500000, 1, 4, NOW() - INTERVAL '18 MINUTES'),

(9500000, 2, 2, NOW() - INTERVAL '10 MINUTES'),
(10000000, 2, 3, NOW() - INTERVAL '8 MINUTES'),

(7500000, 3, 5, NOW() - INTERVAL '5 MINUTES'),
(7800000, 3, 6, NOW() - INTERVAL '3 MINUTES');

-- Room 2 bids
INSERT INTO bids (bid_amount, item_id, user_id, bid_time)
VALUES
(52000000, 4, 7, NOW() - INTERVAL '20 HOURS'),
(54000000, 4, 7, NOW() - INTERVAL '19 HOURS'),

(23000000, 5, 8, NOW() - INTERVAL '16 HOURS'),
(25000000, 5, 9, NOW() - INTERVAL '15 HOURS'),

(16000000, 6, 10, NOW() - INTERVAL '12 HOURS'),
(17000000, 6, 7, NOW() - INTERVAL '11 HOURS');

-- Room 3 bids (Past auctions)
INSERT INTO bids (bid_amount, item_id, user_id, bid_time)
VALUES
(3500000, 7, 3, NOW() - INTERVAL '2 DAYS 18 HOURS'),
(4000000, 7, 4, NOW() - INTERVAL '2 DAYS 17 HOURS'),

(6000000, 8, 2, NOW() - INTERVAL '1 DAY 18 HOURS'),
(6500000, 8, 5, NOW() - INTERVAL '1 DAY 17 HOURS'),

(3000000, 9, 6, NOW() - INTERVAL '12 HOURS'),
(3200000, 9, 7, NOW() - INTERVAL '11 HOURS');

-- ============================
-- Insert Chat Messages
-- ============================
INSERT INTO chat_messages (room_id, user_id, message)
VALUES
(1, 2, 'Great items today!'),
(1, 3, 'Interested in the iPhone'),
(1, 4, 'Just won the iPhone 15!'),
(2, 5, 'Love this Rolex!'),
(2, 7, 'Anyone bidding on the bracelet?');

-- ============================
-- Insert Activity Logs
-- ============================
INSERT INTO activity_logs (user_id, action, details)
VALUES
(1, 'USER_LOGIN', 'Login successful'),
(2, 'USER_LOGIN', 'Login successful'),
(3, 'USER_LOGIN', 'Login successful'),
(1, 'ROOM_CREATE', 'Created room: Electronics Auction'),
(2, 'ROOM_CREATE', 'Created room: Luxury Items'),
(1, 'ITEM_CREATE', 'Created item: iPhone 15'),
(1, 'ITEM_CREATE', 'Created item: MacBook Air M2'),
(2, 'ITEM_CREATE', 'Created item: Samsung Galaxy S23'),
(2, 'BID_PLACED', 'Placed bid on iPhone 15: 8500000 VND'),
(3, 'BID_PLACED', 'Placed bid on iPhone 15: 9000000 VND'),
(4, 'BID_PLACED', 'Placed bid on iPhone 15: 9500000 VND (WINNING)'),
(4, 'ITEM_SOLD', 'Item sold to david via bid for 9500000 VND'),
(5, 'DEPOSIT', 'Deposited 30000000 VND to account'),
(2, 'CHAT_MESSAGE', 'Sent message in room 1');