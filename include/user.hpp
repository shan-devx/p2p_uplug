#ifndef PAYLOAD_HPP
#define PAYLOAD_HPP

#include <cstring>
#include <sodium.h>
#include <iostream>
#include <fstream>

// change it later (to support cross platform)
#define USER_SK_SAVE_PATH "~/.config/p2p_uplug/p2p_key"
#define USER_PK_SAVE_PATH "~/.config/p2p_uplug/p2p_key.pub"

class UserManager{
  private:
    unsigned char pk[crypto_sign_PUBLICKEYBYTES];
    bool ok = true;

  public:
    UserManager(){
      std::ifstream file(USER_PK_SAVE_PATH, std::ios::binary);
      if(!file.is_open()){
        std::cerr << "\n[LOG] User is not initizalized\n";
        this->ok = false;
      }

      char pk[crypto_sign_SECRETKEYBYTES];
      file.read(pk, sizeof pk);
      
      memcpy(this->pk, pk, sizeof this->pk);
    }
    int init_user(UserManager &user, const char* passwd){
      unsigned char pk[crypto_sign_PUBLICKEYBYTES];
      unsigned char sk[crypto_sign_SECRETKEYBYTES];
  
      crypto_sign_keypair(pk, sk);
      memcpy(user.pk, pk, crypto_sign_PUBLICKEYBYTES);


      // encrypt secret key
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

      // saving
      std::ofstream file(USER_SK_SAVE_PATH, std::ios::binary);
      if(!file){
        std::cerr << "\n[ERROR] opening sk save path\n";
        return 1;
      }

      file.write(reinterpret_cast<const char*>(salt), sizeof salt);
      file.write(reinterpret_cast<const char*>(nonce), sizeof nonce);
      file.write(reinterpret_cast<const char*>(out), sizeof out);
      file.close();

      file.open(USER_PK_SAVE_PATH, std::ios::binary);
      if(!file){
        std::cerr << "\n[ERROR] opening pk save path\n";
      }
      file.write(reinterpret_cast<const char*>(pk), sizeof pk);
      file.close();

      sodium_memzero(sk, sizeof sk);
      sodium_memzero(key, sizeof key);

      return 0;
    }

    int get_sk(const char* passwd, unsigned char* sk){
      std::fstream file(USER_SK_SAVE_PATH);
      if(!file){
        std::cerr << "\n[ERROR] sk file doesn't exist\n";
        return 1;
      }

      char key[crypto_pwhash_SALTBYTES+crypto_secretbox_NONCEBYTES
        +crypto_secretbox_MACBYTES];
      file.read(key, sizeof key);

      unsigned char salt[crypto_pwhash_SALTBYTES];
      unsigned char nonce[crypto_secretbox_NONCEBYTES];
      unsigned char encrypted[crypto_secretbox_MACBYTES];
      memcpy(salt, key, sizeof salt);
      memcpy(nonce, key+sizeof salt, sizeof nonce);
      memcpy(encrypted, key + sizeof salt + sizeof nonce, sizeof encrypted);

      unsigned char pwkey[crypto_secretbox_KEYBYTES];
      if(crypto_pwhash(pwkey, sizeof pwkey, passwd, strlen(passwd),
            salt, crypto_pwhash_OPSLIMIT_INTERACTIVE, 
        crypto_pwhash_MEMLIMIT_INTERACTIVE, crypto_pwhash_ALG_DEFAULT) != 0)
      {
        std::cerr << "\n[ERROR] while running crypto_pwhash()\n";
        return 1;
      }

      unsigned char decrypted[crypto_sign_SECRETKEYBYTES];
      if(crypto_secretbox_open_easy(decrypted, encrypted, sizeof encrypted, 
            nonce, pwkey) != 0)
      {
        std::cerr << "\n[ERROR] while running crypto_secretbox_easy()\n";
        return 1;
      }

      memcpy(sk, decrypted, sizeof decrypted);

      sodium_memzero(decrypted, sizeof decrypted);
      sodium_memzero(pwkey, sizeof pwkey);
      
      return 0;
    };

    int get_pk(unsigned char* pk){
      std::ifstream file(USER_PK_SAVE_PATH, std::ios::binary);
      if(!file){
        std::cerr << "\n[ERROR] sk file doesn't exist\n";
        return 1;
      }

      file.read((char*)pk, sizeof pk);

      return 0;
    }

};

#endif
