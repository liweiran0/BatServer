#include "ClientNet.h"


int ClientNet::Connect(const char* address, int port)
{
  int rlt = 0;
  int iErrMsg;
  WSAData wsaData;
  iErrMsg = WSAStartup(MAKEWORD(1, 1), &wsaData);
  if (iErrMsg != NO_ERROR)
  {
    printf("failed with wsaStartup error : %d\n", iErrMsg);
    rlt = 1;
    return rlt;
  }
  m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (m_sock == INVALID_SOCKET)
  {
    printf("socket failed with error : %d\n", WSAGetLastError());
    rlt = 2;
    return rlt;
  }
  sockaddr_in sockaddrServer;
  sockaddrServer.sin_family = AF_INET;
  sockaddrServer.sin_port = htons(port);
  sockaddrServer.sin_addr.s_addr = inet_addr(address);
  iErrMsg = connect(m_sock, (sockaddr*) &sockaddrServer, sizeof(sockaddrServer));
  if (iErrMsg < 0)
  {
    printf("connect failed with error : %d\n", iErrMsg);
    rlt = 3;
    return rlt;
  }
  return rlt;
}

int ClientNet::SendMsg(string msg)
{
  int rlt = 0;
  int iErrMsg;
  iErrMsg = send(m_sock, msg.c_str(), msg.length(), 0);
  cout << "send msg :" << msg << endl;
  if (iErrMsg < 0)
  {
    printf("send msg failed with error : %d\n", iErrMsg);
    rlt = -1;
    return rlt;
  }
  return rlt;
}

void ClientNet::Close()
{
  closesocket(m_sock);
}