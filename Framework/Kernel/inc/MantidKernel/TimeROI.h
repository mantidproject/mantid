// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid {
namespace Kernel {

namespace {
// const std::string NAME{"roi"};
}

/** TimeROI : TODO: DESCRIPTION
 */
class MANTID_KERNEL_DLL TimeROI {
public:
  TimeROI();
  double durationInSeconds() const;
  void addROI(const std::string &startTime, const std::string &stopTime);
  void addROI(const Types::Core::DateAndTime &startTime, const Types::Core::DateAndTime &stopTime);
  void addROI(const std::time_t &startTime, const std::time_t &stopTime);

private:
  /**
   * @brief m_roi private member that holds most of the information
   *
   * This handles the details of the ROI and guarantees that views into the underlying structure is sorted by time.
   */
  TimeSeriesProperty<bool> m_roi;
};

} // namespace Kernel
} // namespace Mantid
