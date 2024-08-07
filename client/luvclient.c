#include "luvclient.h"

#include <assert.h>
#include <cvector.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "gameshared.h"

static uv_udp_t client;
static uv_loop_t* loop = NULL;
static struct sockaddr host;
struct Vector2 {
    float x;
    float y;
};
struct PlayerState {
    uint16_t packetid;        // 2
    uint16_t playerid;        // 4
    uint32_t servertick;      // 8
    uint32_t playertick;      // 12
    uint32_t allignemnt;      // 16 //extra for padding
    struct Vector2 position;  // 24
    struct Vector2 velocity;  // 32
};  // pad the structure to allignment for easy memory access and copy
struct GamestateClient {
    struct PlayerState players[kMaxNumberOfPlayers];
};
struct MapInstance {
    struct Rectangle* collisions;
    uint32_t collisioncount;
};
struct ClientSidePrediction {
    struct Vector2 position;
    struct Vector2 velocity;
    uint8_t isflapping;
    uint32_t tick;
};
struct Rectangle {
    float x;
    float y;
    float width;
    float height;
};
struct ClientSidePrediction csp;
extern uint16_t g_myid;
extern uint32_t g_tick;
extern uint32_t g_serverbasetick;
extern uint32_t g_starttick;
extern uint32_t winner;
extern uint32_t winnerid;

static uint32_t simulating = 0;

extern cvector_vector_type(struct GamestateClient) gamestates;
cvector_vector_type(struct ClientSidePrediction) predictions = NULL;

extern int GetNextFramendex();

cvector_vector_type(struct GamestateClient) gamestates = NULL;
extern bool CheckCollisionRecs(struct Rectangle rec1, struct Rectangle rec2);
extern struct Rectangle GetCollisionRec(struct Rectangle rec1,
                                        struct Rectangle rec2);
extern struct Vector2 Vector2Add(struct Vector2 a, struct Vector2 b);
extern const struct MapInstance* GetMapInstance();
int GetServerTick() {
    return g_serverbasetick + (g_tick - g_starttick) - kRenderDelayTicks;
}

int GetNextFramendex() {
    int delay = GetServerTick();
    uint32_t i = 0;
    for (int i = cvector_size(gamestates) - 1; i >= 0; --i) {
        uint32_t quetick = gamestates[i].players[0].servertick;
        if (quetick <= delay) {
            return i;
        } else {
        }
    }
    return -1;
}

