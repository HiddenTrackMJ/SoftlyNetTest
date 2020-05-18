/**
  @author Hidden Track
  @since 2020/05/18
  @time: 19:07
 */

#include "Client.h"

using std::cout;
using std::endl;

Udp_Client::Udp_Client() {
  int ret = init_client();
  if (!ret) {
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

  clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_HOPOPTS);

  I_LOG("Client on {} init successfully!", port);

  return OK;
}

void Udp_Client::sendMsg(char* msg, int len) {
  int ret = sendto(clientSocket, msg, len, 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
  if (ret == -1) {
    E_LOG("msg sent to {} error...", serverAddr.sin_addr);
  }
}

int Udp_Client::recvMsg(char* msg, int len) {
  int ret = recvfrom(clientSocket, msg, len, 0, (sockaddr*)&serverAddr,
                     (socklen_t*)sizeof(serverAddr));

  return ret;
}

void Udp_Client::recvThread() {
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
