cmake_minimum_required(VERSION 3.10)
project(DeribitTradingSystem)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required packages
find_package(Boost REQUIRED COMPONENTS 
    system 
    thread
)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)

# Add include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
)

# Add executable
add_executable(deribit_trader 
    src/main.cpp
    src/websocket/websocket_client.cpp
    src/order_management/order_manager.cpp
    src/order_management/order.cpp
    src/utils/logger.cpp
)

# Link libraries
target_link_libraries(deribit_trader 
    ${Boost_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
)

# Optional: Add compiler warnings
if(MSVC)
    target_compile_options(deribit_trader PRIVATE /W4)
else()
    target_compile_options(deribit_trader PRIVATE 
        -Wall 
        -Wextra 
        -Wpedantic 
        -Werror
    )
endif()