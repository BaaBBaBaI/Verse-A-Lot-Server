#include "AuctionLotController.h"
#include "JwtHelper.h"

static Json::Value makeError(const std::string &msg) {
    Json::Value err;
    err["error"] = msg;
    return err;
}

void AuctionLotController::getAuctions(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    std::string typeFilter = req->getParameter("type");
    
    std::string query = "SELECT auction_id, title, type, start_time, end_time, status FROM Auctions";
    bool hasFilter = !typeFilter.empty();
    if (hasFilter) {
        query += " WHERE type = $1";
    }
    query += " ORDER BY start_time ASC";

    auto db = app().getDbClient();
    auto handler = [callback](const drogon::orm::Result &r) {
        Json::Value arr(Json::arrayValue);
        for (auto const &row : r) {
            Json::Value obj;
            obj["auction_id"] = row["auction_id"].as<std::string>();
            obj["title"] = row["title"].as<std::string>();
            obj["type"] = row["type"].as<std::string>();
            obj["start_time"] = row["start_time"].as<std::string>();
            obj["end_time"] = row["end_time"].as<std::string>();
            obj["status"] = row["status"].as<std::string>();
            arr.append(obj);
        }
        callback(HttpResponse::newHttpJsonResponse(arr));
    };

    auto failHandler = [callback](const drogon::orm::DrogonDbException &e) {
        auto resp = HttpResponse::newHttpJsonResponse(makeError("Failed to fetch auctions: " + std::string(e.base().what())));
        resp->setStatusCode(k500InternalServerError);
        callback(resp);
    };

    if (hasFilter) {
        db->execSqlAsync(query, handler, failHandler, typeFilter);
    } else {
        db->execSqlAsync(query, handler, failHandler);
    }
}

