// i fucked up i used Signature for defination of pk(public key), fix that

#ifndef __PAYLOAD_HPP__
#define __PAYLOAD_HPP__

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>
#include <sodium.h>
#include <array>
#include <map>

struct Sign{
  unsigned char data[crypto_sign_BYTES];

  std::vector<char> serialize(){
    return {data, data + crypto_sign_BYTES};
  }
  int deserialize(std::vector<char> payload){
    if(payload.size() != crypto_sign_BYTES){
      return 1;
    }

    std::copy(payload.begin(), payload.end(), data);
    return 0;
  }
};

struct Message{
  std::vector<unsigned char> data;
  Sign pk; // acts a username too 

  std::vector<char> serialize(){
    std::vector<char> payload(sizeof(uint64_t) + data.size() + crypto_sign_BYTES);

    char* a = (char*)payload.data();
    uint64_t size = data.size();
    memcpy(a, &size, sizeof(size)); a += sizeof(size);

    std::copy(data.begin(), data.end(), a); a += data.size();

    std::vector<char> sign = pk.serialize();
    std::copy(sign.begin(), sign.end(), a); a += sign.size();

    return payload;
  }
  int deserialize(std::vector<char> payload){
    if(payload.size() < sizeof(uint64_t) + crypto_sign_BYTES) return 1;

    char* a = (char*)payload.data();

    uint64_t size;
    memcpy(&size, a, sizeof(uint64_t)); a += sizeof(uint64_t);
    if(size != payload.size() - sizeof(uint64_t) - crypto_sign_BYTES) 
      return 1;
    data.resize(size);

    std::copy(a, a+data.size(), data.begin()); a += data.size();

    if(pk.deserialize({a, payload.data()+payload.size()})){
      data.clear();
      return 1;
    }

    return 0;
  }
};

class PayloadManager{
  private:
    std::map<Sign, Message> messages_;

  public:
    std::vector<char> get_db(){
      if(messages_.empty()){
        return {};
      }

      std::vector<Sign> database;
      for(std::pair<Sign, Message> i : messages_){
        database.push_back(i.first);
      }

      const char* a = reinterpret_cast<const char*>(database.data());
      size_t size = database.size() * 
        sizeof(std::array<unsigned char, crypto_sign_BYTES>);
      return std::vector<char>(a, a+size);
    }

    std::vector<Sign> check_mising(std::vector<Sign> signs){ 
      std::vector<Sign> out;
      for(Sign i : signs){
        if(messages_.find(i) == messages_.end()){
          out.push_back(i);
        }
      }

      return out;
    }

    std::map<Sign, Message> get_stuff(std::vector<Sign> signs){
      std::map<Sign, Message> out;
      for(Sign i : signs){
        if(messages_.find(i) != messages_.end()){
          out[i] = messages_[i];
        }
      }

      return out;
    }
    
    int check_msg(std::pair<Sign, Message> msg){
      if(crypto_sign_verify_detached(msg.first.data, msg.second.data.data(), 
              sizeof msg.second.data, msg.second.pk.data) != 0){
        return 0;
      }

      return 1;
    }

    void push_to_db(std::map<Sign, Message> new_msg)
    {
      for(std::pair<Sign, Message> i : new_msg){
        if(check_msg(i)){
          messages_[i.first] = i.second;
        }
      }
    }
};

#endif
