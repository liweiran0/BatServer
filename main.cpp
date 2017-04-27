#include "Task.h"
#include "Manager.h"
#include "Process.h"
#include "ServerNet.h"

void main(int argv, char* argc[])
{
  string fileName("computers.cfg");
  int port = 30000;
  if (argv == 2)
  {
    port = stoi(string(argc[1]));
  }
  if (argv == 3)
  {
    fileName = argc[1];
    port = stoi(string(argc[2]));
  }
  Manager * manager = Manager::get_instance();
  manager->initComputers(fileName);
  manager->startWork();
  
  ServerNet mainServer;
  //string localIP = getLocalIpAddress();
 // if (localIP == "")
    //localIP = "127.0.0.1";
  string localIP = "";
  mainServer.init(localIP.c_str(), port);
  mainServer.setCallback(bind(&Manager::workerWorkDistribute, manager, placeholders::_1, placeholders::_2));
  mainServer.run();
}
