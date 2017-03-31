#pragma once
#include "CommonDef.h"
using namespace std;

enum class TaskType
{
  HM = 0,
  IMG = 1,
  JEM = 2,
  NONE = 3,
};

class Process;

class Task
{
private:
  string taskOwner = "";     // who create the task
  string taskName = "";      //
  string description = "";   //
  TaskType taskType = TaskType::NONE;    //
  int totalCores = INT_MAX;  // max cores to do the task

  string fileAddress = "";   // where to store the files and results
  string taskID;           // unique task id
  int processNumbers = 0;   // total process numbers

  int finishedNumber = 0;   // the number of finished processes
  int processingNumber = 0; // the number of processes under processing
  string firstProcessIDInQueue = ""; // the first ID of process in queue
  vector<shared_ptr<Process>> processes; // all the processes
  function<void(void)> afterProcessing;
public:
  Task(decltype(taskID) id);
  ~Task();
  string& getTaskOwner();
  string& getTaskName();
  string& getDescription();
  TaskType& getTaskType();
  int &getTotalCores();
  string& getFileAddress();
  decltype(taskID) getTaskID()const;
  int &getProcessNumbers();
  int &getFinishedNumber();
  int &getProcessingNumber();
  decltype(firstProcessIDInQueue) &getFirstProcessIDInQueue();
  vector<shared_ptr<Process>>& getProcesses();
  void setCallback(function<void(void)> cb);
  void doCallback();
};

class TaskInfo
{
private:
  string taskID;
  string taskName;
  string taskOwner;
  TaskType taskType;
  string taskPath;
  chrono::system_clock::time_point finishedTime;
public:
  TaskInfo(decltype(taskID) id, string name, string owner, TaskType type);
  ~TaskInfo();
  string& getTaskPath();
  chrono::system_clock::time_point getTaskFinishedTime() const;
  decltype(taskID) getTaskID()const;
  string getTaskOwner()const;
  string getTaskName()const;
  TaskType getTaskType()const;
  string getTaskPath()const;
};