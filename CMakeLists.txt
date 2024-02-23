cmake_minimum_required(VERSION 3.16)
project(cpp_socket)

set(CMAKE_CXX_STANDARD 20)

add_executable(server src/server.cpp)

add_executable(client src/client.cpp)

target_include_directories(server PUBLIC include)
target_include_directories(client PUBLIC include)

if (UNIX)
	target_link_libraries(server pthread)
endif()
