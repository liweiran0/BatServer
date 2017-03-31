#include "ServerNet.h"
#include <iostream>

bool GetAddressBySocket(SOCKET m_socket)
{
  SOCKADDR_IN m_address;
  memset(&m_address, 0, sizeof(m_address));
  int nAddrLen = sizeof(m_address);
  //根据套接字获取地址信息
  if (::getpeername(m_socket, (SOCKADDR*) &m_address, &nAddrLen) != 0)
  {
    printf("Get IP address by socket failed!n");
    return false;
  }
  //读取IP和Port
  char ipDec[32];
  inet_ntop(AF_INET, &m_address.sin_addr, ipDec, 32);
  cout << "IP: " << ipDec << "  PORT: " << ntohs(m_address.sin_port) << endl;
  return true;
}

int ServerNet::init(const char* address, int port)
{
  if ((Ret = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0)
  {
    cout << "WSAStartup()   failed   with   error   " << Ret << endl;
    WSACleanup();
    return -1;
  }
  // 创建用于监听的套接字 
  if ((ListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
  {
    cout << "WSASocket()   failed   with   error  " << WSAGetLastError() << endl;
    return -1;
  }
  // 设置监听地址和端口号
  InternetAddr.sin_family = AF_INET;
  InternetAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
  InternetAddr.sin_port = htons(port);
  // 绑定监听套接字到本地地址和端口
  if (::bind(ListenSocket, (PSOCKADDR) &InternetAddr, sizeof(InternetAddr)) == SOCKET_ERROR)
  {
    cout << "bind()   failed   with   error  " << WSAGetLastError() << endl;
    return -1;
  }
  return 0;
}

void ServerNet::run()
{
  if (listen(ListenSocket, 5))
  {
    cout << "listen()   failed   with   error   " << WSAGetLastError() << endl;
    return;
  }
  // 设置为非阻塞模式
  ULONG NonBlock = 1;
  if (ioctlsocket(ListenSocket, FIONBIO, &NonBlock) == SOCKET_ERROR)
  {
    cout << "ioctlsocket() failed with error " << WSAGetLastError() << endl;
    return;
  }
  CreateSocketInformation(ListenSocket);// 为ListenSocket套接字创建对应的SOCKET_INFORMATION,把ListenSocket添加到SocketArray数组中
  while (true)
  {
    FD_ZERO(&ReadSet);// 准备用于网络I/O通知的读/写套接字集合
    FD_ZERO(&WriteSet);
    FD_SET(ListenSocket, &ReadSet);// 向ReadSet集合中添加监听套接字ListenSocket
   // 将SocketArray数组中的所有套接字添加到WriteSet和ReadSet集合中,SocketArray数组中保存着监听套接字和所有与客户端进行通信的套接字
   // 这样就可以使用select()判断哪个套接字有接入数据或者读取/写入数据
    for (DWORD i = 0; i < TotalSockets; i++)
    {
      LPSOCKET_INFORMATION SocketInfo = SocketArray[i];
      FD_SET(SocketInfo->Socket, &ReadSet);//这说明该socket有读操作。而读操作是客户端发起的
      FD_SET(SocketInfo->Socket, &WriteSet);//这说明该socket有写操作。
    }
    // 判断读/写套接字集合中就绪的套接字    
    if ((Total = select(0, &ReadSet, &WriteSet, NULL, NULL)) == SOCKET_ERROR)//将NULL以形参传入Timeout，即不传入时间结构，就是将select置于阻塞状态，一定等到监视文件描述符集合中某个文件描述符发生变化为止.服务器会停到这里等待客户端相应
    {
      //cout << "select()   returned   with   error   " << WSAGetLastError() << endl;
      return;
    }
    // 依次处理所有套接字。本服务器是一个回应服务器，即将从客户端收到的字符串再发回到客户端。
    for (DWORD i = 0; i < TotalSockets; i++)
    {
      LPSOCKET_INFORMATION SocketInfo = SocketArray[i];            // SocketInfo为当前要处理的套接字信息
                                                                   // 判断当前套接字的可读性，即是否有接入的连接请求或者可以接收数据
      if (FD_ISSET(SocketInfo->Socket, &ReadSet))
      {
        if (SocketInfo->Socket == ListenSocket)        // 对于监听套接字来说，可读表示有新的连接请求
        {
          Total--;    // 就绪的套接字减1
                      // 接受连接请求，得到与客户端进行通信的套接字AcceptSocket
          if ((AcceptSocket = accept(ListenSocket, NULL, NULL)) != INVALID_SOCKET)
          {
            // 设置套接字AcceptSocket为非阻塞模式
            // 这样服务器在调用WSASend()函数发送数据时就不会被阻塞
            NonBlock = 1;
            if (ioctlsocket(AcceptSocket, FIONBIO, &NonBlock) == SOCKET_ERROR)
            {
              cout << "ioctlsocket()   failed   with   error   " << WSAGetLastError() << endl;
              return;
            }
            // 创建套接字信息，初始化LPSOCKET_INFORMATION结构体数据，将AcceptSocket添加到SocketArray数组中
            //GetAddressBySocket(AcceptSocket);
            if (CreateSocketInformation(AcceptSocket) == FALSE)
              return;
          }
          else
          {
            if (WSAGetLastError() != WSAEWOULDBLOCK)
            {
              cout << "accept()   failed   with   error   " << WSAGetLastError() << endl;
              return;
            }
          }
        }
        else   // 接收数据
        {
          Total--;                // 减少一个处于就绪状态的套接字
          memset(SocketInfo->Buffer, ' ', DATA_BUFSIZE);            // 初始化缓冲区
          SocketInfo->DataBuf.buf = SocketInfo->Buffer;            // 初始化缓冲区位置
          SocketInfo->DataBuf.len = DATA_BUFSIZE;                // 初始化缓冲区长度
                                                                 // 接收数据
          DWORD  Flags = 0;
          if (WSARecv(SocketInfo->Socket, &(SocketInfo->DataBuf), 1, &RecvBytes, &Flags, NULL, NULL) == SOCKET_ERROR)
          {
            // 错误编码等于WSAEWOULDBLOCK表示暂没有数据，否则表示出现异常
            if (WSAGetLastError() != WSAEWOULDBLOCK)
            {
              //cout << "WSARecv()   failed   with   error  " << WSAGetLastError() << endl;
              FreeSocketInformation(i);        // 释放套接字信息
            }
            continue;
          }
          else   // 接收数据
          {
            SocketInfo->BytesRECV = RecvBytes;        // 记录接收数据的字节数
            SocketInfo->DataBuf.buf[RecvBytes] = '\0';
            if (RecvBytes == 0)                                    // 如果接收到0个字节，则表示对方关闭连接
            {
              FreeSocketInformation(i);
              continue;
            }
            else
            {
              SocketInfo->cmd_str += SocketInfo->DataBuf.buf;
              string cmd = getCommandFromString(SocketInfo->cmd_str);
              //cout << SocketInfo->DataBuf.buf << endl;// 如果成功接收数据，则打印收到的数据
              if (cmd != "" && callback)
                callback(cmd, SocketInfo->Socket);
            }
          }
        }
      }
      else
      {
        // 如果当前套接字在WriteSet集合中，则表明该套接字的内部数据缓冲区中有数据可以发送
        if (FD_ISSET(SocketInfo->Socket, &WriteSet))
        {
          Total--;            // 减少一个处于就绪状态的套接字
          SocketInfo->DataBuf.buf = SocketInfo->Buffer + SocketInfo->BytesSEND;            // 初始化缓冲区位置
          SocketInfo->DataBuf.len = SocketInfo->BytesRECV - SocketInfo->BytesSEND;    // 初始化缓冲区长度
          if (SocketInfo->DataBuf.len > 0)        // 如果有需要发送的数据，则发送数据
          {
            if (WSASend(SocketInfo->Socket, &(SocketInfo->DataBuf), 1, &SendBytes, 0, NULL, NULL) == SOCKET_ERROR)
            {
              // 错误编码等于WSAEWOULDBLOCK表示暂没有数据，否则表示出现异常
              if (WSAGetLastError() != WSAEWOULDBLOCK)
              {
                //cout << "WSASend()   failed   with   error   " << WSAGetLastError() << endl;
                FreeSocketInformation(i);        // 释放套接字信息
              }
              continue;
            }
            else
            {
              SocketInfo->BytesSEND += SendBytes;        // 记录发送数据的字节数
                                                         // 如果从客户端接收到的数据都已经发回到客户端，则将发送和接收的字节数量设置为0
              if (SocketInfo->BytesSEND == SocketInfo->BytesRECV)
              {
                SocketInfo->BytesSEND = 0;
                SocketInfo->BytesRECV = 0;
              }
            }
          }
        }
      }    // 如果ListenSocket未就绪，并且返回的错误不是WSAEWOULDBLOCK（该错误表示没有接收的连接请求），则出现异常
    }
  }
}

void ServerNet::setCallback(decltype(callback) cb)
{
  callback = cb;
}


BOOL   ServerNet::CreateSocketInformation(SOCKET   s)
{
  LPSOCKET_INFORMATION   SI;   // 用于保存套接字的信息       
                            //  printf("Accepted   socket   number   %d\n",   s); // 打开已接受的套接字编号
                             // 为SI分配内存空间
  if ((SI = (LPSOCKET_INFORMATION) GlobalAlloc(GPTR, sizeof(SOCKET_INFORMATION))) == NULL)
  {
    printf("GlobalAlloc()   failed   with   error   %d\n", GetLastError());
    return   FALSE;
  }
  // 初始化SI的值    
  SI->Socket = s;
  SI->BytesSEND = 0;
  SI->BytesRECV = 0;
  // 在SocketArray数组中增加一个新元素，用于保存SI对象 
  SocketArray[TotalSockets] = SI;
  TotalSockets++;                        // 增加套接字数量
  return(TRUE);
}

// 从数组SocketArray中删除指定的LPSOCKET_INFORMATION对象
void   ServerNet::FreeSocketInformation(DWORD   Index)
{
  LPSOCKET_INFORMATION SI = SocketArray[Index];    // 获取指定索引对应的LPSOCKET_INFORMATION对象
  DWORD   i;
  closesocket(SI->Socket);       // 关闭套接字
  GlobalFree(SI);   // 释放指定LPSOCKET_INFORMATION对象资源
                    // 将数组中index索引后面的元素前移
  if (Index != (TotalSockets - 1))
  {
    for (i = Index; i < TotalSockets; i++)
    {
      SocketArray[i] = SocketArray[i + 1];
    }
  }
  TotalSockets--;        // 套接字总数减1
}


string getLocalIpAddress()
{
  WSADATA wsaData;
  int iResult;
  DWORD dwRetval;
  int i = 1;
  struct addrinfo *result = NULL;
  struct addrinfo *ptr;
  struct addrinfo hints;
  struct sockaddr_in* sockaddr_ipv4;
  iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != 0)
    return "";
  ZeroMemory(&hints, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  char local[255] = {0};
  gethostname(local, sizeof(local));
  string ip = "";
  dwRetval = getaddrinfo(local, 0, &hints, &result);
  if (dwRetval != 0)
  {
  }
  else
  {
    bool foundIp = false;
    for (ptr = result; ptr != NULL && !foundIp; ptr = ptr->ai_next)
    {
      switch (ptr->ai_family)
      {
        case AF_UNSPEC:
          break;
        case AF_INET:
          sockaddr_ipv4 = (struct sockaddr_in *) ptr->ai_addr;
          char ipDec[32];
          inet_ntop(AF_INET, &sockaddr_ipv4->sin_addr, ipDec, 32);
          ip = ipDec;
          foundIp = true;
          break;
        case AF_INET6:
          break;
        case AF_NETBIOS:
          break;
        default:
          break;
      }
    }
  }
  freeaddrinfo(result);
  WSACleanup();
  return ip;
}

short getUnusedPort(short start_port)
{
  int sock;
  int iErrorMsg;
  WSAData wsaData;
  iErrorMsg = WSAStartup(MAKEWORD(1, 1), &wsaData);
  if (iErrorMsg != NO_ERROR)
  {
    printf("wsastartup failed with error : %d\n", iErrorMsg);
    return -1;
  }
  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  short port;
  int flag = 0;
  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  for (port = start_port; port < 65536; port++)
  {
    addr.sin_port = htons(port);
    if (::bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) >= 0)
    {
      flag = 0;
      if ((port % 2) == 0)
        port++;
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

string getCommandFromString(string& str)
{
  string ret = "";
  auto brace = str.find_first_of("{");
  if (brace != string::npos)
  {
    str = str.substr(brace);
    auto backbrace = str.find_first_of("}");
    if (backbrace != string::npos)
    {
      ret = str.substr(1, backbrace - 1);
      str = str.substr(backbrace + 1);
    }
  }
  else
  {
    str = "";
  }
  return ret;
}

