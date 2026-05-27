#pragma once
#include <drogon/HttpController.h>

class BidController : public drogon::HttpController<BidController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(BidController::placeBid, "/api/bids", drogon::Post, "AuthFilter");
        ADD_METHOD_TO(BidController::getBidHistory, "/api/lots/{id}/bids/history", drogon::Get);
    METHOD_LIST_END

    void placeBid(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void getBidHistory(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback, std::string lotId);
};