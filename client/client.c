

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
struct ClientSidePrediction {
    struct Vector2 position;
    struct Vector2 velocity;
    uint8_t isflapping;
    uint32_t tick;
};
extern cvector_vector_type(struct ClientSidePrediction) predictions;
extern struct ClientSidePrediction csp;
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

        Vector2 myposition  = Vector2Zero();
        viewport.target.x = csp.position.x;
        BeginMode2D(viewport);
        const struct MapInstance* map = GetMapInstance();
        for(int ndex = 0; ndex < map->collisioncount; ++ndex){
            DrawRectangleLinesEx(map->collisions[ndex],1.f,GREEN);
        }
        for (int ndex = 0; ndex < kMaxNumberOfPlayers; ++ndex) {
            Color c = RED;
            if(render[ndex].id == g_myid){
                //
                // Uncomment this if you want to see Your actual position
                // relative to the server
                //
                // DrawText("State",render[ndex].x-7,render[ndex].y-15,15,RED);
                // DrawRectangle(render[ndex].x,render[ndex].y,kFlappyCollisionSize, kFlappyCollisionSize, RED);
            }else{
                //if there are more players, draw them as green
                DrawRectangle(render[ndex].x, render[ndex].y,
                              kFlappyCollisionSize, kFlappyCollisionSize, GREEN);
            }
        }

        //Your predicted position
        //DrawText("Predict", csp.position.x-15, csp.position.y - 15, 15, GOLD);
        DrawRectangle(csp.position.x, csp.position.y, kFlappyCollisionSize,
                      kFlappyCollisionSize, GOLD);

        EndMode2D();
        DrawText(TextFormat("%.2f:%.2f\n", csp.position.y,csp.position.x),
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
