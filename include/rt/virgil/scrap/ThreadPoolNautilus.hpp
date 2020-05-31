/*
 * Copyright 2017 - 2019  Simone Campanoni
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * The ThreadPool class as implemented for Nautilus
 * Keeps a set of threads constantly waiting to execute incoming jobs.
 */
#pragma once

#include "ThreadSafeNautilusQueue.hpp"
#include "ThreadTaskNautilus.hpp"
#include "TaskFuture.hpp"

#define TASK_QUEUE_TYPE ThreadSafeNautilusQueue

#include <vector>

extern "C" void _nk_virgil_worker_trampoline(void *in, void **out);


namespace MARC {

  /*
   * Thread pool.
   */
  class ThreadPool {
    
    friend  void ::_nk_virgil_worker_trampoline(void *in, void **out);
    
    public:

      /*
       * Default constructor.
       *
       * By default, the thread pool is not extendible and it creates at least one thread.
       */
      ThreadPool(void);

      /*
       * Constructor.
       */
      explicit ThreadPool (
        const bool extendible,
        const std::uint32_t numThreads = std::max(nk_get_num_cpus(), 2u) - 1u,
        std::function <void (void)> codeToExecuteAtDeconstructor = nullptr);

      void appendCodeToDeconstructor (std::function<void ()> codeToExecuteAtDeconstructor);

      /*
       * Submit a job to be run by the thread pool.
       */
      template <typename Func, typename... Args>
      auto submit (Func&& func, Args&&... args);

      /*
       * Submit a job to be run by the thread pool pinning the thread to one of the specified cores.
       */
      template <typename Func, typename... Args>
      auto submitToCores (const cpu_set_t& cores, Func&& func, Args&&... args);

      /*
       * Submit a job to be run by the thread pool pinning the thread to the specified core.
       */
      template <typename Func, typename... Args>
      auto submitToCore (uint32_t core, Func&& func, Args&&... args);

      /*
       * Submit a job to be run by the thread pool and detach it from the caller.
       */
      template <typename Func, typename... Args>
      void submitAndDetach (Func&& func, Args&&... args) ;

      /*
       * Return the number of threads that are currently idle.
       */
      std::uint32_t numberOfIdleThreads (void) const ;

      /*
       * Return the number of tasks that did not start executing yet.
       */
      std::uint64_t numberOfTasksWaitingToBeProcessed (void) const ;

      /*
       * Destructor.
       */
      ~ThreadPool(void);

      /*
       * Non-copyable.
       */
      ThreadPool(const ThreadPool& rhs) = delete;

      /*
       * Non-assignable.
       */
      ThreadPool& operator=(const ThreadPool& rhs) = delete;

    private:

      /*
       * Object fields.
       */
      bool m_done;  // atomic - no use of std::atomic
    //TASK_QUEUE_TYPE<IThreadTask *> m_workQueue;
    TASK_QUEUE_TYPE<std::unique_ptr<IThreadTask>> m_workQueue;
      std::vector<nk_thread_id_t> m_threads;
      std::vector<bool *> threadAvailability; // atomic
      TASK_QUEUE_TYPE<std::function<void ()>> codeToExecuteByTheDeconstructor;
      bool extendible;
      LOCK_TYPE lock;

      /*
       * Expand the pool if possible and necessary.
       */
      void expandPool (void);

      /*
       * Constantly running function each thread uses to acquire work items from the queue.
       */
      void worker (bool *availability);  // atomic

      /*
       * Start new threads.
       */
      int newThreads (std::uint32_t newThreadsToGenerate);

      /*
       * Invalidates the queue and joins all running threads.
       */
      void destroy (void);
  };

}

MARC::ThreadPool::ThreadPool(void) 
  : ThreadPool{false} 
  {

  /*
   * Always create at least one thread.  If hardware_concurrency() returns 0,
   * subtracting one would turn it to UINT_MAX, so get the maximum of
   * hardware_concurrency() and 2 before subtracting 1.
   */

  return ;
}

