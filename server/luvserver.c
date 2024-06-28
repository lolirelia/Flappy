#include <assert.h>
#include <cvector.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "gameshared.h"

static uv_loop_t* loop = NULL;
static uv_udp_t server;

#define kPlayerCollisionSize kFlappyCollisionSize
struct Vector2 {
    float x;
    float y;
};
struct Rectangle {
    float x;
    float y;
    float width;
    float height;
};
extern bool CheckCollisionRecs(struct Rectangle rec1, struct Rectangle rec2);
extern struct Rectangle GetCollisionRec(struct Rectangle rec1,
                                        struct Rectangle rec2);
extern struct Vector2 Vector2Add(struct Vector2 a, struct Vector2 b);

struct PlayerClient {
    struct Vector2 position;
    uint16_t id;
};
struct PlayerServerside {
    struct PlayerClient player;
    struct Vector2 velocity;
    uint8_t isflapping;
    uint8_t wasflapping;
    uint32_t ipv4;
    uint32_t playertick;
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

struct GamestateServerside g_servergamestate;

#define kGetAddr(x) ((struct sockaddr_in*)kGetPtr(x))
#define kGetAddrPtr(x) ((struct sockaddr_in*)x)

struct MapInstance {
    struct Rectangle* collisions;
    uint32_t collisioncount;
};
extern const struct MapInstance* GetMapInstance();
extern void LoadLevel();

static void on_send(uv_udp_send_t* req, int status) {
    assert(req != NULL);
    assert(req->data != NULL);
    free(req->data);
    free(req);
    if (status) {
    }
}
static uv_udp_send_t* AllocateBuffer(uv_buf_t* uvbuf, void* data,
                                     uint32_t size) {
    uv_udp_send_t* req = malloc(sizeof(uv_udp_send_t));

    req->data = malloc(size);
    memcpy(req->data, data, size);
    *uvbuf = uv_buf_init(req->data, size);
    return req;
}

static struct PlayerServerside* InsertPlayerIntoGamestate(uint32_t ipv4,
                                                          uint16_t port) {
    struct PlayerServerside* player = NULL;
    ++g_servergamestate.playerid;
    if (g_servergamestate.playercount < kMaxNumberOfPlayers) {
        player = &g_servergamestate.players[g_servergamestate.playercount];

        // for some reason I need to reconstruct the players sockaddr
        // because copying it from recv callback just doesn't work
        player->ipv4 = ipv4;
        player->port = port;
        char ipbuffer[32] = {0};

        sprintf(ipbuffer, "%d.%d.%d.%d", (ipv4 >> 0) & 0xFF, (ipv4 >> 8) & 0xFF,
                (ipv4 >> 16) & 0xFF, (ipv4 >> 24) & 0xFF);
        // printf("Insertng : %s %d\n", ipbuffer, port);
        uv_ip4_addr(ipbuffer, htons(port), &player->addrin);

        player->player.id = g_servergamestate.playerid;
        player->player.position.x = 0;
        player->player.position.y = 540 / 2.f;
        ++g_servergamestate.playercount;
    }
    if (g_servergamestate.playercount == kMaxNumberOfPlayers) {
        g_servergamestate.gamestarted = 1;
    }

    return player;
}
static void on_alloc(uv_handle_t* client, size_t suggested_size,
                     uv_buf_t* buf) {
    buf->base = malloc(suggested_size);
    buf->len = suggested_size;
}
static void on_recv(uv_udp_t* handle, ssize_t nread, const uv_buf_t* rcvbuf,
                    const struct sockaddr* addr, unsigned flags) {
    if (nread == sizeof(uint64_t)) {
        uint64_t packet = 0;
        memcpy(&packet, rcvbuf->base, sizeof(packet));
        uint16_t packetid = kGetPacketId(packet);
        if (packetid == kEFlappyPacketInsertPlayer) {
            uint32_t ipv4 = kGetAddrPtr(addr)->sin_addr.s_addr;
            uint16_t port = kGetAddrPtr(addr)->sin_port;
            struct sockaddr local;
            struct PlayerServerside* inserted =
                InsertPlayerIntoGamestate(ipv4, port);
            assert(inserted != NULL);
            packet = 0;
            kPackPlayerId(packet, inserted->player.id, g_servergamestate.tick);
            uv_buf_t uvbuf;
            uv_udp_send_t* send_req =
                AllocateBuffer(&uvbuf, &packet, sizeof(packet));

            uv_udp_send(send_req, handle, &uvbuf, 1, addr, on_send);

        } else if (packetid == kEFlappyPacketIdInput) {
            if (g_servergamestate.gamestarted) {
                struct PlayerServerside* player = NULL;
                uint32_t ipv4 = kGetAddrPtr(addr)->sin_addr.s_addr;
                uint16_t port = kGetAddrPtr(addr)->sin_port;
                for (uint32_t n = 0; n < kMaxNumberOfPlayers; ++n) {
                    if (g_servergamestate.players[n].ipv4 == ipv4 &&
                        g_servergamestate.players[n].port == port) {
                        player = &g_servergamestate.players[n];
                        break;
                    }
                }
                assert(player != NULL);
                uint32_t playertick = kGetTick(packet);
                player->playertick = playertick;

                player->isflapping = kGetInput(packet);
                if (player->isflapping != player->wasflapping) {
                    player->wasflapping =
                        player->isflapping;  // we need this variable to reset
                                             // velocity accordingly
                    player->velocity.y = 0;
                }
            }
        }
    }

    free(rcvbuf->base);
}
void GameUpdate() {
    static const float kGravity = 0.1f;
    static const float kXVelocity = 1.5f;
    ++g_servergamestate.tick;
    if (g_servergamestate.gamestarted) {
        struct GamestateClient gstateclient;

        // gstateclient.tick = g_servergamestate.tick;
        const struct MapInstance* map = GetMapInstance();
        for (uint32_t n = 0; n < kMaxNumberOfPlayers; ++n) {
            struct PlayerServerside* player = &g_servergamestate.players[n];
            if (player->isflapping) {
                // printf("flapping\n");
            }
            if (player->isflapping) {
                player->velocity.y -= kGravity;
            } else {
                player->velocity.y += kGravity;
            }

            player->velocity.x = kXVelocity;

            // create player collision rectangle
            struct Rectangle playerrec = (struct Rectangle){
                player->player.position.x + player->velocity.x,
                player->player.position.y + player->velocity.y,
                kPlayerCollisionSize, kPlayerCollisionSize};

            uint8_t collisionoccurred = 0;
            struct Rectangle collider;

            for (int ndex = 0; ndex < map->collisioncount; ++ndex) {
                if (CheckCollisionRecs(map->collisions[ndex], playerrec)) {
                    collisionoccurred = true;
                    collider =
                        GetCollisionRec(map->collisions[ndex], playerrec);
                    break;
                }
            }

            if (collisionoccurred) {
                // stop x movement whether x or y collision occurrs
                player->velocity.x = 0;

                // check y collision so we don't fall through colliders
                uint8_t ycollision = 1;
                if (playerrec.x >
                    collider.x +
                        kPlayerCollisionSize)  // our left is past wall right
                    ycollision = 0;
                else if (playerrec.x + collider.width <
                         collider.x)  // our right is past wall left
                    ycollision = 0;

                if (ycollision) {
                    // y collision occured, stop y movement
                    player->velocity.y = 0;
                }
            }

            player->player.position =
                Vector2Add(player->player.position, player->velocity);

            // player->player.x+=player->velocity.x;
            // player->player.y+=player->velocity.y;
            assert(player->player.id != 0);
            kPackPlayerId(gstateclient.players[n].x, player->player.id,
                          g_servergamestate.tick);
            kPackPlayerPosition(gstateclient.players[n].y,
                                player->player.position);
            kPackPlayerPredictonTick(gstateclient.players[n].z,
                                     player->playertick);
            if (player->player.position.x >= 2832.f) {
                g_servergamestate.gamestarted = 0;
                for (uint32_t n = 0; n < kMaxNumberOfPlayers; ++n) {
                    uint64_t packet = 0;
                    kPackPlayerWinner(packet, player->player.id);

                    uv_buf_t uvbuf;
                    uv_udp_send_t* send_req =
                        AllocateBuffer(&uvbuf, &packet, sizeof(packet));
                    uv_udp_send(send_req, &server, &uvbuf, 1,
                                &g_servergamestate.players[n].addrin, on_send);
                }
            }
        }

        g_servergamestate.send = !g_servergamestate.send;
        if (g_servergamestate.send) {
            for (uint32_t n = 0; n < kMaxNumberOfPlayers; ++n) {
                uv_buf_t uvbuf;
                uv_udp_send_t* send_req = AllocateBuffer(
                    &uvbuf, &gstateclient, sizeof(struct GamestateClient));
                uv_udp_send(send_req, &server, &uvbuf, 1,
                            &g_servergamestate.players[n].addrin, on_send);
            }
        }
    }
}
void InitLuv() {
    struct sockaddr_in recv_addr;
    uv_ip4_addr("0.0.0.0", 23456, &recv_addr);
    assert(uv_udp_init(loop, &server) == 0);
    assert(uv_udp_bind(&server, (struct sockaddr*)&recv_addr, 0) == 0);
    assert(uv_udp_recv_start(&server, on_alloc, on_recv) == 0);
    memset(&g_servergamestate, 0, sizeof(struct GamestateServerside));
}

void RunLuvLoop() { uv_run(loop, UV_RUN_NOWAIT); }