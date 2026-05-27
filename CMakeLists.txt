cmake_minimum_required(VERSION 3.16)
project(AuctionBackend CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon REQUIRED)
find_package(Jsoncpp REQUIRED)
find_package(Threads REQUIRED)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

# Подключаем SSL (требуется для jwt-cpp)
find_package(OpenSSL REQUIRED)

add_executable(${PROJECT_NAME}
    main.cpp
    filters/AuthFilter.cpp
    controllers/AuthController.cpp
    controllers/LotController.cpp
    controllers/BidController.cpp
    controllers/LiveBidWebSocketController.cpp
    controllers/TransactionController.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    Drogon::Drogon
    OpenSSL::SSL
    OpenSSL::Crypto
)