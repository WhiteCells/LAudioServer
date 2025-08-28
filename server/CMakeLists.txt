cmake_minimum_required(VERSION 3.10)

project(server LANGUAGES CXX VERSION 1.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC
    src/main.cc
    src/session_impl.h
    src/session_impl.cc
    src/session_mgr.h
    src/session_mgr.cc
)

add_executable(${PROJECT_NAME} ${SRC})

target_include_directories(${PROJECT_NAME} PRIVATE)

target_link_directories(${PROJECT_NAME} PRIVATE )

target_link_libraries(${PROJECT_NAME} PRIVATE)