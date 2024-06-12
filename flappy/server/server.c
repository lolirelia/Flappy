#include <stdio.h>

#include "gameshared.h"
#include "levelmap.h"
#include "netinterface.h"
#include <raymath.h>
#include <uv.h>
struct GamestateServerside gstate;

void InsertPlayerIntoGamestate() {
    if (gstate.playercount < kMaxNumberOfPlayers) {


///
        gstate.players[gstate.playercount].player.id = ++gstate.playercount;
    }
}

/*
void Callback(const NetInterface* ni, NetInterfaceAddr* niAddr,
              const char* buffer, NITransferSize niTransferSize) {
                printf("got a message\n");
    if (niTransferSize == sizeof(uint16_t)) {
        uint16_t packet;
        memcpy(&packet, buffer, sizeof(uint16_t));
        if (packet == 0xCAFE) {
            printf("%d.%d.%d.%d - Wants to join\n",
                   niAddr->m_niRemoteAddr.sin_addr.s_addr >> 0 & 0xFF,
                   niAddr->m_niRemoteAddr.sin_addr.s_addr >> 8 & 0xFF,
                   niAddr->m_niRemoteAddr.sin_addr.s_addr >> 16 & 0xFF,
                   niAddr->m_niRemoteAddr.sin_addr.s_addr >> 24 & 0xFF);
            // insert player into the game
            InsertPlayerIntoGamestate(niAddr);
        }
    } else if (niTransferSize == sizeof(uint8_t)) {
        uint8_t packet;
        memcpy(&packet, 0, sizeof(uint8_t));
        struct PlayerServerside* player = NULL;
        if(gstate.playercount>0){
            for (uint32_t n = 0; n < kMaxNumberOfPlayers; ++n) {

                if (player->netid.m_niRemoteAddr.sin_addr.s_addr ==
                    niAddr->m_niRemoteAddr.sin_addr.s_addr) {
                    player = &gstate.players[n];
                    break;
                }
            }
            if(player != NULL){
                player->isflapping = packet;
            }
        }

    }
}
*/

void GameUpdate() {
    static const float kGravity = 9.8f;
    static const float kXVelocity = 1.f;
    if(gstate.playercount==kMaxNumberOfPlayers){
        struct GamestateClient gstateclient;

        ++gstate.tick;

        gstateclient.tick = gstate.tick;
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

            // gstateclient.players[n].id = player->player.id;
            gstateclient.players[n].id = player->player.id;
            gstateclient.players[n].position = player->player.position;
        }
        gstateclient.tick = gstate.tick;

        for (uint32_t n = 0; n < kMaxNumberOfPlayers; ++n) {
            //SendPacket(ni, &gstate.players[n].netid, (char*)&gstateclient,sizeof(struct GamestateClient));
        }
    }
  
}

static void on_recv(uv_udp_t* handle, ssize_t nread, const uv_buf_t* rcvbuf,
                    const struct sockaddr* addr, unsigned flags) {
    if (nread == sizeof(uint16_t)) {
        uint16_t packet = 0;
        memcpy(&packet,rcvbuf->base,sizeof(uint16_t));
        if(packet==0xCAFE){
            printf("Player wants to join\n");
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
    
    
    uv_ip4_addr("127.0.0.1", 23456,&recv_addr);
    assert(uv_udp_init(loop, &server)==0);
    assert(uv_udp_bind(&server,(struct sockaddr*)&recv_addr, 0) == 0);
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
            GameUpdate();
        }

        BeginDrawing();

        ClearBackground(BLACK);
        DrawFPS(100,100);
        EndDrawing();

        uv_run(loop, UV_RUN_NOWAIT);
    }
    CloseWindow();
    uv_loop_close(loop);
    return 0;
}
