cmake_minimum_required(VERSION 3.16)
project(task1_zmq LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_BINARY_DIR}/bin/${OUTPUTCONFIG})
endforeach()


set(FILE 
            src/main.cpp
            src/server.cpp
            src/server.hpp
            src/student.hpp
            src/data_loader.hpp
            src/data_loader.cpp
)

add_executable(app ${FILE})   


find_package(cppzmq)
find_package(nlohmann_json)
find_package(CLI11)

target_link_libraries(app PRIVATE 
    cppzmq 
    nlohmann_json::nlohmann_json 
    CLI11::CLI11
)

