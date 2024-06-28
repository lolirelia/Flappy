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

#define kGetPacketId(x) ( (uint16_t)((uint64_t)x& 0xFFFF) )

// allign all structures to 8 bytes
#define kSizeOfPlayerInsertPacket (sizeof(uint64_t) * 1)
struct PlayerInsert{
    uint16_t packetid;//2
    uint16_t padding;//4
    uint32_t alligntment;//8
};

#define kSizeOfPlayerInputPacket (sizeof(uint64_t) * 1)
struct PlayerInput{
    uint16_t packetid;//2
    uint16_t isflapping;//4
    uint32_t playertick;//8
};

#define kSizeOfPlayerIdPacket (sizeof(uint64_t) * 1)
struct PlayerId{
    uint16_t packetid;//2
    uint16_t playerid;//4
    uint32_t servertick;//8
};

#define kSizeOfPlayerWinnerPacket (sizeof(uint64_t) * 1)
struct PlayerWinner{
    uint16_t packetid;//2
    uint16_t playerid;//4
    uint32_t allignment;//8
};

#define kSizeOfPlayerStatePacket (sizeof(uint64_t) * 3)

#define kFlappyCollisionSize 16
#endif