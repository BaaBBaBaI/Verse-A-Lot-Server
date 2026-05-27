#pragma once
#include <drogon/WebSocketController.h>
#include <shared_mutex>
#include <unordered_set>
#include <unordered_map>

class WebSocketManager {
public:
    static WebSocketManager& instance() {
        static WebSocketManager inst;
        return inst;
    }

    void subscribe(const std::string& lotId, const drogon::WebSocketConnectionPtr& conn) {
        std::unique_lock lock(mutex_);
        subscriptions_[lotId].insert(conn);
    }

    void unsubscribe(const std::string& lotId, const drogon::WebSocketConnectionPtr& conn) {
        std::unique_lock lock(mutex_);
        auto it = subscriptions_.find(lotId);
        if (it != subscriptions_.end()) {
            it->second.erase(conn);
            if (it->second.empty()) {
                subscriptions_.erase(it);
            }
        }
    }

    void broadcast(const std::string& lotId, const std::string& message) {
        std::shared_lock lock(mutex_);
        auto it = subscriptions_.find(lotId);
        if (it != subscriptions_.end()) {
            for (auto& conn : it->second) {
                if (conn->connected()) {
                    conn->send(message);
                }
            }
        }
    }

private:
    WebSocketManager() = default;
    std::shared_mutex mutex_;
    std::unordered_map<std::string, std::unordered_set<drogon::WebSocketConnectionPtr>> subscriptions_;
};