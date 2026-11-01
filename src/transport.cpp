#include <boost/asio.hpp>
#include <istream>
#include <iostream>
#include <transport.hpp>

using boost::asio::ip::udp;

boost::asio::ip::address_v4 my_ip(boost::asio::io_context& io){
  udp::socket socket(io, udp::endpoint(udp::v4(), 0));
  socket.connect({boost::asio::ip::make_address_v4("192.168.0.1"), 53});
  return socket.local_endpoint().address().to_v4();
}

int main(){
  LanTransport a;
  a.start(); 

  std::cin.get();
}
