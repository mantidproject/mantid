#include "MantidKernel/ThreadPoolRunnable.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace Kernel
{


  //-----------------------------------------------------------------------------------
  /** Constructor
   *
   * @param threadnum :: the thread ID that this runnable is running in.
   * @param scheduler :: ThreadScheduler used by the thread pool
   * @param prog :: optional pointer to a Progress reporter object. If passed, then
   *        automatic progress reporting will be handled by the thread pool.
   */
  ThreadPoolRunnable::ThreadPoolRunnable(size_t threadnum, ThreadScheduler * scheduler,
      ProgressBase * prog)
  :  m_threadnum(threadnum), m_scheduler(scheduler), m_prog(prog)
  {
    if (!m_scheduler)
      throw std::invalid_argument("NULL ThreadScheduler passed to ThreadPoolRunnable::ctor()");
  }

  //-----------------------------------------------------------------------------------
  /** Destructor
   */
  ThreadPoolRunnable::~ThreadPoolRunnable()
  {}


  //-----------------------------------------------------------------------------------
  /** Thread method. Will wait for new tasks and run them
   * as scheduled to it.
   */
  void ThreadPoolRunnable::run()
  {
    Task * task;

    while (m_scheduler->size() > 0)
    {
      // Request the task from the scheduler.
      // Will be NULL if not found.
      task = m_scheduler->pop(m_threadnum);

      if (task)
      {
        //Task-specific mutex if specified?
        Mutex * mutex = task->getMutex();
        if (mutex)
          mutex->lock();

        try
        {
          // Run the task (synchronously within this thread)
          task->run();
        }
        catch (std::exception &e)
        {
          // The task threw an exception!
          // This will clear out the list of tasks, allowing all threads to finish.
          m_scheduler->abort(std::runtime_error(e.what()));
        }

        // Tell the scheduler that we finished this task
        m_scheduler->finished(task, m_threadnum);

        // Report progress, if specified.
        if (m_prog)
          m_prog->report();

        // Unlock the mutex, if any.
        if (mutex)
          mutex->unlock();

        // We now delete the task to free up memory
        delete task;
      }
      else
      {
        // No appropriate task for this thread (perhaps a mutex is locked)
        // but there are more tasks.
        // So we wait a bit before checking again.
        Poco::Thread::sleep(50); // millisec
      }
    }
    // Ran out of tasks that could be run.
    // Thread now will exit
  }



} // namespace Mantid
} // namespace Kernel

