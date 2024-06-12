#include "gameshared.h"
#include "levelmap.h"
#include "netinterface.h"


void Callback(NetInterface* ni, NetInterfaceAddr* niAddr,
              char* buffer, NITransferSize niTransferSize) {
    printf("got a message\n");
}
void GameUpdate(struct Gamestate* gstate) {}
int main() {
    NetInterface* ni = MakeNetInterface();
    if (MakeSocketBind(ni)) {
        SetTargetFPS(240);
        InitWindow(960, 540, "Flappy");
        double accumulator = 0.0;
        struct GamestateServerside gstate;
        while (!WindowShouldClose()) {
            accumulator += GetFrameTime();
            while (accumulator >= kTimestep) {
                accumulator -= kTimestep;
                GameUpdate(&gstate);
            }
            Poll(ni, Callback);
        }
    }
    return 0;
}
