#ifndef _gameshared_h
#define _gameshared_h

#include <stdint.h>

#define kTimestep (1 / 60.0)
#define kMaxNumberOfPlayers (1)
#define kRenderDelayTicks (6)
#define kMaxNumberOfStates (10)

#define kGetPtr(x) (&x)

typedef uint64_t FlappyPacket;
enum EFlappyPacketId {
    kEFlappyPacketIdNone = 0,
    kEFlappyPacketIdInput,
    kEFlappyPacketIdPlayerState,
    kEFlappyPacketIdPlayerId,
    kEFlappyPacketInsertPlayer,
    kEFlappyPacketWinner,
    kEFlappyPacketIdMex,
};
#define kPackInsertPlayer(x) (x = (kEFlappyPacketInsertPlayer))

#define kPackPlayerId(x, y, z)                      \
    (x = (((uint64_t)y << 48) | (uint64_t)z << 16 | \
          (uint16_t)kEFlappyPacketIdPlayerId))

#define kPackPlayerInput(x, y, z) \
    (x = (((uint64_t)y << 48) | (uint64_t)z << 16 |  (uint16_t)kEFlappyPacketIdInput))

#define kPackPlayerPosition(x, y) \
    (memcpy(kGetPtr(x), kGetPtr(y), sizeof(uint64_t)))

#define kPackPlayerWinner(x,y)\
(x = ( (uint64_t)y << 48) | (uint16_t)kEFlappyPacketWinner )

#define kPackPlayerPredictonTick(x,y)\
(x = ( (uint64_t)y << 32 ))

#define kGetPlayerId(x) ( (x >> 48) & 0xFFFF )
#define kGetInput(x) ( (x >> 48) & 0xFFFF )
#define kGetTick(x) ( (x >> 16) & 0xFFFFFFFF )
#define kGetPacketId(x) ( x & 0xFFFF ) 
#define kGetPredictionTick(x) (x >> 32 &0xFFFFFFFF)
struct PlayerState {
    uint64_t x;
    uint64_t y;
    uint64_t z;
};
struct GamestateClient {
    struct PlayerState players[kMaxNumberOfPlayers];
};


#define kFlappyCollisionSize 16
#endif