#include "Manager.h"
#include "Computer.h"
#include "Process.h"
#include "ServerNet.h"

once_flag init_flag;
Manager * Manager::instance = nullptr;

Manager::Manager():queueMutex(),queueCv(),computerMutex(),computerCv(),taskMutex(),taskCv()
{
  instance = nullptr;
}

Manager* Manager::get_instance()
{
  call_once(init_flag, []()
  {
    instance = new Manager();
  });
  return instance;
}

void Manager::initComputers(string fileName)
{
  FILE *fp = fopen(fileName.c_str(), "r");
  if (fp)
  {
    char buffer[1024];
    while (!feof(fp))
    {
      fgets(buffer, 1023, fp);
      string tmp(buffer);
      stringstream ss(tmp);
      string ipAddr;
      int coresNumber;
      ss >> ipAddr >> coresNumber;
      if (ipAddr.find("rem") != string::npos)
        continue;
      if (ipAddr.find("//") != string::npos)
        continue;
      shared_ptr<Computer> computer(new Computer);
      computer->getIpAddr() = ipAddr;
      computer->setProcessorNum(coresNumber);
      unique_lock<mutex> lck(computerMutex);
      unregisteredComputers.push_back(computer);
    }
  }
}

void Manager::startWork()
{
  workThread = thread(&Manager::working, this);
  telnetThread = thread(&Manager::telnetWork, this);
}

void Manager::working()
{
  while (!stopFlag)
  {
    bool waitFlag = false;
    shared_ptr<Computer> computer;
    {
      unique_lock<mutex> lck(computerMutex);
      while (idleComputers.empty())
        computerCv.wait(lck);
      if (stopFlag)
        break;
      computer = idleComputers.front();
    }
    if (computer)
    {
      unique_lock<mutex> lck(queueMutex);
      shared_ptr<Process> process;
      while (processQueue.empty())
        queueCv.wait(lck);
      if (stopFlag)
        break;
      if (computerChangeFlag)
      {
        computerChangeFlag = false;
        continue;
      }
      auto iter = processQueue.begin();
      while (iter!= processQueue.end() && !process)
      {
        process = *iter;
        auto task = process->getTask();
        lock_guard<mutex> lckTask(taskMutex);
        if (task && task->getProcessingNumber() < task->getTotalCores())
        {
          processQueue.erase(iter);
          break;
        }
        else
        {
          ++iter;
          process.reset();
        }
      }
      if (process)
      {
        {
          lock_guard<mutex> lckTask(taskMutex);
          auto task = process->getTask();
          task->getProcessingNumber()++;
        }
        process->setCallback(bind(&Manager::processCallback, this, placeholders::_1));
        computer->startOneTask(process);
        int idleProcessorNum = computer->getIdleNum();
        if (idleProcessorNum == 0)
        {
          idleComputers.pop_front();
          fullWorkingComputers.push_back(computer);
        }
      }
    }
    
    if (waitFlag)
    {
      unique_lock<mutex> lck(taskMutex);
      taskCv.wait(lck);
    }
  }
}

void Manager::addAvailableComputer(shared_ptr<Computer> computer)
{
  idleComputers.push_back(computer);
  fullWorkingComputers.remove(computer);
  computerCv.notify_one();
  //cout << "computer " << computer->getIpAddr() << " now available" << endl;
}

void Manager::addNewComputer(shared_ptr<Computer> computer)
{
  unique_lock<mutex> lck(computerMutex);
  idleComputers.push_back(computer);
  computerCv.notify_one();
  //cout << "new computer " << computer->getIpAddr() << " now available" << endl;
}

void Manager::processCallback(shared_ptr<Process> process)
{
  shared_ptr<Task> task = process->getTask();
  if (task)
  {
    unique_lock<mutex> lck(taskMutex);
    task->getFinishedNumber()++;
    task->getProcessingNumber()--;
    if (task->getFinishedNumber() == task->getProcessNumbers())
    {
      task->doCallback();
      TaskInfo info(task->getTaskID(), task->getTaskName(), task->getTaskOwner(), task->getTaskType());
      info.getTaskPath() = task->getFileAddress();
      finishedTasks.emplace_back(move(info));
      processingTaskQueue.erase(task);
    }
    taskCv.notify_one();
  }
  string ipAddr = process->getIpAddr();
  unique_lock<mutex> lck(computerMutex);
  for (auto computer : fullWorkingComputers)
  {
    if (computer->getIpAddr() == ipAddr)
    {
      computer->removeProcess(process);
      if (computer->getIdleNum() != 0)
        addAvailableComputer(computer);
      break;
    }
  }
}


