cmake_minimum_required(VERSION 3.16)

project(cpp_market_making_simulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(market_sim
    src/main.cpp
    src/OrderBook.cpp
    src/MarketMaker.cpp
    src/Simulator.cpp
)