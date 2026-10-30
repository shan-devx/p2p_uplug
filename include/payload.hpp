#ifndef PAYLOAD_HPP
#define PAYLOAD_HPP
/*
#include <cstdint>
#include <vector>
#include <string>

#define MAX_BLUETOOTH_CHUNK_SIZE 512
#define MAX_LAN_CHUNK_SIZE 65536

struct ChunkHeader{
  std::string id;
  std::string type;

  uint32_t index;
  uint32_t length;
  uint32_t total;
};

struct Chunk{
  ChunkHeader header;
  std::vector<uint8_t> payload;

  std::string target_id;
};

struct Out{
  std::string type;
  std::vector<uint8_t> data;
};

class PayloadManager{
  public:
    Chunk chunkify(uint8_t *buffer, char type[8], 
      uint32_t index, uint32_t size, uint32_t total);
};

void linux_payload(uint8_t *buffer, Payload &payload);
uint32_t generare_hash();
*/
#endif
