// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/AlgoTimeRegister.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace API {

//---------------------------------------------------------------------------------------------
/** The actions to be performed by the algorithm on a dataset. This method is
 *  invoked for top level algorithms by the application manager.
 *  This method invokes exec() method.
 *  For Child Algorithms either the execute() method or exec() method
 *  must be EXPLICITLY invoked by the parent algorithm.
 *
 *  @throw runtime_error Thrown if algorithm or Child Algorithm cannot be
 *executed
 *  @return true if executed successfully.
 */
bool Algorithm::execute() {
  Instrumentation::AlgoTimeRegister::Instance();
  Instrumentation::AlgoTimeRegisterImpl::Dump dmp(name());
  return executeInternal();
}
void Algorithm::addTimer(const std::string &name, const Kernel::time_point_ns &begin,
                         const Kernel::time_point_ns &end) {
  Instrumentation::AlgoTimeRegister::Instance().addTime(name, begin, end);
}
} // namespace API
} // namespace Mantid
