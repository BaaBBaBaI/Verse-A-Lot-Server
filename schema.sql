CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

CREATE TABLE Users (
    user_id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    username VARCHAR(255) NOT NULL UNIQUE,
    email VARCHAR(255) NOT NULL UNIQUE,
    password_hash VARCHAR(255) NOT NULL,
    role VARCHAR(50) NOT NULL DEFAULT 'user', -- 'user', 'admin'
    balance DECIMAL(12, 2) NOT NULL DEFAULT 0.00 CHECK (balance >= 0.00),
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    is_blocked BOOLEAN NOT NULL DEFAULT FALSE
);

CREATE TABLE Auctions (
    auction_id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    title VARCHAR(255) NOT NULL,
    type VARCHAR(50) NOT NULL, -- 'online', 'offline'
    start_time TIMESTAMP NOT NULL,
    end_time TIMESTAMP NOT NULL,
    status VARCHAR(50) NOT NULL DEFAULT 'scheduled' -- 'scheduled', 'active', 'ended'
);

CREATE TABLE Lots (
    lot_id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    auction_id UUID NOT NULL REFERENCES Auctions(auction_id) ON DELETE CASCADE,
    seller_id UUID NOT NULL REFERENCES Users(user_id) ON DELETE CASCADE,
    winner_id UUID REFERENCES Users(user_id) ON DELETE SET NULL,
    title VARCHAR(255) NOT NULL,
    description TEXT,
    start_price DECIMAL(12, 2) NOT NULL DEFAULT 0.00 CHECK (start_price >= 0.00),
    reserve_price DECIMAL(12, 2) NOT NULL DEFAULT 0.00 CHECK (reserve_price >= 0.00),
    current_price DECIMAL(12, 2) NOT NULL DEFAULT 0.00 CHECK (current_price >= 0.00),
    status VARCHAR(50) NOT NULL DEFAULT 'pending_moderation' -- 'pending_moderation', 'approved', 'rejected', 'active', 'sold', 'unsold'
);

CREATE TABLE Images (
    image_id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    lot_id UUID NOT NULL REFERENCES Lots(lot_id) ON DELETE CASCADE,
    image_url VARCHAR(512) NOT NULL,
    is_primary BOOLEAN NOT NULL DEFAULT FALSE
);

CREATE TABLE Bids (
    bid_id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    lot_id UUID NOT NULL REFERENCES Lots(lot_id) ON DELETE CASCADE,
    user_id UUID NOT NULL REFERENCES Users(user_id) ON DELETE CASCADE,
    amount DECIMAL(12, 2) NOT NULL CHECK (amount > 0.00),
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE Transactions (
    transaction_id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID NOT NULL REFERENCES Users(user_id) ON DELETE CASCADE,
    lot_id UUID REFERENCES Lots(lot_id) ON DELETE SET NULL,
    amount DECIMAL(12, 2) NOT NULL,
    type VARCHAR(50) NOT NULL, -- 'deposit', 'hold', 'payment', 'refund'
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_users_username ON Users(username);
CREATE INDEX idx_lots_status ON Lots(status);
CREATE INDEX idx_lots_auction ON Lots(auction_id);
CREATE INDEX idx_bids_lot_timestamp ON Bids(lot_id, created_at DESC);
CREATE INDEX idx_transactions_user ON Transactions(user_id);