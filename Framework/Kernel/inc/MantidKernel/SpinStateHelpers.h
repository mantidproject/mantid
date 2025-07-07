// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "MantidKernel/DllConfig.h"

#include <optional>
#include <string>
#include <vector>

namespace Mantid::Kernel {
namespace SpinStateHelpers {
MANTID_KERNEL_DLL std::optional<size_t> indexOfWorkspaceForSpinState(const std::vector<std::string> &spinStateOrder,
                                                                     std::string targetSpinState);
MANTID_KERNEL_DLL std::vector<std::string> splitSpinStateString(const std::string &spinStates);
} // namespace SpinStateHelpers
} // namespace Mantid::Kernel
