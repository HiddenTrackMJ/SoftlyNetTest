/**
  @author Hidden Track
  @since 2020/05/18
  @time: 14:23
 */

#include "Server.h"


using std::cout;
using std::endl;

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
  } else {
    //此处必须赋初值，不然会导致服务器端无法正常发送
    //int len_Client = sizeof(sockaddr);
    //char recBuf[1025];
    //int len = recvfrom(serverSocket, recBuf, 1024, 0, (sockaddr*)&clientAddr, &len_Client);
    //if (len > 0) {
    //  recBuf[len] = '\0';
    //  std::cout << "Client say:" << recBuf << std::endl;
    //}
  }

  clientAddrSize = sizeof(clientAddr);


  //clientAddr.sin_family = AF_INET;



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
  int ret = recvfrom(serverSocket, msg, len, 0, (sockaddr*)&clientAddr, (socklen_t*)&clientAddrSize);
  //int ret = recvfrom(serverSocket, msg, len, 0, 0, 0);
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
 
      switch (msg.getMsgType()) {

        case MsgType::test_req: {
          TestReq req(buf);
          I_LOG("Got TestRequest, msgId={}, testType={}", req.msg_id, (int)req.test_type);
          TestRsp response(1, test_id_gen.fetch_add(1), req.msg_id, Message::genMid());
          I_LOG("Reply Msg TestConfirm, msgId={}, testType={}, rst={}", response.msg_id,
                (uint16_t)response.msg_type, response.result);
          replyMsg(response);
          break;
        }

        case MsgType::rtt_test_msg: {
          sendMsg((char*)buf, recvLen);
          D_LOG("Reply rttTestMsg, msgId={}", msg.getMsgId());
          break;
        }

        default:
          W_LOG("Got unknown msg, ignore it. msgType={}, msgId={}", msg.getMsgType(), msg.getMsgId());
          break;
      }
    } else {
      throw std::runtime_error("msg receive error.");
    }
  }

  //while (1) {
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
