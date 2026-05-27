#pragma once
#include <drogon/HttpController.h>

class TransactionController : public drogon::HttpController<TransactionController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(TransactionController::deposit, "/api/transactions/deposit", drogon::Post, "AuthFilter");
        ADD_METHOD_TO(TransactionController::getMyTransactions, "/api/transactions/my", drogon::Get, "AuthFilter");
        ADD_METHOD_TO(TransactionController::getTransactionsAdmin, "/api/admin/transactions", drogon::Get, "AuthFilter");
    METHOD_LIST_END

    void deposit(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void getMyTransactions(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback);
    void getTransactionsAdmin(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};