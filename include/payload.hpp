#ifndef PAYLOAD_HPP
#define PAYLOAD_HPP

#include <cstdint>
#include <vector>

struct ChunkHeader{
  uint32_t id;
  char type[8];

  uint32_t index;
  uint32_t length;
  uint32_t total;
};

struct Chunk{ // 529
  ChunkHeader header; // 17 (136 bits)
  std::vector<uint8_t> payload; // 512
};

struct Payload{
  std::vector<Chunk> chunks;
  bool send();
};

Chunk chunkify(uint8_t *buffer, char type[8], 
    uint32_t index, uint32_t size, uint32_t total);
void linux_payload(uint8_t *buffer, Payload &payload);
uint32_t get_id();

#endif
