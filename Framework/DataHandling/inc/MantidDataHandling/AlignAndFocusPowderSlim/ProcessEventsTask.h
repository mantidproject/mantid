// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "MantidDataHandling/AlignAndFocusPowderSlim/BankCalibration.h"
#include <ranges>
#include <tbb/tbb.h>
#include <vector>

namespace Mantid::DataHandling::AlignAndFocusPowderSlim {

template <typename DetIDsVector, typename TofVector> class ProcessEventsTask {
public:
  ProcessEventsTask(DetIDsVector *detids, TofVector *tofs, const BankCalibration *calibration,
                    const std::vector<double> *binedges)
      : y_temp(binedges->size() - 1, 0), m_detids(detids), m_tofs(tofs), m_calibration(calibration),
        m_binedges(binedges) {}

  ProcessEventsTask(ProcessEventsTask &other, tbb::split)
      : y_temp(other.y_temp.size(), 0), m_detids(other.m_detids), m_tofs(other.m_tofs),
        m_calibration(other.m_calibration), m_binedges(other.m_binedges) {}

  void operator()(const tbb::blocked_range<size_t> &range) {
    if (m_calibration->empty()) {
      return;
    }
    // Cache values to reduce number of function calls
    const auto &range_end = range.end();
    const auto &binedges_cbegin = m_binedges->cbegin();
    const auto &binedges_cend = m_binedges->cend();
    const auto &tof_min = m_binedges->front();
    const auto &tof_max = m_binedges->back();

    // Calibrate and histogram the data
    auto detid_iter = std::ranges::next(m_detids->begin(), range.begin());
    auto tof_iter = std::ranges::next(m_tofs->begin(), range.begin());
    for (size_t i = range.begin(); i < range_end; ++i) {
      const auto &detid = *detid_iter;
      const auto &calib_factor = m_calibration->value_calibration(detid);
      if (calib_factor < IGNORE_PIXEL) {
        // Apply calibration
        const double &tof = static_cast<double>(*tof_iter) * calib_factor;
        if ((tof < tof_max) && (!(tof < tof_min))) { // check against max first to allow skipping second
          // Find the bin index using binary search
          const auto &it = std::upper_bound(binedges_cbegin, binedges_cend, tof);

          // Increment the count if a bin was found
          const auto &bin = static_cast<size_t>(std::distance(binedges_cbegin, it) - 1);
          y_temp[bin]++;
        }
      }
      ++detid_iter;
      ++tof_iter;
    }
  }

  void join(const ProcessEventsTask &other) {
    // Combine local histograms
    std::transform(y_temp.begin(), y_temp.end(), other.y_temp.cbegin(), y_temp.begin(), std::plus<>{});
  }

  /// Local histogram for this block/thread
  std::vector<uint32_t> y_temp;

private:
  DetIDsVector *m_detids;
  TofVector *m_tofs;
  const BankCalibration *m_calibration;
  const std::vector<double> *m_binedges;
};

} // namespace Mantid::DataHandling::AlignAndFocusPowderSlim
