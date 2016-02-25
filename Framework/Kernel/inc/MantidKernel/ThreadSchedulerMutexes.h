#ifndef MANTID_KERNEL_THREADSCHEDULERMUTEXES_H_
#define MANTID_KERNEL_THREADSCHEDULERMUTEXES_H_

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/ThreadScheduler.h"
#include <mutex>
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
  ThreadSchedulerMutexes() : ThreadScheduler() {}

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
    if (m_supermap.size() > 0) {
      // We iterate in reverse as to take the NULL mutex last, even if no mutex
      // is busy
      SuperMap::iterator it = m_supermap.begin();
      SuperMap::iterator it_end = m_supermap.end();
      for (; it != it_end; ++it) {
        // The key is the mutex associated with the inner map
        boost::shared_ptr<std::mutex> mapMutex = it->first;
        if ((!mapMutex) || (m_mutexes.empty()) ||
            (m_mutexes.find(mapMutex) == m_mutexes.end())) {
          // The mutex of this map is free!
          InnerMap &map = it->second;

          if (map.size() > 0) {
            // Look for the largest cost item in it.
            InnerMap::iterator it2 = it->second.end();
            it2--;
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
        SuperMap::iterator it = m_supermap.begin();
        SuperMap::iterator it_end = m_supermap.end();
        for (; it != it_end; it++) {
          if (it->second.size() > 0) {
            InnerMap &map = it->second;
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
    size_t total = 0;
    SuperMap::iterator it = m_supermap.begin();
    SuperMap::iterator it_end = m_supermap.end();
    for (; it != it_end; it++)
      total += it->second.size();
    return total;
  }

  //-------------------------------------------------------------------------------
  /// @return true if the queue is empty
  bool empty() override {
    std::lock_guard<std::mutex> lock(m_queueLock);
    SuperMap::iterator it = m_supermap.begin();
    SuperMap::iterator it_end = m_supermap.end();
    for (; it != it_end; it++)
      if (!it->second.empty())
        return false;
    return true;
  }

  //-------------------------------------------------------------------------------
  void clear() override {
    std::lock_guard<std::mutex> lock(m_queueLock);

    // Empty out the queue and delete the pointers!
    SuperMap::iterator it = m_supermap.begin();
    SuperMap::iterator it_end = m_supermap.end();
    for (; it != it_end; it++) {
      InnerMap &map = it->second;
      InnerMap::iterator it2 = map.begin();
      InnerMap::iterator it2_end = map.end();
      for (; it2 != it2_end; it2++)
        delete it2->second;
      map.clear();
    }
    m_supermap.clear();
    m_cost = 0;
    m_costExecuted = 0;
  }

protected:
  /// Map to tasks, sorted by cost
  typedef std::multimap<double, Task *> InnerMap;
  /// Map to maps, sorted by Mutex*
  typedef std::map<boost::shared_ptr<std::mutex>, InnerMap> SuperMap;

  /** A super map; first key = a Mutex *
   * Inside it: second key = the cost. */
  SuperMap m_supermap;

  /// Vector of currently used mutexes.
  std::set<boost::shared_ptr<std::mutex>> m_mutexes;
};

} // namespace Mantid
} // namespace Kernel

#endif /* MANTID_KERNEL_THREADSCHEDULERMUTEXES_H_ */
