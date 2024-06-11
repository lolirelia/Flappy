

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "gameshared.h"
#include "levelmap.h"
#define CVECTOR_LOGARITHMIC_GROWTH
#include "cvector.h"
#include "cvector_utils.h"

uint32_t g_myid = 0;
uint32_t g_tick = 0;
uint32_t g_serverbasetick = 0;
uint32_t g_starttick = 0;

cvector_vector_type(struct GamestateClient) gamestates = NULL;

int GetServerTick() { return g_serverbasetick + (g_tick - g_starttick) - kRenderDelayTicks; }

int GetNextFramendex() {
    int delay = GetServerTick();
    uint32_t i = 0;
    for (int i = cvector_size(gamestates) - 1; i >= 0; --i) {
        uint32_t quetick = kGetTick(gamestates[i].players[0].x);
        if (quetick <= delay) {
            return i;
        } else {
        }
    }
    return -1;
}


void on_recv(uv_udp_t* handle, ssize_t nread, const uv_buf_t* rcvbuf,
             const struct sockaddr* addr, unsigned flags) {
    if (nread > 0) {
        if (nread == sizeof(uint64_t)) {
            uint64_t packet = 0;
            memcpy(&packet, rcvbuf->base, sizeof(packet));
            if (kGetPacketId(packet) == kEFlappyPacketIdPlayerId) {
                g_myid = kGetPlayerId(packet);
                printf("my id:%u\n",g_myid);

            } else {
            }

        } else if (nread == sizeof(struct GamestateClient)) {
            struct GamestateClient gstate;
            memcpy(&gstate, rcvbuf->base, sizeof(struct GamestateClient));
            cvector_push_back(gamestates, gstate);
            uint32_t t = kGetTick(gstate.players[0].x);
            if(g_starttick == 0&&g_serverbasetick==0){
                g_starttick = g_tick;
                g_serverbasetick = t;
            }
            for(int n = 0; n < kMaxNumberOfPlayers;++n){
                uint32_t id = kGetPlayerId(gstate.players[n].x);
            }
            // printf("Got state\n");
            // printf("%u\t%u\n",
            // kGetPlayerId(gstate.players[0].x),kGetPlayerId(gstate.players[1].x));
            int ndex = GetNextFramendex();

            if (ndex > -1) {
                for (int n = 0; n < ndex; ++n) {
                    uint32_t t = kGetTick(gamestates[n].players[0].x);
                    cvector_erase(gamestates, n);
                }
            }
        }
    }

    free(rcvbuf->base);
}

void on_alloc(uv_handle_t* client, size_t suggested_size, uv_buf_t* buf) {
    buf->base = malloc(suggested_size);
    buf->len = suggested_size;
}
void on_send(uv_udp_send_t* req, int status) {
    assert(req != NULL);
    assert(req->data != NULL);
    uv_buf_t* buf = (uv_buf_t*)req->data;
    assert(buf->base != NULL);
    free(buf->base);
    free(req->data);
    free(req);
    if (status) {
        printf("status:%s\n", uv_strerror(status));
    }
}
uv_udp_send_t* AllocateBuffer(void* data, uint32_t size) {
    uv_udp_send_t* req = malloc(sizeof(uv_udp_send_t));
    req->data = malloc(sizeof(uv_buf_t));
    void* buffer = malloc(size);
    memcpy(buffer, data, size);
    uv_buf_t buf = uv_buf_init(buffer, size);
    memcpy(req->data, &buf, sizeof(uv_buf_t));
    return req;
}

