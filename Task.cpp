#include "Task.h"

static int usedTaskId = 0;

Task::Task()
{
  taskID = "task_" + to_string(++usedTaskId);
}

Task::~Task()
{
  processes.clear();
}

string& Task::getTaskOwner()
{
  return taskOwner;
}

string& Task::getTaskName()
{
  return taskName;
}

string& Task::getWorkDir()
{
  return workDir;
}

string& Task::getReletiveDir()
{
  return reletiveDir;
}

string& Task::getTaskType()
{
  return taskType;
}

string& Task::getLogName()
{
  return logName;
}

int& Task::getTotalCores()
{
  return totalCores;
}

auto Task::getTaskID() const->decltype(taskID)
{
  return taskID;
}

int& Task::getProcessNumbers()
{
  return processNumbers;
}

int& Task::getFinishedNumber()
{
  return finishedNumber;
}

int& Task::getProcessingNumber()
{
  return processingNumber;
}

auto Task::getFirstProcessIDInQueue()->decltype(firstProcessIDInQueue)&
{
  return firstProcessIDInQueue;
}

vector<shared_ptr<Process>>& Task::getProcesses()
{
  return processes;
}

void Task::setCallback(function<void(void)> cb)
{
  afterProcessing = cb;
}


void Task::doCallback()
{
  cout << "Task " << taskID << " finished." << endl;
  if (afterProcessing)
  {
    afterProcessing();
  }
}

bool& Task::getStatus()
{
  return status;
}


TaskInfo::TaskInfo(decltype(taskID) id, string name, string owner, string type):taskID(id),taskName(name),taskOwner(owner),taskType(type)
{
  finishedTime = chrono::system_clock::now();
}

TaskInfo::~TaskInfo()
{
}


string& TaskInfo::getTaskPath()
{
  return taskPath;
}


chrono::system_clock::time_point TaskInfo::getTaskFinishedTime() const
{
  return finishedTime;
}

auto TaskInfo::getTaskID() const->decltype(taskID)
{
  return taskID;
}

string TaskInfo::getTaskOwner() const
{
  return taskOwner;
}

string TaskInfo::getTaskName() const
{
  return taskName;
}

string TaskInfo::getTaskType() const
{
  return taskType;
}

string TaskInfo::getTaskPath() const
{
  return taskPath;
}

