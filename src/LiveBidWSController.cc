#include "LiveBidWSController.h"

std::unordered_map<std::string, std::unordered_set<WebSocketConnectionPtr>> LiveBidWSController::subscribers_;
std::mutex LiveBidWSController::mutex_;

void LiveBidWSController::handleNewConnection(const HttpRequestPtr &req, const WebSocketConnectionPtr &conn) {
    auto lotId = req->getParameter("id");
    if (lotId.empty()) {
        conn->forceClose();
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    subscribers_[lotId].insert(conn);
    conn->setContext(std::make_shared<std::string>(lotId));
}

void LiveBidWSController::handleConnectionClosed(const WebSocketConnectionPtr &conn) {
    auto context = conn->getContext<std::string>();
    if (context) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string lotId = *context;
        subscribers_[lotId].erase(conn);
        if (subscribers_[lotId].empty()) {
            subscribers_.erase(lotId);
        }
    }
}

void LiveBidWSController::handleNewMessage(const WebSocketConnectionPtr &conn, std::string &&message, const WebSocketMessageType &type) {
    // Входящие сообщения игнорируются
}

void LiveBidWSController::broadcastNewBid(const std::string &lotId, const std::string &winnerId, double amount) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = subscribers_.find(lotId);
    if (it != subscribers_.end()) {
        Json::Value updateMsg;
        updateMsg["type"] = "new_bid";
        updateMsg["lot_id"] = lotId;
        updateMsg["winner_id"] = winnerId;
        updateMsg["current_price"] = amount;

        Json::FastWriter writer;
        std::string payload = writer.write(updateMsg);

        for (auto const &conn : it->second) {
            conn->send(payload);
        }
    }
}