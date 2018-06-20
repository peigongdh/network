project(redis-assio)

add_executable(redis-cli client.cpp)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

target_include_directories(redis-cli PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(redis-cli pthread)