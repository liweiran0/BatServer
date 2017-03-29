#include "Task.h"

Task::Task(int id)
{
  taskID = id;
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

string& Task::getDescription()
{
  return description;
}

TaskType& Task::getTaskType()
{
  return taskType;
}

int& Task::getTotalCores()
{
  return totalCores;
}

string& Task::getFileAddress()
{
  return fileAddress;
}

int Task::getTaskID() const
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

int& Task::getFirstProcessIDInQueue()
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


TaskInfo::TaskInfo(int id, string name, string owner, TaskType type):taskID(id),taskName(name),taskOwner(owner),taskType(type)
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

int TaskInfo::getTaskID() const
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

TaskType TaskInfo::getTaskType() const
{
  return taskType;
}

string TaskInfo::getTaskPath() const
{
  return taskPath;
}
