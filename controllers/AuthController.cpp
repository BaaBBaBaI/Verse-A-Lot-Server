#include "AuthController.h"
#include <jwt-cpp/jwt.h>
#include <picosha2.h> // Простая библиотека хэширования SHA-256

void AuthController::login(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    auto json = req->getJsonObject();
    if (!json) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    std::string username = (*json)["username"].asString();
    std::string password = (*json)["password"].asString();
    std::string password_hash = picosha2::hash256_hex_string(password);

    auto db = drogon::app().getDbClient();
    db->execSqlAsync(
        "SELECT user_id, password_hash, role, balance FROM Users WHERE username = $1",
        [callback, password_hash, username](const drogon::orm::Result &result) {
            if (result.empty() || result[0]["password_hash"].as<std::string>() != password_hash) {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k401Unauthorized);
                callback(resp);
                return;
            }

            auto row = result[0];
            std::string userId = row["user_id"].as<std::string>();
            std::string role = row["role"].as<std::string>();
            double balance = row["balance"].as<double>();

            // Генерация JWT-токена
            auto token = jwt::create()
                .set_issuer("auction_platform")
                .set_type("JWS")
                .set_payload_claim("user_id", jwt::claim(userId))
                .set_payload_claim("role", jwt::claim(role))
                .sign(jwt::algorithm::hs256{"secret_key_here"});

            Json::Value respJson;
            respJson["token"] = token;
            respJson["role"] = role;
            respJson["balance"] = balance;

            auto resp = drogon::HttpResponse::newHttpJsonResponse(respJson);
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
        },
        username
    );
}

void AuthController::registerUser(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    auto json = req->getJsonObject();
    if (!json) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    std::string username = (*json)["username"].asString();
    std::string email = (*json)["email"].asString();
    std::string password = (*json)["password"].asString();
    std::string role = (*json)["role"].asString(); // 'user' или 'admin'
    std::string password_hash = picosha2::hash256_hex_string(password);

    auto db = drogon::app().getDbClient();
    db->execSqlAsync(
        "INSERT INTO Users (username, email, password_hash, role, balance) VALUES ($1, $2, $3, $4, 0.00) RETURNING user_id",
        [callback](const drogon::orm::Result &result) {
            Json::Value respJson;
            respJson["status"] = "success";
            respJson["user_id"] = result[0]["user_id"].as<std::string>();
            auto resp = drogon::HttpResponse::newHttpJsonResponse(respJson);
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k400BadRequest);
            callback(resp);
        },
        username, email, password_hash, role
    );
}

void AuthController::getProfile(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    auto userId = req->attributes()->get<std::string>("user_id");
    auto db = drogon::app().getDbClient();

    db->execSqlAsync(
        "SELECT user_id, username, email, role, balance FROM Users WHERE user_id = $1",
        [callback](const drogon::orm::Result &result) {
            if (result.empty()) {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k404NotFound);
                callback(resp);
                return;
            }
            auto row = result[0];
            Json::Value respJson;
            respJson["user_id"] = row["user_id"].as<std::string>();
            respJson["username"] = row["username"].as<std::string>();
            respJson["email"] = row["email"].as<std::string>();
            respJson["role"] = row["role"].as<std::string>();
            respJson["balance"] = row["balance"].as<double>();
            
            callback(drogon::HttpResponse::newHttpJsonResponse(respJson));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
        },
        userId
    );
}

void AuthController::getUsersAdmin(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
    auto role = req->attributes()->get<std::string>("role");
    if (role != "admin") {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k403Forbidden);
        callback(resp);
        return;
    }

    auto db = drogon::app().getDbClient();
    db->execSqlAsync(
        "SELECT user_id, username, email, role, balance FROM Users",
        [callback](const drogon::orm::Result &result) {
            Json::Value arr(Json::arrayValue);
            for (auto const &row : result) {
                Json::Value item;
                item["user_id"] = row["user_id"].as<std::string>();
                item["username"] = row["username"].as<std::string>();
                item["email"] = row["email"].as<std::string>();
                item["role"] = row["role"].as<std::string>();
                item["balance"] = row["balance"].as<double>();
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

void AuthController::creditUser(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback, std::string userId) {
    auto role = req->attributes()->get<std::string>("role");
    if (role != "admin") {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k403Forbidden);
        callback(resp);
        return;
    }

    auto jsonPtr = req->getJsonObject();
    if (!jsonPtr || !jsonPtr->isMember("amount")) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    double amount = (*jsonPtr)["amount"].asDouble();
    if (amount <= 0.0) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        callback(resp);
        return;
    }

    auto db = drogon::app().getDbClient();
    db->newTransactionAsync([callback, userId, amount](const std::shared_ptr<drogon::orm::Transaction> &trans) {
        if (!trans) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k500InternalServerError);
            callback(resp);
            return;
        }

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

                trans->execSqlAsync(
                    "INSERT INTO Transactions (user_id, amount, type) VALUES ($1, $2, 'deposit')",
                    [trans, callback, userId, newBalance](const drogon::orm::Result &) {
                        Json::Value respJson;
                        respJson["status"] = "success";
                        respJson["user_id"] = userId;
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