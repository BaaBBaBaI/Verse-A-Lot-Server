#include "LotController.h"

void LotController::getAuctions(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    auto format = req->getParameter("type"); // online/offline
    auto db = drogon::app().getDbClient();
    
    std::string sql = "SELECT auction_id, title, type, start_time, end_time, status FROM Auctions";
    if (!format.empty()) {
        sql += " WHERE type = $1";
    }

    auto callbackSuccess = [callback](const drogon::orm::Result &result) {
        Json::Value arr(Json::arrayValue);
        for (auto const &row : result) {
            Json::Value item;
            item["auction_id"] = row["auction_id"].as<std::string>();
            item["title"] = row["title"].as<std::string>();
            item["type"] = row["type"].as<std::string>();
            item["start_time"] = row["start_time"].as<std::string>();
            item["end_time"] = row["end_time"].as<std::string>();
            item["status"] = row["status"].as<std::string>();
            arr.append(item);
        }
        callback(drogon::HttpResponse::newHttpJsonResponse(arr));
    };

    if (!format.empty()) {
        db->execSqlAsync(sql, callbackSuccess, [callback](const drogon::orm::DrogonDbException &e) { 
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp); 
        }, format);
    } else {
        db->execSqlAsync(sql, callbackSuccess, [callback](const drogon::orm::DrogonDbException &e) { 
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp); 
        });
    }
}

void LotController::createAuctionAdmin(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    auto role = req->attributes()->get<std::string>("role");
    if (role != "admin") {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k403Forbidden);
        callback(resp);
        return;
    }

    auto json = req->getJsonObject();
    if (!json) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    std::string title = (*json)["title"].asString();
    std::string type = (*json)["type"].asString();
    std::string startTime = (*json)["start_time"].asString();
    std::string endTime = (*json)["end_time"].asString();

    auto db = drogon::app().getDbClient();
    db->execSqlAsync(
        "INSERT INTO Auctions (title, type, start_time, end_time, status) VALUES ($1, $2, $3, $4, 'scheduled') RETURNING auction_id",
        [callback](const drogon::orm::Result &result) {
            Json::Value respJson;
            respJson["auction_id"] = result[0]["auction_id"].as<std::string>();
            callback(drogon::HttpResponse::newHttpJsonResponse(respJson));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
        },
        title, type, startTime, endTime
    );
}

void LotController::getLots(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    auto db = drogon::app().getDbClient();
    db->execSqlAsync(
        "SELECT lot_id, auction_id, seller_id, winner_id, title, description, start_price, reserve_price, current_price, status FROM Lots WHERE status = 'approved'",
        [callback](const drogon::orm::Result &result) {
            Json::Value arr(Json::arrayValue);
            for (auto const &row : result) {
                Json::Value item;
                item["lot_id"] = row["lot_id"].as<std::string>();
                item["auction_id"] = row["auction_id"].as<std::string>();
                item["seller_id"] = row["seller_id"].as<std::string>();
                item["winner_id"] = row["winner_id"].isNull() ? Json::Value(Json::nullValue) : row["winner_id"].as<std::string>();
                item["title"] = row["title"].as<std::string>();
                item["description"] = row["description"].as<std::string>();
                item["start_price"] = row["start_price"].as<double>();
                item["reserve_price"] = row["reserve_price"].as<double>();
                item["current_price"] = row["current_price"].as<double>();
                item["status"] = row["status"].as<std::string>();
                arr.append(item);
            }
            callback(drogon::HttpResponse::newHttpJsonResponse(arr));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
        }
    );
}

