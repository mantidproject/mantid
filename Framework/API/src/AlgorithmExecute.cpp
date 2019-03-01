// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

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
bool Algorithm::execute() { return executeInternal(); }
} // namespace API
} // namespace Mantid
