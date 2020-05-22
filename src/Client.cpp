/**
  @author Hidden Track
  @since 2020/05/18
  @time: 19:07
 */

#include "Client.h"
#include <cassert>
#include <cmath>
#include <Message.hpp>
#include "DrawUtil.hpp"



using std::cout;
using std::endl;

extern std::string formatTransfer(const uint64_t& dataSize);
extern std::string formatBandwidth(const uint32_t& bytesPerSecond);

// extern void py_init();
// extern void py_finalize();

// template <class T, class V = int>
// extern void plot(T* x, int N1, V* y = NULL, bool equal = false);


Udp_Client::Udp_Client(std::string ip, int port) {
  server_ip = ip;
  server_port = port;
  int ret = init_client();
  if (ret != 0) {
    E_LOG("Client init error...");
    exit(1);
  }
};

Udp_Client::~Udp_Client() {
  I_LOG("Udp_Client is destructed ...");
#ifdef WIN32
  closesocket(clientSocket);
  WSACleanup();
#else
  close(clientSocket);
#endif
}

int Udp_Client::init_client() {
  I_LOG("Udp_Client init ...");
  WSADATA wsaData = {};
#ifdef WIN32
  if (WSAStartup(SOCKET_VERSION, &wsaData) != GOOD) {
    E_LOG("WSAStartup error!");
    return ERR;
  }
#endif

  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(server_port);

#ifdef WIN32
  serverAddr.sin_addr.S_un.S_addr = inet_addr(server_ip.c_str());  // htonl(INADDR_ANY);
#else
  serverAddr.sin_addr.s_addr = INADDR_ANY;
#endif

  clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_HOPOPTS);

  if (clientSocket == -1) {
    E_LOG("Socket start error!");
    return ERR;
  } else {
    //发送信息与服务器建立连接(必须加)
    // sendto(clientSocket, "hello server", strlen("hello server"), 0, (sockaddr*)&serverAddr,
    //       sizeof(serverAddr));
  }


  I_LOG("Client on {} init successfully!", server_port);

  return GOOD;
}

