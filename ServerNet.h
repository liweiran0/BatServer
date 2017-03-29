#pragma once
#include "CommonDef.h"
#pragma comment (lib,"ws2_32.lib")
using namespace std;

class ServerNet
{
private:
  SOCKET m_sock;
  function<void(string, SOCKET)> callback;
public:
  //初始化服务器,返回0表示成功
  int init(const char* address, int port);
  //更新数据
  void run();
  void setCallback(decltype(callback) cb);
};

string getLocalIpAddress();
short getUnusedPort(short start_port);