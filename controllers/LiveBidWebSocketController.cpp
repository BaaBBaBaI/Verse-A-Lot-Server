#include "LiveBidWebSocketController.h"
#include "../utils/WebSocketManager.h"

void LiveBidWebSocketController::handleNewConnection(const drogon::HttpRequestPtr &req, const drogon::WebSocketConnectionPtr &conn) {
    std::string path = req->path();
    std::string prefix = "/api/lots/";
    std::string suffix = "/live";
    
    std::string lotId = "";
    auto startPos = path.find(prefix);
    if (startPos != std::string::npos) {
        startPos += prefix.length();
        auto endPos = path.find(suffix, startPos);
        if (endPos != std::string::npos) {
            lotId = path.substr(startPos, endPos - startPos);
        }
    }

    if (!lotId.empty()) {
        conn->setContext(std::make_shared<std::string>(lotId));
        WebSocketManager::instance().subscribe(lotId, conn);
    } else {
        conn->forceClose();
    }
}

void LiveBidWebSocketController::handleNewMessage(const drogon::WebSocketConnectionPtr &conn, std::string &&message, const drogon::WebSocketMessageType &type) {
    // Входящие сообщения от клиентов по данному каналу не обрабатываются (только односторонняя подписка)
}

void LiveBidWebSocketController::handleConnectionClosed(const drogon::WebSocketConnectionPtr &conn) {
    auto lotIdPtr = conn->getContext<std::string>();
    if (lotIdPtr) {
        WebSocketManager::instance().unsubscribe(*lotIdPtr, conn);
    }
}