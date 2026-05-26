#pragma once
#include <drogon/WebSocketController.h>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <jsoncpp/json/json.h>

using namespace drogon;

class LiveBidWSController : public WebSocketController<LiveBidWSController> {
public:
    LiveBidWSController() = default;

    void handleNewMessage(const WebSocketConnectionPtr &, std::string &&, const WebSocketMessageType &) override;
    void handleNewConnection(const HttpRequestPtr &, const WebSocketConnectionPtr &) override;
    void handleConnectionClosed(const WebSocketConnectionPtr &) override;

    static void broadcastNewBid(const std::string &lotId, const std::string &winnerId, double amount);

    // Специфичные макросы путей для WebSocket-контроллеров в Drogon
    WS_PATH_LIST_BEGIN
        WS_PATH_ADD("/api/lots/{id}/live");
    WS_PATH_LIST_END

private:
    static std::unordered_map<std::string, std::unordered_set<WebSocketConnectionPtr>> subscribers_;
    static std::mutex mutex_;
};