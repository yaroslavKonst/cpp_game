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
    
    std::vector<int> Pregen(CHUNKSIZE * CUNKSIZE);
    for (int i = 0; i < CHUNKSIZE * CUNKSIZE; i++) {
        Pregen[i] = std:rand() % 3;
    }

    std::vector<int> Postgen(CHUNKSIZE * CUNKSIZE);
    Layer L;
    for (int i = 0; i < CHUNKSIZE * CUNKSIZE; i++) {
        int lu = i - CHUNKSIZE - 1, mu = i - CHUNKSIZE, ru = i - CHUNKSIZE + 1, 
                rm = i + 1, rd = i + CHUNKSIZE + 1, md = i + CHUNKSIZE, 
                ld = i + CHUNKSIZE - 1, lm = i - 1;
        if (i % CHUNKSIZE == 0) {
            lu += CHUNKSISE;
            lm += CHUNKSISE;
            ld += CHUNKSISE;
        } else if (i % CHUNKSIZE == CHUNKSIZE - 1) {
            ru -= CHUNKSISE;
            rm -= CHUNKSISE;
            rd -= CHUNKSISE;
        }
        if (i / CHUNKSIZE == 0) {
            lu += CHUNKSISE * CHUNKSIZE;
            mu += CHUNKSISE * CHUNKSIZE;
            ru += CHUNKSISE * CHUNKSIZE;
        } else if (i / CHUNKSIZE == CHUNKSIZE - 1) {
            ld -= CHUNKSISE * CHUNKSIZE;
            md -= CHUNKSISE * CHUNKSIZE;
            rd -= CHUNKSISE * CHUNKSIZE;
        }
        Postgen[i] = Predgen[lu] + Predgen[mu] + Predgen[ru] + Predgen[rm] + 
                Predgen[rd] + Predgen[md] + Predgen[ld] + Predgen[lm];
        if (Postgen[i] > 3 && Postgen[i] < 13) {
            L.SetTile(i % CHUNKSIZE, i / CHUNKSIZE, Tile::Grass);
        } else {
            L.SetTile(i % CHUNKSIZE, i / CHUNKSIZE, Tile::Stone);
        }
    }
    Chunk C;
    C.AddLayer(&L);
    return &C;
}




#endif
