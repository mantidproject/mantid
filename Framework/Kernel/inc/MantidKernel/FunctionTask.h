// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Task.h"
#include <functional>
#include <stdexcept>

#ifndef Q_MOC_RUN
#include <boost/function.hpp>
#include <utility>

#endif

namespace Mantid {

namespace Kernel {

//==============================================================================================
/** A FunctionTask can easily create a Task from a method pointer.
 *
 * The FunctionTask will simply run the provided method.
 *
 * @author Janik Zikovsky, SNS
 * @date Feb 8, 2011
 */
class MANTID_KERNEL_DLL FunctionTask final : public Task {
public:
  /// Typedef for a function with no arguments and no return
  using voidFunction = void (*)();
  //---------------------------------------------------------------------------------------------
  /** Constructor for a simple void function.
   *
   * Pro-tip: use std::bind(f, argument1, argument2) (for example) to turn a
   *function that takes
   * an argument into a argument-less function pointer.
   *
   * Use std::bind(&ClassName::function, &*this, arg1, arg2) to bind to a
   * class method of this.
   *
   * @param func :: pointer to a void function()
   * @param cost :: computational cost
   */
  FunctionTask(voidFunction func, double cost = 1.0) : Task(cost), m_voidFunc(func) {}

  //---------------------------------------------------------------------------------------------
  /** Constructor for a simple boost bound function.
   *
   * Pro-tip: use std::bind(f, argument1, argument2) (for example) to turn a
   *function that takes
   * an argument into a argument-less function pointer.
   *
   * Use std::bind(&ClassName::function, &*this, arg1, arg2) to bind to a
   *class method of this.
   *
   * @param func :: std::function<> returned by std::bind()
   * @param cost :: computational cost
   */
  FunctionTask(std::function<void()> func, double cost = 1.0) : Task(cost), m_voidFunc(std::move(func)) {}

  //---------------------------------------------------------------------------------------------
  /** Main method that performs the work for the task. */
  void run() override {
    if (m_voidFunc)
      m_voidFunc();
    else
      throw std::runtime_error("FunctionTask: NULL method pointer provided.");
  }

protected:
  std::function<void()> m_voidFunc;
};

} // namespace Kernel
} // namespace Mantid
