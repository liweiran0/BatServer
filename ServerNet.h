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
  //��ʼ��������,����0��ʾ�ɹ�
  int init(const char* address, int port);
  //��������
  void run();
  void setCallback(decltype(callback) cb);
};

string getLocalIpAddress();
short getUnusedPort(short start_port);