void AuctionLotController::createAuction(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    auto auth = JwtHelper::verifyToken(req->getHeader("Authorization"));
    if (!auth || auth->role != "admin") {
        auto resp = HttpResponse::newHttpJsonResponse(makeError("Forbidden"));
        resp->setStatusCode(k403Forbidden);
        callback(resp);
        return;
    }

    auto json = req->getJsonObject();
    if (!json || !json->isMember("title") || !json->isMember("type") || !json->isMember("start_time") || !json->isMember("end_time")) {
        auto resp = HttpResponse::newHttpJsonResponse(makeError("Missing parameters"));
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    auto db = app().getDbClient();
    db->execSqlAsync(
        "INSERT INTO Auctions (title, type, start_time, end_time) VALUES ($1, $2, $3, $4) RETURNING auction_id",
        [callback](const drogon::orm::Result &r) {
            Json::Value out;
            out["auction_id"] = r[0]["auction_id"].as<std::string>();
            out["status"] = "created";
            callback(HttpResponse::newHttpJsonResponse(out));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = HttpResponse::newHttpJsonResponse(makeError("Failed to create auction: " + std::string(e.base().what())));
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
        },
        (*json)["title"].asString(),
        (*json)["type"].asString(),
        (*json)["start_time"].asString(),
        (*json)["end_time"].asString()
    );
}

void AuctionLotController::getLots(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    auto db = app().getDbClient();
    db->execSqlAsync(
        "SELECT l.lot_id, l.title, l.description, l.current_price, l.status, i.image_url "
        "FROM Lots l "
        "LEFT JOIN Images i ON l.lot_id = i.lot_id AND i.is_primary = TRUE "
        "WHERE l.status = 'approved' OR l.status = 'active'",
        [callback](const drogon::orm::Result &r) {
            Json::Value arr(Json::arrayValue);
            for (auto const &row : r) {
                Json::Value obj;
                obj["lot_id"] = row["lot_id"].as<std::string>();
                obj["title"] = row["title"].as<std::string>();
                obj["description"] = row["description"].as<std::string>();
                obj["current_price"] = row["current_price"].as<double>();
                obj["status"] = row["status"].as<std::string>();
                obj["primary_image"] = row["image_url"].isNull() ? "" : row["image_url"].as<std::string>();
                arr.append(obj);
            }
            callback(HttpResponse::newHttpJsonResponse(arr));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = HttpResponse::newHttpJsonResponse(makeError("Internal error: " + std::string(e.base().what())));
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
        }
    );
}

void AuctionLotController::getLotById(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string lotId) {
    auto db = app().getDbClient();
    db->execSqlAsync(
        "SELECT lot_id, auction_id, seller_id, winner_id, title, description, start_price, reserve_price, current_price, status "
        "FROM Lots WHERE lot_id = $1",
        [db, callback, lotId](const drogon::orm::Result &r) {
            if (r.empty()) {
                auto resp = HttpResponse::newHttpJsonResponse(makeError("Lot not found"));
                resp->setStatusCode(k404NotFound);
                callback(resp);
                return;
            }

            auto row = r[0];
            Json::Value obj;
            obj["lot_id"] = row["lot_id"].as<std::string>();
            obj["auction_id"] = row["auction_id"].as<std::string>();
            obj["seller_id"] = row["seller_id"].as<std::string>();
            obj["winner_id"] = row["winner_id"].isNull() ? "" : row["winner_id"].as<std::string>();
            obj["title"] = row["title"].as<std::string>();
            obj["description"] = row["description"].as<std::string>();
            obj["start_price"] = row["start_price"].as<double>();
            obj["reserve_price"] = row["reserve_price"].as<double>();
            obj["current_price"] = row["current_price"].as<double>();
            obj["status"] = row["status"].as<std::string>();

            db->execSqlAsync(
                "SELECT image_url, is_primary FROM Images WHERE lot_id = $1",
                [callback, obj](const drogon::orm::Result &imgRes) mutable {
                    Json::Value imgs(Json::arrayValue);
                    for (auto const &imgRow : imgRes) {
                        Json::Value img;
                        img["url"] = imgRow["image_url"].as<std::string>();
                        img["is_primary"] = imgRow["is_primary"].as<bool>();
                        imgs.append(img);
                    }
                    obj["images"] = imgs;
                    callback(HttpResponse::newHttpJsonResponse(obj));
                },
                [callback, obj](const drogon::orm::DrogonDbException &) mutable {
                    callback(HttpResponse::newHttpJsonResponse(obj));
                },
                lotId
            );
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = HttpResponse::newHttpJsonResponse(makeError("Failed to fetch lot details: " + std::string(e.base().what())));
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
        },
        lotId
    );
}

void AuctionLotController::createLot(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    auto auth = JwtHelper::verifyToken(req->getHeader("Authorization"));
    if (!auth) {
        auto resp = HttpResponse::newHttpJsonResponse(makeError("Unauthorized"));
        resp->setStatusCode(k401Unauthorized);
        callback(resp);
        return;
    }

    auto json = req->getJsonObject();
    if (!json || !json->isMember("auction_id") || !json->isMember("title") || !json->isMember("start_price") || !json->isMember("reserve_price")) {
        auto resp = HttpResponse::newHttpJsonResponse(makeError("Missing parameters"));
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    auto db = app().getDbClient();
    db->execSqlAsync(
        "INSERT INTO Lots (auction_id, seller_id, title, description, start_price, reserve_price, current_price, status) "
        "VALUES ($1, $2, $3, $4, $5, $6, $5, 'pending_moderation') RETURNING lot_id",
        [callback](const drogon::orm::Result &r) {
            Json::Value out;
            out["lot_id"] = r[0]["lot_id"].as<std::string>();
            out["status"] = "pending_moderation";
            callback(HttpResponse::newHttpJsonResponse(out));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = HttpResponse::newHttpJsonResponse(makeError("Failed to create lot: " + std::string(e.base().what())));
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
        },
        (*json)["auction_id"].asString(),
        auth->userId,
        (*json)["title"].asString(),
        (*json)["description"].asString(),
        (*json)["start_price"].asDouble(),
        (*json)["reserve_price"].asDouble()
    );
}

void AuctionLotController::moderateLot(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string lotId) {
    auto auth = JwtHelper::verifyToken(req->getHeader("Authorization"));
    if (!auth || auth->role != "admin") {
        auto resp = HttpResponse::newHttpJsonResponse(makeError("Forbidden"));
        resp->setStatusCode(k403Forbidden);
        callback(resp);
        return;
    }

    auto json = req->getJsonObject();
    if (!json || !json->isMember("status")) {
        auto resp = HttpResponse::newHttpJsonResponse(makeError("Missing parameter 'status'"));
        resp->setStatusCode(k400BadRequest);
        callback(resp);
        return;
    }

    std::string newStatus = (*json)["status"].asString();
    auto db = app().getDbClient();
    db->execSqlAsync(
        "UPDATE Lots SET status = $1 WHERE lot_id = $2",
        [callback, newStatus](const drogon::orm::Result &) {
            Json::Value out;
            out["status"] = newStatus;
            callback(HttpResponse::newHttpJsonResponse(out));
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = HttpResponse::newHttpJsonResponse(makeError("Moderation update failed: " + std::string(e.base().what())));
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
        },
        newStatus, lotId
    );
}

void AuctionLotController::deleteLot(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string lotId) {
    auto auth = JwtHelper::verifyToken(req->getHeader("Authorization"));
    if (!auth) {
        auto resp = HttpResponse::newHttpJsonResponse(makeError("Unauthorized"));
        resp->setStatusCode(k401Unauthorized);
        callback(resp);
        return;
    }

    auto db = app().getDbClient();
    db->execSqlAsync(
        "SELECT seller_id, status FROM Lots WHERE lot_id = $1",
        [db, callback, lotId, auth](const drogon::orm::Result &r) {
            if (r.empty()) {
                auto resp = HttpResponse::newHttpJsonResponse(makeError("Lot not found"));
                resp->setStatusCode(k404NotFound);
                callback(resp);
                return;
            }

            auto row = r[0];
            std::string sellerId = row["seller_id"].as<std::string>();
            std::string status = row["status"].as<std::string>();

            if (auth->userId != sellerId && auth->role != "admin") {
                auto resp = HttpResponse::newHttpJsonResponse(makeError("Forbidden"));
                resp->setStatusCode(k403Forbidden);
                callback(resp);
                return;
            }

            if (status != "pending_moderation" && status != "rejected" && auth->role != "admin") {
                auto resp = HttpResponse::newHttpJsonResponse(makeError("Cannot delete active lot"));
                resp->setStatusCode(k400BadRequest);
                callback(resp);
                return;
            }

            db->execSqlAsync(
                "DELETE FROM Lots WHERE lot_id = $1",
                [callback](const drogon::orm::Result &) {
                    Json::Value out;
                    out["status"] = "deleted";
                    callback(HttpResponse::newHttpJsonResponse(out));
                },
                [callback](const drogon::orm::DrogonDbException &e) {
                    auto resp = HttpResponse::newHttpJsonResponse(makeError("Deletion failed: " + std::string(e.base().what())));
                    resp->setStatusCode(k500InternalServerError);
                    callback(resp);
                },
                lotId
            );
        },
        [callback](const drogon::orm::DrogonDbException &e) {
            auto resp = HttpResponse::newHttpJsonResponse(makeError("Verification query failed: " + std::string(e.base().what())));
            resp->setStatusCode(k500InternalServerError);
            callback(resp);
        },
        lotId
    );
}