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
API::MatrixWorkspace_sptr WorkspaceForSpinState(API::WorkspaceGroup_sptr group, std::string spinStateOrder,
                                                std::string targetSpinState) {
  std::vector<std::string> spinStateVector;
  boost::split(spinStateVector, spinStateOrder, boost::is_any_of(","));
  const auto wsIndex =
      std::find(spinStateVector.cbegin(), spinStateVector.cend(), targetSpinState) - spinStateVector.cbegin();
  return std::dynamic_pointer_cast<API::MatrixWorkspace>(group->getItem(wsIndex));
}
} // namespace Mantid::Algorithms::PolarizationCorrectionsHelpers