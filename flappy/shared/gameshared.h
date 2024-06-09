#ifndef _gameshared_h
#define _gameshared_h
#include <netinterface.h>
#include <raylib.h>
#define kTimestep (1 / 60.0)
#define kMaxNumberOfPlayers (2)

struct PlayerClient {
    Vector2 position;
    uint32_t id;
};
struct PlayerServerside {
    struct PlayerClient player;
    Vector2 velocity;
    NetInterfaceAddr netid;
    uint8_t isflapping;
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
};

enum NetInterfacePacketId {
    kNetPacketIdNone = 0,
    kNetPacketIdInput,
    kNetPacketIdGamestate,
    kNetPacketIdPlayerId,
    kNetPacketIdMex,
};

#endif