void GetGamestateToRender(struct PlayerRenderData* result) {
    int nextframe = GetNextFramendex();
    if (nextframe < 0) {
        // game hasn't started yet
        memset(result, 0,
               sizeof(struct PlayerRenderData) * kMaxNumberOfPlayers);
    } else if (nextframe == cvector_size(gamestates) - 1) {
        // next packet hasn't arrived
        // server is late so just return the latest packet we have
        struct GamestateClient dst = gamestates[cvector_size(gamestates) - 1];

        for (int ndex = 0; ndex < kMaxNumberOfPlayers; ++ndex) {
            result[ndex].x = dst.players[ndex].position.x;
            result[ndex].y = dst.players[ndex].position.y;
            result[ndex].id = dst.players[ndex].playerid;
        }

    } else {
        // we have a current packet and a next packet to interpolate t
        uint32_t servertick = GetServerTick();
        struct GamestateClient next = gamestates[nextframe + 1];
        struct GamestateClient render = gamestates[nextframe];

        uint32_t tick = render.players[0].servertick;
        uint32_t nexttick = next.players[0].servertick;

        float alpha = (((float)servertick * 1.0) - ((float)tick * 1.0)) /
                      (((float)nexttick * 1.0) - ((float)tick * 1.0));

        for (int ndex = 0; ndex < kMaxNumberOfPlayers; ++ndex) {
            struct Vector2 start = {0};
            struct Vector2 end = {0};
            uint32_t startid = render.players[ndex].playerid;
            uint32_t endid = next.players[ndex].playerid;
            assert(startid == endid);

            start = render.players[ndex].position;
            end = next.players[ndex].position;

            struct Vector2 interpolated;
            interpolated.x = start.x + (end.x - start.x) * alpha;
            interpolated.y = start.y + (end.y - start.y) * alpha;

            result[ndex].id = startid;
            result[ndex].x = interpolated.x;
            result[ndex].y = interpolated.y;
        }

        // return render;
    }
}
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
static void on_alloc(uv_handle_t* client, size_t suggested_size,
                     uv_buf_t* buf) {
    buf->base = malloc(suggested_size);
    buf->len = suggested_size;
}
static void on_recv(uv_udp_t* handle, ssize_t nread, const uv_buf_t* rcvbuf,
                    const struct sockaddr* addr, unsigned flags) {
    if (nread > 0) {
        if (nread == sizeof(uint64_t)) {
            uint64_t packet = *(uint64_t*)rcvbuf->base;
            if (kGetPacketId(packet) == kEFlappyPacketIdPlayerId) {
                struct PlayerId pid;
                memcpy(&pid, rcvbuf->base, kSizeOfPlayerIdPacket);
                g_myid = pid.playerid;

            } else if (kGetPacketId(packet) == kEFlappyPacketWinner) {
                winner = 1;
                struct PlayerWinner win;
                memcpy(&win, rcvbuf->base, kSizeOfPlayerWinnerPacket);
                winnerid = win.playerid;
                simulating = 0;
            }

        } else if (nread == kSizeOfPlayerStatePacket) {
            struct GamestateClient gstate;
            memcpy(&gstate, rcvbuf->base, sizeof(struct GamestateClient));
            cvector_push_back(gamestates, gstate);
            uint32_t t = gstate.players[0].servertick;

            if (g_starttick == 0 && g_serverbasetick == 0) {
                g_starttick = g_tick;
                g_serverbasetick = t;
                simulating = 1;
                csp.position.y = 540 / 2.f;
            }

            for (int n = 0; n < kMaxNumberOfPlayers; ++n) {
                uint32_t id = gstate.players[n].playerid;

                // this is each client's predicted or reconciliation
                // tick sent to the server during update this tick will
                // be used to check the map or container of the
                // [tick][position] If the player's predicted position
                // with their tick matches the server position with tick
                // then no reconciliation needs to be done

                uint32_t predictedtick = gstate.players[n].playertick;

                if (id == g_myid && predictedtick != 0) {
                    struct Vector2 position = gstate.players[n].position;

                    uint8_t found = 0;
                    int predictindex = 0;
                    for (; predictindex < cvector_size(predictions);
                         ++predictindex) {
                        struct ClientSidePrediction* predict =
                            cvector_at(predictions, predictindex);
                        if (predict->tick == predictedtick) {
                            found = 1;
                            break;
                        }
                    }
                    if (found) {
                        struct ClientSidePrediction predict;
                        memcpy(&predict, cvector_at(predictions, predictindex),
                               sizeof(struct ClientSidePrediction));
                        if (predict.position.x == position.x) {
                        } else {
                            // reset position,velocity, tick
                            struct Vector2 velocity =
                                gstate.players[n].velocity;
                            csp.position = position;
                            csp.velocity = velocity;
                            g_tick = predictedtick;
                            cvector_clear(predictions);
                        }
                        for (int n = 0; n < predictindex; ++n) {
                            cvector_erase(predictions, n);
                        }
                    }
                }

                // printf("%u\n",predictedtick);
            }
            // printf("Got state\n");
            // printf("%u\t%u\n",
            // kGetPlayerId(gstate.players[0].x),kGetPlayerId(gstate.players[1].x));
            int ndex = GetNextFramendex();

            if (ndex > -1) {
                for (int n = 0; n < ndex; ++n) {
                    cvector_erase(gamestates, n);
                }
            }
        }
    }

    free(rcvbuf->base);
}
#define kPlayerCollisionSize kFlappyCollisionSize
void UpdateFlappy(uint8_t isflapping) {
    uv_buf_t uvbuf;
    uint64_t packet = 0;
    struct PlayerInput input;
    input.packetid = kEFlappyPacketIdInput;
    input.isflapping = isflapping;
    input.playertick = g_tick;

    uv_udp_send_t* req =
        AllocateBuffer(&uvbuf, &input, kSizeOfPlayerInputPacket);
    uv_udp_send(req, &client, &uvbuf, 1, &host, on_send);
    if (!simulating) {
        return;
    }
    static const float kGravity = 0.1f;
    static const float kXVelocity = 1.5f;
    csp.tick = g_tick;
    if(isflapping != csp.isflapping){
        csp.isflapping = isflapping;
        csp.velocity.y = 0;
    }
    if (csp.isflapping) {
        csp.velocity.y -= kGravity;

    } else {
        csp.velocity.y += kGravity;
    }

    csp.velocity.x = kXVelocity;

    // create player collision rectangle
    struct Rectangle playerrec = (struct Rectangle){
        csp.position.x + csp.velocity.x, csp.position.y + csp.velocity.y,
        kPlayerCollisionSize, kPlayerCollisionSize};

    uint8_t collisionoccurred = 0;
    struct Rectangle collider;
    const struct MapInstance* map = GetMapInstance();
    for (int ndex = 0; ndex < map->collisioncount; ++ndex) {
        if (CheckCollisionRecs(map->collisions[ndex], playerrec)) {
            collisionoccurred = 1;
            collider = GetCollisionRec(map->collisions[ndex], playerrec);
            break;
        }
    }

    if (collisionoccurred) {
        // stop x movement whether x or y collision occurrs
        csp.velocity.x = 0;

        // check y collision so we don't fall through colliders
        uint8_t ycollision = 1;
        if (playerrec.x >
            collider.x + kPlayerCollisionSize)  // our left is past wall right
            ycollision = 0;
        else if (playerrec.x + collider.width <
                 collider.x)  // our right is past wall left
            ycollision = 0;

        if (ycollision) {
            // y collision occured, stop y movement
            csp.velocity.y = 0;
        }
    }

    csp.position = Vector2Add(csp.position, csp.velocity);

    cvector_push_back(predictions, csp);
    assert(cvector_size(predictions) <= 50);
}
void RunLuvLoop() { uv_run(loop, UV_RUN_NOWAIT); }
void InitLuv() {
    loop = uv_default_loop();

    assert(uv_udp_init(loop, &client) == 0);
    assert(uv_ip4_addr("0.0.0.0", 23456, (struct sockaddr_in*)&host) == 0);
    assert(uv_udp_recv_start(&client, on_alloc, on_recv) == 0);
    struct PlayerInsert insert;
    insert.packetid = kEFlappyPacketInsertPlayer;

    uv_buf_t uvb;
    uv_udp_send_t* send_req =
        AllocateBuffer(&uvb, &insert, kSizeOfPlayerInsertPacket);
    uv_udp_send(send_req, &client, &uvb, 1, &host, NULL);
    memset(&csp, 0, sizeof(struct ClientSidePrediction));
}
