
/**
  @author Hidden Track
  @since 2020/05/18
  @time: 19:05
 */

#pragma once
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
//#define OK 0
#define ERR -1
#define SOCKET_VERSION MAKEWORD(2, 2)


#include "config.h"
#include <opencv2/opencv.hpp>

#include <iostream>
#include "seeker/loggerApi.h"
#include <stdio.h>
#include <conio.h>
#include <vector>






#define CLIENT_BUFF_LEN 2048

class Udp_Client {
 public:
  Udp_Client(std::string ip, int port);

  ~Udp_Client();

  int init_client();

  void sendMsg(char *msg, int len);

  int recvMsg(char *msg, int len);

  void recvThread();

  void rttTest(int testTimes, int packetSize);

  void bandwidthTest(int bw, int pkt_size, int test_time, int report_interval);

 struct BW_Report{
    int testId;
    double interval;
    std::string total_data;
    double bandwidth;
    double jitterMicroSec;
    int loss_pkt;
    int total_pkt;
    double datagrams;
  };



 private:
  std::string server_ip = "127.0.0.1";
  int server_port = 40440;

  std::vector<cv::Point> points;
  std::atomic<int> point_x_gen{1};

  SOCKADDR_IN serverAddr = {0};
  SOCKADDR_IN clientAddr;

  cv::Mat X;

  bool polynomial_curve_fit(std::vector<cv::Point>& key_point, int n, cv::Mat& A);

  BW_Report bwRound(int bw, int pkt_size, int test_time, int report_interval);

    void draw(std::vector<cv::Point> points, std::string name);

#ifdef WIN32
  WSADATA wsaData;
  SOCKET clientSocket{INVALID_SOCKET};

#else
  int clientSocket;
#endif
};