MARC::ThreadPool::ThreadPool (
  const bool extendible,
  const std::uint32_t numThreads,
  std::function <void (void)> codeToExecuteAtDeconstructor)
  :
  m_workQueue{},
  m_threads{},
  codeToExecuteByTheDeconstructor{}

  {

  atomic_store_bool(&m_done,false);
    
  LOCK_INIT(&this->lock);
    
  /*
   * Set whether or not the thread pool can dynamically change its number of threads.
   */
  this->extendible = extendible;

  /*
   * Start threads.
   */
  if (this->newThreads(numThreads)) {
    destroy();
  }

  if (codeToExecuteAtDeconstructor != nullptr){
    this->codeToExecuteByTheDeconstructor.push(codeToExecuteAtDeconstructor);
  }

  return ;
}

void MARC::ThreadPool::appendCodeToDeconstructor (std::function<void ()> codeToExecuteAtDeconstructor){
  this->codeToExecuteByTheDeconstructor.push(codeToExecuteAtDeconstructor);

  return ;
}

template <typename Func, typename... Args>
auto MARC::ThreadPool::submit (Func&& func, Args&&... args){

  /*
   * Making the task.
   */
  auto boundTask = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
  using ResultType = std::result_of_t<decltype(boundTask)()>;
  using PackagedTask = std::packaged_task<ResultType()>;
  using TaskType = ThreadTask<PackagedTask>;
  PackagedTask task{std::move(boundTask)};

  /*
   * Create the future.
   */
  TaskFuture<ResultType> result{task.get_future()};
  
  /*
   * Submit the task.
   */
  // This is hacked to avoid the smart pointer (we think)
  // We also found that we are using the wrong libstdc++
  // let's not use locate for that
  // this is a smart pointer (std::make_unique) PAD
  m_workQueue.push(std::make_unique<TaskType>(std::move(task)));
  //m_workQueue.push(new TaskType{std::move(*task)});

  /*
   * Expand the pool if possible and necessary.
   */
  this->expandPool();

  return result;
}
  
template <typename Func, typename... Args>
auto MARC::ThreadPool::submitToCores (const cpu_set_t& cores, Func&& func, Args&&... args){

  /*
   * Making the task.
   */
  auto boundTask = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
  using ResultType = std::result_of_t<decltype(boundTask)()>;
  using PackagedTask = std::packaged_task<ResultType()>;
  using TaskType = ThreadTask<PackagedTask>;
  PackagedTask task{std::move(boundTask)};

  /*
   * Create the future.
   */
  TaskFuture<ResultType> result{task.get_future()};
  
  /*
   * Submit the task.
   */
  m_workQueue.push(std::make_unique<TaskType>(cores, std::move(task)));

  /*
   * Expand the pool if possible and necessary.
   */
  this->expandPool();

  return result;
}

template <typename Func, typename... Args>
auto MARC::ThreadPool::submitToCore (uint32_t core, Func&& func, Args&&... args){

  /*
   * Making the task.
   */
  auto boundTask = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
  using ResultType = std::result_of_t<decltype(boundTask)()>;
  using PackagedTask = std::packaged_task<ResultType()>;
  using TaskType = ThreadTask<PackagedTask>;
  PackagedTask task{std::move(boundTask)};

  /*
   * Create the future.
   */
  TaskFuture<ResultType> result{task.get_future()};

  /*
   * Set the affinity.
   */
  cpu_set_t cores;
  CPU_ZERO(&cores);
  CPU_SET(core, &cores);

  /*
   * Submit the task.
   */
  m_workQueue.push(std::make_unique<TaskType>(cores, std::move(task)));

  /*
   * Expand the pool if possible and necessary.
   */
  this->expandPool();

  return result;
}

template <typename Func, typename... Args>
void MARC::ThreadPool::submitAndDetach (Func&& func, Args&&... args){

  /*
   * Making the task.
   */
  auto boundTask = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
  using ResultType = std::result_of_t<decltype(boundTask)()>;
  using PackagedTask = std::packaged_task<ResultType()>;
  using TaskType = ThreadTask<PackagedTask>;
  PackagedTask task{std::move(boundTask)};

  /*
   * Submit the task.
   */
  m_workQueue.push(std::make_unique<TaskType>(std::move(task)));

  /*
   * Expand the pool if possible and necessary.
   */
  this->expandPool();

  return ;
}

