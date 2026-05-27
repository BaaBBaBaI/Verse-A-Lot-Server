#pragma once
#include <drogon/WebSocketController.h>

class LiveBidWebSocketController : public drogon::WebSocketController<LiveBidWebSocketController> {
public:
    void handleNewMessage(const drogon::WebSocketConnectionPtr &, std::string &&, const drogon::WebSocketMessageType &) override;
    void handleNewConnection(const drogon::HttpRequestPtr &, const drogon::WebSocketConnectionPtr &) override;
    void handleConnectionClosed(const drogon::WebSocketConnectionPtr &) override;

    WS_PATH_LIST_BEGIN
        WS_PATH_ADD("/api/lots/{id}/live");
    WS_PATH_LIST_END
};