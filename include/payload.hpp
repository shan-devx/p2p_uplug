#ifndef PAYLOAD_HPP
#define PAYLOAD_HPP

#include <cstring>
#include <sys/types.h>
#include <vector>
#include <string>
#include <cstdint>
#include <sodium.h>
#include <iostream>
#include <fstream>

#define USER_SK_SAVE_PATH "./sk"

class UserManager{
  private:
    std::string username;
    std::string pk;
  public:
    int init_user(UserManager &user, const char* passwd){
      unsigned char pk[crypto_sign_PUBLICKEYBYTES];
      unsigned char sk[crypto_sign_SECRETKEYBYTES];
  
      crypto_sign_keypair(pk, sk);

      unsigned char salt[crypto_pwhash_SALTBYTES];
      unsigned char key[crypto_secretbox_KEYBYTES];

      randombytes_buf(salt, sizeof salt);

      if(crypto_pwhash(key, sizeof key, passwd, strlen(passwd),
        salt, crypto_pwhash_OPSLIMIT_INTERACTIVE, 
        crypto_pwhash_MEMLIMIT_INTERACTIVE, crypto_pwhash_ALG_DEFAULT) != 0)
      {
      std::cerr << "\n[ERROR] while running crypto_pwhash()\n";
      return 1;
      }

      unsigned char nonce[crypto_secretbox_NONCEBYTES];
      randombytes_buf(nonce, sizeof nonce);
      unsigned char out[crypto_secretbox_MACBYTES + sizeof sk];
  
      if(crypto_secretbox_easy(out, sk, sizeof sk, nonce, key) != 0){
          std::cerr << "\n[ERROR] while running crypto_secretbox_easy()\n";
          return 1;
      }

      std::ofstream file(USER_SK_SAVE_PATH, std::ios::binary);
      if(!file){
        std::cerr << "\n[ERROR] opening sk save path\n";
        return 1;
      }

      file.write(reinterpret_cast<const char*>(salt), sizeof salt);
      file.write(reinterpret_cast<const char*>(nonce), sizeof nonce);
      file.write(reinterpret_cast<const char*>(out), sizeof out);
      file.close();

      sodium_memzero(sk, sizeof sk);
      sodium_memzero(key, sizeof key);

      return 0;
    }

    unsigned char get_sk(const char* passwd){
      std::fstream file(USER_SK_SAVE_PATH);
      if(!file){
        std::cerr << "\n[ERROR] sk file doesn't exist\n";
        return 1;
      }

    };
};

struct Message{
  std::string username;
  std::vector<uint8_t> data;

  std::string signature;
  std::string public_key;
};

class PayloadManager{
  private:
    std::vector<Message> database;
  public:
  
};

#endif
