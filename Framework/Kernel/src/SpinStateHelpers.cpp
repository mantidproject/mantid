// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidKernel/SpinStateHelpers.h"
#include "MantidKernel/StringTokenizer.h"

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <vector>

namespace Mantid {
namespace Kernel {
namespace SpinStateHelpers {

/*
For a given workspace group, spin state order, and desired spin state, this method will
return the index of the specified workspace in the group, using the position of the desired spin
state in the spin state order.
*/
std::optional<size_t> indexOfWorkspaceForSpinState(const std::vector<std::string> &spinStateOrder,
                                                   std::string targetSpinState) {
  boost::trim(targetSpinState);
  size_t const index =
      std::find(spinStateOrder.cbegin(), spinStateOrder.cend(), targetSpinState) - spinStateOrder.cbegin();
  if (index == spinStateOrder.size()) {
    return std::nullopt;
  }
  return index;
}

/*
For a given spin state input string of the form e.g. "01,11,00,10", split the string
into a vector of individual spin states. This will also trim any leading/trailing
whitespace in the individual spin states.
*/
std::vector<std::string> splitSpinStateString(const std::string &spinStates) {
  StringTokenizer tokens{spinStates, ",", StringTokenizer::TOK_TRIM};
  return std::vector<std::string>{tokens.begin(), tokens.end()};
}
} // namespace SpinStateHelpers
} // namespace Kernel
} // namespace Mantid
