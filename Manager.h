#pragma once
#include "Task.h"
#include "CommonDef.h"
#include "ThreadPool.h"

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
  void workerWorkDistribute(string cmd, SOCKET sock);
  void registerComputer(string ip, int port, int cores, string netDir);
  void selectComputerToCallback(string cmd, string ip, string taskID, string processID, string processorID);
private:
  Manager();
  void addAvailableComputer(shared_ptr<Computer> computer);
  void working();
  void accelerateTaskByID(string id);
  void killTaskByID(string id);
  void setTaskAttr(string id, int cores);
  void addNewComputer(shared_ptr<Computer> computer);
  void removeComputer(string ip);
  void setComputerAttr(string ip, int cores);
  void lazySetComputerAttr(string ip, int cores);
  void addTaskFromTelnet(string taskName, string owner, string type, string logName, string cores, string dir1, string dir2, string cb);
  shared_ptr<Computer> getComputerByIP(string ip);
  void parseCommand(string cmd, map<string, string>& param);
  void telnetThreadSelect(string cmd, SOCKET sock);
  void telnetCallback(string cmd, SOCKET sock);
  void workerCallback(string cmd, SOCKET sock);
  void reassignProcess(string tid, string pid);

  static Manager* instance;
  list<shared_ptr<Process>> processQueue;
  set<shared_ptr<Task>> processingTaskQueue;
  vector<TaskInfo> finishedTasks;
  list<shared_ptr<Computer>> fullWorkingComputers;
  list<shared_ptr<Computer>> idleComputers;
  list<shared_ptr<Computer>> unregisteredComputers;
  map<string, int> logNameCores;
  map<string, int> logNameUsingCores;
  mutex queueMutex;
  condition_variable queueCv;
  mutex computerMutex;
  condition_variable computerCv;
  mutex taskMutex;
  condition_variable taskCv;
  mutex logMutex;
  thread telnetThread;
  thread workThread;
  ThreadPool telnetThreadPool;
  ThreadPool workingThreadPool;

  bool stopFlag = false;
  bool computerChangeFlag = false;
};

