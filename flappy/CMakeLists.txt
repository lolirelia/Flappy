cmake_minimum_required(VERSION 3.8)

set(CLIENT_SRC "client/client.c")
set(SERVER_SRC "server/server.c")
set(SHARED_DIR "shared")

add_subdirectory(raylib)
add_subdirectory(libuv)

project(client)

add_executable(client ${CLIENT_SRC} )

target_include_directories(client PUBLIC ${SHARED_DIR} raylib libuv/include)

target_link_libraries(client PUBLIC raylib uv)

project (server)

add_executable(server ${SERVER_SRC} )

target_include_directories(server PUBLIC ${SHARED_DIR} raylib libuv/include)

target_link_libraries(server PUBLIC raylib uv)