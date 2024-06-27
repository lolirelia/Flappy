#ifndef _gameshared_h
#define _gameshared_h

#include <raylib.h>
#include <raymath.h>
#include <stdint.h>

#define kTimestep (1 / 60.0)
#define kMaxNumberOfPlayers (1)
#define kRenderDelayTicks (6)
#define kMaxNumberOfStates (10)

#define kGetPtr(x) (&x)
#define kGetAddr(x) ((struct sockaddr_in*)kGetPtr(x))
#define kGetAddrPtr(x) ((struct sockaddr_in*)x)
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

#define kPackPlayerInput(x, y) \
    (x = (((uint64_t)y << 48) | (uint16_t)kEFlappyPacketIdInput))

#define kPackPlayerPosition(x, y) \
    (memcpy(kGetPtr(x), kGetPtr(y), sizeof(uint64_t)))

#define kPackPlayerWinner(x,y)\
(x = ( (uint64_t)y << 48) | (uint16_t)kEFlappyPacketWinner )

#define kGetPlayerId(x) ( (x >> 48) & 0xFFFF )
#define kGetInput(x) ( (x >> 48) & 0xFFFF )
#define kGetTick(x) ( (x >> 16) & 0xFFFFFFFF )
#define kGetPacketId(x) ( x & 0xFFFF ) 
struct PlayerState {
    uint64_t x;
    uint64_t y;
};
struct GamestateClient {
    struct PlayerState players[kMaxNumberOfPlayers];
};
struct PlayerClient {
    struct Vector2 position;
    uint16_t id;
};
struct PlayerServerside {
    struct PlayerClient player;
    Vector2 velocity;
    uint8_t isflapping;
    uint8_t wasflapping;
    uint32_t ipv4;
    uint16_t port;
    struct sockaddr_in addrin;
    
};

struct GamestateServerside {
    struct PlayerServerside players[kMaxNumberOfPlayers];
    uint32_t playercount;
    uint32_t tick;
    uint8_t gamestarted;
    uint8_t send;
    uint32_t playerid;
};

struct PlayerRenderData {
    uint32_t id;
    Vector2 position;
};
#define kFlappyCollisionSize 16
#endif