void GetGamestateToRender(struct PlayerRenderData* result) {
    int nextframe = GetNextFramendex();
    if (nextframe < 0) {
        // game hasn't started yet
        memset(result,0,sizeof(struct PlayerRenderData)*kMaxNumberOfPlayers);
    } else if (nextframe == cvector_size(gamestates) - 1) {
        // next packet hasn't arrived
        // server is late so just return the latest packet we have
        struct GamestateClient dst = gamestates[cvector_size(gamestates) - 1];

        for(int ndex = 0; ndex < kMaxNumberOfPlayers; ++ndex){
            memcpy(&result[ndex].position,&dst.players[ndex].y,sizeof(uint64_t));
            uint32_t id = kGetPlayerId(dst.players[ndex].x);
            result[ndex].id = id;
        }
        
    } else {
        // we have a current packet and a next packet to interpolate t
        uint32_t servertick = GetServerTick();
        struct GamestateClient next = gamestates[nextframe + 1];
        struct GamestateClient render = gamestates[nextframe];


        uint32_t tick = kGetTick(render.players[0].x);
        uint32_t nexttick = kGetTick(next.players[0].x);
        float alpha = (((float)servertick * 1.0) - ((float)tick * 1.0)) /
                      (((float)nexttick * 1.0) - ((float)tick * 1.0));

        for(int ndex = 0; ndex < kMaxNumberOfPlayers; ++ndex){
            Vector2 start = {0};
            Vector2 end = {0};
            uint32_t startid = kGetPlayerId(render.players[ndex].x);
            uint32_t endid = kGetPlayerId(next.players[ndex].x);
            assert(startid == endid);

        
            memcpy(&start,&render.players[ndex].y, sizeof(uint64_t));
            memcpy(&end,&next.players[ndex].y,sizeof(uint64_t));


            Vector2 interpolated;
            interpolated.x = start.x + (end.x - start.x) * alpha;
            interpolated.y = start.y + (end.y - start.y) * alpha;

            result[ndex].id = startid;
            result[ndex].position = interpolated;
        }

        //return render;
    }
}


//
int main() {
    uv_udp_t client;
    uv_loop_t* loop = uv_default_loop();
    struct sockaddr host;
    assert(uv_udp_init(loop, &client) == 0);
    assert(uv_ip4_addr("0.0.0.0", 23456, (struct sockaddr_in*)&host) == 0);
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
    Camera2D viewport;
    viewport.offset = (Vector2){960/2.f,0};
    viewport.target = (Vector2){0.f,0.f};
    viewport.rotation = 0.f;
    viewport.zoom = 1.f;
    LoadLevel();
    while (!WindowShouldClose()) {
        accumulator += GetFrameTime();
        while (accumulator >= kTimestep) {
            accumulator -= kTimestep;
            ++g_tick;
        }
        if (IsMouseButtonPressed(0)) {
            uint64_t packet = 0;
            kPackPlayerInput(packet, 1);
            uv_udp_send_t* req = AllocateBuffer(&packet, sizeof(packet));
            uv_udp_send(req, &client, req->data, 1, &host, on_send);
        } else if (IsMouseButtonReleased(0)) {
            uint64_t packet = 0;
            kPackPlayerInput(packet, 0);
            uv_udp_send_t* req = AllocateBuffer(&packet, sizeof(packet));
            uv_udp_send(req, &client, req->data, 1, &host, on_send);
        }
        struct PlayerRenderData render[kMaxNumberOfPlayers];

        GetGamestateToRender(render);
        BeginDrawing();
        ClearBackground(BLACK);
        DrawFPS(100, 100);
        BeginMode2D(viewport);
        const struct MapInstance* map = GetMapInstance();
        for(int ndex = 0; ndex < map->collisioncount; ++ndex){
            DrawRectangleLinesEx(map->collisions[ndex],1.f,GREEN);
        }
        for (int ndex = 0; ndex < kMaxNumberOfPlayers; ++ndex) {
            Color c = RED;
            if(render[ndex].id == g_myid){
                c = GOLD;
                viewport.target.x = render[ndex].position.x;
                
            }

            DrawRectangle(render[ndex].position.x, render[ndex].position.y, kFlappyCollisionSize, kFlappyCollisionSize, c);
        }

        EndMode2D();
        EndDrawing();

        uv_run(loop, UV_RUN_NOWAIT);
    }
    CloseWindow();
    return 0;
}
