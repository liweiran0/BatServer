#include "Task.h"
#include "Manager.h"
#include "Process.h"
#include "ServerNet.h"

void mainCallback(string cmd, SOCKET sock);

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
  //manager->registerComputer("10.8.0.132", 2000, 1);
  //manager->registerComputer("10.8.0.131", 2000, 2);
  manager->startWork();
  for (int i = 0; i < 10;i ++)
  {
    shared_ptr<Task> task(new Task(i));
    task->getTaskOwner() = "alanlee";
    task->getTaskName() = to_string(i);
    task->getProcessNumbers() = 50;
    task->getTotalCores() = 10;
    if (task->getTotalCores() > 40)
      task->getTotalCores() = 40;
    for (int j = 0; j < task->getProcessNumbers(); j++)
    {
      shared_ptr<Process> process(new Process(i, i * 100 + j, task));
      task->getProcesses().push_back(process);
    }
    manager->addNewTask(task);
  }
  
  ServerNet mainServer;
  string localIP = getLocalIpAddress();
 // if (localIP == "")
    localIP = "127.0.0.1";
  mainServer.init(localIP.c_str(), port);
  mainServer.setCallback(bind(mainCallback, placeholders::_1, placeholders::_2));
  mainServer.run();
}

void mainCallback(string cmd, SOCKET sock)
{
  string ret;
  if (cmd.find("register") == 0)
  {
    //cmd: register:IPAddr:port:CoreNumber
    cmd = cmd.substr(cmd.find_first_of(":") + 1);
    string ip = cmd.substr(0, cmd.find_first_of(":"));
    cmd = cmd.substr(cmd.find_first_of(":") + 1);
    int port = stoi(cmd.substr(0, cmd.find_first_of(":")));
    cmd = cmd.substr(cmd.find_first_of(":") + 1);
    int cores = stoi(cmd);
    Manager::get_instance()->registerComputer(ip, port, cores);
    ret = "OK";
  }
  else if (cmd.find("finish") == 0)
  {
    //cmd: finish:IPAddr:taskID:processID:processorID
    cmd = cmd.substr(cmd.find_first_of(":") + 1);
    string ip = cmd.substr(0, cmd.find_first_of(":"));
    cmd = cmd.substr(cmd.find_first_of(":") + 1);
    int tid = stoi(cmd.substr(0, cmd.find_first_of(":")));
    cmd = cmd.substr(cmd.find_first_of(":") + 1);
    int pid = stoi(cmd.substr(0, cmd.find_first_of(":")));
    cmd = cmd.substr(cmd.find_first_of(":") + 1);
    int processorID = stoi(cmd);
    Manager::get_instance()->processorFinishOneTask(ip, tid, pid, processorID);
    ret = "OK";
  }
  send(sock, ret.c_str(), ret.length(), 0);
}