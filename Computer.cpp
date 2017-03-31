#include "Computer.h"
#include "ClientNet.h"
#include "Process.h"
#include "Task.h"
#include "Manager.h"

Computer::Computer():processorMutex(),processMutex()
{
}

Computer::~Computer()
{
}

string& Computer::getIpAddr()
{
  return ipAddr;
}

int& Computer::getFixPort()
{
  return fixPort;
}


int Computer::getProcessorNum() const
{
  return processorNum;
}

void Computer::setProcessorNum(int num)
{
  if (!initFlag)
  {
    processorNum = num;
  }
  else
  {
    if (num < processorNum)
    {
      processorNum = num;
    }
    else
    {
      int newCores = min(num, actualProcessNum);
      for (auto i = 0; i < newCores - processorNum; i++)
        addProcessor();
      processorNum = newCores;
    }
  }
}

int Computer::getActualProcessorNum() const
{
  return actualProcessNum;
}

void Computer::setActualProcessorNum(int num)
{
  actualProcessNum = num;
  if (processorNum > num)
    processorNum = num;
}

int Computer::getIdleNum()
{
  lock_guard<mutex> lck(processorMutex);
  return idleProcessor.size();
}

int Computer::getWorkingNum()
{
  lock_guard<mutex> lck(processorMutex);
  return workingProcessor.size();
}

int Computer::getUnusedNum()
{
  lock_guard<mutex> lck(processorMutex);
  return unUseProcessor.size();
}

void Computer::init()
{
  if (!initFlag)
  {
    lock_guard<mutex> lck(processorMutex);
    for (auto i = 0; i < processorNum; i++)
    {
      idleProcessor.push_back(to_string(i));
    }
    if (processorNum < actualProcessNum)
    {
      for (auto i = processorNum; i < actualProcessNum; i++)
      {
        unUseProcessor.push_back(to_string(i));
      }
    }
    initFlag = true;
  }
}

void Computer::reset()
{
  clearProcesses();
  processorNum = actualProcessNum;
  initFlag = false;
  idleProcessor.clear();
  workingProcessor.clear();
  fixPort = 0;
}


void Computer::startOneTask(shared_ptr<Process> process)
{
  //copy
  //start
  unique_lock<mutex> lck(processorMutex);
  auto processor = idleProcessor.front();
  idleProcessor.pop_front();
  workingProcessor.push_back(processor);
  addProcess(process);
  process->getIpAddr() = ipAddr;
  process->getProcessorIndex() = processor;
  //cout << "start process " << process->getProcessID() << " on computer " << ipAddr << " processor " << processor <<endl;
  workingThread[processor] = thread(&Computer::doingThread, this, process);
  workingThread[processor].detach();
}

void Computer::doingThread(shared_ptr<Process> process)
{
  process->startProcess();
  //random_device rd;
  //mt19937 gen(rd());
  //uniform_int_distribution<int> distribution(30, 40);
  //int time = distribution(gen);
  //this_thread::sleep_for(chrono::seconds(time));
  ////cout << "process " << process->getProcessID() << " finished in " << time << " seconds." << endl;
  //{
  //  unique_lock<mutex> lck(processorMutex);
  //  int processor = process->getProcessorIndex();
  //  workingProcessor.remove(processor);
  //  idleProcessor.push_back(processor);
  //}
  //process->doCallback();
  process->getRemoteAddr() = "d:/alanlee/e2/";
  process->getRemoteBat() = "\\\\10.8.0.132\\AlanLee\\bat.exe";
  string cmd = "cmd=\"start\":taskid=\"";
  auto task = process->getTask();
  string taskName = "";
  if (task)
    taskName = task->getTaskName();
  cmd += process->getTaskID() + "\":taskname=\"" + taskName + "\":processid=\"" + process->getProcessID() + "\":coreid=\"" + process->getProcessorIndex() + "\":bat=\"" + process->getRemoteBat() + "\":logdir=\"" + process->getRemoteAddr() + "\"";
  //cmd="start":taskid="TaskID":taskname="TaskName":processid="ProcessID":coreid="ProcessorID":bat="LocalScriptName":logdir="RemoteLogDir"
  ClientNet client;
  client.Connect(ipAddr.c_str(), fixPort);
  client.SendMsg(cmd);
  client.Close();
}

void Computer::addProcess(shared_ptr<Process> process)
{
  lock_guard<mutex> lck(processMutex);
  doingProcesses.push_back(process);
}

