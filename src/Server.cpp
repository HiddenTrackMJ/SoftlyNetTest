/**
  @author Hidden Track
  @since 2020/05/18
  @time: 14:23
 */

#include "Server.h"

using std::cout;
using std::endl;

Udp_Server::Udp_Server() {
  int ret = init_server();
  if (!ret) {
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
  if (WSAStartup(SOCKET_VERSION, &wsaData) != OK) {
    E_LOG("WSAStartup error!");
    return ERR;
  }
#endif

  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(port);

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

  I_LOG("Server on {} init successfully!", port);

  return OK;
}

void Udp_Server::sendMsg(char* msg, int len) { 
  int ret = sendto(serverSocket, msg, len, 0, (sockaddr*)&clientAddr, sizeof(clientAddr));
  if (ret == -1) {
     E_LOG("msg sent to {} error...", clientAddr.sin_addr);
  }
}

int Udp_Server::recvMsg(char* msg, int len) {
  int ret = recvfrom(serverSocket, msg, len, 0, (sockaddr*)&clientAddr,
                     (socklen_t*)sizeof(clientAddr));
 
  return ret;
}

void Udp_Server::recvThread() {
  I_LOG("recv thread is starting ...");
  char buf[BUFF_LEN];
  while (1) {
    memset(buf, 0, BUFF_LEN);
    int ret = recvMsg(buf, BUFF_LEN);
    if (ret > 0) {
      buf[ret] = '\0';
      std::cout << "recv msg: " << buf << std::endl;
    } else {
      E_LOG("recv msg from {} error...", clientAddr.sin_addr);
    }
  }
}
