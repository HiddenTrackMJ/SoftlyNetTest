/**
  @author Hidden Track
  @since 2020/05/18
  @time: 19:07
 */

#include "Client.h"




using std::cout;
using std::endl;

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
  } else  {
    //发送信息与服务器建立连接(必须加)
    //sendto(clientSocket, "hello server", strlen("hello server"), 0, (sockaddr*)&serverAddr,
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
      //E_LOG("recv msg from 2 error...");
    }
  }
}

void Udp_Client::rttTest(int testTimes, int packetSize) {
  uint8_t recvBuf[CLIENT_BUFF_LEN]{0};

  assert(packetSize >= 24);

  TestReq req(TestType::rtt, 0, Message::genMid());

  uint8_t Buf[CLIENT_BUFF_LEN]{0};
  req.writeHead(Buf);

  sendMsg((char*)Buf, CLIENT_BUFF_LEN);
  I_LOG("send TestRequest, msgId={} testType={}", req.msg_id, req.test_type);
  memset(Buf, 0, CLIENT_BUFF_LEN);

  auto testId = 0;
  auto recvLen = recvMsg((char*)recvBuf, CLIENT_BUFF_LEN);
  if (recvLen > 0) {
    TestRsp rsp(recvBuf);
    I_LOG("receive TestConfirm, result={} reMsgId={} testId={}",
          rsp.result,
          rsp.reMsgId,
          req.test_id);
    testId = rsp.test_id;
    memset(recvBuf, 0, recvLen);
  } else {
    throw std::runtime_error("TestConfirm receive error. recvLen=" + std::to_string(recvLen));
  }


  while (testTimes > 0) {
    testTimes--;
    RttTestMsg msg(packetSize, testId, Message::genMid());
    msg.writeHead(Buf);
    sendMsg((char*)Buf, CLIENT_BUFF_LEN);
    T_LOG("send RttTestMsg, msgId={} testId={} time={}", msg.msgId, msg.testId, msg.timestamp);
    memset(Buf, 0, CLIENT_BUFF_LEN);

    recvLen = recvMsg((char*)recvBuf, CLIENT_BUFF_LEN);
    if (recvLen > 0) {
      RttTestMsg rttResponse(recvBuf);
      
      auto diffTime = Tools::getCurrentTimeMicro() - rttResponse.timestamp;
      I_LOG("receive RttTestMsg, msgId={} testId={} time={} diff={}ms",
            rttResponse.msg_id,
            rttResponse.test_id,
            rttResponse.timestamp,
            (double)diffTime / 1000);

      points.push_back(cv::Point(point_x_gen.fetch_add(1), diffTime));

      memset(recvBuf, 0, recvLen);
    } else {
      throw std::runtime_error("RttTestMsg receive error. recvLen=" + std::to_string(recvLen));
    }
  }

  draw();
}

void Udp_Client::draw() {
  cv::Mat image = cv::Mat::zeros(480, 640, CV_8UC3);
  image.setTo(cv::Scalar(255, 255, 255));


  //将拟合点绘制到空白图上
  for (int i = 0; i < points.size(); i++) {
    cv::circle(image, points[i], 5, cv::Scalar(0, 0, 0), 3, 8, 0);
  }
  //绘制折线
  cv::polylines(image, points, false, cv::Scalar(0, 0, 0), 2, 8, 0);
  cv::Mat A;
  polynomial_curve_fit(points, 3, A);
  std::cout << "A = " << A << std::endl;
  std::vector<cv::Point> points_fitted;
  for (int x = 0; x < 400; x++) {
    double y = A.at<double>(0, 0) + A.at<double>(1, 0) * x +
               A.at<double>(2, 0) * std::pow((double)x, 2) +
               A.at<double>(3, 0) * std::pow((double)x, 3);
    points_fitted.push_back(cv::Point(x, y));
  }
  cv::polylines(image, points_fitted, false, cv::Scalar(0, 255, 255), 1, 8, 0);
  cv::imshow("RTT", image);
  cv::waitKey(0);
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