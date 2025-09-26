cmake_minimum_required(VERSION 3.16)
project(task1_zmq LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FILE 
    src/main.cpp
    src/analyze.hpp
    src/analyze.cpp
    src/types.hpp
    src/utils.hpp
)

add_executable(app ${FILE})   

find_package(Crow)
find_package(CLI11)
find_package(nlohmann_json)

target_link_libraries(app PRIVATE 
    Crow::Crow
    CLI11::CLI11
    nlohmann_json::nlohmann_json 
)

target_compile_definitions(app PRIVATE _WIN32_WINNT=0x0601)