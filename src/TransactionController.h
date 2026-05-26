#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

class TransactionController : public HttpController<TransactionController> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(TransactionController::deposit, "/api/transactions/deposit", Post);
        ADD_METHOD_TO(TransactionController::getMyTransactions, "/api/transactions/my", Get);
        ADD_METHOD_TO(TransactionController::getAdminTransactions, "/api/admin/transactions", Get);
    METHOD_LIST_END

    void deposit(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getMyTransactions(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
    void getAdminTransactions(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
};