#pragma once
#include <string>
#include <memory>
#include <optional>
#include <drogon/orm/DbClient.h>
#include <jsoncpp/json/json.h>
#include <drogon/orm/Result.h>

struct AuthenticatedUser {
    std::string userId;
    std::string role;
};

class JwtHelper {
public:
    static std::string generateToken(const std::string &userId, const std::string &role) {
        return "mock-jwt-token-for-" + userId + ":" + role;
    }

    static std::optional<AuthenticatedUser> verifyToken(const std::string &authHeader) {
        if (authHeader.rfind("Bearer ", 0) != 0) {
            return std::nullopt;
        }
        std::string token = authHeader.substr(7);
        size_t prefixOpt = token.find("mock-jwt-token-for-");
        if (prefixOpt == std::string::npos) {
            return std::nullopt;
        }
        
        std::string payload = token.substr(19);
        size_t colon = payload.find(':');
        if (colon == std::string::npos) {
            return std::nullopt;
        }

        AuthenticatedUser user;
        user.userId = payload.substr(0, colon);
        user.role = payload.substr(colon + 1);
        return user;
    }
};