void Manager::addNewTask(shared_ptr<Task> task)
{
  {
    lock_guard<mutex> lck(taskMutex);
    processingTaskQueue.insert(task);
  }
  {
    unique_lock<mutex> lck(queueMutex);
    for (auto process: task->getProcesses())
    {
      processQueue.push_back(process);
    }
    if (!processQueue.empty())
      queueCv.notify_one();
  }
}

void Manager::telnetWork()
{
  ServerNet server;
  //string localIP = getLocalIpAddress();
  //if (localIP == "")
  //  localIP = "127.0.0.1";
  string localIP = "";
  server.init(localIP.c_str(), 20000);
  server.setCallback(bind(&Manager::telnetCallback, this, placeholders::_1, placeholders::_2));
  server.run();
}

void Manager::telnetCallback(string cmd, SOCKET sock)
{
  cmd = cmd.substr(0, cmd.find_first_of("\r\n"));
  string ret("");
  if (cmd == "info")
  {
    ret += "==========================================================\r\n";
    {
      lock_guard<mutex> lck(taskMutex);
      ret += "Finished task number:" + to_string(finishedTasks.size()) + "\r\n";
      ret += "Processing task number:" + to_string(processingTaskQueue.size()) + "\r\n";
    }
    {
      lock_guard<mutex> lck(queueMutex);
      ret += "Processes in queue:" + to_string(processQueue.size()) + "\r\n";
    }
    {
      lock_guard<mutex> lck(computerMutex);
      ret += "Full working computer number:" + to_string(fullWorkingComputers.size()) + "\r\n";
      ret += "Idle computer number:" + to_string(idleComputers.size()) + "\r\n";
      ret += "Unregistered computer number:" + to_string(unregisteredComputers.size()) + "\r\n";
    }
  }
  else if (cmd == "task")
  {
    ret += "==========================================================\r\n";
    {
      lock_guard<mutex> lck(taskMutex);
      if (finishedTasks.size() > 0)
      {
        int count = 0;
        ret += "Last finished tasks" + string("\r\n");
        for (auto iter = finishedTasks.end(); count < min(5, (int)finishedTasks.size()); count++)
        {
          --iter;
          time_t time = chrono::system_clock::to_time_t((*iter).getTaskFinishedTime());
          stringstream ss;
          ss << put_time(localtime(&time), "%F %T");
          string timeConvert = ss.str();
          ret += "  id:" + (*iter).getTaskID() + "  owner:" + (*iter).getTaskOwner() + "  name:" + (*iter).getTaskName() + "\r\n"
            + "    finished time:" + timeConvert + "\r\n";
        }
      }
      if (processingTaskQueue.size() > 0)
        ret += "TaskID\tTotal\tDone\tDoing\tTaskOwner\tTaskName\r\n";
      for (auto task : processingTaskQueue)
      {
        ret += task->getTaskID() + "\t" + to_string(task->getProcessNumbers()) + "\t"
          + to_string(task->getFinishedNumber()) + "\t" + to_string(task->getProcessingNumber()) + "\t"
          + task->getTaskOwner() + "\t\t" + task->getTaskName() + "\r\n";
      }
    }
  }
  else if (cmd == "process")
  {
    ret += "==========================================================\r\n";
    {
      ret += "Total " + to_string(processQueue.size()) + " processes in queue." + "\r\n";
      lock_guard<mutex> lck(queueMutex);
      if (processQueue.size() > 0)
      {
        auto process = processQueue.front();
        ret += string("The first process id:") + process->getProcessID() + " belongs to taskID:" + process->getTaskID() + "\r\n";
      }
    }
  }
  else if (cmd == "computer")
  {
    ret += "==========================================================\r\n";
    {
      ret += "ComputerIP\tCores\tInuse\tUnused\tIdle\tWorking\r\n";
      lock_guard<mutex> lck(computerMutex);
      list<shared_ptr<Computer>> computerList;
      for (auto computer: fullWorkingComputers)
        computerList.push_back(computer);
      for (auto computer : idleComputers)
        computerList.push_back(computer);
      for (auto computer : computerList)
      {
        ret += computer->getIpAddr() + "\t" + to_string(computer->getActualProcessorNum()) + "\t"
          + to_string(computer->getProcessorNum()) + "\t" + to_string(computer->getUnusedNum()) + "\t"
          + to_string(computer->getIdleNum()) + "\t" + to_string(computer->getWorkingNum()) + "\r\n";
      }

      if (unregisteredComputers.size() > 0)
      {
        ret += "Unregistered computers\r\n";
        for (auto computer : unregisteredComputers)
        {
          ret += computer->getIpAddr() + "\r\n";
        }
      }
    }
  }
  else if (cmd.find("acctask") == 0)
  {
    stringstream ss(cmd);
    string taskID = "";
    string tmp;
    ss >> tmp >> taskID;
    accelerateTaskByID(taskID);
  }
  else if (cmd.find("killtask") == 0)
  {
    stringstream ss(cmd);
    string taskID = "";
    string tmp;
    ss >> tmp >> taskID;
    killTaskByID(taskID);
  }
  else if (cmd.find("settask") == 0)
  {
    stringstream ss(cmd);
    string taskID = "";
    int newCores = 0;
    string tmp;
    ss >> tmp >> taskID >> newCores;
    setTaskAttr(taskID, newCores);
  }
  else if (cmd.find("removecomputer") == 0)
  {
    stringstream ss(cmd);
    string tmp;
    string ip;
    ss >> tmp >> ip;
    removeComputer(ip);
  }
  else if (cmd.find("setcomputer") == 0)
  {
    stringstream ss(cmd);
    string tmp;
    string ip;
    int cores;
    ss >> tmp >> ip >> cores;
    setComputerAttr(ip, cores);
  }
  else if (cmd.find("lazysetcomputer") == 0)
  {
    stringstream ss(cmd);
    string tmp;
    string ip;
    int cores;
    ss >> tmp >> ip >> cores;
    lazySetComputerAttr(ip, cores);
  }
  if (ret != "")
    send(sock, ret.c_str(), ret.length(), 0);
}

