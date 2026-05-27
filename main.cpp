#include <drogon/drogon.h>
#include <iostream>

int main() {
    std::cout << "[SYSTEM] Starting Auction Backend..." << std::endl;

    // Программное создание и регистрация клиента БД
    drogon::app().createDbClient(
        "postgresql",           // Тип СУБД (rdbms)
        "127.0.0.1",            // Хост (host)
        5432,                   // Порт (port)
        "auction_db",           // Имя базы данных (dbname)
        "postgres",             // Имя пользователя (user)
        "password", // Пароль (passwd) - укажите ваш!
        10,                     // Макс. количество соединений в пуле (connectionNum)
        "default"               // Имя клиента (name) — по нему контроллеры будут искать базу
    );

    std::cout << "[SYSTEM] DB Client 'default' registered programmatically." << std::endl;

    try {
        // Загружаем только сетевые настройки (порты, слушатели) из файла
        drogon::app().loadConfigFile("../config.json");
        std::cout << "[SYSTEM] Network config loaded." << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "[SYSTEM] Warning/Error loading config: " << e.what() << std::endl;
    }

    // CORS Pre-routing Advice to handle OPTIONS requests
    drogon::app().registerPreRoutingAdvice([](const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr &)> &&callback, std::function<void()> &&pass) {
        if (req->method() == drogon::Options) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->addHeader("Access-Control-Allow-Origin", "*");
            resp->addHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS, PUT, DELETE");
            resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
            resp->setStatusCode(drogon::k200OK);
            callback(resp);
            return;
        }
        pass();
    });

    // CORS Pre-sending Advice to add headers to responses
    drogon::app().registerPreSendingAdvice([](const drogon::HttpRequestPtr &req, const drogon::HttpResponsePtr &resp) {
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS, PUT, DELETE");
        resp->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
    });

    std::cout << "[SYSTEM] Running Event Loop..." << std::endl;
    drogon::app().run();
    return 0;
}