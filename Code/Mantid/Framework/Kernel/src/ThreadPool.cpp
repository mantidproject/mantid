//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/ThreadPool.h"
#include "MantidKernel/Task.h"
#include <algorithm>
#include <cmath>
#include <cfloat>

namespace Mantid
{
namespace Kernel
{

//--------------------------------------------------------------------------------
  /** Constructor
   *
   * @param scheduler :: an instance of a ThreadScheduler to schedule tasks.
   * @param numThreads :: number of cores to use; default = 0, meaning auto-detect all
   *        available physical cores.
   */
  ThreadPool::ThreadPool( ThreadScheduler * scheduler, size_t numThreads)
    : m_scheduler(scheduler)
  {
    if (numThreads == 0)
    {
      //TODO: auto-detect
      numThreads = 1;
    }
    else
      m_numThreads = numThreads;
  }


  //--------------------------------------------------------------------------------
  /** Schedule a task for later execution.
   *
   * @param task :: pointer to a Task object to run.
   */
  void ThreadPool::schedule(Task * task)
  {
    if (task)
    {
      m_scheduler->push(task);
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
    while (m_scheduler->size() > 0)
    {
      Task * task = m_scheduler->pop();

      // Run it!
      if (task)
      {
        task->run();
      }
    }

  }



} // namespace Kernel
} // namespace Mantid
