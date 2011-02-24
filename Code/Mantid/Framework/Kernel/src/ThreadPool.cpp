//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/ThreadPool.h"
#include "MantidKernel/ThreadPoolRunnable.h"
#include "MantidKernel/Task.h"
#include <algorithm>
#include <sstream>
#include <cmath>
#include <cfloat>
#include <Poco/Mutex.h>
#include <Poco/Runnable.h>
#include <Poco/Thread.h>

namespace Mantid
{
namespace Kernel
{

  //--------------------------------------------------------------------------------
  /** Constructor
   *
   * @param scheduler :: an instance of a ThreadScheduler to schedule tasks.
   *        NOTE: The ThreadPool destructor will delete this.
   * @param numThreads :: number of cores to use; default = 0, meaning auto-detect all
   *        available physical cores.
   */
  ThreadPool::ThreadPool( ThreadScheduler * scheduler, size_t numThreads)
    : m_scheduler(scheduler), m_started(false)
  {
    if (!m_scheduler)
      throw std::invalid_argument("NULL ThreadScheduler passed to ThreadPool constructor.");

    if (numThreads == 0)
    {
      //Uses OpenMP to find how many cores there are.
      m_numThreads = getNumPhysicalCores();
    }
    else
      m_numThreads = numThreads;
    //std::cout << m_numThreads << " m_numThreads \n";
  }


  //--------------------------------------------------------------------------------
  /** Destructor. Deletes the ThreadScheduler.
   */
  ThreadPool::~ThreadPool()
  {
    if (m_scheduler)
      delete m_scheduler;
  }

  //--------------------------------------------------------------------------------
  /** Return the number of physical cores available on the system.
   * NOTE: Uses OpenMP getMaxThreads to find the number.
   * @return how many cores are present. 1 if no OpenMP is installed.
   */
  size_t ThreadPool::getNumPhysicalCores()
  {
    return PARALLEL_GET_MAX_THREADS;
  }



  //--------------------------------------------------------------------------------
  /** Start the threads and begin looking for tasks.
   * @throw runtime_error if called when it has already started.
   */
  void ThreadPool::start()
  {
    if (m_started)
      throw std::runtime_error("Threads have already started.");

    // Now, launch that many threads and let them wait for new tasks.
    m_threads.clear();
    m_runnables.clear();
    for (size_t i = 0; i < m_numThreads; i++)
    {
      // Make a descriptive name
      std::ostringstream name;
      name << "Thread" << i;
      // Create the thread
      Poco::Thread * thread = new Poco::Thread(name.str());
      m_threads.push_back(thread);

      // Make the runnable object and run it
      ThreadPoolRunnable * runnable = new ThreadPoolRunnable(i, m_scheduler);
      m_runnables.push_back(runnable);

      thread->start(*runnable);
    }
    // Yep, all the threads are running.
    m_started = true;
  }

  //--------------------------------------------------------------------------------
  /** Schedule a task for later execution. If the threadpool is running,
   * it will be picked up by the next available thread.
   *
   * @param task :: pointer to a Task object to run.
   * @param start :: start the thread at the same time; default false
   */
  void ThreadPool::schedule(Task * task, bool start)
  {
    if (task)
    {
      m_scheduler->push(task);
      // Start all the threads if they were not already.
      if (start && !m_started)
        this->start();
    }
  }



  //--------------------------------------------------------------------------------
  /** Method to perform sorting of task lists.
   * This prioritizes long tasks, so they end up at start of the list.
   *
   * @param lhs :: Task*
   * @param rhs :: Task*
   * @return true if lhs < rhs (aka lhs should be first)
   */
  bool compareTasks(Task * lhs, Task * rhs)
  {
    return (lhs->cost() > rhs->cost());
  }


  //--------------------------------------------------------------------------------
  /** Begin the execution of all scheduled tasks.
   * TODO: Make it parallel! For now, serial execution.
   *
   */
  void ThreadPool::joinAll()
  {
    // Start all the threads if they were not already.
    if (!m_started)
      this->start();

    // Sequentially join all the threads.
    for (size_t i=0; i < m_threads.size(); i++)
    {
      m_threads[i]->join();
      // Delete the old thread
      delete m_threads[i];
    }

    // Clear the vectors (the threads are deleted now).
    m_threads.clear();
    for (size_t i=0; i < m_runnables.size(); i++)
      delete m_runnables[i];
    m_runnables.clear();

    // Did one of the threads abort or throw an exception?
    if (m_scheduler->getAborted())
    {
      // Re-raise the error
      throw m_scheduler->getAbortException();
    }

  }



} // namespace Kernel
} // namespace Mantid
