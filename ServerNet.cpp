#include "ServerNet.h"
#include <iostream>

int ServerNet::init(const char* address, int port)
{
  int rlt = 0;
  int iErrorMsg;
  WSAData wsaData;
  iErrorMsg = WSAStartup(MAKEWORD(1, 1), &wsaData);
  if (iErrorMsg != NO_ERROR)
  {
    printf("wsastartup failed with error : %d\n", iErrorMsg);
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
  sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(port);
  serverAddr.sin_addr.s_addr = inet_addr(address);
  iErrorMsg = ::bind(m_sock, (sockaddr*) &serverAddr, sizeof(serverAddr));
  if (iErrorMsg < 0)
  {
    printf("bind failed with error : %d\n", iErrorMsg);
    rlt = 3;
    return rlt;
  }
  return rlt;
}

void ServerNet::run()
{
  int err = listen(m_sock, 5);
  if (SOCKET_ERROR == err)
  {
    cout << "listen failed!" << endl;
    closesocket(m_sock);  
    WSACleanup();         
    return;
  }
  sockaddr_in tcpAddr;
  int len = sizeof(sockaddr);
  SOCKET newSocket;
  char buf[1024];
  int rval;
  do
  {
    newSocket = accept(m_sock, (sockaddr*) &tcpAddr, &len);
    if (newSocket == INVALID_SOCKET)
    {
    }
    else
    {
      printf("new socket connect : %d\n", newSocket);
      do
      {
        memset(buf, 0, sizeof(buf));
        rval = recv(newSocket, buf, 1024, 0);
        if (rval == SOCKET_ERROR)
        {
          printf("recv socket error\n");
          break;
        }
        if (rval == 0)
          printf("ending connection\n");
        else
        {
          //printf("recv %s\n", buf);
          string cmd(buf);
          cmd = cmd.substr(0, cmd.find_first_of("\r\n"));
          if (cmd == "quit")
            break;
          if (callback)
            callback(cmd, newSocket);
        }
      } while (rval != 0);
      closesocket(newSocket);
    }
  } while (true);
  closesocket(m_sock);
}

void ServerNet::setCallback(decltype(callback) cb)
{
  callback = cb;
}

string getLocalIpAddress()
{
  WORD wVersionRequested = MAKEWORD(2, 2);
  WSADATA wsaData;
  if (WSAStartup(wVersionRequested, &wsaData) != 0)
    return "";
  char local[255] = {0};
  gethostname(local, sizeof(local));
  hostent* ph = gethostbyname(local);
  if (ph == nullptr)
    return "";
  in_addr addr;
  memcpy(&addr, ph->h_addr_list[0], sizeof(in_addr)); // 这里仅获取第一个ip  
  string localIP;
  localIP.assign(inet_ntoa(addr));
  WSACleanup();
  return localIP;
}

short getUnusedPort(short start_port)
{
  int sock;
  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  short port;
  int flag = 0;
  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  for (port = start_port; port < 65536; port++)
  {
    addr.sin_port = htons(port);
    if (::bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) != 0)
    {
      flag = 0;
      if ((port % 2) == 0)
        port++;
      continue;
    }
    else
    {
      if ((++flag) == 2)
      {
        closesocket(sock);
        return port - 1;
      }
      else
      {
        closesocket(sock);
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
      }
    }
  }
  return 0;
}