void LotController::createLot(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    auto userId = req->attributes()->get<std::string>("user_id");
    auto json = req->getJsonObject();
    if (!json) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    std::string auctionId = json->isMember("auction_id") ? (*json)["auction_id"].asString() : "";
    std::string title = (*json)["title"].asString();
    std::string description = (*json)["description"].asString();
    double startPrice = (*json)["start_price"].asDouble();
    double reservePrice = (*json)["reserve_price"].asDouble();

    auto db = drogon::app().getDbClient();
    auto insertLot = [db, callback, userId, title, description, startPrice, reservePrice](const std::string &aucId) {
        db->execSqlAsync(
            "INSERT INTO Lots (auction_id, seller_id, title, description, start_price, reserve_price, current_price, status) "
            "VALUES ($1, $2, $3, $4, $5, $6, $5, 'moderation') RETURNING lot_id",
            [callback](const drogon::orm::Result &result) {
                Json::Value respJson;
                respJson["lot_id"] = result[0]["lot_id"].as<std::string>();
                respJson["status"] = "moderation";
                callback(drogon::HttpResponse::newHttpJsonResponse(respJson));
            },
            [callback](const drogon::orm::DrogonDbException &) {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k500InternalServerError);
                callback(resp);
            },
            aucId, userId, title, description, startPrice, reservePrice
        );
    };

    if (auctionId.empty()) {
        db->execSqlAsync(
            "SELECT auction_id FROM Auctions WHERE status = 'active' AND type = 'online' LIMIT 1",
            [insertLot, callback, db](const drogon::orm::Result &result) {
                if (!result.empty()) {
                    insertLot(result[0]["auction_id"].as<std::string>());
                } else {
                    db->execSqlAsync(
                        "SELECT auction_id FROM Auctions LIMIT 1",
                        [insertLot, callback](const drogon::orm::Result &res2) {
                            if (!res2.empty()) {
                                insertLot(res2[0]["auction_id"].as<std::string>());
                            } else {
                                auto resp = drogon::HttpResponse::newHttpResponse();
                                resp->setStatusCode(drogon::k500InternalServerError);
                                callback(resp);
                            }
                        },
                        [callback](const drogon::orm::DrogonDbException &) {
                            auto resp = drogon::HttpResponse::newHttpResponse();
                            resp->setStatusCode(drogon::k500InternalServerError);
                            callback(resp);
                        }
                    );
                }
            },
            [callback](const drogon::orm::DrogonDbException &) {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k500InternalServerError);
                callback(resp);
            }
        );
    } else {
        insertLot(auctionId);
    }
}

void LotController::moderateLotAdmin(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback, std::string lotId) {
    auto role = req->attributes()->get<std::string>("role");
    if (role != "admin") {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k403Forbidden);
        callback(resp);
        return;
    }

    auto json = req->getJsonObject();
    if (!json || !json->isMember("status")) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }
    std::string status = (*json)["status"].asString();

    auto db = drogon::app().getDbClient();
    db->execSqlAsync(
        "UPDATE Lots SET status = $1 WHERE lot_id = $2 RETURNING lot_id",
        [callback, status](const drogon::orm::Result &result) {
            if (result.empty()) {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k404NotFound);
                callback(resp);
                return;
            }
            Json::Value respJson;
            respJson["lot_id"] = result[0]["lot_id"].as<std::string>();
            respJson["status"] = status;
            callback(drogon::HttpResponse::newHttpJsonResponse(respJson));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
        },
        status, lotId
    );
}

void LotController::getLotsAdmin(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    auto role = req->attributes()->get<std::string>("role");
    auto userId = req->attributes()->get<std::string>("user_id");
    auto db = drogon::app().getDbClient();

    std::string sql;
    if (role == "admin") {
        sql = "SELECT lot_id, auction_id, seller_id, winner_id, title, description, start_price, reserve_price, current_price, status FROM Lots";
    } else {
        sql = "SELECT lot_id, auction_id, seller_id, winner_id, title, description, start_price, reserve_price, current_price, status FROM Lots WHERE seller_id = $1";
    }

    auto callbackSuccess = [callback](const drogon::orm::Result &result) {
        Json::Value arr(Json::arrayValue);
        for (auto const &row : result) {
            Json::Value item;
            item["lot_id"] = row["lot_id"].as<std::string>();
            item["auction_id"] = row["auction_id"].as<std::string>();
            item["seller_id"] = row["seller_id"].as<std::string>();
            item["winner_id"] = row["winner_id"].isNull() ? Json::Value(Json::nullValue) : row["winner_id"].as<std::string>();
            item["title"] = row["title"].as<std::string>();
            item["description"] = row["description"].as<std::string>();
            item["start_price"] = row["start_price"].as<double>();
            item["reserve_price"] = row["reserve_price"].as<double>();
            item["current_price"] = row["current_price"].as<double>();
            item["status"] = row["status"].as<std::string>();
            arr.append(item);
        }
        callback(drogon::HttpResponse::newHttpJsonResponse(arr));
    };

    auto callbackFail = [callback](const drogon::orm::DrogonDbException &e) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k500InternalServerError);
        callback(resp);
    };

    if (role == "admin") {
        db->execSqlAsync(sql, callbackSuccess, callbackFail);
    } else {
        db->execSqlAsync(sql, callbackSuccess, callbackFail, userId);
    }
}

