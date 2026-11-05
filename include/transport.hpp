#ifndef __TRANSPORT_HPP__
#define __TRANSPORT_HPP__

#include <array>
#include <boost/asio.hpp>
#include <cstddef>
#include <cstring>
#include <vector>
#include <thread>
#include <queue>
#include <memory.h>
#include <iostream>
#include <sodium.h>

#define INTRODUCER_WAIT 5
#define USER_PIPE_PORT 0
#define INTRODUCER_PORT 60007

using boost::asio::ip::udp;
using boost::asio::ip::tcp;


boost::asio::ip::address_v4 my_ip(boost::asio::io_context& io);

struct Message{
    std::vector<uint8_t> data;
    unsigned char signature[crypto_sign_BYTES];
    unsigned char pk[crypto_sign_PUBLICKEYBYTES]; // acts a username + signature check
};

class PayloadManager{
  private:
    std::vector<std::array<unsigned char, crypto_sign_BYTES>> database_;

  public:
    int add_to_db(){


      return 0;
    }
    std::vector<char> get_db(){
      if(database_.empty()){
        return {};
      }

      const char* a = reinterpret_cast<const char*>(database_.data());
      size_t size = database_.size() * sizeof(std::array<unsigned char, crypto_sign_BYTES>);
      return std::vector<char>(a, a+size);
    }
};


class LanUserPipePeers : public std::enable_shared_from_this<LanUserPipePeers>{
  private:
    tcp::socket socket_;
    std::queue<std::vector<char>> send_queue_;
    bool sending_;
    boost::asio::streambuf recv_buff_;

    void raw_send(){
      if(sending_) return;

      sending_ = true;
      std::shared_ptr<std::vector<char>> msg 
        = std::make_shared<std::vector<char>>(std::move(send_queue_.front()));
      send_queue_.pop();

      boost::asio::async_write(socket_, 
          boost::asio::buffer(*msg), 
          [msg,this](const boost::system::error_code& e, std::size_t byte)
        {
        if(e){
          std::cerr << "\n[ERROR] msg not sent\n";
          // handle error properly
        }
        else{
        std::cout << "\n[LOG] " << byte << " sent\n";
        }
        
        sending_ = false;
        if(!send_queue_.empty()){
          raw_send();
        }
      });
    }

  public:
    LanUserPipePeers(boost::asio::io_context& io_context) : socket_(io_context){}

    typedef std::shared_ptr<LanUserPipePeers> pointer;
    static pointer create(boost::asio::io_context& io_context){
      return pointer(new LanUserPipePeers(io_context));
    }

    tcp::socket& socket(){
      return socket_;
    }

    void send(std::vector<char> msg){
      // !!! not sure if it will work cross platform(for struct n stuff)
//      std::shared_ptr<std::vector<char>> b 
//        = std::make_shared<std::vector<char>>(std::move(msg));
//      (*b).push_back('\n');

      msg.push_back('\n');
      boost::asio::post(socket_.get_executor(), [msg, this]()
        {
          send_queue_.push(std::move(msg));
        });
    }

    // no need of this ig
    std::string read(){
//      boost::asio::async_read_until(socket_, recv_buff_, '\0' ,
//          [](const boost::system::error_code& e, std::size_t bytes)
//          {
//          });
      return "\n[DEBUG] you'r dumb\n";
    }
};

class LanTransport{
  private:
    boost::asio::io_context io_context_;
    
    boost::asio::steady_timer introducer_timer_;
    udp::socket introducer_socket_;
    std::thread introducer_thread_;
    std::array<char, 128> recv_buff_;
    udp::endpoint sender_endpoint_;

    tcp::endpoint user_pipe_endpoint_;
    tcp::acceptor user_pipe_acceptor_;
    std::vector<LanUserPipePeers::pointer> user_pipe_peers_;
    std::vector<LanUserPipePeers::pointer> connected_pipe_peers_;

    PayloadManager payload_;

