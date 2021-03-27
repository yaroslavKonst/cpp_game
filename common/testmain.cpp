#include "generator.h"

int main() {
    Chunk* C = generator(1, 0, 0);
    C->PrintChunk();
    return 0;
}
