

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <raylib.h>
#include <raymath.h>

#include "gameshared.h"
#include "levelmap.h"
#include "luvserver.h"

//#2 Complete

int main() {

    InitLuv();
    SetTraceLogLevel(LOG_NONE);
    SetTargetFPS(240);
    InitWindow(960, 540, "Flappy - Server");
    
    
    double accumulator = 0.0;

    while (!WindowShouldClose()) {
        accumulator += GetFrameTime();
        while (accumulator >= kTimestep) {
            accumulator -= kTimestep;
            GameUpdate();
        }

        BeginDrawing();

        ClearBackground(BLACK);
        static const char* displaymessage = "RESPECT MY AUTHORITAH";
        int xmeasurement = MeasureText(displaymessage, 40);
        DrawText(displaymessage,960/2.f - (xmeasurement/2.f),540/2.f,40,WHITE);
            // Don't really need to render anything since we are the server
            // after all
            /*
            const struct MapInstance* map = GetMapInstance();
            for (int ndex = 0; ndex < map->collisioncount; ++ndex) {
                DrawRectangleLinesEx(map->collisions[ndex], 1.f, GREEN);
            }

            for (int n = 0; n < kMaxNumberOfPlayers; ++n) {
                DrawRectangle(g_servergamestate.players[n].player.position.x,
                              g_servergamestate.players[n].player.position.y,
            16, 16, RED);
            }
            DrawFPS(100, 100);
            */

            EndDrawing();

        RunLuvLoop();
    }
    CloseWindow();
    
    return 0;
}
