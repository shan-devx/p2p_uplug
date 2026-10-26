#include <array>
#include <boost/asio.hpp>
#include <thread>
#include <iostream>
#include <transport.hpp>

using boost::asio::ip::udp;
using boost::asio::ip::tcp;

class LanTransport{
  private:
    boost::asio::io_context io_context_;
    
    boost::asio::steady_timer introducer_timer_;
    udp::socket introducer_socket_;
    std::thread introducer_thread_;
    std::array<char, 128> recv_buff_;
    udp::endpoint sender_endpoint_;

    void start_introducer(){
      introducer_timer_.expires_after(boost::asio::chrono::seconds(INTRODUCER_WAIT));
      introducer_timer_.async_wait([this](const boost::system::error_code& e){
        if(!e){
          std::cout << "sent\n";
          introducer_socket_.send_to(boost::asio::buffer("P2P_INTRODUCER_\n"),
              udp::endpoint(boost::asio::ip::address_v4::broadcast(), 50000));
        }
        start_introducer();
      });
    }

    void introducer_listener(){
      
      introducer_socket_.async_receive_from(boost::asio::buffer(recv_buff_),
          sender_endpoint_, [this](const boost::system::error_code& e, std::size_t bytes){
            if(!e){
              std::cout << "received\n";
              std::cout.write(recv_buff_.data(), bytes);
            }
            introducer_listener();
          });
    }

  public:
    LanTransport() : introducer_timer_(io_context_), 
      introducer_socket_(io_context_, udp::endpoint(udp::v4(), 50000)){
      introducer_socket_.set_option(boost::asio::socket_base::broadcast(true));
    }
    ~LanTransport(){
      io_context_.stop();
      if(introducer_thread_.joinable()) introducer_thread_.join();
    }

    void start(){
      start_introducer();
      introducer_listener();
      
      introducer_thread_ = std::thread([this](){
        io_context_.run();
      });
    }
};

int main(){
  LanTransport a;
  a.start(); 

  while(true){};
}
