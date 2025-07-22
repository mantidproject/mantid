#include "ProcessEventsTask.h"

namespace Mantid::DataHandling::AlignAndFocusPowderSlim {

ProcessEventsTask::ProcessEventsTask(const std::vector<uint32_t> *detids, const std::vector<float> *tofs,
                                     const BankCalibration *calibration, const std::vector<double> *binedges)
    : y_temp(binedges->size() - 1, 0), m_detids(detids), m_tofs(tofs), m_calibration(calibration),
      m_binedges(binedges) {}

ProcessEventsTask::ProcessEventsTask(ProcessEventsTask &other, tbb::split)
    : y_temp(other.y_temp.size(), 0), m_detids(other.m_detids), m_tofs(other.m_tofs),
      m_calibration(other.m_calibration), m_binedges(other.m_binedges) {}

void ProcessEventsTask::operator()(const tbb::blocked_range<size_t> &range) {
  // Cache values to reduce number of function calls
  const auto &range_end = range.end();
  const auto &binedges_cbegin = m_binedges->cbegin();
  const auto &binedges_cend = m_binedges->cend();
  const auto &tof_min = m_binedges->front();
  const auto &tof_max = m_binedges->back();

  // Calibrate and histogram the data
  auto detid_iter = m_detids->cbegin() + range.begin();
  auto tof_iter = m_tofs->cbegin() + range.begin();
  for (size_t i = range.begin(); i < range_end; ++i) {
    const auto &detid = *detid_iter;
    const auto &calib_factor = m_calibration->value(detid);
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

void ProcessEventsTask::join(const ProcessEventsTask &other) {
  // Combine local histograms
  std::transform(y_temp.begin(), y_temp.end(), other.y_temp.cbegin(), y_temp.begin(), std::plus<>{});
}

} // namespace Mantid::DataHandling::AlignAndFocusPowderSlim
