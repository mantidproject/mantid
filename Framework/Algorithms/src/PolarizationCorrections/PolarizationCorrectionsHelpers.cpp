// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/PolarizationCorrections/PolarizationCorrectionsHelpers.h"

#include <boost/algorithm/string.hpp>
#include <vector>

namespace Mantid::Algorithms::PolarizationCorrectionsHelpers {
/*
For a given workspace group, spin state order, and desired spin state, this method will
extract the specified workspace from the group, using the position of the desired spin
state in the spin state order as the index of the workspace in the group.
*/
API::MatrixWorkspace_sptr WorkspaceForSpinState(API::WorkspaceGroup_sptr group, const std::string &spinStateOrder,
                                                const std::string &targetSpinState) {
  const auto wsIndex = IndexOfWorkspaceForSpinState(group, spinStateOrder, targetSpinState);
  return std::dynamic_pointer_cast<API::MatrixWorkspace>(group->getItem(wsIndex));
}

/*
For a given workspace group, spin state order, and desired spin state, this method will
return the index of the specified workspace from the group, using the position of the desired spin
state in the spin state order.
*/
size_t IndexOfWorkspaceForSpinState(API::WorkspaceGroup_sptr group, const std::string &spinStateOrder,
                                    const std::string &targetSpinState) {
  std::vector<std::string> spinStateVector;
  boost::split(spinStateVector, spinStateOrder, boost::is_any_of(","));
  return std::find(spinStateVector.cbegin(), spinStateVector.cend(), targetSpinState) - spinStateVector.cbegin();
}
} // namespace Mantid::Algorithms::PolarizationCorrectionsHelpers