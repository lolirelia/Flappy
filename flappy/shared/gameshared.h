#ifndef _gameshared_h
#define _gameshared_h

#include <raylib.h>
#include <raymath.h>
#include <stdint.h>
#define kTimestep (1 / 60.0)
#define kMaxNumberOfPlayers (1)
#define kRenderDelayTicks (6)
#define kGetPtr(x) (&x)
#define kGetAddr(x) ((struct sockaddr_in*)kGetPtr(x))
#define kGetAddrPtr(x) ((struct sockaddr_in*)x)
struct PlayerClient {
    struct Vector2 position;
    uint8_t id;
};
struct PlayerServerside {
    struct PlayerClient player;
    Vector2 velocity;
    uint8_t isflapping;
    struct sockaddr addr;
};

;
struct GamestateClient {
    struct PlayerClient players[kMaxNumberOfPlayers];
    uint32_t tick;
};

struct GamestateServerside {
    struct PlayerServerside players[kMaxNumberOfPlayers];
    uint32_t playercount;
    uint32_t tick;
    uint8_t gamestarted;
};

enum NetInterfacePacketId {
    kNetPacketIdNone = 0,
    kNetPacketIdInput,
    kNetPacketIdGamestate,
    kNetPacketIdPlayerId,
    kNetPacketIdMex,
};

#endif