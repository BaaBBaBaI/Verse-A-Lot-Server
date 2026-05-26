#include "TransactionController.h"
#include "JwtHelper.h"

static Json::Value makeError(const std::string &msg) {
    Json::Value err;
    err["error"] = msg;
    return err;
}

void TransactionController::deposit(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    auto auth = JwtHelper::verifyToken(req->getHeader("Authorization"));
    if (!auth) {
        auto resp = HttpResponse::newHttpJsonResponse(makeError("Unauthorized"));
        resp->setStatusCode(k401Unauthorized);
        callback(resp);
        return;
    }

    auto json = req->getJsonObject();
    if (!json || !json->isMember("amount")) {
        auto resp = HttpResponse::newHttpJsonResponse(makeError("Missing parameter 'amount'"));
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    double depositAmount = (*json)["amount"].asDouble();
    if (depositAmount <= 0.0) {
        auto resp = HttpResponse::newHttpJsonResponse(makeError("Amount must be positive"));
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    std::string userId = auth->userId;
    auto db = app().getDbClient();

    db->newTransactionAsync([userId, depositAmount, callback](const std::shared_ptr<drogon::orm::Transaction> &trans) {
        
        // Устанавливаем коллбек фиксации транзакции
        trans->setCommitCallback([callback](bool success) {
            if (success) {
                Json::Value out;
                out["status"] = "success";
                out["message"] = "Balance topped up successfully";
                callback(HttpResponse::newHttpJsonResponse(out));
            } else {
                auto resp = HttpResponse::newHttpJsonResponse(makeError("Database commit failed"));
                resp->setStatusCode(k500InternalServerError);
                callback(resp);
            }
        });

        // Начинаем обновление баланса
        trans->execSqlAsync(
            "UPDATE Users SET balance = balance + $1 WHERE user_id = $2",
            [trans, userId, depositAmount, callback](const drogon::orm::Result &) {
                trans->execSqlAsync(
                    "INSERT INTO Transactions (user_id, amount, type) VALUES ($1, $2, 'deposit')",
                    [trans](const drogon::orm::Result &) {
                        // Конец цепочки. `trans` уничтожается, запуская асинхронный COMMIT.
                    },
                    [trans, callback](const drogon::orm::DrogonDbException &) {
                        trans->rollback();
                        auto resp = HttpResponse::newHttpJsonResponse(makeError("Deposit logging failed"));
                        resp->setStatusCode(k500InternalServerError);
                        callback(resp);
                    },
                    userId, depositAmount
                );
            },
            [trans, callback](const drogon::orm::DrogonDbException &) {
                trans->rollback();
                auto resp = HttpResponse::newHttpJsonResponse(makeError("Balance update failed"));
                resp->setStatusCode(k500InternalServerError);
                callback(resp);
            },
            depositAmount, userId
        );
    });
}

void TransactionController::getMyTransactions(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    auto auth = JwtHelper::verifyToken(req->getHeader("Authorization"));
    if (!auth) {
        auto resp = HttpResponse::newHttpJsonResponse(makeError("Unauthorized"));
        resp->setStatusCode(k401Unauthorized);
        callback(resp);
        return;
    }

    auto db = app().getDbClient();
    db->execSqlAsync(
        "SELECT transaction_id, lot_id, amount, type, created_at FROM Transactions WHERE user_id = $1 ORDER BY created_at DESC",
        [callback](const drogon::orm::Result &r) {
            Json::Value arr(Json::arrayValue);
            for (auto const &row : r) {
                Json::Value item;
                item["transaction_id"] = row["transaction_id"].as<std::string>();
                item["lot_id"] = row["lot_id"].isNull() ? "" : row["lot_id"].as<std::string>();
                item["amount"] = row["amount"].as<double>();
                item["type"] = row["type"].as<std::string>();
                item["created_at"] = row["created_at"].as<std::string>();
                arr.append(item);
            }
            callback(HttpResponse::newHttpJsonResponse(arr));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = HttpResponse::newHttpJsonResponse(makeError("Database query failed: " + std::string(e.base().what())));
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
        },
        auth->userId
    );
}

void TransactionController::getAdminTransactions(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    auto auth = JwtHelper::verifyToken(req->getHeader("Authorization"));
    if (!auth || auth->role != "admin") {
        auto resp = HttpResponse::newHttpJsonResponse(makeError("Forbidden"));
        resp->setStatusCode(k403Forbidden);
        callback(resp);
        return;
    }

    auto db = app().getDbClient();
    db->execSqlAsync(
        "SELECT t.transaction_id, t.user_id, u.username, t.lot_id, t.amount, t.type, t.created_at "
        "FROM Transactions t "
        "JOIN Users u ON t.user_id = u.user_id "
        "ORDER BY t.created_at DESC",
        [callback](const drogon::orm::Result &r) {
            Json::Value arr(Json::arrayValue);
            for (auto const &row : r) {
                Json::Value item;
                item["transaction_id"] = row["transaction_id"].as<std::string>();
                item["user_id"] = row["user_id"].as<std::string>();
                item["username"] = row["username"].as<std::string>();
                item["lot_id"] = row["lot_id"].isNull() ? "" : row["lot_id"].as<std::string>();
                item["amount"] = row["amount"].as<double>();
                item["type"] = row["type"].as<std::string>();
                item["created_at"] = row["created_at"].as<std::string>();
                arr.append(item);
            }
            callback(HttpResponse::newHttpJsonResponse(arr));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = HttpResponse::newHttpJsonResponse(makeError("Failed to build analytics: " + std::string(e.base().what())));
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
        }
    );
}