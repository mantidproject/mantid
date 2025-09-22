// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidKernel/Task.h"
#include <deque>
#include <map>
#include <memory>
#include <vector>

namespace Mantid {

namespace Kernel {

/** The ThreadScheduler object defines how tasks are
 * allocated to threads and in what order.
 * It holds the queue of tasks.
 *
  @author Janik Zikovsky, SNS
  @date Feb 7, 2011
 */

//===========================================================================
//===========================================================================
//===========================================================================
class MANTID_KERNEL_DLL ThreadScheduler {
public:
  /** Constructor
   */
  ThreadScheduler() : m_cost(0), m_costExecuted(0), m_abortException(""), m_aborted(false) {}

  /// Destructor
  virtual ~ThreadScheduler() = default;

  //-----------------------------------------------------------------------------------
  /** Add a Task to the queue.
   * @param newTask :: Task to add to queue
   */
  virtual void push(std::shared_ptr<Task> newTask) = 0;

  //-----------------------------------------------------------------------------------
  /** Retrieves the next Task to execute.
   * @param threadnum :: ID of the calling thread.
   * @return a Task pointer to execute.
   */
  virtual std::shared_ptr<Task> pop(size_t threadnum) = 0;

  //-----------------------------------------------------------------------------------
  /** Signal to the scheduler that a task is complete.
   *
   * @param task :: the Task that was completed.
   * @param threadnum :: Thread ID that launched the task
   */
  virtual void finished(Task *task, size_t threadnum) {
    UNUSED_ARG(task);
    UNUSED_ARG(threadnum);
  }

  //-----------------------------------------------------------------------------------
  /** Signal to the scheduler that a task is complete. The
   * scheduler may release mutexes, etc.
   *
   * @param exception :: the exception that aborted the run.
   */
  virtual void abort(std::runtime_error exception) {
    // Save the exception for re-throwing
    m_abortException = exception;
    m_aborted = true;
    // Clear (and delete) the queue
    clear();
  }

  //-----------------------------------------------------------------------------------
  /// Returns the size of the queue
  virtual size_t size() = 0;

  /// Returns true if the queue is empty
  virtual bool empty() = 0;

  /// Empty out the queue
  virtual void clear() = 0;

  //-------------------------------------------------------------------------------
  /// Returns the total cost of all Task's in the queue.
  double totalCost() { return m_cost; }

  //-------------------------------------------------------------------------------
  /// Returns the total cost of all Task's in the queue.
  double totalCostExecuted() { return m_costExecuted; }

  //-------------------------------------------------------------------------------
  /// Returns the exception that was caught, if any.
  std::runtime_error getAbortException() { return m_abortException; }
  //-------------------------------------------------------------------------------
  /// Returns true if the execution was aborted.
  bool getAborted() { return m_aborted; }

protected:
  /// Total cost of all tasks
  double m_cost;
  /// Accumulated cost of tasks that have been executed (popped)
  double m_costExecuted;
  /// Mutex to prevent simultaneous access to the queue.
  std::mutex m_queueLock;
  /// The exception that aborted the run.
  std::runtime_error m_abortException;
  /// The run was aborted due to an exception
  bool m_aborted;
};

//===========================================================================
//===========================================================================
//===========================================================================
/** A First-In-First-Out Thread Scheduler.
 *
 * A queue of tasks is maintained and are run in the order
 * they were submitted.
 *
 */
class MANTID_KERNEL_DLL ThreadSchedulerFIFO : public ThreadScheduler {
public:
  ThreadSchedulerFIFO() : ThreadScheduler() {}

  /// Destructor
  ~ThreadSchedulerFIFO() override { clear(); }

  //-------------------------------------------------------------------------------
  /// @return true if the queue is empty
  bool empty() override {
    std::lock_guard<std::mutex> _lock(m_queueLock);
    return m_queue.empty();
  }

  //-------------------------------------------------------------------------------
  void push(std::shared_ptr<Task> newTask) override {
    // Cache the total cost
    m_queueLock.lock();
    m_cost += newTask->cost();
    m_queue.emplace_back(newTask);
    m_queueLock.unlock();
  }

