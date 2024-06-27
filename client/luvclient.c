#include <assert.h>
#include <cvector.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "gameshared.h"
static uv_udp_t client;
static uv_loop_t* loop = NULL;
struct sockaddr host;

extern uint32_t g_myid;
extern uint32_t g_tick;
extern uint32_t g_serverbasetick;
extern uint32_t g_starttick;
extern uint32_t winner;
extern uint32_t winnerid;
extern cvector_vector_type(struct GamestateClient) gamestates;
extern int GetNextFramendex();

static void on_send(uv_udp_send_t* req, int status) {
    assert(req != NULL);
    assert(req->data != NULL);
    free(req->data);
    free(req);
    if (status) {

    }
}
static uv_udp_send_t* AllocateBuffer(uv_buf_t* uvbuf, void* data, uint32_t size) {
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
            uint64_t packet = 0;
            memcpy(&packet, rcvbuf->base, sizeof(packet));
            if (kGetPacketId(packet) == kEFlappyPacketIdPlayerId) {
                g_myid = kGetPlayerId(packet);

            } else if (kGetPacketId(packet) == kEFlappyPacketWinner) {
                winner = 1;
                winnerid = kGetPlayerId(packet);
            }

        } else if (nread == sizeof(struct GamestateClient)) {
            struct GamestateClient gstate;
            memcpy(&gstate, rcvbuf->base, sizeof(struct GamestateClient));
            cvector_push_back(gamestates, gstate);
            uint32_t t = kGetTick(gstate.players[0].x);

            if (g_starttick == 0 && g_serverbasetick == 0) {
                g_starttick = g_tick;
                g_serverbasetick = t;
            }
            for (int n = 0; n < kMaxNumberOfPlayers; ++n) {
                uint32_t id = kGetPlayerId(gstate.players[n].x);

                // this is each client's predicted or reconciliation tick sent
                // to the server during update this tick will be used to check
                // the map or container of the [tick][position] If the player's
                // predicted position with their tick matches the server
                // position with tick then no reconciliation needs to be done

                uint32_t predictedtick =
                    kGetPredictionTick(gstate.players[n].z);
                // printf("%u\n",predictedtick);
            }
            // printf("Got state\n");
            // printf("%u\t%u\n",
            // kGetPlayerId(gstate.players[0].x),kGetPlayerId(gstate.players[1].x));
            int ndex = GetNextFramendex();

            if (ndex > -1) {
                for (int n = 0; n < ndex; ++n) {
                    uint32_t t = kGetTick(gamestates[n].players[0].x);
                    cvector_erase(gamestates, n);
                }
            }
        }
    }

    free(rcvbuf->base);
}

void UpdateFlappy( uint8_t isflapping) {
    uv_buf_t uvbuf;
    uint64_t packet = 0;
    kPackPlayerInput(packet, isflapping, g_tick);

    uv_udp_send_t* req = AllocateBuffer(&uvbuf, &packet, sizeof(packet));
    uv_udp_send(req, &client, &uvbuf, 1, &host, on_send);
}
void RunLuvLoop() { uv_run(loop, UV_RUN_NOWAIT); }
void InitLuv() {
    loop = uv_default_loop();
    assert(uv_udp_init(loop, &client) == 0);
    assert(uv_ip4_addr("0.0.0.0", 23456, (struct sockaddr_in*)&host) == 0);
    assert(uv_udp_recv_start(&client, on_alloc, on_recv) == 0);

    uint64_t packet = 0;
    kPackInsertPlayer(packet);
    uv_buf_t uvb;
    uv_udp_send_t* send_req = AllocateBuffer(&uvb, &packet, sizeof(packet));
    uv_udp_send(send_req, &client, &uvb, 1, &host, NULL);
}
