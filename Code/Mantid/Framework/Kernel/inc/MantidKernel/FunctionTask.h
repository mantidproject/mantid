#ifndef MANTID_KERNEL_FUNCTIONTASK_H_
#define MANTID_KERNEL_FUNCTIONTASK_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Task.h"
#include <stdexcept>
#include "boost/bind.hpp"
#include "boost/function.hpp"

namespace Mantid
{

namespace Kernel
{

//==============================================================================================
/** A FunctionTask can easily create a Task from a method pointer.
 *
 * The FunctionTask will simply run the provided method.
 *
 * @author Janik Zikovsky, SNS
 * @date Feb 8, 2011
 */
class FunctionTask : public Task
{
public:

  /// Typedef for a function with no arguments and no return
  typedef void(*voidFunction)();

  //---------------------------------------------------------------------------------------------
  /** Constructor for a simple void function.
   *
   * Pro-tip: use boost::bind(f, argument1, argument2) (for example) to turn a function that takes
   * an argument into a argument-less function pointer.
   *
   * @param func :: pointer to a void function()
   * @param cost :: computational cost
   */
  FunctionTask(voidFunction func, double cost=1.0)
    : Task(cost), m_voidFunc(func)
  {
  }

  //---------------------------------------------------------------------------------------------
  /** Constructor for a simple boost bound function.
   *
   * Pro-tip: use boost::bind(f, argument1, argument2) (for example) to turn a function that takes
   * an argument into a argument-less function pointer.
   *
   * @param func :: boost::function<> returned by boost::bind()
   * @param cost :: computational cost
   */
  FunctionTask(boost::function<void ()> func, double cost=1.0)
    : Task(cost), m_voidFunc(func)
  {
  }

  //---------------------------------------------------------------------------------------------
  /** Main method that performs the work for the task. */
  virtual void run()
  {
    if (m_voidFunc != NULL)
      m_voidFunc();
    else
      throw std::runtime_error("FunctionTask: NULL method pointer provided.");
  }


protected:
  boost::function<void ()> m_voidFunc;
};


} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_FUNCTIONTASK_H_ */
