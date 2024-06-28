

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
uint16_t g_myid = 0;
uint32_t g_tick = 0;
uint32_t g_serverbasetick = 0;
uint32_t g_starttick = 0;
uint32_t winner = 0;
uint32_t winnerid = 0;


struct PlayerClient {
    Vector2 position;
    uint16_t id;
};

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
                viewport.target.x = render[ndex].x;
               myposition = (struct Vector2){render[ndex].x,render[ndex].y};
            }

            DrawRectangle(render[ndex].x, render[ndex].y, kFlappyCollisionSize, kFlappyCollisionSize, c);
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
