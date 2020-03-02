cmake_minimum_required(VERSION 3.10.2)
project(Server)

set(CMAKE_CXX_STANDARD 14)

ADD_DEFINITIONS(-DBOOST_LOG_DYN_LINK) 

# for std::thread
find_package(Threads REQUIRED)

# boost
find_package(Boost REQUIRED COMPONENTS system filesystem thread log_setup log)

# libevent library
SET(LIBEVENT_NAME "event" CACHE STRING "Name of the libevent library (as used in the linker argument).")
SET(LIBEVENT_LIB_DIR "/usr/local/lib" CACHE STRING "Path to libevent libraries.")
SET(LIBEVENT_INCLUDE_DIR "/usr/local/include" CACHE STRING "Path to libevent headers.")

MESSAGE("LIBEVENT_NAME: " ${LIBEVENT_NAME})
MESSAGE("LIBEVENT_LIB_DIR: " ${LIBEVENT_LIB_DIR})
MESSAGE("LIBEVENT_INCLUDE_DIR: " ${LIBEVENT_INCLUDE_DIR})

link_directories("${LIBEVENT_LIB_DIR}/")

# Includes
include_directories("${LIBEVENT_INCLUDE_DIR}")
include_directories(SYSTEM ${Boost_INCLUDE_DIRS} 
    ${CMAKE_CURRENT_SOURCE_DIR})

# Sources
set(SRC_LIST
    thread_pool/thread_pool.cpp
    server/server.cpp
    config_parser/config_parser.cpp
    main.cpp
)

add_executable(Server ${SRC_LIST})

target_link_libraries(${CMAKE_PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(${CMAKE_PROJECT_NAME} ${Boost_LIBRARIES})
target_link_libraries(${CMAKE_PROJECT_NAME} ${LIBEVENT_NAME})