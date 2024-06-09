cmake_minimum_required(VERSION 3.8)

set(CLIENT_SRC "client/client.c")
set(SERVER_SRC "server/server.c")
set(NET_SHARED_DIR "shared")

add_subdirectory(raylib)


project(client)

add_executable(client ${CLIENT_SRC} ${NET_SHARED_DIR}/netinterface.c)

target_include_directories(client PUBLIC ${NET_SHARED_DIR} raylib)

target_link_libraries(client PUBLIC raylib)

project (server)

add_executable(server ${SERVER_SRC} ${NET_SHARED_DIR}/netinterface.c)

target_include_directories(server PUBLIC ${NET_SHARED_DIR} raylib)

target_link_libraries(server PUBLIC raylib)