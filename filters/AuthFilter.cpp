#include "AuthFilter.h"
#include <jwt-cpp/jwt.h>

void AuthFilter::doFilter(const drogon::HttpRequestPtr &req,
                          drogon::FilterCallback &&fcb,
                          drogon::FilterChainCallback &&fccb) {
    auto authHeader = req->getHeader("Authorization");
    if (authHeader.empty() || authHeader.find("Bearer ") != 0) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k401Unauthorized);
        fcb(resp);
        return;
    }

    std::string token = authHeader.substr(7);
    try {
        auto decoded = jwt::decode(token);
        // Запись claims в сессию/атрибуты запроса для использования в контроллерах
        req->attributes()->insert("user_id", decoded.get_payload_claim("user_id").as_string());
        req->attributes()->insert("role", decoded.get_payload_claim("role").as_string());
        fccb(); // Токен валиден, продолжаем выполнение
    } catch (...) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k401Unauthorized);
        fcb(resp);
    }
}