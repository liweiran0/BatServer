#pragma once
#include "Task.h"
#include "CommonDef.h"

class Computer;
template<class T>
bool operator<(weak_ptr<T> a, weak_ptr<T> b)
{
  if (!a.lock())
    return false;
  if (!b.lock())
    return false;
  return a.lock().get() < b.lock().get();
}

class Manager
{
public:
  static Manager* get_instance(); 
  void initComputers(string fileName);
  void processCallback(shared_ptr<Process> process);
  void startWork();
  void addNewTask(shared_ptr<Task>);
  void telnetWork();
  void telnetCallback(string cmd, SOCKET sock);
  void registerComputer(string ip, int port, int cores);
  void processorFinishOneTask(string ip, int taskID, int processID, int processorID);
private:
  Manager();
  void addAvailableComputer(shared_ptr<Computer> computer);
  void working();
  void accelerateTaskByID(int id);
  void killTaskByID(int id);
  void setTaskAttr(int id, int cores);
  void addNewComputer(shared_ptr<Computer> computer);
  void removeComputer(string ip);
  void setComputerAttr(string ip, int cores);
  void lazySetComputerAttr(string ip, int cores);

  static Manager* instance;
  list<shared_ptr<Process>> processQueue;
  set<shared_ptr<Task>> processingTaskQueue;
  vector<TaskInfo> finishedTasks;
  list<shared_ptr<Computer>> fullWorkingComputers;
  list<shared_ptr<Computer>> idleComputers;
  list<shared_ptr<Computer>> unregisteredComputers;
  thread workThread;
  mutex queueMutex;
  condition_variable queueCv;
  mutex computerMutex;
  condition_variable computerCv;
  mutex taskMutex;
  condition_variable taskCv;
  thread telnetThread;
  bool stopFlag = false;
  bool computerChangeFlag = false;
};
