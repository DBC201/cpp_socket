cmake_minimum_required(VERSION 3.16)
project(cpp_socket)

set(CMAKE_CXX_STANDARD 20)

include_directories(
	PUBLIC ${CMAKE_SOURCE_DIR}/include
)

add_executable(tcp_server examples/transportlayer/tcp/server.cpp)
add_executable(tcp_client examples/transportlayer/tcp/client.cpp)

if (UNIX)
	add_executable(eth examples/linklayer/eth.cpp)
    add_executable(netlink examples/netlink/netlink_test.cpp)
    add_executable(unix_proc_a examples/unix/proc_a.cpp)
    add_executable(unix_proc_b examples/unix/proc_b.cpp)
	target_link_libraries(tcp_server pthread)
endif()
