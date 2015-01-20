#include "MantidKernel/ThreadPoolRunnable.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Kernel {

//-----------------------------------------------------------------------------------
/** Constructor
 *
 * @param threadnum :: the thread ID that this runnable is running in.
 * @param scheduler :: ThreadScheduler used by the thread pool
 * @param prog :: optional pointer to a Progress reporter object. If passed,
 *then
 *        automatic progress reporting will be handled by the thread pool.
 * @param waitSec :: how many seconds the thread is allowed to wait with no
 *tasks.
 */
ThreadPoolRunnable::ThreadPoolRunnable(size_t threadnum,
                                       ThreadScheduler *scheduler,
                                       ProgressBase *prog, double waitSec)
    : m_threadnum(threadnum), m_scheduler(scheduler), m_prog(prog),
      m_waitSec(waitSec) {
  if (!m_scheduler)
    throw std::invalid_argument(
        "NULL ThreadScheduler passed to ThreadPoolRunnable::ctor()");
}

//-----------------------------------------------------------------------------------
/** Destructor
 */
ThreadPoolRunnable::~ThreadPoolRunnable() {}

//-----------------------------------------------------------------------------------
/** Clear the wait time of the runnable so that it stops waiting for tasks. */
void ThreadPoolRunnable::clearWait() { m_waitSec = 0.0; }

//-----------------------------------------------------------------------------------
/** Thread method. Will wait for new tasks and run them
 * as scheduled to it.
 */
void ThreadPoolRunnable::run() {
  Task *task;

  // If there are no tasks yet, wait up to m_waitSec for them to come up
  while (m_scheduler->empty() && m_waitSec > 0.0) {
    Poco::Thread::sleep(10); // millisec
    m_waitSec -= 0.01; // Subtract ten millisec from the time left to wait.
  }

  while (!m_scheduler->empty()) {
    // Request the task from the scheduler.
    // Will be NULL if not found.
    task = m_scheduler->pop(m_threadnum);

    if (task) {
      // Task-specific mutex if specified?
      boost::shared_ptr<Mutex> mutex = task->getMutex();
      if (bool(mutex))
        mutex->lock();

      try {
        // Run the task (synchronously within this thread)
        task->run();
      } catch (std::exception &e) {
        // The task threw an exception!
        // This will clear out the list of tasks, allowing all threads to
        // finish.
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
    } else {
      // No appropriate task for this thread (perhaps a mutex is locked)
      // but there are more tasks.
      // So we wait a bit before checking again.
      Poco::Thread::sleep(10); // millisec
    }
  }
  // Ran out of tasks that could be run.
  // Thread now will exit
}

} // namespace Mantid
} // namespace Kernel
