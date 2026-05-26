cmake_minimum_required(VERSION 3.12)
project(auction_backend CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon REQUIRED)

set(SRC_FILES
    src/main.cc
    src/AuthController.cc
    src/AuctionLotController.cc
    src/BidController.cc
    src/LiveBidWSController.cc
    src/TransactionController.cc
)

add_executable(${PROJECT_NAME} ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE src)

target_link_libraries(${PROJECT_NAME} PRIVATE Drogon::Drogon)