void Manager::workerCallback(string cmd, SOCKET sock)
{
  string ret;
  map<string, string> param;
  parseCommand(cmd, param);
  if (param["cmd"] == "register")
  {
    //cmd="register":ip="IPAddr":port="port":corenum="CoreNumber"
    Manager::get_instance()->registerComputer(param["ip"], stoi(param["port"]), stoi(param["corenum"]));
    ret = "OK";
  }
  else if (param["cmd"] == "finish")
  {
    //cmd="finish":ip="IPAddr":taskid="taskID":processid="processID":coreid="processorID"
    Manager::get_instance()->selectComputerToCallback("finish", param["ip"], param["taskid"], param["processid"], param["coreid"]);
    ret = "OK";
  }
  else if (param["cmd"] == "killed")
  {
    //cmd="kill":ip="IPAddr":taskid="taskID":processid="processID":coreid="processorID"
    Manager::get_instance()->selectComputerToCallback("killed", param["ip"], param["taskid"], param["processid"], param["coreid"]);
    ret = "OK";
  }
  send(sock, ret.c_str(), ret.length(), 0);
}


void Manager::accelerateTaskByID(string id)
{
  bool foundTask = false; 
  {
    lock_guard<mutex> lck(taskMutex);
    for (auto task : processingTaskQueue)
    {
      if (task->getTaskID() == id)
      {
        foundTask = true;
        break;
      }
    }
  }
  if (foundTask)
  {
    decltype(processQueue) tmpList;
    unique_lock<mutex> lck(queueMutex);
    for (auto iter = processQueue.begin(); iter != processQueue.end();)
    {
      shared_ptr<Process> proc = *iter;
      ++iter;
      if (proc && proc->getTaskID() == id)
      {
        processQueue.remove(proc);
        tmpList.push_back(proc);
      }
    }
    processQueue.splice(processQueue.begin(), tmpList);
  }
}


void Manager::killTaskByID(string id)
{
  shared_ptr<Task> taskToBeKilled;
  {
    lock_guard<mutex> lck(taskMutex);
    for (auto task : processingTaskQueue)
    {
      if (task->getTaskID() == id)
      {
        taskToBeKilled = task;
        processingTaskQueue.erase(task);
        break;
      }
    }
  }
  if (taskToBeKilled)
  {
    //remove processes that not have been started
    {
      unique_lock<mutex> lck(queueMutex);
      for (auto iter = processQueue.begin(); iter != processQueue.end();)
      {
        shared_ptr<Process> proc = *iter;
        ++iter;
        if (proc && proc->getTaskID() == id)
        {
          processQueue.remove(proc);
        }
      }
    }
    //kill the processes that have already been started
    {
      lock_guard<mutex> lck(computerMutex);
      for (auto computer : idleComputers)
      {
        computer->killTask(taskToBeKilled);
      }
      for (auto computer: fullWorkingComputers)
      {
        computer->killTask(taskToBeKilled);
      }
    }
  }
}