  //-------------------------------------------------------------------------------
  std::shared_ptr<Task> pop(size_t threadnum) override {
    UNUSED_ARG(threadnum);
    std::shared_ptr<Task> temp = nullptr;
    m_queueLock.lock();
    // Check the size within the same locking block; otherwise the size may
    // change before you get the next item.
    if (!m_queue.empty()) {
      // TODO: Would a try/catch block be smart here?
      temp = m_queue.front();
      m_queue.pop_front();
    }
    m_queueLock.unlock();
    return temp;
  }

  //-------------------------------------------------------------------------------
  size_t size() override {
    m_queueLock.lock();
    size_t temp = m_queue.size();
    m_queueLock.unlock();
    return temp;
  }

  //-------------------------------------------------------------------------------
  void clear() override final {
    m_queueLock.lock();
    // Empty out the queue
    m_queue.clear();
    m_cost = 0;
    m_costExecuted = 0;
    m_queueLock.unlock();
  }

protected:
  /// Queue of tasks
  std::deque<std::shared_ptr<Task>> m_queue;
};

//===========================================================================
//===========================================================================
//===========================================================================
/** A Last-In-First-Out Thread Scheduler.
 *
 * A queue of tasks is maintained;
 * the last Task added is the first one returned.
 *
 */
class MANTID_KERNEL_DLL ThreadSchedulerLIFO : public ThreadSchedulerFIFO {

  //-------------------------------------------------------------------------------
  std::shared_ptr<Task> pop(size_t threadnum) override {
    UNUSED_ARG(threadnum);
    std::shared_ptr<Task> temp = nullptr;
    m_queueLock.lock();
    // Check the size within the same locking block; otherwise the size may
    // change before you get the next item.
    if (!m_queue.empty()) {
      // TODO: Would a try/catch block be smart here?
      temp = m_queue.back();
      m_queue.pop_back();
    }
    m_queueLock.unlock();
    return temp;
  }
};

//===========================================================================
//===========================================================================
//===========================================================================
/** A Largest Cost Thread Scheduler.
 *
 * The scheduled tasks are run so that the most time-consuming
 * (highest cost) taks are run first.
 * This tends to optimize task allocation the best.
 * http://en.wikipedia.org/wiki/Bin_packing_problem
 *
 * Interally, it uses a multimap to keep elements sorted while inserting them.
 */
class MANTID_KERNEL_DLL ThreadSchedulerLargestCost : public ThreadScheduler {
public:
  ThreadSchedulerLargestCost() : ThreadScheduler() {}

  /// Destructor
  ~ThreadSchedulerLargestCost() override { clear(); }

  //-------------------------------------------------------------------------------
  /// @return true if the queue is empty
  bool empty() override {
    std::lock_guard<std::mutex> _lock(m_queueLock);
    return m_map.empty();
  }

  //-------------------------------------------------------------------------------
  void push(std::shared_ptr<Task> newTask) override {
    // Cache the total cost
    m_queueLock.lock();
    m_cost += newTask->cost();
    m_map.emplace(newTask->cost(), newTask);
    m_queueLock.unlock();
  }

  //-------------------------------------------------------------------------------
  std::shared_ptr<Task> pop(size_t threadnum) override {
    UNUSED_ARG(threadnum);
    std::shared_ptr<Task> temp = nullptr;
    m_queueLock.lock();
    // Check the size within the same locking block; otherwise the size may
    // change before you get the next item.
    if (!m_map.empty()) {
      // Since the map is sorted by cost, we want the LAST item.
      auto it = m_map.end();
      it--;
      temp = it->second;
      m_map.erase(it);
    }
    m_queueLock.unlock();
    return temp;
  }

  //-------------------------------------------------------------------------------
  size_t size() override {
    m_queueLock.lock();
    size_t temp = m_map.size();
    m_queueLock.unlock();
    return temp;
  }

  //-------------------------------------------------------------------------------
  void clear() override final {
    m_queueLock.lock();
    // Empty out the queue and delete the pointers!
    m_map.clear();
    m_cost = 0;
    m_costExecuted = 0;
    m_queueLock.unlock();
  }

protected:
  /// A multimap keeps tasks sorted by the key (cost)
  std::multimap<double, std::shared_ptr<Task>> m_map;
};

} // namespace Kernel
} // namespace Mantid
