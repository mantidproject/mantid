#ifndef MANTID_KERNEL_TASK_H_
#define MANTID_KERNEL_TASK_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/MultiThreaded.h"


namespace Mantid
{

namespace Kernel
{

  //==============================================================================================
  /** A Task is a unit of work to be scheduled and run by a ThreadPool.
   *
   * This class is abstract and will be overridden.
   * Its main method is the run() method, which does the work.
   *
   * @author Janik Zikovsky, SNS
   * @date Feb 8, 2011
   */
  class Task
  {
  public:
    //---------------------------------------------------------------------------------------------
    /** Default constructor */
    Task() :
      m_cost(1.0)
    { }

    //---------------------------------------------------------------------------------------------
    /** Constructor with cost
     *
     * @param cost :: computational cost
     */
    Task(double cost) :
      m_cost(cost)
    { }

    //---------------------------------------------------------------------------------------------
    /** Main method that performs the work for the task. */
    virtual void run() = 0;

    //---------------------------------------------------------------------------------------------
    /** What is the computational cost of this task?
     * @return a double that scales with the computational time.
     */
    virtual double cost()
    {
      return m_cost;
    }

    //---------------------------------------------------------------------------------------------
    /** Use an arbitrary pointer to lock (mutex) the execution of this task.
     * For example, you might point to a particular EventList in memory to signify that
     * this task will operate on this object.
     *
     * @param object :: any pointer.
     */
    void setMutexObject(void * object)
    {
      throw Kernel::Exception::NotImplementedError("Not impl.");
    }


  protected:

    /// Cached computational cost for the thread.
    double m_cost;
  };


} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_TASK_H_ */
