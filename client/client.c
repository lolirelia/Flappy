

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
uint32_t winner = 0;
uint32_t winnerid = 0;
void on_recv(uv_udp_t* handle, ssize_t nread, const uv_buf_t* rcvbuf,
             const struct sockaddr* addr, unsigned flags) {
    if (nread > 0) {
        if (nread == sizeof(uint64_t)) {
            uint64_t packet = 0;
            memcpy(&packet, rcvbuf->base, sizeof(packet));
            if (kGetPacketId(packet) == kEFlappyPacketIdPlayerId) {
                g_myid = kGetPlayerId(packet);
                printf("my id:%u\n",g_myid);

            } else if(kGetPacketId(packet) == kEFlappyPacketWinner){
                winner = 1;
                winnerid = kGetPlayerId(packet);
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


                //this is each client's predicted or reconciliation tick sent to the server during update
                //this tick will be used to check the map or container of the [tick][position]
                //If the player's predicted position with their tick matches the server position with tick
                //then no reconciliation needs to be done
                
                uint32_t predictedtick = kGetPredictionTick(gstate.players[n].z);
                //printf("%u\n",predictedtick);
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
    free(req->data);
    free(req);
    if (status) {
        printf("status:%s\n", uv_strerror(status));
    }
}
uv_udp_send_t* AllocateBuffer(uv_buf_t* uvbuf, void* data, uint32_t size) {
    uv_udp_send_t* req = malloc(sizeof(uv_udp_send_t));

    req->data = malloc(size);
    memcpy(req->data, data, size);
    *uvbuf = uv_buf_init(req->data, size);
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

void UpdateFlappy(uv_udp_t* client, struct sockaddr* host,uint8_t isflapping) {
    uv_buf_t uvbuf;
    uint64_t packet = 0;
    kPackPlayerInput(packet, isflapping,g_tick);

    uv_udp_send_t* req = AllocateBuffer(&uvbuf, &packet, sizeof(packet));
    uv_udp_send(req, client, &uvbuf, 1, host, on_send);
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

    uint64_t packet = 0;
    kPackInsertPlayer(packet);
    uv_buf_t uvb;
    uv_udp_send_t* send_req = AllocateBuffer(&uvb, &packet, sizeof(packet));
    uv_udp_send(send_req, &client, &uvb, 1, &host, NULL);

    SetTraceLogLevel(LOG_NONE);
    SetTargetFPS(240);
    InitWindow(960, 540, "Flappy - Client");
    double accumulator = 0.0;
    Camera2D viewport;
    viewport.offset = (Vector2){960/2.f,0};
    viewport.target = (Vector2){0.f,0.f};
    viewport.rotation = 0.f;
    viewport.zoom = 1.f;
    LoadLevel();
    uint8_t isflapping = 0;
    while (!WindowShouldClose()) {
        accumulator += GetFrameTime();
        while (accumulator >= kTimestep) {
            accumulator -= kTimestep;
            ++g_tick;
            UpdateFlappy(&client, &host, isflapping);
        }
        if (IsMouseButtonPressed(0)) {
            isflapping = 1;
        } else if (IsMouseButtonReleased(0)) {
            isflapping = 0;
        }
        struct PlayerRenderData render[kMaxNumberOfPlayers];

        GetGamestateToRender(render);
        BeginDrawing();
        ClearBackground(BLACK);
        //DrawFPS(100, 100);

        Vector2 myposition ;
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
               myposition = render[ndex].position;
            }

            DrawRectangle(render[ndex].position.x, render[ndex].position.y, kFlappyCollisionSize, kFlappyCollisionSize, c);
        }

        EndMode2D();
        DrawText(TextFormat("%.2f:%.2f\n", myposition.x,myposition.y),
                 25, 25, 20, GOLD);

        if(winner){
            if(winnerid==g_myid){
                DrawText("YOU WON!",960/2.f,540/2.f,30,WHITE);
            }
            else {
                DrawText("YOU LOSE!", 960 / 2.f, 540 / 2.f, 30, WHITE);
            }
        }
        EndDrawing();

        uv_run(loop, UV_RUN_NOWAIT);
    }
    CloseWindow();
    return 0;
}
