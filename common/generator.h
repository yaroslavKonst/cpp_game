#ifndef GENERATOR_H
#define GENERATOR_H

#include "map.h"
#include <random>
#include <vector>

Chunk* generator (uint64_t seed, int32_t x, int32_t y) {
    uint64_t ChunkSeed = seed;
    ChunkSeed = ChunkSeed ^ (int64_t(x) << 32);
    ChunkSeed = ChunkSeed ^ (int64_t(y) & 0xFFFFFFFF);
    std::srand(ChunkSeed);
    
    std::vector<int> Pregen(CHUNKSIZE * CHUNKSIZE);
    for (int i = 0; i < CHUNKSIZE * CHUNKSIZE; i++) {
        Pregen[i] = std::rand() % 3;
    }

    std::vector<int> Postgen(CHUNKSIZE * CHUNKSIZE);
    Layer L;
    for (int i = 0; i < CHUNKSIZE * CHUNKSIZE; i++) {
        int lu = i - CHUNKSIZE - 1, mu = i - CHUNKSIZE, ru = i - CHUNKSIZE + 1, 
                rm = i + 1, rd = i + CHUNKSIZE + 1, md = i + CHUNKSIZE, 
                ld = i + CHUNKSIZE - 1, lm = i - 1;
        if (i % CHUNKSIZE == 0) {
            lu += CHUNKSIZE;
            lm += CHUNKSIZE;
            ld += CHUNKSIZE;
        } else if (i % CHUNKSIZE == CHUNKSIZE - 1) {
            ru -= CHUNKSIZE;
            rm -= CHUNKSIZE;
            rd -= CHUNKSIZE;
        }
        if (i / CHUNKSIZE == 0) {
            lu += CHUNKSIZE * CHUNKSIZE;
            mu += CHUNKSIZE * CHUNKSIZE;
            ru += CHUNKSIZE * CHUNKSIZE;
        } else if (i / CHUNKSIZE == CHUNKSIZE - 1) {
            ld -= CHUNKSIZE * CHUNKSIZE;
            md -= CHUNKSIZE * CHUNKSIZE;
            rd -= CHUNKSIZE * CHUNKSIZE;
        }
        Postgen[i] = Pregen[lu] + Pregen[mu] + Pregen[ru] + Pregen[rm] + 
                Pregen[rd] + Pregen[md] + Pregen[ld] + Pregen[lm];
        if (Postgen[i] > 3 && Postgen[i] < 13) {
            L.SetTile(i % CHUNKSIZE, i / CHUNKSIZE, Tile(Tile::Grass));
        } else {
            L.SetTile(i % CHUNKSIZE, i / CHUNKSIZE, Tile(Tile::Stone));
        }
    }
    Chunk* C = new Chunk;
    C->AddLayer(&L);
    return C;
}




#endif
