#ifndef THREADSCHEDULER_H_
#define THREADSCHEDULER_H_

#include "MantidKernel/SingletonHolder.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Task.h"
#include <vector>
#include <deque>
#include <map>

namespace Mantid
{

namespace Kernel
{

  /** The ThreadScheduler object defines how tasks are
   * allocated to threads and in what order.
   * It holds the queue of tasks.
   *
    @author Janik Zikovsky, SNS
    @date Feb 7, 2011
  
    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory
  
    This file is part of Mantid.
  
    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.
  
    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
  
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
  
    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
   */

  //===========================================================================
  //===========================================================================
  //===========================================================================
  class DLLExport ThreadScheduler
  {
  public:
    /** Constructor
     */
    ThreadScheduler()
    : m_cost(0), m_costExecuted(0)
    {
    }
  
    /** Add a Task to the queue.
     * @param newTask :: Task to add to queue
     */
    virtual void push(Task * newTask) = 0;
  
    /// Retrieves the next Task to execute.
    virtual Task * pop() = 0;
  
    /// Returns the size of the queue
    virtual size_t size() = 0;

    /// Empty out the queue
    virtual void clear() = 0;


    //-------------------------------------------------------------------------------
    /// Returns the total cost of all Task's in the queue.
    double totalCost()
    {
      return m_cost;
    }

    //-------------------------------------------------------------------------------
    /// Returns the total cost of all Task's in the queue.
    double totalCostExecuted()
    {
      return m_costExecuted;
    }

  protected:
    /// Total cost of all tasks
    double m_cost;
    /// Accumulated cost of tasks that have been executed (popped)
    double m_costExecuted;

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
  class DLLExport ThreadSchedulerFIFO : public ThreadScheduler
  {
  public:
    ThreadSchedulerFIFO()
      : ThreadScheduler()
    {}
  
    //-------------------------------------------------------------------------------
    void push(Task * newTask)
    {
      // Cache the total cost
      m_cost += newTask->cost();
      //TODO: Thread-safety
      m_queue.push_back(newTask);
    }

    //-------------------------------------------------------------------------------
    virtual Task * pop()
    {
      if (m_queue.size() > 0)
      {
        //TODO: Thread-safety
        Task * temp = m_queue.front();
        m_queue.pop_front();
        return temp;
      }
    }
  
    //-------------------------------------------------------------------------------
    size_t size()
    {
      return m_queue.size();
    }
  
    //-------------------------------------------------------------------------------
    void clear()
    {
      m_queue.clear();
      m_cost = 0;
      m_costExecuted = 0;
    }

  protected:
    std::deque<Task*> m_queue;
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
  class DLLExport ThreadSchedulerLIFO : public ThreadSchedulerFIFO
  {

    //-------------------------------------------------------------------------------
    Task * pop()
    {
      if (m_queue.size() > 0)
      {
        //TODO: Thread-safety
        Task * temp = m_queue.back();
        m_queue.pop_back();
        return temp;
      }
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
  class DLLExport ThreadSchedulerLargestCost : public ThreadScheduler
  {
  public:
    ThreadSchedulerLargestCost()
      : ThreadScheduler()
    {}

    //-------------------------------------------------------------------------------
    void push(Task * newTask)
    {
      // Cache the total cost
      m_cost += newTask->cost();
      //TODO: Thread-safety
      m_map.insert( std::pair<double, Task*>(newTask->cost(), newTask) );
    }

    //-------------------------------------------------------------------------------
    virtual Task * pop()
    {
      //TODO: Thread-safety
      if (m_map.size() > 0)
      {
        // Since the map is sorted by cost, we want the LAST item.
        std::multimap<double, Task*>::iterator it = m_map.end();
        it--;
        Task * temp = it->second;
        m_map.erase(it);
        return temp;
      }
      else
        return NULL;
    }

    //-------------------------------------------------------------------------------
    size_t size()
    {
      return m_map.size();
    }

    //-------------------------------------------------------------------------------
    void clear()
    {
      m_map.clear();
      m_cost = 0;
      m_costExecuted = 0;
    }

  protected:
    /// A multimap keeps tasks sorted by the key (cost)
    std::multimap<double, Task*> m_map;
  };






} // namespace Kernel
} // namespace Mantid

#endif /* THREADSCHEDULER_H_ */
