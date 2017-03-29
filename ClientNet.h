#pragma once
#include "CommonDef.h"

#pragma comment(lib, "Ws2_32.lib")

class ClientNet
{
public:
  //连接上指定服务器
  int Connect(const char* address, int port);
  //发送信息
  int SendMsg(string msg);
  //关闭
  void Close();

private:
  SOCKET m_sock;
};