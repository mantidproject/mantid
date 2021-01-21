// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/DllConfig.h"
#include <vector>

namespace Mantid {
namespace Kernel {
namespace DateAndTimeHelpers {

MANTID_KERNEL_DLL Types::Core::DateAndTime createFromSanitizedISO8601(const std::string &date);

MANTID_KERNEL_DLL std::string verifyAndSanitizeISO8601(const std::string &date, bool displayWarnings = true);

MANTID_KERNEL_DLL Types::Core::DateAndTime averageSorted(const std::vector<Types::Core::DateAndTime> &times);

MANTID_KERNEL_DLL Types::Core::DateAndTime averageSorted(const std::vector<Types::Core::DateAndTime> &times,
                                                         const std::vector<double> &weights);

} // namespace DateAndTimeHelpers
} // namespace Kernel
} // namespace Mantid
