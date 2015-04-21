//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/ThreadPool.h"
#include "MantidKernel/ConfigService.h"
#include <sstream>
#include <Poco/Environment.h>

namespace Mantid {
namespace Kernel {

//--------------------------------------------------------------------------------
/** Constructor
 *
 * @param scheduler :: an instance of a ThreadScheduler to schedule tasks.
 *        NOTE: The ThreadPool destructor will delete this ThreadScheduler.
 * @param numThreads :: number of cores to use; default = 0, meaning auto-detect
 *all
 *        available physical cores.
 * @param prog :: optional pointer to a Progress reporter object. If passed,
 *then
 *        automatic progress reporting will be handled by the thread pool.
 *        NOTE: The ThreadPool destructor will delete this.
 */
ThreadPool::ThreadPool(ThreadScheduler *scheduler, size_t numThreads,
                       ProgressBase *prog)
    : m_scheduler(scheduler), m_started(false), m_prog(prog) {
  if (!m_scheduler)
    throw std::invalid_argument(
        "NULL ThreadScheduler passed to ThreadPool constructor.");

  if (numThreads == 0) {
    // Uses Poco to find how many cores there are.
    m_numThreads = getNumPhysicalCores();
  } else
    m_numThreads = numThreads;
  // std::cout << m_numThreads << " m_numThreads \n";
}

//--------------------------------------------------------------------------------
/** Destructor. Deletes the ThreadScheduler.
 */
ThreadPool::~ThreadPool() {
  if (m_scheduler)
    delete m_scheduler;
  if (m_prog)
    delete m_prog;
}

//--------------------------------------------------------------------------------
/** Return the number of physical cores available on the system.
 * NOTE: Uses Poco::Environment::processorCount() to find the number.
 * @return how many cores are present.
 */
size_t ThreadPool::getNumPhysicalCores() {
  int maxCores(0);
  int retVal = Kernel::ConfigService::Instance().getValue(
      "MultiThreaded.MaxCores", maxCores);
  if (retVal > 0 && maxCores > 0)
    return maxCores;
  else
    return Poco::Environment::processorCount();
}

//--------------------------------------------------------------------------------
/** Start the threads and begin looking for tasks.
 *
 * @param waitSec :: how many seconds will each thread be allowed to wait (with
 *no tasks scheduled to it)
 *        before exiting. Default 0.0 (exit right away).
 *        This allows you to start a ThreadPool before you start adding tasks.
 *        You still need to call joinAll() after you've finished!
 *
 * @throw runtime_error if called when it has already started.
 */
void ThreadPool::start(double waitSec) {
  if (m_started)
    throw std::runtime_error("Threads have already started.");

  // Delete any old threads (they should NOT be running!)
  for (size_t i = 0; i < m_threads.size(); ++i)
    delete m_threads[i];
  for (size_t i = 0; i < m_runnables.size(); ++i)
    delete m_runnables[i];

  // Now, launch that many threads and let them wait for new tasks.
  m_threads.clear();
  m_runnables.clear();
  for (size_t i = 0; i < m_numThreads; i++) {
    // Make a descriptive name
    std::ostringstream name;
    name << "Thread" << i;
    // Create the thread
    Poco::Thread *thread = new Poco::Thread(name.str());
    m_threads.push_back(thread);

    // Make the runnable object and run it
    ThreadPoolRunnable *runnable =
        new ThreadPoolRunnable(i, m_scheduler, m_prog, waitSec);
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
void ThreadPool::schedule(Task *task, bool start) {
  if (task) {
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
bool compareTasks(Task *lhs, Task *rhs) { return (lhs->cost() > rhs->cost()); }

//--------------------------------------------------------------------------------
/** Wait for all threads that have started to finish. If the
 * threads had not started, start them first.
 *
 * @throw std::runtime_error rethrown if one of the tasks threw something. E.g.
 *        CancelException in the case of aborting an algorithm. Any exception
 *        gets downgraded to runtime_error.
 */
void ThreadPool::joinAll() {
  // Are the threads REALLY started, or did they exit due to lack of tasks?
  if (m_started) {
    m_started = false;
    // If any of the threads are running, then YES, it is really started.
    for (size_t i = 0; i < m_threads.size(); ++i)
      m_started = m_started || m_threads[i]->isRunning();
  }

  // Start all the threads if they were not already.
  if (!m_started)
    this->start();

  // Clear any wait times so that the threads stop waiting for new tasks.
  for (size_t i = 0; i < m_runnables.size(); i++)
    m_runnables[i]->clearWait();

  // Sequentially join all the threads.
  for (size_t i = 0; i < m_threads.size(); i++) {
    m_threads[i]->join();
    delete m_threads[i];
  }

  // Clear the vectors (the threads are deleted now).
  m_threads.clear();

  // Get rid of the runnables too
  for (size_t i = 0; i < m_runnables.size(); i++)
    delete m_runnables[i];
  m_runnables.clear();

  // This will make threads restart
  m_started = false;

  // Did one of the threads abort or throw an exception?
  if (m_scheduler->getAborted()) {
    // Re-raise the error
    throw m_scheduler->getAbortException();
  }
}

} // namespace Kernel
} // namespace Mantid
