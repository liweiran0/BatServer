#include "Computer.h"
#include "ClientNet.h"
#include "Process.h"
#include "Task.h"

void getFiles(string path, vector<string>& files);

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

string& Computer::getNetDir()
{
  return netDir;
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


void Computer::startOneTask(shared_ptr<Process> process, function<void()> cb)
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
  workingThread[processor] = thread(&Computer::doingThread, this, process, cb);
  workingThread[processor].detach();
}

void Computer::doingThread(shared_ptr<Process> process, function<void()> cb)
{
  //send file
  auto task = process->getTask();
  string taskName = "";
  if (task)
    taskName = task->getTaskName();
  string reletiveDir = task->getReletiveDir();
  string sys_cmd = string("md ") + netDir + reletiveDir;
  //cout << sys_cmd << endl;
  system(sys_cmd.c_str());
  string from_dir = task->getWorkDir() + process->getLocalDir();
  if (from_dir[from_dir.length() - 1] == '\\')
  {
    from_dir = from_dir.substr(0, from_dir.length() - 1);
  }
  vector<string> files;
  getFiles(from_dir, files);
  getFiles(task->getWorkDir() + "programs", files);
  //generate filelist.txt
  string fileName = from_dir + "\\filelist.txt";
  FILE* fpFileList = fopen(fileName.c_str(), "w");
  if (!fpFileList)
  {
    string processor = process->getProcessorIndex();
    workingProcessor.remove(processor);
    idleProcessor.push_back(processor);
    removeProcess(process);
    process->getIpAddr() = "";
    process->getProcessorIndex() = "";
    cb();
    return;
  }
  else
  {
    for (auto file : files)
    {
      fprintf(fpFileList, "%s\n", file.c_str());
    }
    fclose(fpFileList);
  }
  sys_cmd = string("xcopy /q /y /e /i ") + from_dir + " " + netDir + reletiveDir + process->getLocalDir();
  //cout << sys_cmd << endl;
  system(sys_cmd.c_str());
  sys_cmd = string("xcopy /q /y /e /i ") + task->getWorkDir() + "programs " + netDir + reletiveDir + process->getLocalDir();
  //cout << sys_cmd << endl;
  system(sys_cmd.c_str());
  process->startProcess();
  string cmd = "cmd=\"start\":taskid=\"";
  
  cmd += process->getTaskID() + "\":taskname=\"" + taskName + "\":processid=\"" + process->getProcessID() + "\":coreid=\"" + process->getProcessorIndex() + "\":bat=\"" + process->getRemoteBat() + "\":logdir=\"" + reletiveDir + process->getLocalDir() + "\"";
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

int Computer::removeProcess(shared_ptr<Process> process)
{
  int ret = 0;
  lock_guard<mutex> lck(processMutex);
  if (find(doingProcesses.begin(), doingProcesses.end(), process) != doingProcesses.end())
  {
    doingProcesses.remove(process);
    ret = 1;
  }
  return ret;
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
  if (removeProcess(process))
  {
    string cmd = "cmd=\"kill\":taskid=\"" + process->getTaskID() + "\":processid=\"" + process->getProcessID();
    cmd += "\":coreid=\"" + process->getProcessorIndex() + "\":bat=\"" + process->getRemoteBat() + "\"";
    //cmd="kill":taskid="taskID":processid="processID":coreid="ProcessorID":bat="RemoteScriptBat"
    ClientNet client;
    client.Connect(ipAddr.c_str(), fixPort);
    client.SendMsg(cmd);
    client.Close();
    cout << "killed" << endl;
  }
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

void getFiles(string path, vector<string>& files)
{
  //文件句柄  
  long   hFile;
  //文件信息  
  struct _finddata_t fileinfo;
  string p;
  if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1)
  {
    do
    {
      //如果是目录,迭代之  
      //如果不是,加入列表  
      if ((fileinfo.attrib &  _A_SUBDIR))
      {
        if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
          getFiles(p.assign(path).append("\\").append(fileinfo.name), files);
      }
      else
      {
        files.push_back(fileinfo.name);
      }
    } while (_findnext(hFile, &fileinfo) == 0);
    _findclose(hFile);
  }
}