#include "Process.h"

static int usedProcessId = 0;

Process::Process(decltype(taskID) t_id, shared_ptr<Task> t)
{
  taskID = t_id;
  processID = to_string(++usedProcessId);
  task = t;
}

Process::~Process()
{
}


auto Process::getProcessID() const ->decltype(processID)
{
  return processID;
}

auto Process::getTaskID() const ->decltype(taskID)
{
  return taskID;
}

shared_ptr<Task> Process::getTask() const
{
  shared_ptr<Task> ret = task.lock();
  return ret;
}

auto Process::getIpAddr() ->decltype(ipAddr)&
{
  return ipAddr;
}

auto Process::getProcessorIndex()->decltype(processorNumber)&
{
  return processorNumber;
}

decltype(Process::remoteExe)& Process::getRemoteBat()
{
  return remoteExe;
}

decltype(Process::localDir)& Process::getLocalDir()
{
  return localDir;
}


void Process::startProcess()
{
  startTime = chrono::system_clock::now();
}


auto Process::getStartTime() const
{
  return startTime;
}

auto Process::getRunningTime()
{
  return chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - startTime);
}

void Process::setCallback(decltype(callback) cb, decltype(callback2) cb2)
{
  callback = cb;
  callback2 = cb2;
}

void Process::doCallback()
{
  if (callback)
    callback(shared_from_this());
}

void Process::doCallback2()
{
  if (callback2)
    callback2(shared_from_this());
}

void Process::reset()
{
  callback = nullptr;
  ipAddr = "";
  processorNumber = -1;
}
