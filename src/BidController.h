#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

class BidController : public HttpController<BidController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(BidController::placeBid, "/api/bids", Post);
        ADD_METHOD_TO(BidController::getBidHistory, "/api/lots/{id}/bids/history", Get);
    METHOD_LIST_END

    void placeBid(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getBidHistory(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string lotId);
};