void Computer::removeProcess(shared_ptr<Process> process)
{
  lock_guard<mutex> lck(processMutex);
  doingProcesses.remove(process);
}

shared_ptr<Process> Computer::suspendProcess()
{
  shared_ptr<Process> process;
  {
    lock_guard<mutex> lck(processMutex);
    process = doingProcesses.front();
    doingProcesses.pop_front();
  }
  return process;
}

void Computer::killProcess(shared_ptr<Process> process)
{
  removeProcess(process);
  string cmd = "cmd=\"kill\":taskid=\"" + process->getTaskID() + "\":processid=\"" + process->getProcessID();
  cmd += "\":coreid=\"" + process->getProcessorIndex() + "\":bat=\"" + process->getRemoteBat() + "\"";
  //cmd="kill":taskid="taskID":processid="processID":coreid="ProcessorID":bat="RemoteScriptBat"
  ClientNet client;
  client.Connect(ipAddr.c_str(), fixPort);
  client.SendMsg(cmd);
  client.Close();
}

void Computer::killTask(shared_ptr<Task> task)
{
  if (task)
  {
    for (auto process : task->getProcesses())
    {
      if (process->getIpAddr() == ipAddr)
      {
        killProcess(process);
      }
    }
  }
}

void Computer::clearProcesses()
{
  lock_guard<mutex> lck(processMutex);
  doingProcesses.clear();
}


decltype(Computer::doingProcesses) Computer::getDoingProcesses()
{
  lock_guard<mutex> lck(processMutex);
  auto ret = doingProcesses;
  return ret;
}

int Computer::finishProcess(string processID, string processorID)
{
  shared_ptr<Process> process;
  for (auto p : doingProcesses)
  {
    if (processID == p->getProcessID())
    {
      if (p->getProcessorIndex() == processorID)
      {
        cout << "process " << processID << " finished." << endl;
        process = p;
        break;
      }
    }
  }
  if (process)
  {
    {
      unique_lock<mutex> lck(processorMutex);
      if (find(workingProcessor.begin(), workingProcessor.end(), processorID) != workingProcessor.end())
      {
        workingProcessor.remove(processorID);
        idleProcessor.push_back(processorID);
        cout << "processor " << processorID << " now idle." << endl;
      }
      else
      {
        cout << "processor " << processorID << " is lazily shutdown." << endl;
        if (actualProcessNum - processorNum <= unUseProcessor.size())
        {
          //here when lazyset again
          idleProcessor.push_back(processorID);
        }
        else
        {
          unUseProcessor.push_back(processorID);
        }
      }
    }
    process->doCallback();
  }
  else
  {
    cout << "process " + processID + " is restarted." << endl;
  }
  return 0;
}

int Computer::killedProcess(string processID, string processorID)
{
  int ret = 0;
  unique_lock<mutex> lck(processorMutex);
  cout << "process " + processID + " at core " + processorID + " is killed." << endl;
  if (find(workingProcessor.begin(), workingProcessor.end(), processorID) != workingProcessor.end())
  {
    workingProcessor.remove(processorID);
    if (actualProcessNum - processorNum > unUseProcessor.size())
    {
      unUseProcessor.push_back(processorID);
    }
    else
    {
      idleProcessor.push_back(processorID);
      cout << "processor " << processorID << " now idle." << endl;
      if (idleProcessor.size() == 1)
        ret = 1;
    }
  }
  return ret;
}

void Computer::removeIdleProcessor()
{
  lock_guard<mutex> lck(processorMutex);
  if (idleProcessor.size() > 0)
  {
    auto processor = idleProcessor.front();
    idleProcessor.pop_front();
    unUseProcessor.push_back(processor);
  }
}


void Computer::removeWorkingProcessor(string processorId)
{
  lock_guard<mutex> lck(processorMutex);
  auto iter = find(workingProcessor.begin(), workingProcessor.end(), processorId);
  if (iter != workingProcessor.end())
  {
    workingProcessor.remove(processorId);
    unUseProcessor.push_back(processorId);
  }
}

void Computer::lazyRemoveProcessor(int num)
{
  lock_guard<mutex> lck(processorMutex);
  assert(num <= workingProcessor.size());
  for (auto i = 0; i < num; i++)
  {
    workingProcessor.pop_front();
  }
}


void Computer::addProcessor()
{
  unique_lock<mutex> lck(processorMutex);
  if (unUseProcessor.size() > 0)
  {
    auto processor = unUseProcessor.front();
    unUseProcessor.pop_front();
    idleProcessor.push_back(processor);
  }
}
