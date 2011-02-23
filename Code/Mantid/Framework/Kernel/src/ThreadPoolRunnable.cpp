#include "MantidKernel/ThreadPoolRunnable.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace Kernel
{


  //-----------------------------------------------------------------------------------
  /** Constructor
   * @param scheduler :: ThreadScheduler used by the thread pool
   */
  ThreadPoolRunnable::ThreadPoolRunnable(size_t threadnum, ThreadScheduler * scheduler)
  :  m_threadnum(threadnum), m_scheduler(scheduler)
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
        //TODO: task-specific mutex if specified?

        // Run the task (synchronously within this thread)
        task->run();

        // Tell the scheduler that we finished this task
        m_scheduler->finished(task);

        // We now delete the task to free up memory
        delete task;
      }
      else
      {
        // No appropriate task for this thread (perhaps a mutex is locked)
        // but there are more tasks.
        // So we wait a bit before checking again.
        Poco::Thread::sleep(100); // millisec
      }
    }
    // Ran out of tasks that could be run.
    // Thread now will exit
  }



} // namespace Mantid
} // namespace Kernel

