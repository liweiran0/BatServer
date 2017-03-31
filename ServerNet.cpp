#include "ServerNet.h"
#include <iostream>

bool GetAddressBySocket(SOCKET m_socket)
{
  SOCKADDR_IN m_address;
  memset(&m_address, 0, sizeof(m_address));
  int nAddrLen = sizeof(m_address);
  //�����׽��ֻ�ȡ��ַ��Ϣ
  if (::getpeername(m_socket, (SOCKADDR*) &m_address, &nAddrLen) != 0)
  {
    printf("Get IP address by socket failed!n");
    return false;
  }
  //��ȡIP��Port
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
  // �������ڼ������׽��� 
  if ((ListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
  {
    cout << "WSASocket()   failed   with   error  " << WSAGetLastError() << endl;
    return -1;
  }
  // ���ü�����ַ�Ͷ˿ں�
  InternetAddr.sin_family = AF_INET;
  InternetAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
  InternetAddr.sin_port = htons(port);
  // �󶨼����׽��ֵ����ص�ַ�Ͷ˿�
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
  // ����Ϊ������ģʽ
  ULONG NonBlock = 1;
  if (ioctlsocket(ListenSocket, FIONBIO, &NonBlock) == SOCKET_ERROR)
  {
    cout << "ioctlsocket() failed with error " << WSAGetLastError() << endl;
    return;
  }
  CreateSocketInformation(ListenSocket);// ΪListenSocket�׽��ִ�����Ӧ��SOCKET_INFORMATION,��ListenSocket��ӵ�SocketArray������
  while (true)
  {
    FD_ZERO(&ReadSet);// ׼����������I/O֪ͨ�Ķ�/д�׽��ּ���
    FD_ZERO(&WriteSet);
    FD_SET(ListenSocket, &ReadSet);// ��ReadSet��������Ӽ����׽���ListenSocket
   // ��SocketArray�����е������׽�����ӵ�WriteSet��ReadSet������,SocketArray�����б����ż����׽��ֺ�������ͻ��˽���ͨ�ŵ��׽���
   // �����Ϳ���ʹ��select()�ж��ĸ��׽����н������ݻ��߶�ȡ/д������
    for (DWORD i = 0; i < TotalSockets; i++)
    {
      LPSOCKET_INFORMATION SocketInfo = SocketArray[i];
      FD_SET(SocketInfo->Socket, &ReadSet);//��˵����socket�ж����������������ǿͻ��˷����
      FD_SET(SocketInfo->Socket, &WriteSet);//��˵����socket��д������
    }
    // �ж϶�/д�׽��ּ����о������׽���    
    if ((Total = select(0, &ReadSet, &WriteSet, NULL, NULL)) == SOCKET_ERROR)//��NULL���βδ���Timeout����������ʱ��ṹ�����ǽ�select��������״̬��һ���ȵ������ļ�������������ĳ���ļ������������仯Ϊֹ.��������ͣ������ȴ��ͻ�����Ӧ
    {
      //cout << "select()   returned   with   error   " << WSAGetLastError() << endl;
      return;
    }
    // ���δ��������׽��֡�����������һ����Ӧ�������������ӿͻ����յ����ַ����ٷ��ص��ͻ��ˡ�
    for (DWORD i = 0; i < TotalSockets; i++)
    {
      LPSOCKET_INFORMATION SocketInfo = SocketArray[i];            // SocketInfoΪ��ǰҪ������׽�����Ϣ
                                                                   // �жϵ�ǰ�׽��ֵĿɶ��ԣ����Ƿ��н��������������߿��Խ�������
      if (FD_ISSET(SocketInfo->Socket, &ReadSet))
      {
        if (SocketInfo->Socket == ListenSocket)        // ���ڼ����׽�����˵���ɶ���ʾ���µ���������
        {
          Total--;    // �������׽��ּ�1
                      // �����������󣬵õ���ͻ��˽���ͨ�ŵ��׽���AcceptSocket
          if ((AcceptSocket = accept(ListenSocket, NULL, NULL)) != INVALID_SOCKET)
          {
            // �����׽���AcceptSocketΪ������ģʽ
            // �����������ڵ���WSASend()������������ʱ�Ͳ��ᱻ����
            NonBlock = 1;
            if (ioctlsocket(AcceptSocket, FIONBIO, &NonBlock) == SOCKET_ERROR)
            {
              cout << "ioctlsocket()   failed   with   error   " << WSAGetLastError() << endl;
              return;
            }
            // �����׽�����Ϣ����ʼ��LPSOCKET_INFORMATION�ṹ�����ݣ���AcceptSocket��ӵ�SocketArray������
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
        else   // ��������
        {
          Total--;                // ����һ�����ھ���״̬���׽���
          memset(SocketInfo->Buffer, ' ', DATA_BUFSIZE);            // ��ʼ��������
          SocketInfo->DataBuf.buf = SocketInfo->Buffer;            // ��ʼ��������λ��
          SocketInfo->DataBuf.len = DATA_BUFSIZE;                // ��ʼ������������
                                                                 // ��������
          DWORD  Flags = 0;
          if (WSARecv(SocketInfo->Socket, &(SocketInfo->DataBuf), 1, &RecvBytes, &Flags, NULL, NULL) == SOCKET_ERROR)
          {
            // ����������WSAEWOULDBLOCK��ʾ��û�����ݣ������ʾ�����쳣
            if (WSAGetLastError() != WSAEWOULDBLOCK)
            {
              //cout << "WSARecv()   failed   with   error  " << WSAGetLastError() << endl;
              FreeSocketInformation(i);        // �ͷ��׽�����Ϣ
            }
            continue;
          }
          else   // ��������
          {
            SocketInfo->BytesRECV = RecvBytes;        // ��¼�������ݵ��ֽ���
            SocketInfo->DataBuf.buf[RecvBytes] = '\0';
            if (RecvBytes == 0)                                    // ������յ�0���ֽڣ����ʾ�Է��ر�����
            {
              FreeSocketInformation(i);
              continue;
            }
            else
            {
              SocketInfo->cmd_str += SocketInfo->DataBuf.buf;
              string cmd = getCommandFromString(SocketInfo->cmd_str);
              //cout << SocketInfo->DataBuf.buf << endl;// ����ɹ��������ݣ����ӡ�յ�������
              if (cmd != "" && callback)
                callback(cmd, SocketInfo->Socket);
            }
          }
        }
      }
      else
      {
        // �����ǰ�׽�����WriteSet�����У���������׽��ֵ��ڲ����ݻ������������ݿ��Է���
        if (FD_ISSET(SocketInfo->Socket, &WriteSet))
        {
          Total--;            // ����һ�����ھ���״̬���׽���
          SocketInfo->DataBuf.buf = SocketInfo->Buffer + SocketInfo->BytesSEND;            // ��ʼ��������λ��
          SocketInfo->DataBuf.len = SocketInfo->BytesRECV - SocketInfo->BytesSEND;    // ��ʼ������������
          if (SocketInfo->DataBuf.len > 0)        // �������Ҫ���͵����ݣ���������
          {
            if (WSASend(SocketInfo->Socket, &(SocketInfo->DataBuf), 1, &SendBytes, 0, NULL, NULL) == SOCKET_ERROR)
            {
              // ����������WSAEWOULDBLOCK��ʾ��û�����ݣ������ʾ�����쳣
              if (WSAGetLastError() != WSAEWOULDBLOCK)
              {
                //cout << "WSASend()   failed   with   error   " << WSAGetLastError() << endl;
                FreeSocketInformation(i);        // �ͷ��׽�����Ϣ
              }
              continue;
            }
            else
            {
              SocketInfo->BytesSEND += SendBytes;        // ��¼�������ݵ��ֽ���
                                                         // ����ӿͻ��˽��յ������ݶ��Ѿ����ص��ͻ��ˣ��򽫷��ͺͽ��յ��ֽ���������Ϊ0
              if (SocketInfo->BytesSEND == SocketInfo->BytesRECV)
              {
                SocketInfo->BytesSEND = 0;
                SocketInfo->BytesRECV = 0;
              }
            }
          }
        }
      }    // ���ListenSocketδ���������ҷ��صĴ�����WSAEWOULDBLOCK���ô����ʾû�н��յ��������󣩣�������쳣
    }
  }
}

void ServerNet::setCallback(decltype(callback) cb)
{
  callback = cb;
}


BOOL   ServerNet::CreateSocketInformation(SOCKET   s)
{
  LPSOCKET_INFORMATION   SI;   // ���ڱ����׽��ֵ���Ϣ       
                            //  printf("Accepted   socket   number   %d\n",   s); // ���ѽ��ܵ��׽��ֱ��
                             // ΪSI�����ڴ�ռ�
  if ((SI = (LPSOCKET_INFORMATION) GlobalAlloc(GPTR, sizeof(SOCKET_INFORMATION))) == NULL)
  {
    printf("GlobalAlloc()   failed   with   error   %d\n", GetLastError());
    return   FALSE;
  }
  // ��ʼ��SI��ֵ    
  SI->Socket = s;
  SI->BytesSEND = 0;
  SI->BytesRECV = 0;
  // ��SocketArray����������һ����Ԫ�أ����ڱ���SI���� 
  SocketArray[TotalSockets] = SI;
  TotalSockets++;                        // �����׽�������
  return(TRUE);
}

// ������SocketArray��ɾ��ָ����LPSOCKET_INFORMATION����
void   ServerNet::FreeSocketInformation(DWORD   Index)
{
  LPSOCKET_INFORMATION SI = SocketArray[Index];    // ��ȡָ��������Ӧ��LPSOCKET_INFORMATION����
  DWORD   i;
  closesocket(SI->Socket);       // �ر��׽���
  GlobalFree(SI);   // �ͷ�ָ��LPSOCKET_INFORMATION������Դ
                    // ��������index���������Ԫ��ǰ��
  if (Index != (TotalSockets - 1))
  {
    for (i = Index; i < TotalSockets; i++)
    {
      SocketArray[i] = SocketArray[i + 1];
    }
  }
  TotalSockets--;        // �׽���������1
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

