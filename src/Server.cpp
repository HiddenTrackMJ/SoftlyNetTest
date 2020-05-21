/**
  @author Hidden Track
  @since 2020/05/18
  @time: 14:23
 */

#include "Server.h"



using std::cout;
using std::endl;

std::string formatTransfer(const uint64_t& dataSize) {
  static uint32_t Gbytes = 1024 * 1024 * 1024;
  static uint32_t Mbytes = 1024 * 1024;
  static uint32_t Kbytes = 1024;
  std::string rst{};
  if (dataSize > Gbytes) {
    rst = fmt::format("{:.{}f}Gbytes", (double)dataSize / Gbytes, 3);
  } else if (dataSize > Mbytes) {
    rst = fmt::format("{:.{}f}Mbytes", (double)dataSize / Mbytes, 3);
  } else if (dataSize > Kbytes) {
    rst = fmt::format("{:.{}f}Kbytes", (double)dataSize / Kbytes, 3);
  } else {
    rst = fmt::format("{}Gbytes", dataSize);
  }
  return rst;
}

std::string formatBandwidth(const uint32_t& bytesPerSecond) {
  static uint32_t Gbits = 1024 * 1024 * 1024;
  static uint32_t Mbits = 1024 * 1024;
  static uint32_t Kbits = 1024;
  uint64_t bitsPerSecond = (uint64_t)bytesPerSecond * 8;

  std::string rst{};
  if (bitsPerSecond > Gbits) {
    rst = fmt::format("{:.{}f}Gbits/sec", (double)bitsPerSecond / Gbits, 3);
  } else if (bitsPerSecond > Mbits) {
    rst = fmt::format("{:.{}f}Mbits/sec", (double)bitsPerSecond / Mbits, 3);
  } else if (bitsPerSecond > Kbits) {
    rst = fmt::format("{:.{}f}Kbits/sec", (double)bitsPerSecond / Kbits, 3);
  } else {
    rst = fmt::format("{}bits/sec", bitsPerSecond);
  }
  return rst;
}


Udp_Server::Udp_Server(int port) {
  listen_port = port;
  int ret = init_server();
  if (ret != 0) {
    E_LOG("Server init error...");
    exit(1);
  }
};

Udp_Server::~Udp_Server() {
  I_LOG("Udp_Server is destructed ...");
#ifdef WIN32
  closesocket(serverSocket);
  WSACleanup();
#else
  close(clientSocket);
#endif
}

int Udp_Server::init_server() {
  I_LOG("Udp_Server init ...");
  WSADATA wsaData = {};
#ifdef WIN32
  if (WSAStartup(SOCKET_VERSION, &wsaData) != GOOD) {
    E_LOG("WSAStartup error!");
    return ERR;
  }
#endif

  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(listen_port);

#ifdef WIN32
  serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
#else
  serverAddr.sin_addr.s_addr = INADDR_ANY;
#endif

  serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_HOPOPTS);
  if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(SOCKADDR_IN)) == SOCKET_ERROR) {
    closesocket(serverSocket);
    WSACleanup();
    E_LOG("Bind error!");
    return ERR;
  }

  clientAddrSize = sizeof(clientAddr);


  // clientAddr.sin_family = AF_INET;

  I_LOG("Server on {} init successfully!", listen_port);

  return GOOD;
}

void Udp_Server::sendMsg(char* msg, int len) {
  int ret = sendto(serverSocket, msg, len, 0, (struct sockaddr*)&clientAddr, clientAddrSize);
  if (ret == -1) {
    E_LOG("send msg to client error...");
  }
}

int Udp_Server::recvMsg(char* msg, int len) {
  int ret =
      recvfrom(serverSocket, msg, len, 0, (sockaddr*)&clientAddr, (socklen_t*)&clientAddrSize);
  return ret;
}

void Udp_Server::replyMsg(Message& msg) {
  static uint8_t buf[SERVER_BUFF_LEN]{0};
  msg.writeHead(buf);
  auto len = sizeof(buf);
  sendMsg((char*)buf, len);
  memset(buf, 0, len);
}


