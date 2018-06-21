project(redis-assio)

add_executable(mt-redis-cli multi-thread-client.cpp)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

target_include_directories(mt-redis-cli PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(mt-redis-cli pthread)