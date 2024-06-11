

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <uv.h>
#include <string.h>
#include "gameshared.h"
#include "levelmap.h"
#define CVECTOR_LOGARITHMIC_GROWTH
#include "cvector.h"
#include "cvector_utils.h"

uint32_t myid = 0;
uint32_t tick = 0;
uint32_t serverbasetick = 0;
uint32_t starttick = 0;

cvector_vector_type(struct GamestateClient) gamestates = NULL;

int GetServerTick() { return serverbasetick + (tick - starttick) -6; }

int GetNextFramendex() {
    int delay = GetServerTick();
    uint32_t i = 0;
    for (int i = cvector_size(gamestates)-1; i >= 0; --i) {
        uint32_t quetick = kGetTick(gamestates[i].players[0].x);
        if (quetick <= delay) {
            return i;
        } else {
        }
    }
    return -1;
}

struct GamestateClient GetGamestateToRender(){
    int nextframe = GetNextFramendex();
    if(nextframe <0){

        struct GamestateClient empty;
    
       return empty;
    }
    else if(nextframe == cvector_size(gamestates) -1 ){

        return gamestates[cvector_size(gamestates) - 1];
    }else{

        return gamestates[nextframe];
    }
}
static void on_recv(uv_udp_t* handle, ssize_t nread, const uv_buf_t* rcvbuf,
                    const struct sockaddr* addr, unsigned flags) {
    if (nread > 0) {
        if (nread == sizeof(uint64_t)) {
            uint64_t packet = 0;
            memcpy(&packet, rcvbuf->base, sizeof(packet));
            if (kGetPacketId(packet) == kEFlappyPacketIdPlayerId) {
                myid = kGetPlayerId(packet);
                starttick = tick;
                serverbasetick = kGetTick(packet);
                printf("My player id:%u\n",myid);
            }else{
                
            }

        } else if (nread == sizeof(struct GamestateClient)) {

            struct GamestateClient gstate;
            memcpy(&gstate, rcvbuf->base, sizeof(struct GamestateClient));
            cvector_push_back(gamestates,gstate);
            uint32_t t = kGetTick(gstate.players[0].x);
            printf("Got state\n");
            printf("%u\t%u\n", kGetPlayerId(gstate.players[0].x),kGetPlayerId(gstate.players[1].x));
            int ndex = GetNextFramendex();
            
            if (ndex > -1) {
                for(int n = 0; n < ndex; ++n){
                    
                    uint32_t t = kGetTick(gamestates[n].players[0].x);
                    cvector_erase(gamestates,n);
                }
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
    uv_udp_t client;
    uv_loop_t* loop = uv_default_loop();
    struct sockaddr host;
    assert(uv_udp_init(loop, &client) == 0);
    assert(uv_ip4_addr("127.0.0.1", 23456, (struct sockaddr_in*)&host) == 0);
    assert(uv_udp_recv_start(&client, on_alloc, on_recv) == 0);

    // join the game
    uv_udp_send_t send_req;
    uint64_t packet = 0;
    kPackInsertPlayer(packet);
    uv_buf_t buffer = uv_buf_init((char*)&packet, sizeof(packet));
    uv_udp_send(&send_req, &client, &buffer, 1, &host, NULL);

    SetTraceLogLevel(LOG_NONE);
    SetTargetFPS(240);
    InitWindow(960, 540, "Flappy");
    double accumulator = 0.0;

    while (!WindowShouldClose()) {
        accumulator += GetFrameTime();
        while (accumulator >= kTimestep) {
            accumulator -= kTimestep;
            ++tick;
        }
        if (IsMouseButtonPressed(0)) {
            uv_udp_send_t send_req;
            uint64_t packet = 0;
            kPackPlayerInput(packet,1);
            uv_buf_t buffer = uv_buf_init((char*)&packet, sizeof(packet));
            uv_udp_send(&send_req, &client, &buffer, 1, &host, NULL);
        } else if (IsMouseButtonReleased(0)) {
            uv_udp_send_t send_req;
            uint64_t packet = 0;
            kPackPlayerInput(packet, 0);
            uv_buf_t buffer = uv_buf_init((char*)&packet, sizeof(packet));
            uv_udp_send(&send_req, &client, &buffer, 1, &host, NULL);
        }
        struct GamestateClient render = GetGamestateToRender();
        BeginDrawing();
        ClearBackground(BLACK);
        DrawFPS(100, 100);
        for(int n = 0; n < kMaxNumberOfPlayers; ++n){
            
            Vector2 position = {0};
            memcpy(&position,&render.players[n].y,sizeof(uint64_t));
            DrawRectangle(position.x,position.y,16,16,RED);
            
        }
        EndDrawing();

        uv_run(loop, UV_RUN_NOWAIT);
    }
    CloseWindow();
    return 0;
}
