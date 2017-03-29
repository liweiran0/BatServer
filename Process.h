#pragma once
#include "CommonDef.h"
class Task;

class Process : public enable_shared_from_this<Process>
{
private:
  int taskID;           // taskID
  int processID;        // unique processID
  chrono::system_clock::time_point startTime; // process start time
  string ipAddr;        // ip address
  int processorNumber = 0;  // processor number
  string remoteAddr;    // remote file address
  weak_ptr<Task> task;  // task ptr
  function<void(shared_ptr<Process>)> callback;
public:
  Process(int t_id, int p_id, shared_ptr<Task> task);
  ~Process();
  int getTaskID()const;
  int getProcessID()const;
  shared_ptr<Task> getTask()const;
  string& getIpAddr();
  int & getProcessorIndex();
  string& getRemoteAddr();
  void startProcess();
  auto getStartTime() const;
  auto getRunningTime();
  void setCallback(decltype(callback) cb);
  void doCallback();
  void reset();
};