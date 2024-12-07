cmake_minimum_required(VERSION 3.10)
project(MyProject)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find Boost
find_package(Boost REQUIRED COMPONENTS system thread)

# Include directories for Boost
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

# Add dependencies
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)

# Add nlohmann/json (header-only library)
set(JSON_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/json/include")
include_directories(${JSON_INCLUDE_DIR})

# Add executable
add_executable(${PROJECT_NAME} main.cpp)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    Boost::system
    Boost::thread
    spdlog::spdlog
    fmt::fmt
)
