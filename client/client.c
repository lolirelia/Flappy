

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <raylib.h>
#include <raymath.h>


#include "gameshared.h"
#include "levelmap.h"
#define CVECTOR_LOGARITHMIC_GROWTH
#include "cvector.h"
#include "cvector_utils.h"
#include "luvclient.h"
uint32_t g_myid = 0;
uint32_t g_tick = 0;
uint32_t g_serverbasetick = 0;
uint32_t g_starttick = 0;
uint32_t winner = 0;
uint32_t winnerid = 0;
struct PlayerClient {
    Vector2 position;
    uint16_t id;
};

struct PlayerRenderData {
    uint32_t id;
    Vector2 position;
};
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


    // join the game

    InitLuv();
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
            UpdateFlappy( isflapping);
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

        RunLuvLoop();
    }
    CloseWindow();
    return 0;
}
