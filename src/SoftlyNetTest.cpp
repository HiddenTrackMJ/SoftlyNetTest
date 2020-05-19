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
int main(int argc, char* argv[]) {
  seeker::Logger::init();

  auto res = parse(argc, argv);

  if (res.count("server")) {
    Udp_Server server;
    std::thread recv_thread(&Udp_Server::recvThread, &server);
    while (1) {
      std::string input;
      std::cin >> input;
      if (input == "end") break;
      server.sendMsg((char*)input.c_str(), input.length());
    }
    recv_thread.join();
  }

  if (res.count("client")) {
    auto client = Udp_Client();
    std::thread recv_thread(&Udp_Client::recvThread, &client);
    while (1) {
      std::string input;
      std::cin >> input;
      if (input == "end") break;
      client.sendMsg((char*)input.c_str(), input.length());
    }
    recv_thread.join();
  }

  return 0;
}
