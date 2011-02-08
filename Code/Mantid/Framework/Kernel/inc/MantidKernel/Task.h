#ifndef MANTID_KERNEL_TASK_H_
#define MANTID_KERNEL_TASK_H_

#include "MantidKernel/System.h"

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
    {
    }

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


  protected:

    /// Cached computational cost for the thread.
    double m_cost;
  };


} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_TASK_H_ */
