#include "BidController.h"
#include "../utils/WebSocketManager.h"

void BidController::placeBid(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    auto userId = req->attributes()->get<std::string>("user_id");
    auto json = req->getJsonObject();
    if (!json) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    std::string lotId = (*json)["lot_id"].asString();
    double bidAmount = (*json)["amount"].asDouble();

    auto db = drogon::app().getDbClient();

    db->newTransactionAsync([callback, userId, lotId, bidAmount](const std::shared_ptr<drogon::orm::Transaction> &trans) {
        if (!trans) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        // Шаг 1: Блокировка баланса пользователя
        trans->execSqlAsync(
            "SELECT balance FROM Users WHERE user_id = $1 FOR UPDATE",
            [trans, callback, userId, lotId, bidAmount](const drogon::orm::Result &userRes) {
                if (userRes.empty()) {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k404NotFound);
                    callback(resp);
                    return;
                }

                double balance = userRes[0]["balance"].as<double>();
                if (balance < bidAmount) {
                    Json::Value errorJson;
                    errorJson["error"] = "Insufficient funds";
                    auto resp = drogon::HttpResponse::newHttpJsonResponse(errorJson);
                    resp->setStatusCode(drogon::k400BadRequest);
                    callback(resp);
                    return;
                }

                // Шаг 2: Получение и блокировка лота для сверки цен
                trans->execSqlAsync(
                    "SELECT current_price, status, seller_id FROM Lots WHERE lot_id = $1 FOR UPDATE",
                    [trans, callback, userId, lotId, bidAmount](const drogon::orm::Result &lotRes) {
                        if (lotRes.empty()) {
                            auto resp = drogon::HttpResponse::newHttpResponse();
                            resp->setStatusCode(drogon::k404NotFound);
                            callback(resp);
                            return;
                        }

                        auto lotRow = lotRes[0];
                        double currentPrice = lotRow["current_price"].as<double>();
                        std::string sellerId = lotRow["seller_id"].as<std::string>();

                        if (userId == sellerId) {
                            Json::Value errorJson;
                            errorJson["error"] = "Seller cannot bid on their own lot";
                            auto resp = drogon::HttpResponse::newHttpJsonResponse(errorJson);
                            resp->setStatusCode(drogon::k400BadRequest);
                            callback(resp);
                            return;
                        }

                        if (bidAmount <= currentPrice) {
                            Json::Value errorJson;
                            errorJson["error"] = "Bid must be higher than current price";
                            auto resp = drogon::HttpResponse::newHttpJsonResponse(errorJson);
                            resp->setStatusCode(drogon::k400BadRequest);
                            callback(resp);
                            return;
                        }

                        // Шаг 3: Вставка записи о ставке
                        trans->execSqlAsync(
                            "INSERT INTO Bids (lot_id, user_id, amount) VALUES ($1, $2, $3)",
                            [trans, callback, userId, lotId, bidAmount](const drogon::orm::Result &) {
                                // Шаг 4: Обновление текущей цены лота и определение лидера
                                trans->execSqlAsync(
                                    "UPDATE Lots SET current_price = $1, winner_id = $2 WHERE lot_id = $3",
                                    [trans, callback, userId, lotId, bidAmount](const drogon::orm::Result &) {
                                        
                                        // Рассылка по WebSockets
                                        Json::Value eventJson;
                                        eventJson["event"] = "new_bid";
                                        eventJson["lot_id"] = lotId;
                                        eventJson["current_price"] = bidAmount;
                                        eventJson["winner_id"] = userId;
                                        
                                        WebSocketManager::instance().broadcast(lotId, eventJson.toStyledString());

                                        Json::Value successJson;
                                        successJson["status"] = "success";
                                        callback(drogon::HttpResponse::newHttpJsonResponse(successJson));
                                        
                                        // Транзакция сделает автоматический коммит здесь
                                    },
                                    [callback](const drogon::orm::DrogonDbException &) {
                                        auto resp = drogon::HttpResponse::newHttpResponse();
                                        resp->setStatusCode(drogon::k500InternalServerError);
                                        callback(resp);
                                    },
                                    bidAmount, userId, lotId
                                );
                            },
                            [callback](const drogon::orm::DrogonDbException &) {
                                auto resp = drogon::HttpResponse::newHttpResponse();
                                resp->setStatusCode(drogon::k500InternalServerError);
                                callback(resp);
                            },
                            lotId, userId, bidAmount
                        );
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
            userId
        );
    });
}

void BidController::getBidHistory(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback, std::string lotId) {
    auto db = drogon::app().getDbClient();
    db->execSqlAsync(
        "SELECT amount, created_at FROM Bids WHERE lot_id = $1 ORDER BY created_at ASC",
        [callback](const drogon::orm::Result &result) {
            Json::Value arr(Json::arrayValue);
            for (auto const &row : result) {
                Json::Value item;
                item["amount"] = row["amount"].as<double>();
                item["created_at"] = row["created_at"].as<std::string>();
                arr.append(item);
            }
            callback(drogon::HttpResponse::newHttpJsonResponse(arr));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
        },
        lotId
    );
}