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
  //manager->registerComputer("10.8.0.132", 2000, 1);
  //manager->registerComputer("10.8.0.131", 2000, 2);
  manager->startWork();
  //for (int i = 0; i < 10;i ++)
  //{
  //  string task_name = "task_" + to_string(i);
  //  shared_ptr<Task> task(new Task());
  //  task->getTaskOwner() = "alanlee";
  //  task->getTaskName() = task_name;
  //  task->getProcessNumbers() = 50;
  //  task->getTotalCores() = 10;
  //  if (task->getTotalCores() > 40)
  //    task->getTotalCores() = 40;
  //  for (int j = 0; j < task->getProcessNumbers(); j++)
  //  {
  //    shared_ptr<Process> process(new Process(task->getTaskID(), task));
  //    task->getProcesses().push_back(process);
  //  }
  //  manager->addNewTask(task);
  //}
  
  ServerNet mainServer;
  //string localIP = getLocalIpAddress();
 // if (localIP == "")
    //localIP = "127.0.0.1";
  string localIP = "";
  mainServer.init(localIP.c_str(), port);
  mainServer.setCallback(bind(&Manager::workerWorkDistribute, manager, placeholders::_1, placeholders::_2));
  mainServer.run();
}
