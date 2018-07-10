project(network)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "-Wall -std=c++11 -DASIO_STANDALONE")

set(ASIO_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/asio/asio/include)
set(BINARY_OUTPUT_DIR ${PROJECT_SOURCE_DIR}/bin)

add_subdirectory(shadowsocks-asio)
add_subdirectory(timer-performance)
add_subdirectory(httpclient-https)
add_subdirectory(redis-asio)
add_subdirectory(fixed-size-package)
add_subdirectory(io_context)
add_subdirectory(simplest_httpsvr)
