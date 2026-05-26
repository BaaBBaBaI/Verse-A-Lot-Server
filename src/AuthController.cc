#include "AuthController.h"
#include "JwtHelper.h"

static Json::Value makeError(const std::string &msg) {
    Json::Value err;
    err["error"] = msg;
    return err;
}

void AuthController::login(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("username") || !json->isMember("password")) {
        auto resp = HttpResponse::newHttpJsonResponse(makeError("Missing fields"));
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    std::string username = (*json)["username"].asString();
    std::string password = (*json)["password"].asString();

    auto db = app().getDbClient();
    db->execSqlAsync(
        "SELECT user_id, password_hash, role, balance, is_blocked FROM Users WHERE username = $1",
        [username, callback](const drogon::orm::Result &r) {
            if (r.empty()) {
                auto resp = HttpResponse::newHttpJsonResponse(makeError("User not found"));
                resp->setStatusCode(k401Unauthorized);
                callback(resp);
                return;
            }

            auto row = r[0];
            if (row["is_blocked"].as<bool>()) {
                auto resp = HttpResponse::newHttpJsonResponse(makeError("User is blocked"));
                resp->setStatusCode(k403Forbidden);
                callback(resp);
                return;
            }

            std::string userId = row["user_id"].as<std::string>();
            std::string role = row["role"].as<std::string>();
            double balance = row["balance"].as<double>();

            std::string token = JwtHelper::generateToken(userId, role);

            Json::Value out;
            out["token"] = token;
            out["role"] = role;
            out["balance"] = balance;

            auto resp = HttpResponse::newHttpJsonResponse(out);
            callback(resp);
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = HttpResponse::newHttpJsonResponse(makeError("Database error: " + std::string(e.base().what())));
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
        },
        username
    );
}

void AuthController::registerUser(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    auto json = req->getJsonObject();
    if (!json || !json->isMember("username") || !json->isMember("email") || !json->isMember("password")) {
        auto resp = HttpResponse::newHttpJsonResponse(makeError("Required fields missing"));
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    std::string username = (*json)["username"].asString();
    std::string email = (*json)["email"].asString();
    std::string password = (*json)["password"].asString();

    auto db = app().getDbClient();
    db->execSqlAsync(
        "INSERT INTO Users (username, email, password_hash) VALUES ($1, $2, $3) RETURNING user_id",
        [callback](const drogon::orm::Result &r) {
            Json::Value out;
            out["user_id"] = r[0]["user_id"].as<std::string>();
            out["status"] = "registered";
            callback(HttpResponse::newHttpJsonResponse(out));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = HttpResponse::newHttpJsonResponse(makeError("Username or email already exists: " + std::string(e.base().what())));
            resp->setStatusCode(k400BadRequest);
            callback(resp);
        },
        username, email, password
    );
}

void AuthController::getProfile(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    auto auth = JwtHelper::verifyToken(req->getHeader("Authorization"));
    if (!auth) {
        auto resp = HttpResponse::newHttpJsonResponse(makeError("Unauthorized"));
        resp->setStatusCode(k401Unauthorized);
        callback(resp);
        return;
    }

    auto db = app().getDbClient();
    db->execSqlAsync(
        "SELECT username, email, role, balance FROM Users WHERE user_id = $1",
        [callback](const drogon::orm::Result &r) {
            if (r.empty()) {
                auto resp = HttpResponse::newHttpJsonResponse(makeError("User not found"));
                resp->setStatusCode(k404NotFound);
                callback(resp);
                return;
            }
            auto row = r[0];
            Json::Value out;
            out["username"] = row["username"].as<std::string>();
            out["email"] = row["email"].as<std::string>();
            out["role"] = row["role"].as<std::string>();
            out["balance"] = row["balance"].as<double>();
            callback(HttpResponse::newHttpJsonResponse(out));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = HttpResponse::newHttpJsonResponse(makeError("Internal error: " + std::string(e.base().what())));
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
        },
        auth->userId
    );
}

void AuthController::getAdminUsers(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    auto auth = JwtHelper::verifyToken(req->getHeader("Authorization"));
    if (!auth || auth->role != "admin") {
        auto resp = HttpResponse::newHttpJsonResponse(makeError("Forbidden"));
        resp->setStatusCode(k403Forbidden);
        callback(resp);
        return;
    }

    auto db = app().getDbClient();
    db->execSqlAsync(
        "SELECT user_id, username, email, role, balance, is_blocked FROM Users ORDER BY created_at DESC",
        [callback](const drogon::orm::Result &r) {
            Json::Value arr(Json::arrayValue);
            for (auto const &row : r) {
                Json::Value obj;
                obj["user_id"] = row["user_id"].as<std::string>();
                obj["username"] = row["username"].as<std::string>();
                obj["email"] = row["email"].as<std::string>();
                obj["role"] = row["role"].as<std::string>();
                obj["balance"] = row["balance"].as<double>();
                obj["is_blocked"] = row["is_blocked"].as<bool>();
                arr.append(obj);
            }
            callback(HttpResponse::newHttpJsonResponse(arr));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = HttpResponse::newHttpJsonResponse(makeError("Internal database error: " + std::string(e.base().what())));
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
        }
    );
}

void AuthController::blockUser(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string userId) {
    auto auth = JwtHelper::verifyToken(req->getHeader("Authorization"));
    if (!auth || auth->role != "admin") {
        auto resp = HttpResponse::newHttpJsonResponse(makeError("Forbidden"));
        resp->setStatusCode(k403Forbidden);
        callback(resp);
        return;
    }

    auto json = req->getJsonObject();
    if (!json || !json->isMember("block")) {
        auto resp = HttpResponse::newHttpJsonResponse(makeError("Missing parameter 'block'"));
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    bool block = (*json)["block"].asBool();
    auto db = app().getDbClient();
    db->execSqlAsync(
        "UPDATE Users SET is_blocked = $1 WHERE user_id = $2",
        [callback, block](const drogon::orm::Result &) {
            Json::Value out;
            out["status"] = "success";
            out["blocked"] = block;
            callback(HttpResponse::newHttpJsonResponse(out));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = HttpResponse::newHttpJsonResponse(makeError("Failed to update user status: " + std::string(e.base().what())));
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
        },
        block, userId
    );
}