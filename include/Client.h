#pragma once
/**
  @author Hidden Track
  @since 2020/05/18
  @time: 19:05
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

#define OK 0
#define ERR -1
#define SOCKET_VERSION MAKEWORD(2, 2)

#include "config.h"
#include <iostream>
#include "seeker/loggerApi.h"
#include <stdio.h>
#include <conio.h>

#define BUFF_LEN 2048

class Udp_Client {
 public:
  Udp_Client();
  ~Udp_Client();
  int init_client();
  void sendMsg(char *msg, int len);
  int recvMsg(char *msg, int len);
  void recvThread();

 private:
  std::string addr = "127.0.0.1";
  int port = 40440;

  SOCKADDR_IN serverAddr = {0};
  SOCKADDR_IN clientAddr;

#ifdef WIN32
  WSADATA wsaData;
  SOCKET clientSocket{INVALID_SOCKET};

#else
  int clientSocket;
#endif
};