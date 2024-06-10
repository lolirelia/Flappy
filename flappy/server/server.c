

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <uv.h>

#include "gameshared.h"
#include "levelmap.h"
struct GamestateServerside gstate;

void InsertPlayerIntoGamestate(uint32_t ipv4, uint16_t port) {
    if (gstate.playercount < kMaxNumberOfPlayers) {
        gstate.players[gstate.playercount].ipv4 = ipv4;
        gstate.players[gstate.playercount].port = port;
        char ipbuffer[32] = {0};

        sprintf(ipbuffer, "%d.%d.%d.%d", (ipv4 >> 0) & 0xFF, (ipv4 >> 8) & 0xFF,
                (ipv4 >> 16) & 0xFF, (ipv4 >> 24) & 0xFF);
        printf("%s %d\n", ipbuffer, port);
        uv_ip4_addr(ipbuffer, htons(port),
                    &gstate.players[gstate.playercount].addrin);
        gstate.players[gstate.playercount].player.id = ++gstate.playercount;
    }
    if (gstate.playercount == kMaxNumberOfPlayers) {
        gstate.gamestarted = 1;
    }
}

void GameUpdate(uv_udp_t* server) {
    static const float kGravity = 5.f;
    static const float kXVelocity = 1.f;
    if (gstate.playercount == kMaxNumberOfPlayers) {
        struct GamestateClient gstateclient;

        ++gstate.tick;

        // gstateclient.tick = gstate.tick;
        for (uint32_t n = 0; n < kMaxNumberOfPlayers; ++n) {
            struct PlayerServerside* player = &gstate.players[n];
            if (player->isflapping) {
                player->velocity.y = -kGravity;
            } else {
                player->velocity.y += kGravity;
            }

            player->velocity.x = kXVelocity;

            player->player.position =
                Vector2Add(player->player.position, player->velocity);
            // player->player.x+=player->velocity.x;
            // player->player.y+=player->velocity.y;

            kPackPlayerId(gstateclient.players[n].x, player->player.id,
                          gstate.tick);
            kPackPlayerPosition(gstateclient.players[n].y,
                                player->player.position);
        }
        uv_udp_send_t send_req;
        uv_buf_t buffer =
            uv_buf_init((char*)&gstateclient, sizeof(struct GamestateClient));
        for (uint32_t n = 0; n < kMaxNumberOfPlayers; ++n) {
            uv_udp_send(&send_req, server, &buffer, 1,
                        &gstate.players[n].addrin, NULL);
        }
    }
}

static void on_recv(uv_udp_t* handle, ssize_t nread, const uv_buf_t* rcvbuf,
                    const struct sockaddr* addr, unsigned flags) {
    if (nread == sizeof(uint64_t)) {
        uint64_t packet = 0;
        memcpy(&packet, rcvbuf->base, sizeof(packet));
        uint16_t packetid = kGetPacketId(packet);
        if (packetid == kEFlappyPacketInsertPlayer) {
            printf("inserting player into game: ");
            uint32_t ipv4 = kGetAddrPtr(addr)->sin_addr.s_addr;
            uint16_t port = kGetAddrPtr(addr)->sin_port;
            printf("%d.%d.%d.%d %d\n", ipv4 >> 0 & 0xFF, ipv4 >> 8 & 0xFF,
                   ipv4 >> 16 & 0xFF, ipv4 >> 24 & 0xFF, port);
            struct sockaddr local;
            InsertPlayerIntoGamestate(ipv4, port);
        } else if (packetid == kEFlappyPacketIdInput) {
            if (gstate.gamestarted) {
                struct PlayerServerside* player = NULL;
                uint32_t ipv4 = kGetAddrPtr(addr)->sin_addr.s_addr;
                uint16_t port = kGetAddrPtr(addr)->sin_port;
                for (uint32_t n = 0; n < kMaxNumberOfPlayers; ++n) {
                    if (gstate.players[n].ipv4 == ipv4 &&
                        gstate.players[n].port == port) {
                        player = &gstate.players[n];
                        break;
                    }
                }
                assert(player != NULL);
                player->isflapping = kGetInput(packet);
            }
        }
    }

    free(rcvbuf->base);
}
static void on_alloc(uv_handle_t* client, size_t suggested_size,
                     uv_buf_t* buf) {
    buf->base = malloc(suggested_size);
    buf->len = suggested_size;
}
int main() {
    uv_loop_t* loop = uv_default_loop();
    struct sockaddr_in recv_addr;
    uv_udp_t server;

    uv_ip4_addr("127.0.0.1", 23456, &recv_addr);
    assert(uv_udp_init(loop, &server) == 0);
    assert(uv_udp_bind(&server, (struct sockaddr*)&recv_addr, 0) == 0);
    assert(uv_udp_recv_start(&server, on_alloc, on_recv) == 0);

    SetTraceLogLevel(LOG_NONE);
    SetTargetFPS(240);
    InitWindow(960, 540, "Flappy");
    memset(&gstate, 0, sizeof(struct GamestateServerside));
    double accumulator = 0.0;

    while (!WindowShouldClose()) {
        accumulator += GetFrameTime();
        while (accumulator >= kTimestep) {
            accumulator -= kTimestep;
            GameUpdate(&server);
        }

        BeginDrawing();

        ClearBackground(BLACK);
        DrawFPS(100, 100);
        EndDrawing();

        uv_run(loop, UV_RUN_NOWAIT);
    }
    CloseWindow();
    uv_loop_close(loop);
    return 0;
}
