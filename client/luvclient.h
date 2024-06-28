#ifndef _luvclient_h_
#define _luvclient_h_
#include <stdint.h>
struct PlayerRenderData {
    uint32_t id;
    float x;
    float y;
};
void InitLuv();
void UpdateFlappy( uint8_t isflapping);
void RunLuvLoop();
void GetGamestateToRender(struct PlayerRenderData* result);
#endif