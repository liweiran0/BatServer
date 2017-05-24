#pragma once
#include "CommonDef.h"

class Process;
class Task;
class Computer
{
private:
  string ipAddr = "";
  int fixPort = 0;
  string netDir = "";
  int processorNum = 0;
  int actualProcessNum = 12;
  bool initFlag = false;
  mutex processorMutex;
  mutex processMutex;
  list<string> workingProcessor;
  list<string> idleProcessor;
  list<string> unUseProcessor;
  map<string, thread> workingThread;
  list<shared_ptr<Process>> doingProcesses;
public:
  Computer();
  ~Computer();
  string &getIpAddr();
  int& getFixPort();
  string &getNetDir();
  int getProcessorNum() const;
  void setProcessorNum(int num);
  int getActualProcessorNum() const;
  void setActualProcessorNum(int num);
  int getIdleNum();
  int getWorkingNum();
  int getUnusedNum();
  void init();
  void reset();
  void startOneTask(shared_ptr<Process> process, function<void()> cb);
  void doingThread(shared_ptr<Process> process, function<void()> cb);
  void addProcess(shared_ptr<Process> process);
  int removeProcess(shared_ptr<Process> process);
  shared_ptr<Process> suspendProcess();
  void killProcess(shared_ptr<Process> process);
  int killTask(shared_ptr<Task> task);
  void clearProcesses();
  list<shared_ptr<Process>> getDoingProcesses();
  int finishProcess(string processID, string processorID);
  int killedProcess(string processID, string processorID);
  int failedProcess(string processID, string processorID);
  void removeIdleProcessor();
  void removeWorkingProcessor(string processorId);
  void lazyRemoveProcessor(int num);
  void addProcessor();
};