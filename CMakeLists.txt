cmake_minimum_required(VERSION 3.16)
project(cpp_socket)

set(CMAKE_CXX_STANDARD 20)

include_directories(
	PUBLIC ${CMAKE_SOURCE_DIR}/include
)

add_executable(tcp_server src/transportlayer/tcp/server.cpp)
add_executable(tcp_client src/transportlayer/tcp/server.cpp)

if (UNIX)
	add_executable(eth src/linklayer/eth.cpp)
	target_link_libraries(tcp_server pthread)
endif()
