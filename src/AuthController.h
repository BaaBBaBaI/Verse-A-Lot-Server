#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

class AuthController : public HttpController<AuthController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(AuthController::login, "/api/auth/login", Post);
        ADD_METHOD_TO(AuthController::registerUser, "/api/auth/register", Post);
        ADD_METHOD_TO(AuthController::getProfile, "/api/users/profile", Get);
        ADD_METHOD_TO(AuthController::getAdminUsers, "/api/admin/users", Get);
        ADD_METHOD_TO(AuthController::blockUser, "/api/admin/users/{id}/block", Put);
    METHOD_LIST_END

    void login(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void registerUser(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getProfile(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getAdminUsers(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void blockUser(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, std::string userId);
};