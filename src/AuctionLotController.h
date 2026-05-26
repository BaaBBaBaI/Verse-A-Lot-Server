#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

class AuctionLotController : public HttpController<AuctionLotController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(AuctionLotController::getAuctions, "/api/auctions", Get);
        ADD_METHOD_TO(AuctionLotController::createAuction, "/api/admin/auctions", Post);
        ADD_METHOD_TO(AuctionLotController::getLots, "/api/lots", Get);
        ADD_METHOD_TO(AuctionLotController::getLotById, "/api/lots/{id}", Get);
        ADD_METHOD_TO(AuctionLotController::createLot, "/api/lots", Post);
        ADD_METHOD_TO(AuctionLotController::moderateLot, "/api/admin/lots/{id}/moderate", Put);
        ADD_METHOD_TO(AuctionLotController::deleteLot, "/api/lots/{id}", Delete);
    METHOD_LIST_END

    void getAuctions(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void createAuction(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getLots(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getLotById(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string lotId);
    void createLot(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void moderateLot(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string lotId);
    void deleteLot(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string lotId);
};