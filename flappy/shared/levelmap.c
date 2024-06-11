#include "levelmap.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static struct MapInstance g_levelmap;
void LoadLevel() {
    FILE* fMap = fopen("resources/map.dat", "rb");
    assert(fMap != NULL);
    fseek(fMap, 0, SEEK_END);
    uint32_t collisioncount = ftell(fMap);
    fseek(fMap, 0, SEEK_SET);
    collisioncount /= sizeof(struct Rectangle);
    assert(collisioncount != 0);
    g_levelmap.collisions = calloc(collisioncount, sizeof(struct Rectangle));
    g_levelmap.collisioncount = collisioncount;
    struct Rectangle index;
    uint32_t n = 0;
    while (fread(&index, sizeof(struct Rectangle), 1, fMap) == 1) {
        g_levelmap.collisions[n++] = index;
    }
    assert(n == collisioncount);
    fclose(fMap);
}

void DestroyMap() {
    free(g_levelmap.collisions);
    g_levelmap.collisions = 0;
    g_levelmap.collisioncount = 0;
}

const struct MapInstance* GetMapInstance(){
    return &g_levelmap;
}