void MARC::ThreadPool::worker (bool *availability){

  VIRGIL_DEBUG("threadpool: worker alive\n");

  //  while(1) {}
  
  while(!atomic_load_bool(&m_done)) {
    atomic_store_bool(availability, true);
    std::unique_ptr<IThreadTask> pTask{nullptr};
    //    IThreadTask *pTask=0;
    if(m_workQueue.waitPop(pTask)) {
      (*availability) = false;
      VIRGIL_DEBUG("threadpool: launching task\n");
      pTask->execute();
    }
  }

  return ;
}

typedef struct {
  MARC::ThreadPool  *pool;
  bool              *avail; // atomic
} nk_virgil_worker_thunk_t;

extern "C" void _nk_virgil_worker_trampoline(void *in, void **out)
{
  nk_virgil_worker_thunk_t *t = (nk_virgil_worker_thunk_t *)in;
  VIRGIL_DEBUG("threadpool: starting worker (thread=%p)\n",nk_virgil_get_cur_thread());;
  t->pool->worker(t->avail);
  VIRGIL_DEBUG("threadpool: worker finishing (thread=%p)\n",nk_virgil_get_cur_thread());
}

int MARC::ThreadPool::newThreads (std::uint32_t newThreadsToGenerate){
  for (auto i = 0; i < newThreadsToGenerate; i++){
    
    /*
     * Create the availability flag.
     */
    auto flag = new bool(true);
    this->threadAvailability.push_back(flag);

    nk_virgil_worker_thunk_t *t = new nk_virgil_worker_thunk_t;
    t->pool = this;
    t->avail = flag;
    nk_thread_id_t tid;

    // create thread unbound to CPU, with default stack size,
    // that starts in trampoline and is passed the thunk
    if (nk_thread_start(_nk_virgil_worker_trampoline, t, 0, 0, TSTACK_DEFAULT, &tid, -1)) {
      VIRGIL_ERROR("threadpool: failed to create thread for worker\n");
      return -1;
    }

    m_threads.push_back(tid);

    VIRGIL_DEBUG("threadpool: started new worker %p\n",tid);

  }

  return 0;
}

void MARC::ThreadPool::destroy (void){

  VIRGIL_DEBUG("threadpool: destory\n");

  /*
   * Execute the user code.
   */
  while (codeToExecuteByTheDeconstructor.size() > 0){
    std::function<void ()> code;
    codeToExecuteByTheDeconstructor.waitPop(code);
    code();
  }

  /*
   * Signal threads to quite.
   */
  atomic_store_bool(&m_done,true);
  m_workQueue.invalidate();

  /*
   * Join threads.
   */
  for(auto& thread : m_threads) {
    VIRGIL_DEBUG("threadpool: join with thread %p start\n",thread);
    if (nk_join(thread,0)) {
      VIRGIL_ERROR("threadpool: failed to join with thread %p\n",thread);
    }
    VIRGIL_DEBUG("threadpool: join with thread %p end\n",thread);
  }
  for (auto flag : this->threadAvailability){
    delete flag;
  }

  return ;
}

std::uint32_t MARC::ThreadPool::numberOfIdleThreads (void) const {
  std::uint32_t n = 0;

  for (auto i=0; i < this->m_threads.size(); i++){
    auto isThreadAvailable = this->threadAvailability[i];
    if (atomic_load_bool(isThreadAvailable)){
      n++;
    }
  }

  return n;
}

std::uint64_t MARC::ThreadPool::numberOfTasksWaitingToBeProcessed (void) const {
  auto s = this->m_workQueue.size();

  return s;
}

void MARC::ThreadPool::expandPool (void) {

  /*
   * Check whether we are allow to expand the pool or not.
   */
  if (!this->extendible){
     return ;
  }

  /*
   * We are allow to expand the pool.
   *
   * Check whether we should expand the pool.
   */
  if (this->numberOfIdleThreads() < this->m_workQueue.size()){

    /*
     * Spawn new threads.
     */
    LOCK_DECL;
    LOCK(&this->lock);
    this->newThreads(2);
    UNLOCK(&this->lock);
  }

  return ;
}

MARC::ThreadPool::~ThreadPool (void){
  destroy();

  return ;
}