void Manager::setTaskAttr(string id, int cores)
{
  lock_guard<mutex> lck(taskMutex);
  for (auto task : processingTaskQueue)
  {
    if (task->getTaskID() == id)
    {
      task->getTotalCores() = cores;
      return;
    }
  }
}

void Manager::registerComputer(string ip, int port, int cores)
{
  shared_ptr<Computer> computer;
  unique_lock<mutex> lck(computerMutex);
  for (auto c : unregisteredComputers)
  {
    if (c->getIpAddr() == ip)
    {
      computer = c;
      unregisteredComputers.remove(c);
      break;
    }
  }
  if (!computer)
  {
    computer.reset(new Computer);
    computer->getIpAddr() = ip;
    computer->setProcessorNum(cores);
  }
  computer->getFixPort() = port;
  computer->setActualProcessorNum(cores);
  computer->init();
  idleComputers.push_back(computer);
  computerCv.notify_one();
  cout << "register a computer :" << ip << ":" << port << ", cores:" << cores << endl;
}

void Manager::removeComputer(string ip)
{
  {
    lock_guard<mutex> lck(queueMutex);
    computerChangeFlag = true;
  }
  shared_ptr<Computer> computer;
  list<shared_ptr<Process>> processes;
  {
    lock_guard<mutex> lck(computerMutex);
    for (auto c : idleComputers)
    {
      if (c->getIpAddr() == ip)
      {
        computer = c;
        idleComputers.remove(c);
      }
      break;
    }
    if (!computer)
    {
      for (auto c : fullWorkingComputers)
      {
        if (c->getIpAddr() == ip)
        {
          computer = c;
          fullWorkingComputers.remove(c);
        }
        break;
      }
    }
    if (computer)
    {
      processes = computer->getDoingProcesses();
      computer->reset();
      unregisteredComputers.push_back(computer);
    }
  }
  {
    auto count = processes.size();
    for (auto process : processes)
    {
      process->reset();
      lock_guard<mutex> lck(taskMutex);
      shared_ptr<Task> task = process->getTask();
      if (task)
        task->getProcessingNumber()--;
    }
    unique_lock<mutex> lck(queueMutex);
    processQueue.splice(processQueue.begin(), processes);
    if (count != 0)
      queueCv.notify_one();
  }
}

void Manager::setComputerAttr(string ip, int cores)
{
  shared_ptr<Computer> computer;
  {
    unique_lock<mutex> lck(computerMutex);
    for (auto c : idleComputers)
    {
      if (c->getIpAddr() == ip)
      {
        computer = c;
        break;
      }
    }
    if (computer)
    {
      //  when set idle computer
      cores = min(cores, computer->getActualProcessorNum());
      if (cores < computer->getProcessorNum())
      {
        //reduce cores
        if (cores < computer->getWorkingNum())
        {
          //the computer will not be idle
          idleComputers.remove(computer);
          fullWorkingComputers.push_back(computer);
          list<shared_ptr<Process>> processes;
          for (auto i = 0; i < computer->getWorkingNum() - cores; i++)
          {
            //need to restart such process
            auto process = computer->suspendProcess();
            computer->removeWorkingProcessor(process->getProcessorIndex());
            process->reset();
            processes.push_back(process);
            lock_guard<mutex> lckTask(taskMutex);
            shared_ptr<Task> task = process->getTask();
            if (task)
              task->getProcessingNumber()--;
          }
          int idleNum = computer->getIdleNum();
          for (auto i = 0; i < idleNum; i++)
            computer->removeIdleProcessor();
          unique_lock<mutex> lckQueue(queueMutex);
          processQueue.splice(processQueue.begin(), processes);
          computerChangeFlag = true;
          queueCv.notify_one();
        }
      }
      computer->setProcessorNum(cores);
    }
    else
    {
      for (auto c : fullWorkingComputers)
      {
        if (c->getIpAddr() == ip)
        {
          computer = c;
          break;
        }
      }
      if (computer)
      {
        cores = min(cores, computer->getActualProcessorNum());
        if (cores > computer->getProcessorNum())
        {
          // set the computer be idle and not full working
          computer->setProcessorNum(cores);
          if (computer->getIdleNum() > 0)
          {
            fullWorkingComputers.remove(computer);
            idleComputers.push_back(computer);
            computerCv.notify_one();
          }
        }
        else
        {
          list<shared_ptr<Process>> processes;
          for (auto i = 0; i < computer->getProcessorNum() - cores; i++)
          {
            //need to restart such process
            auto process = computer->suspendProcess();
            computer->removeWorkingProcessor(process->getProcessorIndex());
            process->reset();
            processes.push_back(process);
            lock_guard<mutex> lckTask(taskMutex);
            shared_ptr<Task> task = process->getTask();
            if (task)
              task->getProcessingNumber()--;
          }
          unique_lock<mutex> lckQueue(queueMutex);
          processQueue.splice(processQueue.begin(), processes);
          computerChangeFlag = true;
          queueCv.notify_one();
          computer->setProcessorNum(cores);
        }
      }
    }
  }
}