void LotController::updateLot(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback, std::string lotId) {
    auto userId = req->attributes()->get<std::string>("user_id");
    auto role = req->attributes()->get<std::string>("role");
    
    auto json = req->getJsonObject();
    if (!json) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    std::string title = (*json)["title"].asString();
    std::string description = (*json)["description"].asString();
    double startPrice = (*json)["start_price"].asDouble();
    double reservePrice = (*json)["reserve_price"].asDouble();

    auto db = drogon::app().getDbClient();
    db->execSqlAsync(
        "SELECT seller_id, status FROM Lots WHERE lot_id = $1",
        [db, callback, lotId, title, description, startPrice, reservePrice, userId, role](const drogon::orm::Result &result) {
            if (result.empty()) {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k404NotFound);
                callback(resp);
                return;
            }

            auto row = result[0];
            std::string sellerId = row["seller_id"].as<std::string>();
            std::string status = row["status"].as<std::string>();

            if (role != "admin" && (sellerId != userId || status != "moderation")) {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k403Forbidden);
                callback(resp);
                return;
            }

            db->execSqlAsync(
                "UPDATE Lots SET title = $1, description = $2, start_price = $3, reserve_price = $4 WHERE lot_id = $5",
                [callback, lotId](const drogon::orm::Result &) {
                    Json::Value respJson;
                    respJson["status"] = "success";
                    respJson["lot_id"] = lotId;
                    callback(drogon::HttpResponse::newHttpJsonResponse(respJson));
                },
                [callback](const drogon::orm::DrogonDbException &) {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k500InternalServerError);
                    callback(resp);
                },
                title, description, startPrice, reservePrice, lotId
            );
        },
        [callback](const drogon::orm::DrogonDbException &) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
        },
        lotId
    );
}

void LotController::deleteLot(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback, std::string lotId) {
    auto userId = req->attributes()->get<std::string>("user_id");
    auto role = req->attributes()->get<std::string>("role");

    auto db = drogon::app().getDbClient();
    db->execSqlAsync(
        "SELECT seller_id, status FROM Lots WHERE lot_id = $1",
        [db, callback, lotId, userId, role](const drogon::orm::Result &result) {
            if (result.empty()) {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k404NotFound);
                callback(resp);
                return;
            }

            auto row = result[0];
            std::string sellerId = row["seller_id"].as<std::string>();
            std::string status = row["status"].as<std::string>();

            if (role != "admin" && (sellerId != userId || status != "moderation")) {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k403Forbidden);
                callback(resp);
                return;
            }

            db->execSqlAsync(
                "DELETE FROM Lots WHERE lot_id = $1",
                [callback, lotId](const drogon::orm::Result &) {
                    Json::Value respJson;
                    respJson["status"] = "success";
                    respJson["deleted_lot_id"] = lotId;
                    callback(drogon::HttpResponse::newHttpJsonResponse(respJson));
                },
                [callback](const drogon::orm::DrogonDbException &) {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k500InternalServerError);
                    callback(resp);
                },
                lotId
            );
        },
        [callback](const drogon::orm::DrogonDbException &) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
        },
        lotId
    );
}