void Udp_Client::sendMsg(char* msg, int len) {
  int ret = sendto(clientSocket, msg, len, 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
  if (ret == -1) {
    E_LOG("send msg to server error...");
  }
}

int Udp_Client::recvMsg(char* msg, int len) {
  auto addrLen = sizeof(serverAddr);
  int ret = recvfrom(clientSocket, msg, len, 0, (sockaddr*)&serverAddr, (socklen_t*)&addrLen);
  // int ret = recvfrom(clientSocket, msg, len, 0, 0, 0);
  return ret;
}

void Udp_Client::recvThread() {
  I_LOG("recv thread is starting ...");
  char buf[CLIENT_BUFF_LEN];
  while (1) {
    memset(buf, 0, CLIENT_BUFF_LEN);
    int ret = recvMsg(buf, CLIENT_BUFF_LEN);
    if (ret > 0) {
      buf[ret] = '\0';
      std::cout << "recv msg: " << buf << std::endl;
    } else {
      // E_LOG("recv msg from 2 error...");
    }
  }
}

void Udp_Client::rttTest(int testTimes, int packetSize) {
  uint8_t recvBuf[CLIENT_BUFF_LEN]{0};

  assert(packetSize >= 24);

  double point_x[100];
  double point_y[100];
  int point_size = 0;

  TestReq req(TestType::rtt, 0, Message::genMid());

  uint8_t Buf[CLIENT_BUFF_LEN]{0};
  req.writeHead(Buf);

  sendMsg((char*)Buf, CLIENT_BUFF_LEN);
  I_LOG("send TestReq, msgId={} testType={}", req.get_msg_id(), req.test_type);
  memset(Buf, 0, CLIENT_BUFF_LEN);

  auto testId = 0;
  auto recvLen = recvMsg((char*)recvBuf, CLIENT_BUFF_LEN);
  if (recvLen > 0) {
    TestRsp rsp(recvBuf);
    I_LOG("receive TestRsp, result={} reMsgId={} testId={}",
          rsp.result,
          rsp.reMsgId,
          req.get_test_id());
    testId = rsp.get_test_id();
    memset(recvBuf, 0, recvLen);
  } else {
    throw std::runtime_error("TestRsp receive error. recvLen=" + std::to_string(recvLen));
  }


  while (testTimes > 0) {
    testTimes--;
    RttTestMsg msg(packetSize, testId, Message::genMid());
    msg.writeHead(Buf);
    sendMsg((char*)Buf, CLIENT_BUFF_LEN);
    T_LOG(
        "send rtt_test_msg, msgId={} testId={} time={}", msg.msgId, msg.testId, msg.timestamp);
    memset(Buf, 0, CLIENT_BUFF_LEN);

    recvLen = recvMsg((char*)recvBuf, CLIENT_BUFF_LEN);
    if (recvLen > 0) {
      RttTestMsg rttResponse(recvBuf);

      auto diffTime = Tools::getCurrentTimeMicro() - rttResponse.get_timestamp();
      I_LOG("receive rtt_test_msg, msgId={} testId={} time={} diff={}ms",
            rttResponse.get_msg_id(),
            rttResponse.get_test_id(),
            rttResponse.get_timestamp(),
            (double)diffTime / 1000);

      auto x = point_x_gen.fetch_add(1);
      points.push_back(cv::Point(x, diffTime));
      point_x[point_size] = (double)x;
      point_y[point_size] = (double)diffTime;
      point_size++;

      memset(recvBuf, 0, recvLen);
    } else {
      throw std::runtime_error("rtt_test_msg receive error. recvLen=" +
                               std::to_string(recvLen));
    }
  }


  plot(point_x, point_size, point_y);

  I_LOG("Draw RTT plot...");
  // draw(points, "RTT");
}

void Udp_Client::bandwidthTest(int bw, int pkt_size, int test_time, int report_interval) {
  double point_bw_x[100];
  double point_bw_y[100];

  double point_ji_x[100];
  double point_ji_y[100];
  int point_size = 0;

  point_x_gen.store(0);

  std::vector<std::tuple<double*, double*>> vec;
  for (int i = 1; i <= test_time; i++) {
    I_LOG(" ============ Round {} ==============", i);
    
    auto res = bwRound(bw, pkt_size, 10, report_interval);
    auto x = point_x_gen.fetch_add(1);

    I_LOG("bw={}, jitter={}, x={}", res.bandwidth, res.jitterMicroSec, x);

    point_bw_x[point_size] = point_size + 1;
    point_bw_y[point_size] = res.bandwidth;

    point_ji_x[point_size] = point_size + 1;
    point_ji_y[point_size] = res.jitterMicroSec;
    point_size++;
  }

  vec.push_back(std::tie(point_bw_x, point_bw_y));
  vec.push_back(std::tie(point_ji_x, point_ji_y));
  plotMore(vec, point_size);
  // draw(point4bw, "TransferData");
}



Udp_Client::BW_Report Udp_Client::bwRound(int bw,
                                          int pkt_size,
                                          int test_time,
                                          int report_interval) {
  assert(test_time <= 100 && pkt_size >= 24 && pkt_size <= 1500);
  int bwByte = bw * 1024;
  std::vector<cv::Point> point4bw;
  I_LOG("bandwidth in byte: {}", bwByte);

  uint8_t sendBuf[CLIENT_BUFF_LEN]{0};
  uint8_t recvBuf[CLIENT_BUFF_LEN]{0};


  // send test bandwidth req
  TestReq req(TestType::bandwidth, test_time, Message::genMid());
  req.writeIntoBuf(sendBuf);
  sendMsg((char*)sendBuf, CLIENT_BUFF_LEN);
  I_LOG("send bandwidth test request, msg_id={}, test_type={}, test_time={}",
        req.get_msg_id(),
        req.test_type,
        req.test_time);
  memset(sendBuf, 0, CLIENT_BUFF_LEN);

  // wait for bandwidth test rsp
  int testId = 0;
  auto recvLen = recvMsg((char*)recvBuf, CLIENT_BUFF_LEN);
  if (recvLen > 0) {
    TestRsp rsp(recvBuf);
    I_LOG("recv test bandwidth rsp, result={}, test_id={}", rsp.result, rsp.get_test_id());
    testId = rsp.get_test_id();
    memset(recvBuf, 0, recvLen);
  } else {
    // throw std::runtime_error("test bandwidth rsp receive error. recvLen=" +
    //                         std::to_string(recvLen));
  }


  // send bandwidth test msg
  BwTestMsg test_msg(pkt_size, testId, 0, Message::genMid());
  size_t msg_len = test_msg.getLength();
  test_msg.writeData(sendBuf);


  int group_time = 5;
  uint64_t pkt_per_sec = bwByte / pkt_size;
  int pkt_per_group = std::ceil((double)pkt_per_sec * group_time / 1000);
  double pkt_interval = (double)1000 / pkt_per_sec;

  int64_t passed_time = 0;
  int total_pkt = 0;
  int64_t start_time = Tools::getCurrentTimeMillis();
  int64_t end_time = start_time + ((int64_t)test_time * 1000);
  int64_t last_time = 0;
  uint64_t total_data = 0;
  point_x_gen.store(0);

  I_LOG("bwByte={}, pkt_per_sec={}, pkt_per_group={}, pkt_interval={}",
        bwByte,
        pkt_per_sec,
        pkt_per_group,
        pkt_interval);

  while (Tools::getCurrentTimeMillis() < end_time) {
    for (int i = 0; i < pkt_per_group; i++) {
      T_LOG("send a BandwidthTestMsg, totalPkt={}", totalPkt);
      BwTestMsg::update(sendBuf, Message::genMid(), total_pkt, Tools::getCurrentTimeMicro());
      sendMsg((char*)sendBuf, msg_len);
      total_data += msg_len;
      total_pkt += 1;
    }

    passed_time = Tools::getCurrentTimeMillis() - start_time;

    if (passed_time - last_time > (int64_t)report_interval * 1000) {
      last_time = passed_time;
      // std::cout << "point: " << point_x_gen.load() << ", total_data: " << total_data
      //          << std::endl;
      point4bw.push_back(cv::Point(point_x_gen.fetch_add(50), (double)(total_data / 100000)));
      I_LOG("[ID] test time   Data transfer          Packets send");

      I_LOG("[{}]       {}s     {}            {}      ",
            testId,
            passed_time / 1000,
            formatTransfer(total_data),
            total_pkt);
    }


    int aheadTime = (total_pkt * pkt_interval) - passed_time;
    if (aheadTime > 5) {
      std::this_thread::sleep_for(std::chrono::milliseconds(aheadTime - 2));
    }
  }
  memset(sendBuf, 0, CLIENT_BUFF_LEN);

  // send bandwidth finish msg
  BwFinishMsg finishMsg(testId, total_pkt, Message::genMid());
  finishMsg.writeData(sendBuf, CLIENT_BUFF_LEN);
  sendMsg((char*)sendBuf, CLIENT_BUFF_LEN);

  // wait for report
  T_LOG("waiting report.");
  recvLen = recvMsg((char*)recvBuf, CLIENT_BUFF_LEN);
  if (recvLen > 0) {
    BandWidthReport report(recvBuf);
    I_LOG("bandwidth test report:");
    I_LOG("[ID] Interval    Transfer    Bandwidth     Jitter   Lost/Total Datagrams");

    double interval = (double)passed_time / 1000;
    int loss_pkt = total_pkt - report.getReceivedPkt();
    I_LOG("[{}] {}s       {} {}  {}ms   {}/{} ({:.{}f}%)",
          testId,
          interval,
          formatTransfer(total_data),
          formatBandwidth(total_data / interval),
          (double)report.getJitterMicroSec() / 1000,
          loss_pkt,
          total_pkt,
          (double)100 * loss_pkt / total_pkt,
          4);

    static uint32_t Mbits = 1024 * 1024;
    BW_Report a{testId,
                interval,
                formatTransfer(total_data),
                (double)(total_data / interval) * 8 / Mbits,
                (double)report.getJitterMicroSec() / 1000,
                loss_pkt,
                total_pkt,
                (double)100 * loss_pkt / total_pkt};
    return a;
  } else {
    W_LOG("TestConfirm receive error.");
    throw std::runtime_error("TestConfirm receive error. recvLen=" + std::to_string(recvLen));
  }
}

void Udp_Client::draw(std::vector<cv::Point> point, std::string name) {
  cv::Mat image = cv::Mat::zeros(600, 800, CV_8UC3);
  image.setTo(cv::Scalar(255, 255, 255));


  //将拟合点绘制到空白图上
  for (int i = 0; i < point.size(); i++) {
    cv::circle(image, point[i], 5, cv::Scalar(0, 0, 0), 3, 8, 0);
  }
  //绘制折线
  cv::polylines(image, point, false, cv::Scalar(0, 0, 0), 2, 8, 0);
  cv::Mat A;
  polynomial_curve_fit(point, 3, A);
  // std::cout << "A = " << A << std::endl;
  // std::vector<cv::Point> points_fitted;
  // for (int x = 0; x < 400; x++) {
  //  double y = A.at<double>(0, 0) + A.at<double>(1, 0) * x +
  //             A.at<double>(2, 0) * std::pow((double)x, 2) +
  //             A.at<double>(3, 0) * std::pow((double)x, 3);
  //  points_fitted.push_back(cv::Point(x, y));
  //}
  // cv::polylines(image, points_fitted, false, cv::Scalar(0, 255, 255), 1, 8, 0);
  cv::Mat img_transpose;
  flip(image, img_transpose, -1);
  cv::imshow(name, image);
  cv::waitKey(00);
}

bool Udp_Client::polynomial_curve_fit(std::vector<cv::Point>& key_point, int n, cv::Mat& A) {
  // Number of key points
  int N = key_point.size();

  //构造矩阵X
  cv::Mat X = cv::Mat::zeros(n + 1, n + 1, CV_64FC1);
  for (int i = 0; i < n + 1; i++) {
    for (int j = 0; j < n + 1; j++) {
      for (int k = 0; k < N; k++) {
        X.at<double>(i, j) = X.at<double>(i, j) + std::pow((double)key_point[k].x, i + j);
      }
    }
  }

  //构造矩阵Y
  cv::Mat Y = cv::Mat::zeros(n + 1, 1, CV_64FC1);
  for (int i = 0; i < n + 1; i++) {
    for (int k = 0; k < N; k++) {
      Y.at<double>(i, 0) =
          Y.at<double>(i, 0) + std::pow((double)key_point[k].x, i) * key_point[k].y;
    }
  }

  A = cv::Mat::zeros(n + 1, 1, CV_64FC1);
  //求解矩阵A
  cv::solve(X, Y, A, cv::DECOMP_LU);
  return true;
}