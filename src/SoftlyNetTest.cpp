/**
  @author Hidden Track
  @since 2020/05/18
  @time: 12:01
 */

#include "config.h"
#include "seeker/logger.h"
#include "cmdopts.hpp"
#include "Server.h"
#include "Client.h"


#include <iostream>
#include <memory>
#include <thread>


cxxopts::ParseResult parse(int argc, char* argv[]);




int main(int argc, char* argv[]) {
  seeker::Logger::init();

  auto res = parse(argc, argv);

  //uint8_t a[100]{11111};
  //uint16_t b = 0; 
  //std::cout << "before: a: " << (uint32_t)*a << " b: " << (uint32_t)b << std::endl;
  //Tools::readData((uint8_t*)a + 1, b);
  //std::cout << "after: a: " << (uint32_t)*a << " b: " << (uint32_t)b << std::endl;

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

    std::string host = res["c"].as<string>();
    int port = res["p"].as<int>();
    int time = res["t"].as<int>();
    int interval = res["i"].as<int>();

    Udp_Client client(host, port);
    //std::thread recv_thread(&Udp_Client::recvThread, &client);
    //recv_thread.detach();
    if (res.count("b")) {
      string bandwidth = res["b"].as<string>();
      int packetSize = 1400;
      auto bandwidthValue = std::stoi(bandwidth.substr(0, bandwidth.size() - 1));
      client.bandwidthTest(bandwidthValue, packetSize, time, 
                           interval);
    } else {
      client.rttTest(time, 64);
    }
    
    while (1) {
      std::string input;
      std::cin >> input;
      if (input == "end") break;
      client.sendMsg((char*)input.c_str(), input.length());
    }

    
  }

  throw std::runtime_error("msg receive error.");

  return 0;
}

int main1(int argc, char* argv[]) {
    seeker::Logger::init();

    auto res = parse(argc, argv);

    std::cout << "123" << std::endl;
  
    return 0;
}