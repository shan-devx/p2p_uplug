#include <array>
#include <boost/asio.hpp>
#include <memory>
#include <queue>
#include <string>
#include <thread>
#include <iostream>
#include <transport.hpp>
#include <vector>

using boost::asio::ip::udp;
using boost::asio::ip::tcp;

class LanPeers : public std::enable_shared_from_this<LanPeers>{
  private:
    tcp::socket socket_;
    std::queue<std::string> send_queue_;
    bool sending;

  public:
    LanPeers(boost::asio::io_context& io_context) : socket_(io_context){
    }

    typedef std::shared_ptr<LanPeers> pointer;

    static pointer create(boost::asio::io_context& io_context){
      return pointer(new LanPeers(io_context));
    }

    tcp::socket& socket(){
      return socket_;
    }

    void raw_send(){
      if(sending) return;

      sending = true;
      auto msg = std::make_shared<std::string>(std::move(send_queue_.front()));
      send_queue_.pop();

      boost::asio::async_write(socket_, 
          boost::asio::buffer(*msg), [msg,this](const boost::system::error_code& e, std::size_t byte){
        if(e){
          std::cerr << "[ERROR] msg not sent\n";
        }
        else{
        std::cout << "[LOG] " << byte << " sent";
        }
        
        sending = false;
        if(!send_queue_.empty()){
          raw_send();
        }
      });
    }
    void send(std::string message){
      send_queue_.push(std::move(message));
      raw_send();
    }


    std::string read(){
      
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
    tcp::acceptor pipe_acceptor_;
    std::vector<LanPeers::pointer> peers_;

    void start_introducer(){
      introducer_timer_.expires_after(boost::asio::chrono::seconds(INTRODUCER_WAIT));
      introducer_timer_.async_wait([this](const boost::system::error_code& e){
        if(!e){
          std::string msg = "P2P_INTRODUCER"+
            std::to_string(pipe_acceptor_.local_endpoint().port());

            std::cout << "[DEBUG] " << msg << "\n";

          introducer_socket_.send_to(boost::asio::buffer(msg),
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

    void user_pipe(){
      LanPeers::pointer peer = LanPeers::create(io_context_);

      pipe_acceptor_.async_accept(peer->socket(), [this, peer](const boost::system::error_code& e){
        if(!e){
          peers_.push_back(peer);

          // send database (peers send back missing stuff)
        }
        user_pipe();
      });
    }

  public:
    LanTransport() : introducer_timer_(io_context_), 
      introducer_socket_(io_context_, udp::endpoint(udp::v4(), 50000)),
      user_pipe_endpoint_(tcp::v4(), 0), 
      pipe_acceptor_(io_context_, user_pipe_endpoint_)
    {
      introducer_socket_.set_option(boost::asio::socket_base::broadcast(true));
    }
    ~LanTransport(){
      io_context_.stop();
      if(introducer_thread_.joinable()) introducer_thread_.join();
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

int main(){
  LanTransport a;
  a.start(); 

  std::cin.get();
}
