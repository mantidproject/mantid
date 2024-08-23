// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid::Algorithms {
namespace PolarizationCorrectionsHelpers {
MANTID_ALGORITHMS_DLL API::MatrixWorkspace_sptr workspaceForSpinState(API::WorkspaceGroup_sptr group,
                                                                      const std::string &spinStateOrder,
                                                                      const std::string &targetSpinState);
MANTID_ALGORITHMS_DLL size_t indexOfWorkspaceForSpinState(const std::string &spinStateOrder,
                                                          const std::string &targetSpinState);
MANTID_ALGORITHMS_DLL std::vector<std::string> splitSpinStateString(const std::string &spinStates);
} // namespace PolarizationCorrectionsHelpers
} // namespace Mantid::Algorithms