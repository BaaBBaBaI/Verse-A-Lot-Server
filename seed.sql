-- Truncate existing data to start with a clean state
TRUNCATE TABLE Bids, Transactions, Images, Lots, Auctions, Users CASCADE;

-- 1. Insert Users (SHA-256 password hash for 'password123' and 'admin123')
-- password123 hash: ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f
-- admin123 hash: 240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9
INSERT INTO Users (user_id, username, email, password_hash, role, balance, created_at) VALUES
('58a9c9ef-7689-4839-ac70-874f034cf92e', 'auction_winner', 'winner@example.com', 'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f', 'user', 15000.50, '2026-05-26 12:00:00'),
('99a9c9ef-7689-4839-ac70-874f034cf92e', 'auction_seller', 'seller@example.com', 'ef92b778bafe771e89245b89ecbc08a44a4e166c06659911881f383d4473e94f', 'user', 1000.00, '2026-05-26 12:00:00'),
('e8b9c9ef-7689-4839-ac70-874f034cf92e', 'admin', 'admin@example.com', '240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9', 'admin', 1000000.00, '2026-05-26 12:00:00');

-- 2. Insert Auctions (Online and Offline)
INSERT INTO Auctions (auction_id, title, type, start_time, end_time, status) VALUES
('58a9c9ef-7689-4839-ac70-874f034cf92e', 'Первый гибридный аукцион', 'online', '2026-05-26 19:19:14.90797', '2026-05-27 19:19:14.90797', 'active'),
('a1b2c3d4-e5f6-7a8b-9c0d-1e2f3a4b5c6d', 'Ретро Аукцион Автомобилей', 'offline', '2026-06-01 12:00:00', '2026-06-01 18:00:00', 'scheduled'),
('b2c3d4e5-f6a7-8b9c-0d1e-2f3a4b5c6d7e', 'Императорский фарфор и серебро', 'offline', '2026-06-15 10:00:00', '2026-06-15 16:00:00', 'scheduled'),
('c3d4e5f6-a7b8-9c0d-1e2f-3a4b5c6d7e8f', 'Редкие букинистические издания', 'offline', '2026-06-20 11:00:00', '2026-06-20 17:00:00', 'scheduled');

-- 3. Insert Lots
INSERT INTO Lots (lot_id, auction_id, seller_id, winner_id, title, description, start_price, reserve_price, current_price, status) VALUES
('77a8c9ef-7689-4839-ac70-874f034cf92e', '58a9c9ef-7689-4839-ac70-874f034cf92e', '99a9c9ef-7689-4839-ac70-874f034cf92e', '58a9c9ef-7689-4839-ac70-874f034cf92e', 'Картина неизвестного художника', 'Холст, масло, 19 век.', 5000.00, 8000.00, 5500.00, 'approved'),
('88b9c9ef-7689-4839-ac70-874f034cf92e', '58a9c9ef-7689-4839-ac70-874f034cf92e', '99a9c9ef-7689-4839-ac70-874f034cf92e', NULL, 'Золотые карманные часы', 'Швейцария, начало 20-го века, исправны.', 12000.00, 18000.00, 12000.00, 'moderation'),
('f1a8c9ef-7689-4839-ac70-874f034cf92e', 'a1b2c3d4-e5f6-7a8b-9c0d-1e2f3a4b5c6d', '99a9c9ef-7689-4839-ac70-874f034cf92e', NULL, '1957 Chevrolet Bel Air', 'Оригинальный двигатель V8, идеальное состояние.', 45000.00, 60000.00, 45000.00, 'approved'),
('f2a8c9ef-7689-4839-ac70-874f034cf92e', 'b2c3d4e5-f6a7-8b9c-0d1e-2f3a4b5c6d7e', '99a9c9ef-7689-4839-ac70-874f034cf92e', NULL, 'Чайный сервиз Фаберже', 'Серебро 84 пробы, позолота, эмаль. Конец 19 века.', 15000.00, 22000.00, 15000.00, 'approved'),
('f3a8c9ef-7689-4839-ac70-874f034cf92e', 'c3d4e5f6-a7b8-9c0d-1e2f-3a4b5c6d7e8f', '99a9c9ef-7689-4839-ac70-874f034cf92e', NULL, 'Прижизненное издание А.С. Пушкина', '«Евгений Онегин», Санкт-Петербург, 1837 год.', 8000.00, 12000.00, 8000.00, 'approved');

-- 4. Insert Images for Lots
INSERT INTO Images (image_id, lot_id, image_url, is_primary) VALUES
(uuid_generate_v4(), '77a8c9ef-7689-4839-ac70-874f034cf92e', 'https://example.com/images/painting1.jpg', TRUE),
(uuid_generate_v4(), '88b9c9ef-7689-4839-ac70-874f034cf92e', 'https://example.com/images/watch1.jpg', TRUE),
(uuid_generate_v4(), 'f1a8c9ef-7689-4839-ac70-874f034cf92e', 'https://example.com/images/car1.jpg', TRUE);

-- 5. Insert Bids
INSERT INTO Bids (bid_id, lot_id, user_id, amount, created_at) VALUES
('b1a9c9ef-7689-4839-ac70-874f034cf92e', '77a8c9ef-7689-4839-ac70-874f034cf92e', '58a9c9ef-7689-4839-ac70-874f034cf92e', 5000.00, '2026-05-26 18:22:45'),
('b2a9c9ef-7689-4839-ac70-874f034cf92e', '77a8c9ef-7689-4839-ac70-874f034cf92e', '58a9c9ef-7689-4839-ac70-874f034cf92e', 5500.00, '2026-05-26 18:30:12');

-- 6. Insert Transactions
INSERT INTO Transactions (transaction_id, user_id, lot_id, amount, type, created_at) VALUES
('e9c2a3b4-7689-4839-ac70-874f034cf92e', '58a9c9ef-7689-4839-ac70-874f034cf92e', NULL, 5000.00, 'deposit', '2026-05-26 18:22:45');
