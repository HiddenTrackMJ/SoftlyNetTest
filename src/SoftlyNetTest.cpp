/**
  @author Hidden Track
  @since 2020/05/18
  @time: 12:01
 */

#include "config.h"
#include "SoftlyNetTest.h"
#include "seeker/logger.h"
#include "cmdopts.hpp"
#include "Server.h"
#include "Client.h"

#include <Message.hpp>



#include <iostream>
#include <memory>
#include <thread>

using std::cout;
using std::endl;
using std::shared_ptr;

cxxopts::ParseResult parse(int argc, char* argv[]);

class A {
 public:
  int i;
  A(int n) : i(n){};
  ~A() {
    cout << i << " "
         << "destructed" << endl;
  }
};

//#undef main
int main(int argc, char* argv[]) {
  seeker::Logger::init();

  auto res = parse(argc, argv);

  uint8_t a[100]{11111};
  uint16_t b = 0; 
  std::cout << "before: a: " << (uint32_t)*a << " b: " << (uint32_t)b << std::endl;
  Tools::readData((uint8_t*)a + 1, b);
  std::cout << "after: a: " << (uint32_t)*a << " b: " << (uint32_t)b << std::endl;

  if (res.count("server")) {
    Udp_Server server(res["port"].as<int>());
    std::thread recv_thread(&Udp_Server::recvThread, &server);
    recv_thread.detach();

    while (1) {
      std::string input;
      std::cin >> input;
      if (input == "end") break;
      server.sendMsg((char*)input.c_str(), input.length());
    }

  }

 
  if (res.count("client")) {
    auto client = Udp_Client(res["client"].as<string>(), res["port"].as<int>());
    //std::thread recv_thread(&Udp_Client::recvThread, &client);
    //recv_thread.detach();

    client.rttTest(20, 64);

    while (1) {
      std::string input;
      std::cin >> input;
      if (input == "end") break;
      client.sendMsg((char*)input.c_str(), input.length());
    }

    
  }

  return 0;
}
