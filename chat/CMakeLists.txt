project(chat)

add_executable(chat-svr chat_server.cpp)
add_executable(chat-cli chat_client.cpp)
add_executable(posix-cli posix_chat_client.cpp)


#set(EXECUTABLE_OUTPUT_PATH ${BINARY_OUTPUT_DIR})

target_include_directories(chat-svr PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(chat-svr pthread)

target_include_directories(chat-cli PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(chat-cli pthread)

target_include_directories(posix-cli PRIVATE ${ASIO_INCLUDE_DIR})
target_link_libraries(posix-cli pthread)