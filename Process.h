#pragma once
#include "CommonDef.h"
class Task;

class Process : public enable_shared_from_this<Process>
{
private:
  string taskID;           // taskID
  string processID;        // unique processID
  chrono::system_clock::time_point startTime; // process start time
  string ipAddr;        // ip address
  string processorNumber = "";  // processor number
  string remoteAddr;    // remote file address
  string remoteExe;     // remote bat file
  string localDir;      // local directory
  weak_ptr<Task> task;  // task ptr
  function<void(shared_ptr<Process>)> callback;
public:
  Process(decltype(taskID) t_id, shared_ptr<Task> task);
  ~Process();
  decltype(taskID) getTaskID()const;
  decltype(processID) getProcessID()const;
  shared_ptr<Task> getTask()const;
  decltype(ipAddr)& getIpAddr();
  decltype(processorNumber) & getProcessorIndex();
  decltype(remoteAddr)& getRemoteAddr();
  decltype(remoteExe)& getRemoteBat();
  decltype(localDir)& getLocalDir();
  void startProcess();
  auto getStartTime() const;
  auto getRunningTime();
  void setCallback(decltype(callback) cb);
  void doCallback();
  void reset();
};