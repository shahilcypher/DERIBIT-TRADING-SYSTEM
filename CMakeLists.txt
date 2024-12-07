cmake_minimum_required(VERSION 3.10)
project(DeribitTradingSystem)

set(CMAKE_CXX_STANDARD 17)

# Find required packages
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)

# Add include directories (more explicit)
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
)

# Add executable
add_executable(deribit_trader 
    src/main.cpp
    src/websocket/websocket_client.cpp
)

# Link libraries
target_link_libraries(deribit_trader 
    ${Boost_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
)