    void start_introducer(){
      introducer_timer_.expires_after(boost::asio::chrono::seconds(INTRODUCER_WAIT));
      introducer_timer_.async_wait([this](const boost::system::error_code& e){
        if(!e){
          std::string msg = "P2P_INTRODUCER"+
            std::to_string(user_pipe_acceptor_.local_endpoint().port());

            std::cout << "\n[DEBUG] introducer sent (" << msg << ")\n";

          introducer_socket_.send_to(boost::asio::buffer(msg),
              udp::endpoint(boost::asio::ip::address_v4::broadcast(), 60007));
        }
        start_introducer();
      });
    }

    void introducer_listener(){
      
      introducer_socket_.async_receive_from(boost::asio::buffer(recv_buff_),
          sender_endpoint_, [this](const boost::system::error_code& e, std::size_t bytes){
            if(!e){
              LanUserPipePeers::pointer peer = LanUserPipePeers::create(io_context_);

              std::string m(recv_buff_.data(), bytes);
              std::cout << "\n[DEBUG] received\n" << m;
              unsigned short port = static_cast<unsigned short>(std::stoi(m.substr(14)));

              tcp::endpoint connected_pipe_endpoint{sender_endpoint_.address(), port};
 
              if(connected_pipe_endpoint.address() ==  // echo of user introducer
                  my_ip(io_context_)){ introducer_listener(); return; }
              for(auto i : connected_pipe_peers_){ // already connected user pipes
                if(i->socket().remote_endpoint() == connected_pipe_endpoint){
                  introducer_listener();
                  return;
                }
              }

              std::shared_ptr<std::vector<char>> db 
                = std::make_shared<std::vector<char>>(payload_.get_db());
              peer->socket().async_connect(connected_pipe_endpoint, 
                  [this, peer, db](const boost::system::error_code& e){
                    if(!e){
                      std::cout << "\n[DEBUG] connected to peer\n";
                      connected_pipe_peers_.push_back(peer);
                      // send user database to connected pipe 
                      // (peer send back missing stuff)

                      peer->send(*db);

                      // ----test----
/*                      std::shared_ptr<boost::asio::streambuf> recv_buff = 
 *                        std::make_shared<boost::asio::streambuf>();
                      boost::asio::async_read_until(peer->socket(), 
                          *recv_buff, '\n' ,
                          [recv_buff](const boost::system::error_code& e,
                          std::size_t bytes){
                            std::istream stream(recv_buff.get());
                            std::string messg;
                            std::getline(stream, messg);
                            std::cout << "\n[LOG] recived from connected pipe : "
                            << messg << "\n";
                          }); */


                    }
                  });
          peer->read();
            }
            introducer_listener();
          });
    }

    void user_pipe(){
      LanUserPipePeers::pointer peer = LanUserPipePeers::create(io_context_);

      user_pipe_acceptor_.async_accept(peer->socket(), [this, peer](const boost::system::error_code& e){
        if(!e){
          user_pipe_peers_.push_back(peer);

          // listen for database(req what i dont have)
        
          }
        user_pipe();
      });
    }


  public:
    LanTransport(PayloadManager p) : introducer_timer_(io_context_), 
      introducer_socket_(io_context_, udp::endpoint(udp::v4(), INTRODUCER_PORT)),
      user_pipe_endpoint_(tcp::v4(), USER_PIPE_PORT),
      user_pipe_acceptor_(io_context_, user_pipe_endpoint_), payload_(p)
    {
      introducer_socket_.set_option(boost::asio::socket_base::broadcast(true));
    }
    ~LanTransport(){
      io_context_.stop();
      if(introducer_thread_.joinable()) introducer_thread_.join();
    }

    void shout_message(std::vector<char> &msg){
      for(LanUserPipePeers::pointer i : connected_pipe_peers_){
        i->send(msg);
      }
    }

    void start(){
      user_pipe();

      start_introducer();
      introducer_listener();
      
      introducer_thread_ = std::thread([this](){
        io_context_.run();
      });
    }



};

#endif
