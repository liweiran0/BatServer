#include "Process.h"

Process::Process(int t_id, int p_id, shared_ptr<Task> t)
{
  taskID = t_id;
  processID = p_id;
  task = t;
}

Process::~Process()
{
}


int Process::getProcessID() const
{
  return processID;
}

int Process::getTaskID() const
{
  return taskID;
}

shared_ptr<Task> Process::getTask() const
{
  shared_ptr<Task> ret = task.lock();
  return ret;
}

string& Process::getIpAddr()
{
  return ipAddr;
}

int& Process::getProcessorIndex()
{
  return processorNumber;
}

string& Process::getRemoteAddr()
{
  return remoteAddr;
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

void Process::setCallback(decltype(callback) cb)
{
  callback = cb;
}

void Process::doCallback()
{
  if (callback)
    callback(shared_from_this());
}

void Process::reset()
{
  callback = nullptr;
  ipAddr = "";
  processorNumber = -1;
}
