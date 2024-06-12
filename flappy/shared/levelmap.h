#ifndef _levelmap_h
#define _levelmap_h
#include <raylib.h>
#include <stdint.h>

struct MapInstance {
    struct Rectangle* collisions;
    uint32_t collisioncount;
};
void LoadLevel();

#endif