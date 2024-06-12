#include <stdio.h>

#include "gameshared.h"
#include "levelmap.h"
#include "netinterface.h"
#include <raymath.h>

struct GamestateServerside gstate;

void InsertPlayerIntoGamestate(NetInterfaceAddr* addr) {
    if (gstate.playercount < kMaxNumberOfPlayers) {
        gstate.players[gstate.playercount].netid.m_niRemoteAddr =
            addr->m_niRemoteAddr;
        gstate.players[gstate.playercount].netid.m_niUSizeOfAddr =
            addr->m_niUSizeOfAddr;
        gstate.players[gstate.playercount].player.id = ++gstate.playercount;
    }
}
void Callback(const NetInterface* ni, NetInterfaceAddr* niAddr,
              const char* buffer, NITransferSize niTransferSize) {
    if (niTransferSize == sizeof(uint16_t)) {
        uint16_t packet;
        memcpy(&packet, buffer, sizeof(uint16_t));
        if (packet == 0xCAFE) {
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

void GameUpdate(NetInterface* ni) {
    static const float kGravity = 9.8f;
    static const float kXVelocity = 1.f;
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

        player->player.position = Vector2Add(player->player.position,player->velocity);
        //player->player.x+=player->velocity.x;
        //player->player.y+=player->velocity.y;
        


        //gstateclient.players[n].id = player->player.id;
        gstateclient.players[n].id = player->player.id;
        gstateclient.players[n].position = player->player.position;
    }
    gstateclient.tick = gstate.tick;


    for(uint32_t n = 0; n < kMaxNumberOfPlayers; ++n){
        SendPacket(ni, &gstate.players[n].netid, (char*)&gstateclient,sizeof(struct GamestateClient));
    }
}
int main() {
    memset(&gstate, 0, sizeof(struct GamestateServerside));
    NetInterface* ni = MakeNetInterface();
    if (MakeSocketBind(ni)) {
        SetTargetFPS(240);
        InitWindow(960, 540, "Flappy");
        double accumulator = 0.0;

        while (!WindowShouldClose()) {
            accumulator += GetFrameTime();
            while (accumulator >= kTimestep) {
                accumulator -= kTimestep;
                GameUpdate(ni);
            }
            Poll(ni, Callback);

            BeginDrawing();

            ClearBackground(BLACK);

            EndDrawing();
        }
        DestroySocket(ni);
    }
    DestroyNetInterface(ni);
    return 0;
}
