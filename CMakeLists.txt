cmake_minimum_required(VERSION 3.10.2)
project(Server)

set(CMAKE_CXX_STANDARD 14)

find_package(Threads REQUIRED)

add_executable(Server main.cpp)

target_link_libraries(${CMAKE_PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})