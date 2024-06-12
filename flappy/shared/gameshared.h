#ifndef _gameshared_h
#define _gameshared_h
#include <stdint.h>
#define kTimestep (1 / 60.0)
#define kMaxNumberOfPlayers (2)

struct PlayerClient {
    float x;
    float y;
    uint64_t id;
};
struct PlayerServerside {
    struct PlayerClient flapper;
    NetInterfaceAddr netid;
    uint8_t isflapping;
};

;
struct GamestateClient {
    struct PlayerClient players[kMaxNumberOfPlayers];
};

struct GamestateServerside {
    struct PlayerServerside players[kMaxNumberOfPlayers];
};

enum NetInterfacePacketId {
    kNetPacketIdNone = 0,
    kNetPacketIdInput,
    kNetPacketIdGamestate,
    kNetPacketIdPlayerId,
    kNetPacketIdMex,
};

#endif