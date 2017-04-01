#pragma once
#include "CommonDef.h"
/*
 * 
 * Copyright (c) 2012 Jakob Progsch, V¨¢clav Zeman

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.

from GitHub:
https://github.com/progschj/ThreadPool
 */

class ThreadPool
{
public:
  ThreadPool(size_t);
  template<class F, class... Args>
  auto enqueue(F&& f, Args&&... args)
    ->future<typename result_of<F(Args...)>::type>;
  ~ThreadPool();
private:
  // need to keep track of threads so we can join them
  vector< thread > workers;
  // the task queue
  queue< function<void()> > tasks;

  // synchronization
  mutex queue_mutex;
  condition_variable condition;
  bool stop;
};

// the constructor just launches some amount of workers
inline ThreadPool::ThreadPool(size_t threads)
  : stop(false)
{
  for (size_t i = 0; i<threads; ++i)
    workers.emplace_back(
    [this]
  {
    for (;;)
    {
      function<void()> task;
      {
        unique_lock<mutex> lock(this->queue_mutex);
        this->condition.wait(lock,
                             [this]
        {
          return this->stop || !this->tasks.empty();
        });
        if (this->stop && this->tasks.empty())
          return;
        task = move(this->tasks.front());
        this->tasks.pop();
      }

      task();
    }
  }
  );
}

// add new work item to the pool
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
-> future<typename result_of<F(Args...)>::type>
{
  using return_type = typename result_of<F(Args...)>::type;

  auto task = make_shared< packaged_task<return_type()> >(
    bind(forward<F>(f), forward<Args>(args)...)
    );

  future<return_type> res = task->get_future();
  {
    unique_lock<mutex> lock(queue_mutex);

    // don't allow enqueueing after stopping the pool
    if (stop)
      throw runtime_error("enqueue on stopped ThreadPool");

    tasks.emplace([task]()
    {
      (*task)();
    });
  }
  condition.notify_one();
  return res;
}

// the destructor joins all threads
inline ThreadPool::~ThreadPool()
{
  {
    unique_lock<mutex> lock(queue_mutex);
    stop = true;
  }
  condition.notify_all();
  for (thread &worker : workers)
    worker.join();
}
