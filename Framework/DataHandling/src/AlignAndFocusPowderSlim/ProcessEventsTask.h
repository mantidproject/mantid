// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "BankCalibration.h"
#include <tbb/tbb.h>
#include <vector>

namespace Mantid::DataHandling::AlignAndFocusPowderSlim {
class ProcessEventsTask {

public:
  ProcessEventsTask(const std::vector<uint32_t> *detids, const std::vector<float> *tofs,
                    const BankCalibration *calibration, const std::vector<double> *binedges);

  ProcessEventsTask(ProcessEventsTask &other, tbb::split);

  void operator()(const tbb::blocked_range<size_t> &range);

  void join(const ProcessEventsTask &other);

  /// Local histogram for this block/thread
  std::vector<uint32_t> y_temp;

private:
  const std::vector<uint32_t> *m_detids;
  const std::vector<float> *m_tofs;
  const BankCalibration *m_calibration;
  const std::vector<double> *m_binedges;
};
} // namespace Mantid::DataHandling::AlignAndFocusPowderSlim
