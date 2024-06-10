#include "gameshared.h"
#include "levelmap.h"
void Callback(const NetInterface* ni, NetInterfaceAddr* niAddr,
              const char* buffer, NITransferSize niTransferSize) {}
int main() {
    NetInterface* ni = MakeNetInterface();
    if (NIMakeSocket(ni)) {
        NetInterfaceAddr* host = MakeRemoteAddress("127.0.0.1", 23456);
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
			NIPoll(ni,Callback);
            BeginDrawing();
            ClearBackground(BLACK);
            EndDrawing();
        }
        NIDestroySocket(ni);
    }
    DestroyNetInterface(ni);
    return 0;
}
