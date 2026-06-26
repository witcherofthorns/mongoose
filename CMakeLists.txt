cmake_minimum_required(VERSION 3.10)

# project
project(mongoose CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# options
option(MONGOOSE_BUILD_TESTS "Mongoose build tests" OFF)
option(MONGOOSE_LINK_MONGO_STATIC "Mongoose static link mongocxx" ON)
option(MONGOOSE_USE_NLOHMANN_JSON "Mongoose use nlohmann json library" ON)

# check package nlohmann_json
if(MONGOOSE_USE_NLOHMANN_JSON)
    find_package(nlohmann_json QUIET)
    if(nlohmann_json_FOUND)
        message("++ Mongoose: nlohmann json found: ${nlohmann_json_VERSION}")
        add_compile_definitions(MONGOOSE_USE_NLOHMANN_JSON=1)
    else()
        message(WARNING "nlohmann json not found, JSON support disabled")
    endif()
endif()

# check package mongocxx
find_package(mongocxx REQUIRED)

# sources
file(GLOB_RECURSE SRC ${PROJECT_SOURCE_DIR}/src/*.cpp)

# library
add_library(${PROJECT_NAME} STATIC)
target_sources(${PROJECT_NAME} PRIVATE ${SRC})
target_include_directories(${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

# link mongocxx
if(MONGOOSE_LINK_MONGO_STATIC)
    target_link_libraries(${PROJECT_NAME} PUBLIC mongo::mongocxx_static)
else()
    target_link_libraries(${PROJECT_NAME} PUBLIC mongo::mongocxx_shared)
endif()

# сonditionally link nlohmann_json
if(MONGOOSE_USE_NLOHMANN_JSON AND nlohmann_json_FOUND)
    target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json)
    target_compile_definitions(${PROJECT_NAME} PUBLIC MONGOOSE_USE_NLOHMANN_JSON=1)
endif()

# tests
if(MONGOOSE_BUILD_TESTS)
    message("++ Mongoose: Tests enabled")
    add_subdirectory(${PROJECT_SOURCE_DIR}/tests)
endif()