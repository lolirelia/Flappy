cmake_minimum_required(VERSION 3.8)


set(SHARED_DIR "shared")

add_subdirectory(raylib)
add_subdirectory(libuv)

project(client)

add_executable(client client/client.c client/luvclient.c ${SHARED_DIR}/levelmap.c )

target_include_directories(client PUBLIC ${SHARED_DIR} raylib libuv/include)

target_link_libraries(client PUBLIC raylib uv)




project (server)

add_executable(server server/server.c server/luvserver.c ${SHARED_DIR}/levelmap.c )

target_include_directories(server PUBLIC ${SHARED_DIR} raylib libuv/include)

target_link_libraries(server PUBLIC raylib uv)