CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

CREATE TABLE Users (
    user_id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    username VARCHAR(100) UNIQUE NOT NULL,
    email VARCHAR(255) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    role VARCHAR(50) NOT NULL, -- 'user', 'admin'
    balance DECIMAL(12,2) NOT NULL DEFAULT 0.00 CHECK (balance >= 0),
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE Auctions (
    auction_id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    title VARCHAR(255) NOT NULL,
    type VARCHAR(50) NOT NULL, -- 'online', 'offline'
    start_time TIMESTAMP NOT NULL,
    end_time TIMESTAMP NOT NULL,
    status VARCHAR(50) NOT NULL -- 'scheduled', 'active', 'completed'
);

CREATE TABLE Lots (
    lot_id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    auction_id UUID NOT NULL REFERENCES Auctions(auction_id) ON DELETE CASCADE,
    seller_id UUID NOT NULL REFERENCES Users(user_id) ON DELETE CASCADE,
    winner_id UUID REFERENCES Users(user_id) ON DELETE SET NULL, -- nullable
    title VARCHAR(255) NOT NULL,
    description TEXT,
    start_price DECIMAL(12,2) NOT NULL CHECK (start_price >= 0),
    reserve_price DECIMAL(12,2) NOT NULL CHECK (reserve_price >= 0),
    current_price DECIMAL(12,2) NOT NULL CHECK (current_price >= 0),
    status VARCHAR(50) NOT NULL DEFAULT 'moderation' -- 'moderation', 'approved', 'rejected', 'active', 'sold', 'expired'
);

CREATE TABLE Images (
    image_id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    lot_id UUID NOT NULL REFERENCES Lots(lot_id) ON DELETE CASCADE,
    image_url VARCHAR(512) NOT NULL,
    is_primary BOOLEAN NOT NULL DEFAULT false
);

CREATE TABLE Transactions (
    transaction_id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id UUID NOT NULL REFERENCES Users(user_id) ON DELETE CASCADE,
    lot_id UUID REFERENCES Lots(lot_id) ON DELETE SET NULL, -- nullable
    amount DECIMAL(12,2) NOT NULL,
    type VARCHAR(50) NOT NULL, -- 'deposit', 'hold', 'payment'
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);


CREATE TABLE Bids (
    bid_id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    lot_id UUID NOT NULL REFERENCES Lots(lot_id) ON DELETE CASCADE,
    user_id UUID NOT NULL REFERENCES Users(user_id) ON DELETE CASCADE,
    amount DECIMAL(12,2) NOT NULL CHECK (amount > 0),
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_lots_auction ON Lots(auction_id);
CREATE INDEX idx_bids_lot ON Bids(lot_id);
CREATE INDEX idx_transactions_user ON Transactions(user_id);