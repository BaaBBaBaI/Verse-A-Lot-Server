#pragma once
#include <drogon/HttpController.h>

class LotController : public drogon::HttpController<LotController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(LotController::getAuctions, "/api/auctions", drogon::Get);
        ADD_METHOD_TO(LotController::createAuctionAdmin, "/api/admin/auctions", drogon::Post, "AuthFilter");
        ADD_METHOD_TO(LotController::getLots, "/api/lots", drogon::Get);
        ADD_METHOD_TO(LotController::createLot, "/api/lots", drogon::Post, "AuthFilter");
        ADD_METHOD_TO(LotController::moderateLotAdmin, "/api/admin/lots/{id}/moderate", drogon::Put, "AuthFilter");
        ADD_METHOD_TO(LotController::getLotsAdmin, "/api/admin/lots", drogon::Get, "AuthFilter");
        ADD_METHOD_TO(LotController::updateLot, "/api/lots/{id}", drogon::Put, "AuthFilter");
        ADD_METHOD_TO(LotController::deleteLot, "/api/lots/{id}", drogon::Delete, "AuthFilter");
    METHOD_LIST_END

    void getAuctions(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void createAuctionAdmin(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void getLots(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void createLot(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void moderateLotAdmin(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback, std::string lotId);
    void getLotsAdmin(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void updateLot(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback, std::string lotId);
    void deleteLot(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback, std::string lotId);
};