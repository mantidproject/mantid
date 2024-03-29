// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#ifndef Q_MOC_RUN
#include <memory>
#endif
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/Exception.h"
#include <mutex>

namespace Mantid {

namespace Kernel {

//==============================================================================================
/** A Task is a unit of work to be scheduled and run by a ThreadPool.
 *
 * This class is abstract and will be overridden.
 * Its main method is the run() method, which does the work.
 *
 * @author Janik Zikovsky, SNS
 * @date Feb 8, 2011
 */
class MANTID_KERNEL_DLL Task {
public:
  //---------------------------------------------------------------------------------------------
  /** Default constructor */
  Task() : m_cost(1.0) {}

  //---------------------------------------------------------------------------------------------
  /** Constructor with cost
   *
   * @param cost :: computational cost
   */
  Task(double cost) : m_cost(cost) {}

  /// Destructor
  virtual ~Task() = default;

  //---------------------------------------------------------------------------------------------
  /** Main method that performs the work for the task. */
  virtual void run() = 0;

  //---------------------------------------------------------------------------------------------
  /** What is the computational cost of this task?
   * @return a double that scales with the computational time.
   */
  virtual double cost() { return m_cost; }

  //---------------------------------------------------------------------------------------------
  /** Use an arbitrary pointer to lock (mutex) the execution of this task.
   * For example, you might point to a particular EventList in memory to signify
   *that
   * this task will operate on this object.
   *
   * @param object :: any pointer.
   */
  void setMutexObject(void *object) {
    UNUSED_ARG(object);
    throw Kernel::Exception::NotImplementedError("Not impl.");
  }

  //---------------------------------------------------------------------------------------------
  /** Get the mutex object for this Task
   * @return Mutex pointer, or NULL
   */
  std::shared_ptr<std::mutex> getMutex() { return m_mutex; }

  //---------------------------------------------------------------------------------------------
  /** Set the mutex object for this Task
   * @param mutex :: Mutex pointer, or NULL
   */
  void setMutex(const std::shared_ptr<std::mutex> &mutex) { m_mutex = mutex; }

protected:
  /// Cached computational cost for the thread.
  double m_cost;

  /// Mutex associated with this task (can be NULL)
  std::shared_ptr<std::mutex> m_mutex;
};

} // namespace Kernel
} // namespace Mantid
