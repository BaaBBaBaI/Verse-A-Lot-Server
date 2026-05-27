#include "TransactionController.h"

void TransactionController::deposit(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    auto userId = req->attributes()->get<std::string>("user_id");
    auto json = req->getJsonObject();
    if (!json || !json->isMember("amount")) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    double amount = (*json)["amount"].asDouble();
    if (amount <= 0.0) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    auto db = drogon::app().getDbClient();
    
    // Передаем только ОДИН колбек, возвращающий shared_ptr транзакции
    db->newTransactionAsync([callback, userId, amount](const std::shared_ptr<drogon::orm::Transaction> &trans) {
        if (!trans) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

        // Шаг 1: Увеличение баланса пользователя
        trans->execSqlAsync(
            "UPDATE Users SET balance = balance + $1 WHERE user_id = $2 RETURNING balance",
            [trans, callback, userId, amount](const drogon::orm::Result &userRes) {
                if (userRes.empty()) {
                    auto resp = drogon::HttpResponse::newHttpResponse();
                    resp->setStatusCode(drogon::k404NotFound);
                    callback(resp);
                    return;
                }

                double newBalance = userRes[0]["balance"].as<double>();

                // Шаг 2: Создание записи транзакции
                trans->execSqlAsync(
                    "INSERT INTO Transactions (user_id, amount, type) VALUES ($1, $2, 'deposit')",
                    [trans, callback, newBalance](const drogon::orm::Result &) {
                        // Транзакция завершится автоматическим коммитом при выходе `trans` из области видимости
                        Json::Value respJson;
                        respJson["status"] = "success";
                        respJson["new_balance"] = newBalance;
                        callback(drogon::HttpResponse::newHttpJsonResponse(respJson));
                    },
                    [callback](const drogon::orm::DrogonDbException &) {
                        auto resp = drogon::HttpResponse::newHttpResponse();
                        resp->setStatusCode(drogon::k500InternalServerError);
                        callback(resp);
                    },
                    userId, amount
                );
            },
            [callback](const drogon::orm::DrogonDbException &) {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k500InternalServerError);
                callback(resp);
            },
            amount, userId
        );
    });
}

void TransactionController::getMyTransactions(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    auto userId = req->attributes()->get<std::string>("user_id");
    auto db = drogon::app().getDbClient();

    db->execSqlAsync(
        "SELECT transaction_id, lot_id, amount, type, created_at FROM Transactions WHERE user_id = $1 ORDER BY created_at DESC",
        [callback](const drogon::orm::Result &result) {
            Json::Value arr(Json::arrayValue);
            for (auto const &row : result) {
                Json::Value item;
                item["transaction_id"] = row["transaction_id"].as<std::string>();
                item["lot_id"] = row["lot_id"].isNull() ? Json::Value(Json::nullValue) : row["lot_id"].as<std::string>();
                item["amount"] = row["amount"].as<double>();
                item["type"] = row["type"].as<std::string>();
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
        userId
    );
}

void TransactionController::getTransactionsAdmin(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    auto role = req->attributes()->get<std::string>("role");
    if (role != "admin") {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k403Forbidden);
        callback(resp);
        return;
    }

    auto db = drogon::app().getDbClient();
    db->execSqlAsync(
        "SELECT transaction_id, user_id, lot_id, amount, type, created_at FROM Transactions ORDER BY created_at DESC",
        [callback](const drogon::orm::Result &result) {
            Json::Value arr(Json::arrayValue);
            for (auto const &row : result) {
                Json::Value item;
                item["transaction_id"] = row["transaction_id"].as<std::string>();
                item["user_id"] = row["user_id"].as<std::string>();
                item["lot_id"] = row["lot_id"].isNull() ? Json::Value(Json::nullValue) : row["lot_id"].as<std::string>();
                item["amount"] = row["amount"].as<double>();
                item["type"] = row["type"].as<std::string>();
                item["created_at"] = row["created_at"].as<std::string>();
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