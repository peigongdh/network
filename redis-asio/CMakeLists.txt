project(redis-assio)

add_executable(mt-redis-cli multi-thread-client.cpp)
add_executable(keep-alive-cli keep-alive-client.cpp)
add_executable(general-redis-cli general-client.cpp)

set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

target_include_directories(mt-redis-cli PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(mt-redis-cli pthread)

target_include_directories(keep-alive-cli PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(keep-alive-cli pthread)

target_include_directories(general-redis-cli PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(general-redis-cli pthread)