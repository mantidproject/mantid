// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_THREADSCHEDULERMUTEXES_H_
#define MANTID_KERNEL_THREADSCHEDULERMUTEXES_H_

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/ThreadScheduler.h"
#include <mutex>
#include <numeric>
#include <set>

namespace Mantid {
namespace Kernel {

/** ThreadSchedulerMutexes : Version of a ThreadSchedulerLargestCost
 * that also makes sure to not try to schedule two tasks with the
 * same mutex at the same time.
 *
 * This scheduler also sorts by largest cost so as to optimize allocation
 * that way.
 *
 * NOTE: The performance of "pop"ping a task is much slower if you have a very
 * large number of different mutexes; this scheduler is better suited if you
 * only have a few (e.g. one for DiskIO, and NULL for calculations).
 * Popping a task scales with the N^2 where N is the number of different
 *mutexes.
 *
 * @author Janik Zikovsky
 * @date 2011-02-25 16:39:43.233991
 */
class DLLExport ThreadSchedulerMutexes : public ThreadScheduler {
public:
  ThreadSchedulerMutexes() = default;

  ~ThreadSchedulerMutexes() override { clear(); }

  //-------------------------------------------------------------------------------
  void push(Task *newTask) override {
    // Cache the total cost
    std::lock_guard<std::mutex> lock(m_queueLock);
    m_cost += newTask->cost();

    boost::shared_ptr<std::mutex> mut = newTask->getMutex();
    m_supermap[mut].emplace(newTask->cost(), newTask);
  }

  //-------------------------------------------------------------------------------
  Task *pop(size_t threadnum) override {
    UNUSED_ARG(threadnum);

    Task *temp = nullptr;

    std::lock_guard<std::mutex> lock(m_queueLock);
    // Check the size within the same locking block; otherwise the size may
    // change before you get the next item.
    if (!m_supermap.empty()) {
      // We iterate in reverse as to take the NULL mutex last, even if no mutex
      // is busy
      for (auto &mutexedMap : m_supermap) {
        // The key is the mutex associated with the inner map
        boost::shared_ptr<std::mutex> mapMutex = mutexedMap.first;
        if ((!mapMutex) || (m_mutexes.empty()) ||
            (m_mutexes.find(mapMutex) == m_mutexes.end())) {
          // The mutex of this map is free!
          InnerMap &map = mutexedMap.second;

          if (!map.empty()) {
            // Look for the largest cost item in it.
            auto it2 = map.end();
            --it2;
            // Great, we found something.
            temp = it2->second;
            // Take it out of the map (popped)
            map.erase(it2);
            break;
          }
        }
      }
      if (temp == nullptr) {
        // Nothing was found, meaning all mutexes are in use
        // Try the first non-empty map
        for (auto &mutexedMap : m_supermap) {
          if (!mutexedMap.second.empty()) {
            InnerMap &map = mutexedMap.second;
            // Use the first one
            temp = map.begin()->second;
            // And erase that item (pop it)
            map.erase(map.begin());
            break;
          }
        }
      }
      // If temp is still NULL, then no tasks are left.
    }

    // --- Add the mutex (if any) to the list of "busy" ones ---
    if (temp) {
      boost::shared_ptr<std::mutex> mut = temp->getMutex();
      if (mut)
        m_mutexes.insert(mut);
    }

    return temp;
  }

  //-----------------------------------------------------------------------------------
  /** Signal to the scheduler that a task is complete.
   *
   * @param task :: the Task that was completed.
   * @param threadnum :: unused argument
   */
  void finished(Task *task, size_t threadnum) override {
    UNUSED_ARG(threadnum);
    boost::shared_ptr<std::mutex> mut = task->getMutex();
    if (mut) {
      std::lock_guard<std::mutex> lock(m_queueLock);
      // We take this mutex off the list of used ones.
      m_mutexes.erase(mut);
    }
  }

  //-------------------------------------------------------------------------------
  size_t size() override {
    std::lock_guard<std::mutex> lock(m_queueLock);
    // Add up the sizes of all contained maps.
    return std::accumulate(
        m_supermap.cbegin(), m_supermap.cend(), size_t{0},
        [](size_t total,
           const std::pair<boost::shared_ptr<std::mutex>, InnerMap>
               &mutexedMap) { return total + mutexedMap.second.size(); });
  }

  //-------------------------------------------------------------------------------
  /// @return true if the queue is empty
  bool empty() override {
    std::lock_guard<std::mutex> lock(m_queueLock);
    auto mapWithTasks = std::find_if_not(
        m_supermap.cbegin(), m_supermap.cend(),
        [](const std::pair<boost::shared_ptr<std::mutex>, InnerMap>
               &mutexedMap) { return mutexedMap.second.empty(); });
    return mapWithTasks == m_supermap.cend();
  }

  //-------------------------------------------------------------------------------
  void clear() override {
    std::lock_guard<std::mutex> lock(m_queueLock);

    // Empty out the queue and delete the pointers!
    for (auto &it : m_supermap) {
      InnerMap &map = it.second;
      for (auto &it2 : map)
        delete it2.second;
      map.clear();
    }
    m_supermap.clear();
    m_cost = 0;
    m_costExecuted = 0;
  }

protected:
  /// Map to tasks, sorted by cost
  using InnerMap = std::multimap<double, Task *>;
  /// Map to maps, sorted by Mutex*
  using SuperMap = std::map<boost::shared_ptr<std::mutex>, InnerMap>;

  /** A super map; first key = a Mutex *
   * Inside it: second key = the cost. */
  SuperMap m_supermap;

  /// Vector of currently used mutexes.
  std::set<boost::shared_ptr<std::mutex>> m_mutexes;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_THREADSCHEDULERMUTEXES_H_ */
