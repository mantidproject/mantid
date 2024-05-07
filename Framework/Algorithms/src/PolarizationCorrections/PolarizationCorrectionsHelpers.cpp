// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/PolarizationCorrections/PolarizationCorrectionsHelpers.h"
#include "MantidKernel/StringTokenizer.h"

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <vector>

namespace Mantid::Algorithms::PolarizationCorrectionsHelpers {
/*
For a given workspace group, spin state order, and desired spin state, this method will
extract the specified workspace from the group, using the position of the desired spin
state in the spin state order as the index of the workspace in the group.
*/
API::MatrixWorkspace_sptr workspaceForSpinState(API::WorkspaceGroup_sptr group, const std::string &spinStateOrder,
                                                const std::string &targetSpinState) {
  const auto wsIndex = indexOfWorkspaceForSpinState(spinStateOrder, targetSpinState);
  return std::dynamic_pointer_cast<API::MatrixWorkspace>(group->getItem(wsIndex));
}

/*
For a given workspace group, spin state order, and desired spin state, this method will
return the index of the specified workspace in the group, using the position of the desired spin
state in the spin state order.
*/
size_t indexOfWorkspaceForSpinState(const std::string &spinStateOrder, const std::string &targetSpinState) {
  std::vector<std::string> spinStateVector = splitSpinStateString(spinStateOrder);
  auto trimmedTargetSpinState = targetSpinState;
  boost::trim(trimmedTargetSpinState);
  return std::find(spinStateVector.cbegin(), spinStateVector.cend(), trimmedTargetSpinState) - spinStateVector.cbegin();
}

/*
For a given spin state input string of the form e.g. "01,11,00,10", split the string
into a vector of individual spin states. This will also trim any leading/trailing
whitespace in the individual spin states.
*/
std::vector<std::string> splitSpinStateString(const std::string &spinStates) {
  using Mantid::Kernel::StringTokenizer;
  StringTokenizer tokens{spinStates, ",", StringTokenizer::TOK_TRIM};
  return std::vector<std::string>{tokens.begin(), tokens.end()};
}
} // namespace Mantid::Algorithms::PolarizationCorrectionsHelpers