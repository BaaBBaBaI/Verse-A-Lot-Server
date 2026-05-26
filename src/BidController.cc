#include "BidController.h"
#include "JwtHelper.h"
#include "LiveBidWSController.h"

static Json::Value makeError(const std::string &msg) {
    Json::Value err;
    err["error"] = msg;
    return err;
}

static void rollbackTx(const std::shared_ptr<drogon::orm::Transaction> &trans, const std::string &reason, const std::function<void(const HttpResponsePtr &)> &callback, HttpStatusCode code = k400BadRequest) {
    trans->rollback();
    auto resp = HttpResponse::newHttpJsonResponse(makeError(reason));
    resp->setStatusCode(code);
    callback(resp);
}

void BidController::placeBid(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    auto auth = JwtHelper::verifyToken(req->getHeader("Authorization"));
    if (!auth) {
        auto resp = HttpResponse::newHttpJsonResponse(makeError("Unauthorized"));
        resp->setStatusCode(k401Unauthorized);
        callback(resp);
        return;
    }

    auto json = req->getJsonObject();
    if (!json || !json->isMember("lot_id") || !json->isMember("amount")) {
        auto resp = HttpResponse::newHttpJsonResponse(makeError("Missing parameters"));
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    std::string lotId = (*json)["lot_id"].asString();
    double bidAmount = (*json)["amount"].asDouble();
    std::string userId = auth->userId;

    auto db = app().getDbClient();
    db->newTransactionAsync([lotId, userId, bidAmount, callback](const std::shared_ptr<drogon::orm::Transaction> &trans) {
        
        // Устанавливаем коллбек, который сработает автоматически при уничтожении `trans` и успешном COMMIT
        trans->setCommitCallback([lotId, userId, bidAmount, callback](bool success) {
            if (success) {
                LiveBidWSController::broadcastNewBid(lotId, userId, bidAmount);
                Json::Value ok;
                ok["status"] = "success";
                ok["current_price"] = bidAmount;
                callback(HttpResponse::newHttpJsonResponse(ok));
            } else {
                auto resp = HttpResponse::newHttpJsonResponse(makeError("Database commit failed"));
                resp->setStatusCode(k500InternalServerError);
                callback(resp);
            }
        });

        // Блокируем запись лота (SELECT FOR UPDATE)
        trans->execSqlAsync(
            "SELECT seller_id, winner_id, current_price, status FROM Lots WHERE lot_id = $1 FOR UPDATE",
            [trans, lotId, userId, bidAmount, callback](const drogon::orm::Result &lotRes) {
                if (lotRes.empty()) {
                    rollbackTx(trans, "Lot not found", callback, k404NotFound);
                    return;
                }

                auto lotRow = lotRes[0];
                std::string status = lotRow["status"].as<std::string>();
                std::string sellerId = lotRow["seller_id"].as<std::string>();
                std::string prevWinnerId = lotRow["winner_id"].isNull() ? "" : lotRow["winner_id"].as<std::string>();
                double currentPrice = lotRow["current_price"].as<double>();

                if (status != "active") {
                    rollbackTx(trans, "Lot is not active for bidding", callback);
                    return;
                }
                if (userId == sellerId) {
                    rollbackTx(trans, "Seller cannot place bids on their own lot", callback);
                    return;
                }
                if (bidAmount <= currentPrice) {
                    rollbackTx(trans, "Bid must be strictly greater than current price", callback);
                    return;
                }
                if (userId == prevWinnerId) {
                    rollbackTx(trans, "You already hold the highest bid", callback);
                    return;
                }

                // Блокируем баланс пользователя
                trans->execSqlAsync(
                    "SELECT balance, is_blocked FROM Users WHERE user_id = $1 FOR UPDATE",
                    [trans, lotId, userId, bidAmount, prevWinnerId, currentPrice, callback](const drogon::orm::Result &userRes) {
                        if (userRes.empty()) {
                            rollbackTx(trans, "User account not found", callback, k404NotFound);
                            return;
                        }

                        auto userRow = userRes[0];
                        bool isBlocked = userRow["is_blocked"].as<bool>();
                        double userBalance = userRow["balance"].as<double>();

                        if (isBlocked) {
                            rollbackTx(trans, "Your account is blocked", callback, k403Forbidden);
                            return;
                        }
                        if (userBalance < bidAmount) {
                            rollbackTx(trans, "Insufficient funds on balance", callback);
                            return;
                        }

                        // Списываем средства
                        trans->execSqlAsync(
                            "UPDATE Users SET balance = balance - $1 WHERE user_id = $2",
                            [trans, lotId, userId, bidAmount, prevWinnerId, currentPrice, callback](const drogon::orm::Result &) {
                                
                                auto proceedRefundAndInsert = [trans, lotId, userId, bidAmount, prevWinnerId, currentPrice, callback]() {
                                    if (prevWinnerId.empty()) {
                                        // Предыдущих ставок нет
                                        trans->execSqlAsync(
                                            "UPDATE Lots SET current_price = $1, winner_id = $2 WHERE lot_id = $3",
                                            [trans, lotId, userId, bidAmount, callback](const drogon::orm::Result &) {
                                                trans->execSqlAsync(
                                                    "INSERT INTO Bids (lot_id, user_id, amount) VALUES ($1, $2, $3)",
                                                    [trans, lotId, userId, bidAmount, callback](const drogon::orm::Result &) {
                                                        trans->execSqlAsync(
                                                            "INSERT INTO Transactions (user_id, lot_id, amount, type) VALUES ($1, $2, $3, 'hold')",
                                                            [trans](const drogon::orm::Result &) {
                                                                // Конец цепочки. Лямбда завершается, копия `trans` выходит из области видимости.
                                                                // Это автоматически запустит асинхронный COMMIT и вызовет setCommitCallback.
                                                            },
                                                            [trans, callback](const drogon::orm::DrogonDbException &) { rollbackTx(trans, "Hold transaction log failed", callback); },
                                                            userId, lotId, bidAmount
                                                        );
                                                    },
                                                    [trans, callback](const drogon::orm::DrogonDbException &) { rollbackTx(trans, "Insert bid failed", callback); },
                                                    lotId, userId, bidAmount
                                                );
                                            },
                                            [trans, callback](const drogon::orm::DrogonDbException &) { rollbackTx(trans, "Lot current price update failed", callback); },
                                            bidAmount, userId, lotId
                                        );
                                        return;
                                    }

                                    // Возврат предыдущему лидеру
                                    trans->execSqlAsync(
                                        "UPDATE Users SET balance = balance + $1 WHERE user_id = $2",
                                        [trans, lotId, userId, bidAmount, prevWinnerId, currentPrice, callback](const drogon::orm::Result &) {
                                            trans->execSqlAsync(
                                                "INSERT INTO Transactions (user_id, lot_id, amount, type) VALUES ($1, $2, $3, 'refund')",
                                                [trans, lotId, userId, bidAmount, callback](const drogon::orm::Result &) {
                                                    trans->execSqlAsync(
                                                        "UPDATE Lots SET current_price = $1, winner_id = $2 WHERE lot_id = $3",
                                                        [trans, lotId, userId, bidAmount, callback](const drogon::orm::Result &) {
                                                            trans->execSqlAsync(
                                                                "INSERT INTO Bids (lot_id, user_id, amount) VALUES ($1, $2, $3)",
                                                                [trans, lotId, userId, bidAmount, callback](const drogon::orm::Result &) {
                                                                    trans->execSqlAsync(
                                                                        "INSERT INTO Transactions (user_id, lot_id, amount, type) VALUES ($1, $2, $3, 'hold')",
                                                                        [trans](const drogon::orm::Result &) {
                                                                            // Конец цепочки. `trans` выходит из области видимости, инициируя COMMIT.
                                                                        },
                                                                        [trans, callback](const drogon::orm::DrogonDbException &) { rollbackTx(trans, "Log hold failed", callback); },
                                                                        userId, lotId, bidAmount
                                                                    );
                                                                },
                                                                [trans, callback](const drogon::orm::DrogonDbException &) { rollbackTx(trans, "Record bid failed", callback); },
                                                                lotId, userId, bidAmount
                                                            );
                                                        },
                                                        [trans, callback](const drogon::orm::DrogonDbException &) { rollbackTx(trans, "Price update failed", callback); },
                                                        bidAmount, userId, lotId
                                                    );
                                                },
                                                [trans, callback](const drogon::orm::DrogonDbException &) { rollbackTx(trans, "Log refund failed", callback); },
                                                prevWinnerId, lotId, currentPrice
                                            );
                                        },
                                        [trans, callback](const drogon::orm::DrogonDbException &) { rollbackTx(trans, "Refund transaction failed", callback); },
                                        currentPrice, prevWinnerId
                                    );
                                };

                                proceedRefundAndInsert();
                            },
                            [trans, callback](const drogon::orm::DrogonDbException &) { rollbackTx(trans, "Deduct failed", callback); },
                            bidAmount, userId
                        );
                    },
                    [trans, callback](const drogon::orm::DrogonDbException &) { rollbackTx(trans, "User lock failed", callback); },
                    userId
                );
            },
            [trans, callback](const drogon::orm::DrogonDbException &) { rollbackTx(trans, "Lot lock failed", callback); },
            lotId
        );
    });
}

void BidController::getBidHistory(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string lotId) {
    auto db = app().getDbClient();
    db->execSqlAsync(
        "SELECT b.amount, b.created_at, u.username "
        "FROM Bids b "
        "JOIN Users u ON b.user_id = u.user_id "
        "WHERE b.lot_id = $1 "
        "ORDER BY b.created_at ASC",
        [callback](const drogon::orm::Result &r) {
            Json::Value arr(Json::arrayValue);
            for (auto const &row : r) {
                Json::Value point;
                point["amount"] = row["amount"].as<double>();
                point["timestamp"] = row["created_at"].as<std::string>();
                point["bidder"] = row["username"].as<std::string>();
                arr.append(point);
            }
            callback(HttpResponse::newHttpJsonResponse(arr));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = HttpResponse::newHttpJsonResponse(makeError("Failed to fetch bid history: " + std::string(e.base().what())));
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
        },
        lotId
    );
}