void Udp_Server::recvThread() {
  I_LOG("Server recv thread is starting ...");
  uint8_t buf[SERVER_BUFF_LEN]{0};

  while (true) {
    // D_LOG("Waiting msg...");
    auto recvLen = recvMsg((char*)buf, SERVER_BUFF_LEN);
    if (recvLen > 0) {
      Message msg(buf);

      switch (msg.get_msg_type()) {
        case MsgType::test_req: {
          TestReq req(buf);
          I_LOG("Got TestReq, msgId={}, testType={}, testTime={}", req.get_msg_id(), (int)req.test_type, req.test_time);
          TestRsp response(1, test_id_gen.fetch_add(1), req.get_msg_id(), Message::genMid());
          I_LOG("Reply Msg TestRsp, msgId={}, msgType={}, rst={}", response.get_msg_id(),
                (uint16_t)response.get_msg_type(), response.result);
          replyMsg(response);

          if (req.test_type == TestType::bandwidth) {
            currentTest = response.get_test_id();
            bwTest(req.test_time);
            currentTest = 0;
          } else {
            // nothing to do.
          }
          break;
        }

        case MsgType::rtt_test_msg: {
          sendMsg((char*)buf, recvLen);
          D_LOG("Reply rtt_test_msg, msgId={}", msg.get_msg_id());
          break;
        }

        default:
          W_LOG("Got unknown msg, ignore it. msgType={}, msgId={}", msg.get_msg_type(),
                msg.get_msg_id());
          break;
      }
    } else {
      throw std::runtime_error("msg receive error.");
    }
  }
  cout << "lalal" << std::endl;
  // while (1) {
  //  memset(buf, 0, SERVER_BUFF_LEN);
  //  int ret = recvMsg(buf, SERVER_BUFF_LEN);
  //  if (ret > 0) {
  //    buf[ret] = '\0';
  //    std::cout << "recv msg: " << buf << std::endl;
  //  } else {
  //    E_LOG("recv msg from 1 error...");
  //  }
  //}
}


void Udp_Server::bwTest(int testSeconds) {
  uint8_t recvBuf[SERVER_BUFF_LEN]{0};
  uint8_t sendBuf[SERVER_BUFF_LEN]{0};

  uint64_t totalRecvByte = 0;
  int64_t maxDelay = INT_MIN;
  int64_t minDelay = INT_MAX;

  int64_t startTimeMs = -1;
  int64_t lastArrivalTimeMs = -1;

  int pktCount = 0;


  while (true) {
    T_LOG("bandwidthTest Waiting msg...");
    auto recvLen = recvMsg((char*)recvBuf, SERVER_BUFF_LEN);

    if (startTimeMs > 0 &&
        Tools::getCurrentTimeMillis() - startTimeMs > testSeconds * 1000 + 5000) {
      W_LOG("Test timeout. testId={}", currentTest);
      break;
    }

    int64_t delay;
    if (recvLen > 0) {
      Message msg(recvBuf);

      switch (msg.get_msg_type()) {
        case MsgType::bw_test_msg: {
          if (msg.get_test_id() == currentTest) {
            BwTestMsg test_msg(recvBuf);
            T_LOG("receive a BandwidthTestMsg, testNum={}", test_msg.testPacketNumber);
            lastArrivalTimeMs = Tools::getCurrentTimeMillis();
            if (startTimeMs < 0) {
              startTimeMs = lastArrivalTimeMs;
            }
            totalRecvByte += recvLen;
            delay = Tools::getCurrentTimeMicro() - test_msg.get_timestamp();
            if (delay < minDelay) minDelay = delay;
            if (delay > maxDelay) maxDelay = delay;

            pktCount += 1;

            if (test_msg.testPacketNumber % 100 == 0) {
              I_LOG("BandwidthTestMsg testNum={} pktCount={}", test_msg.testPacketNumber,
                    pktCount);
            }
          } else {
            W_LOG("Got the wrong test_id when recving bw_test_msg!");
          }
          break;
        }

        case MsgType::bw_finish: {
          if (msg.get_test_id() == currentTest) {
            BwFinishMsg test_msg(recvBuf);
            auto passedTimeInMs = lastArrivalTimeMs - startTimeMs;
            auto jitter = maxDelay - minDelay;
            int totalPkt = test_msg.totalTestNum;
            int lossPkt = totalPkt - pktCount;
            I_LOG("bandwidth test report:");
            I_LOG("[ ID] Interval    Transfer    Bandwidth      Jitter   totlaReceivePkt");
            I_LOG("[{}]   {}s   {}     {}     {}ms   {}", test_msg.get_test_id(),
                  (double)passedTimeInMs / 1000, formatTransfer(totalRecvByte),
                  formatBandwidth(totalRecvByte * 1000 / passedTimeInMs),
                  (double)jitter / 1000, pktCount);


            BandWidthReport report(jitter, pktCount, totalRecvByte, test_msg.get_test_id(),
                                   Message::genMid());

            report.writeData(sendBuf);
            auto len = sizeof(sendBuf);
            sendMsg((char*)sendBuf, len);
            memset(sendBuf, 0, len);
            I_LOG("finish bandwidth test report.");
          } else {
            W_LOG("Got the wrong test_id when recving bw_finish_msg!");
          }

          break;
        }

        default:
          W_LOG("Got unknown msg, ignore it. msgType={}, msgId={}", msg.get_msg_type(),
                msg.get_msg_id());
          break;
      }

    } else {
      W_LOG("msg receive error.");
      throw std::runtime_error("msg receive error.");
    }
  }
  I_LOG("bandwidthTest finished.");
}