void Manager::lazySetComputerAttr(string ip, int cores)
{
  shared_ptr<Computer> computer;
  {
    unique_lock<mutex> lck(computerMutex);
    for (auto c : idleComputers)
    {
      if (c->getIpAddr() == ip)
      {
        computer = c;
        break;
      }
    }
    if (computer)
    {
      //  when lazy set idle computer
      cores = min(cores, computer->getActualProcessorNum());
      if (cores < computer->getProcessorNum())
      {
        if (cores <= computer->getWorkingNum())
        {
          //the computer will not be idle
          idleComputers.remove(computer);
          fullWorkingComputers.push_back(computer);
          //remove idle processors
          int idleNum = computer->getIdleNum();
          for (auto i = 0; i < idleNum; i++)
            computer->removeIdleProcessor();
          computer->lazyRemoveProcessor(computer->getWorkingNum() - cores);
          lock_guard<mutex> lckQueue(queueMutex);
          computerChangeFlag = true;
        }
        else
        {
          computer->setProcessorNum(cores);
        }
      }
      else
      {
        computer->setProcessorNum(cores);
      }
    }
    else
    {
      for (auto c : fullWorkingComputers)
      {
        if (c->getIpAddr() == ip)
        {
          computer = c;
          break;
        }
      }
      if (computer)
      {
        cores = min(cores, computer->getActualProcessorNum());
        if (cores > computer->getWorkingNum())
        {
          // set the computer be idle and not full working
          computer->setProcessorNum(cores);
          if (computer->getIdleNum() > 0)
          {
            fullWorkingComputers.remove(computer);
            idleComputers.push_back(computer);
            computerCv.notify_one();
          }
        }
        else
        {
          assert(computer->getWorkingNum() == computer->getProcessorNum());
          computer->lazyRemoveProcessor(computer->getProcessorNum() - cores);
          computer->setProcessorNum(cores);
        }
      }
    }
  }
}


void Manager::selectComputerToCallback(std::string cmd, std::string ip, std::string taskID, std::string processID, std::string processorID)
{
  shared_ptr<Computer> computer;
  {
    lock_guard<mutex> lck(computerMutex);
    for (auto c : idleComputers)
    {
      if (c->getIpAddr() == ip)
      {
        computer = c;
        break;
      }
    }
    if (!computer)
    {
      for (auto c : fullWorkingComputers)
      {
        if (c->getIpAddr() == ip)
        {
          computer = c;
          break;
        }
      }
    }
  }
  if (computer)
  {
    int ret;
    if (cmd == "finish")
      ret = computer->finishProcess(processID, processorID);
    if (cmd == "killed")
    {
      ret = computer->killedProcess(processID, processorID);
      if (ret == 1)
      {
        unique_lock<mutex> lck(computerMutex);
        addAvailableComputer(computer);
      }
    }
  }
}

shared_ptr<Computer> Manager::getComputerByIP(string ip)
{
  shared_ptr<Computer> computer;
  lock_guard<mutex> lck(computerMutex);
  for (auto c : idleComputers)
  {
    if (c->getIpAddr() == ip)
    {
      computer = c;
      break;
    }
  }
  if (!computer)
  {
    for (auto c : fullWorkingComputers)
    {
      if (c->getIpAddr() == ip)
      {
        computer = c;
        break;
      }
    }
  }
  return computer;
}


void Manager::parseCommand(string cmd, map<string, string>& param)
{
  while (cmd != "")
  {
    auto firstLoc = cmd.find_first_not_of("=:\"");
    auto lastLoc = cmd.find_first_of("=");
    if (firstLoc != string::npos && lastLoc != string::npos)
    {
      auto len = lastLoc - firstLoc;
      string Key = cmd.substr(firstLoc, len);
      cmd = cmd.substr(lastLoc + 2);
      lastLoc = cmd.find_first_of("\"");
      string Value = "";
      if (lastLoc != string::npos)
        Value = cmd.substr(0, lastLoc);
      param[Key] = Value;
      cmd = cmd.substr(lastLoc + 1);
    }
    else
    {
      break;
    }
  }
}