#pragma once
#include "CommonDef.h"
#include "ClientNet.h"
#include <map>
class Process;
class Computer
{
private:
  string ipAddr = "";
  int fixPort = 0;
  int processorNum = 0;
  int actualProcessNum = 12;
  bool initFlag = false;
  mutex processorMutex;
  mutex processMutex;
  list<int> workingProcessor;
  list<int> idleProcessor;
  map<int,thread> workingThread;
  list<shared_ptr<Process>> doingProcesses;
  ClientNet client;
public:
  Computer();
  ~Computer();
  string &getIpAddr();
  int& getFixPort();
  int getProcessorNum() const;
  void setProcessorNum(int num);
  int getActualProcessorNum() const;
  void setActualProcessorNum(int num);
  int getIdleNum();
  void init();
  void reset();
  void startOneTask(shared_ptr<Process> process);
  void doingThread(shared_ptr<Process> process);
  void addProcess(shared_ptr<Process> process);
  void removeProcess(shared_ptr<Process> process);
  shared_ptr<Process> suspendProcess();
  void clearProcess();
  list<shared_ptr<Process>> getDoingProcesses();
  void finishProcess(int processID, int processorID);
};