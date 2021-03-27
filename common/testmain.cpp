#include "generator.h"

int main() {
    std::shared_ptr<Chunk> C = generator(1, 0, 0);
    C->PrintChunk();
    return 0;
}
