#ifndef __PAYLOAD_HPP__
#define __PAYLOAD_HPP__

#include <algorithm>
#include <compare>
#include <cstdint>
#include <cstring>
#include <vector>
#include <sodium.h>
#include <array>
#include <map>

// ig serialization and deseriazation is most vulnerable to attack

template <unsigned int N>
class SuperChar{
  private:
    std::vector<char> buffer;

  public:
    SuperChar(){
      buffer.resize(N);
    }
    std::strong_ordering operator<=>(const SuperChar& other) const{
      return buffer <=> other.buffer;
    }

    unsigned char* data(){
      return (unsigned char*)buffer.data();
    }

    void save(std::vector<char> b){buffer=b;}

    std::vector<char> serialize() const {return buffer;}
    int deserialize(std::vector<char> payload){
      if(payload.size() != buffer.size()){
        return 1;
      }

      std::copy(payload.begin(), payload.end(), buffer.begin());
      return 0;
    }
};

typedef SuperChar<crypto_sign_BYTES> Sign;

struct Message{
  std::vector<unsigned char> data;
  SuperChar<crypto_sign_PUBLICKEYBYTES> pk; // acts a username too 

  std::vector<char> serialize(){
    std::vector<char> payload(sizeof(uint64_t) + data.size() + crypto_sign_PUBLICKEYBYTES);

    char* a = (char*)payload.data();
    uint64_t size = data.size();
    memcpy(a, &size, sizeof(size)); a += sizeof(size);

    std::copy(data.begin(), data.end(), a); a += data.size();

    std::vector<char> sign = pk.serialize();
    std::copy(sign.begin(), sign.end(), a); a += sign.size();

    return payload;
  }
  int deserialize(std::vector<char> payload){
    if(payload.size() < sizeof(uint64_t) + crypto_sign_PUBLICKEYBYTES) return 1;

    char* a = (char*)payload.data();

    uint64_t size;
    memcpy(&size, a, sizeof(uint64_t)); a += sizeof(uint64_t);
    if(size != payload.size() - sizeof(uint64_t) - crypto_sign_PUBLICKEYBYTES) 
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
    std::vector<char> get_db(){ // [vecotr size][signs(crypto_sign_BYTES)]
      std::vector<char> out;
      size_t sz = messages_.size();
      char* p = reinterpret_cast<char*>(&sz);
      out.insert(out.end(), p, p+sizeof(sz));

      for(const auto& [sign, mess] : messages_){
        std::vector<char> serlz = sign.serialize();

        out.insert(out.end(), serlz.begin(), serlz.end());
      }
      return out;
    }

    std::vector<Sign> check_missing(std::vector<char>& db){ 
      std::vector<Sign> out;

      size_t sz;
      memcpy(&sz, db.data(), sizeof(sz));
      Sign deserlz;
      int it = sizeof(sz);

      // !!! validate it

      for(int i = 0; i < sz; i++){
        deserlz.deserialize(
            std::vector<char>(db.begin()+it, db.begin()+it+crypto_sign_BYTES));
        it+=crypto_sign_BYTES;

        if(messages_.find(deserlz) == messages_.end()){
          out.push_back(deserlz);
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
      if(crypto_sign_verify_detached(msg.first.data(), msg.second.data.data(), 
            msg.second.data.size(), msg.second.pk.data()) != 0){
        return 0;
      }

      return 1;
    }

    void push_to_db(std::map<Sign, Message>& new_msg)
    {
      for(std::pair<Sign, Message> i : new_msg){
        if(check_msg(i)){
          messages_[i.first] = i.second;
        }
      }
    }
};

#endif
