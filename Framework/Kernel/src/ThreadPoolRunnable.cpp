// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/ThreadPoolRunnable.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/Task.h"
#include "MantidKernel/ThreadScheduler.h"

#include <Poco/Thread.h>

namespace Mantid::Kernel {

namespace {
/// Marks a thread busy for its lifetime; idle again when it goes out of scope, so an exception cannot leave it busy
class BusyGuard {
public:
  explicit BusyGuard(ThreadScheduler &scheduler) : m_scheduler(&scheduler) { m_scheduler->markBusy(); }
  BusyGuard(const BusyGuard &) = delete;
  BusyGuard &operator=(const BusyGuard &) = delete;
  ~BusyGuard() { release(); }
  /// Mark the thread idle now rather than at the end of the scope
  void release() {
    if (m_scheduler) {
      m_scheduler->markIdle();
      m_scheduler = nullptr;
    }
  }

private:
  ThreadScheduler *m_scheduler;
};
} // namespace

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
ThreadPoolRunnable::ThreadPoolRunnable(size_t threadnum, ThreadScheduler *scheduler, ProgressBase *prog, double waitSec)
    : m_threadnum(threadnum), m_scheduler(scheduler), m_prog(prog), m_waitSec(waitSec) {
  if (!m_scheduler)
    throw std::invalid_argument("NULL ThreadScheduler passed to ThreadPoolRunnable::ctor()");
}

//-----------------------------------------------------------------------------------
/** Clear the wait time of the runnable so that it stops waiting for tasks. */
void ThreadPoolRunnable::clearWait() { m_waitSec = 0.0; }

//-----------------------------------------------------------------------------------
/** Thread method. Will wait for new tasks and run them
 * as scheduled to it.
 */
void ThreadPoolRunnable::run() {
  std::shared_ptr<Task> task;

  // If there are no tasks yet, wait up to m_waitSec for them to come up
  while (m_scheduler->empty() && m_waitSec > 0.0) {
    Poco::Thread::sleep(10); // millisec
    m_waitSec -= 0.01;       // Subtract ten millisec from the time left to wait.
  }

  // Keep going while there are tasks in the queue, or while another thread is
  // running a task that may still add more.
  while (!m_scheduler->empty() || m_scheduler->hasBusyThreads()) {
    // Mark this thread busy before asking for a task so another thread never
    // sees an empty queue and no busy threads while this one holds the last task.
    BusyGuard busy(*m_scheduler);

    // Request the task from the scheduler.
    // Will be NULL if not found.
    task = m_scheduler->pop(m_threadnum);

    if (task) {
      // Task-specific mutex if specified?
      std::shared_ptr<std::mutex> mutex = task->getMutex();
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
      m_scheduler->finished(task.get(), m_threadnum);

      // Report progress, if specified.
      if (m_prog)
        m_prog->report();

      // Unlock the mutex, if any.
      if (mutex)
        mutex->unlock();

      // We now delete the task to free up memory
      task.reset();
    } else {
      // No appropriate task for this thread (perhaps a mutex is locked, or
      // another thread's task has not added its tasks yet).
      // So we wait a bit before checking again.
      busy.release();
      Poco::Thread::sleep(10); // millisec
    }
  }
  // Ran out of tasks that could be run.
  // Thread now will exit
}

} // namespace Mantid::Kernel
