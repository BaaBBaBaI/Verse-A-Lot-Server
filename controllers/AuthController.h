#pragma once
#include <drogon/HttpController.h>

class AuthController : public drogon::HttpController<AuthController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(AuthController::login, "/api/auth/login", drogon::Post);
        ADD_METHOD_TO(AuthController::registerUser, "/api/auth/register", drogon::Post);
        ADD_METHOD_TO(AuthController::getProfile, "/api/users/profile", drogon::Get, "AuthFilter");
        ADD_METHOD_TO(AuthController::getUsersAdmin, "/api/admin/users", drogon::Get, "AuthFilter");
        ADD_METHOD_TO(AuthController::creditUser, "/api/admin/users/{id}/credit", drogon::Post, "AuthFilter");
    METHOD_LIST_END

    void login(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void registerUser(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void getProfile(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void getUsersAdmin(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void creditUser(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback, std::string userId);
};