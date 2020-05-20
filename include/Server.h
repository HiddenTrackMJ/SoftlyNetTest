#pragma once
/**
  @author Hidden Track
  @since 2020/05/18
  @time: 14:21
 */

#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <cstring>
#include <cstdint>
#include <arpa/inet.h>
#include <sys/socket.h>
#define SOCKET int
#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR (-1)
#endif

#define GOOD 0
#define ERR -1
#define SOCKET_VERSION MAKEWORD(2, 2)


#include "config.h"
#include <iostream>
#include "seeker/loggerApi.h"
#include <stdio.h>
#include <conio.h>

#include <Message.hpp>

#define SERVER_BUFF_LEN 2048

class Udp_Server {
 public:
  Udp_Server(int port);
  ~Udp_Server();
  int init_server();
  void sendMsg(char *msg, int len);
  int recvMsg(char *msg, int len);
  void replyMsg(Message &msg);
  void recvThread();

 private:
  std::string addr = "127.0.0.1";
  int listen_port = 40440;

  SOCKADDR_IN serverAddr = {0};
  SOCKADDR_IN clientAddr = {0};

  int clientAddrSize;
  std::atomic<int> test_id_gen{1};

#ifdef WIN32
  WSADATA wsaData;
  SOCKET serverSocket{INVALID_SOCKET};

#else
  int serverSocket;
#endif
};