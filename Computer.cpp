#include "Computer.h"
#include "Process.h"
#include "Task.h"

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
    processorNum = min(num, actualProcessNum);
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

void Computer::init()
{
  if (!initFlag)
  {
    lock_guard<mutex> lck(processorMutex);
    for (auto i = 0; i < processorNum; i++)
    {
      idleProcessor.push_back(i);
    }
    initFlag = true;
  }
}

void Computer::reset()
{
  clearProcess();
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
  int processor = idleProcessor.front();
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
  string cmd = "start:";
  auto task = process->getTask();
  string taskName = "";
  if (task)
    taskName = task->getTaskName();
  cmd += to_string(process->getTaskID()) + ":" + taskName + ":" + to_string(process->getProcessID()) + ":" + to_string(process->getProcessorIndex()) + ":" + process->getRemoteAddr();
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
  lock_guard<mutex> lck(processMutex);
  shared_ptr<Process> process = doingProcesses.front();
  doingProcesses.pop_front();
  return process;
}


void Computer::clearProcess()
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

void Computer::finishProcess(int processID, int processorID)
{
  cout << "process " << processID << " finished." << endl;
  {
    unique_lock<mutex> lck(processorMutex);
    workingProcessor.remove(processorID);
    idleProcessor.push_back(processorID);
  }
  for (auto process : doingProcesses)
  {
    if (processID == process->getProcessID())
    {
      process->doCallback();
      break;
    }
  }
}
