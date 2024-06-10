#include <stdio.h>
#include <uv.h>

#include "gameshared.h"
#include "levelmap.h"
static void on_recv(uv_udp_t* handle, ssize_t nread, const uv_buf_t* rcvbuf,
                    const struct sockaddr* addr, unsigned flags) {
    if (nread > 0) {
        printf("%lu\n", nread);
        printf("%s", rcvbuf->base);
    }

    free(rcvbuf->base);
}

static void on_alloc(uv_handle_t* client, size_t suggested_size,
                     uv_buf_t* buf) {
    buf->base = malloc(suggested_size);
    buf->len = suggested_size;

}

int main() {
    uv_udp_t client;
    uv_loop_t* loop = uv_default_loop();
    struct sockaddr host;
    assert(uv_udp_init(loop, &client) == 0);
    assert(uv_ip4_addr("127.0.0.1", 23456, (struct sockaddr_in*)&host) == 0);
    assert(uv_udp_recv_start(&client, on_alloc, on_recv) == 0);

    SetTraceLogLevel(LOG_NONE);
    SetTargetFPS(240);
    InitWindow(960, 540, "Flappy");
    double accumulator = 0.0;
    uint32_t tick;

    while (!WindowShouldClose()) {
        accumulator += GetFrameTime();
        while (accumulator >= kTimestep) {
            accumulator -= kTimestep;
            ++tick;
        }
        uv_udp_send_t send_req;
        uint16_t packet = 0xCAFE;
        uv_buf_t buffer = uv_buf_init((char*)&packet, sizeof(uint16_t));
        uv_udp_send(&send_req, &client, &buffer, 1, &host, NULL);
        BeginDrawing();
        ClearBackground(BLACK);
        DrawFPS(100, 100);
        EndDrawing();

        uv_run(loop, UV_RUN_NOWAIT);
    }
    CloseWindow();
    return 0;
}
