#include <cstdint>
#include <cstring>
#include <ios>
#include <iostream>
#include "../include/payload.hpp"

Chunk chunkify(uint8_t *buffer,char type[8], uint32_t id,
    uint32_t index, uint32_t size, uint32_t total)
{
  Chunk c;
  c.payload.assign(buffer, buffer + size);

  ChunkHeader ch;
  ch.id = id;
  memcpy(ch.type, type, 8);
  ch.index = index;
  ch.length = size;
  ch.total = total;

  c.header = ch;

  return c;
}

void linux_payload(Payload &payload){
  uint8_t buffer[512];

  bool first = true;
  uint32_t id = get_id();
  char type[8];
  uint32_t total = 0;
  uint32_t index = 0;
  while(std::cin.read(reinterpret_cast<char*>(buffer), 512) 
      || std::cin.gcount() > 0)
  {
    if(first){
      std::memcpy(type, buffer, 8);
      std::memcpy(&total, buffer+8, sizeof(total));

      first = false;
      continue;
    }
    std::streamsize size = std::cin.gcount();

    payload.chunks.push_back(chunkify(buffer, type, index, size, total));
    if(!payload.send()){
      std::cerr << "[ERROR] sending payload\n";
    }
    index++;
  }
}
