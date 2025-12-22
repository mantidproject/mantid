// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/AlignAndFocusPowderSlim/BankCalibration.h"
#include <algorithm>
#include <stdexcept>

namespace Mantid::DataHandling::AlignAndFocusPowderSlim {

namespace {
void copy_values_from_map_to_offset_vector(const std::map<detid_t, double> &map_values, const detid_t idmin,
                                           const detid_t idmax, std::vector<double> &vector_values) {
  // allocate memory and set the default value to 1
  vector_values.assign(static_cast<size_t>(idmax - idmin + 1), 1.);

  // set up iterators for copying data - assumes ordered map
  auto iter = map_values.find(idmin);
  if (iter == map_values.end())
    throw std::runtime_error("Failed to find idmin=" + std::to_string(idmin) + " in map");
  auto iter_end = map_values.find(idmax);
  if (iter_end != map_values.end())
    ++iter_end;

  // copy over values that matter
  for (; iter != iter_end; ++iter) {
    const auto index = static_cast<size_t>(iter->first - idmin);
    vector_values[index] = iter->second;
  }
}
} // namespace

// ------------------------ BankCalibration object
/**
 * Calibration of a subset of pixels as requested in the constructor. This is used because a vector is faster lookup
 * than a map for dense array of values.
 *
 * @param idmin Minimum detector id to include in the calibration
 * @param idmax Maximum detector id to include in the calibration
 * @param time_conversion Value to bundle into the calibration constant to account for converting the time-of-flight
 * into microseconds. Applying it here is effectively the same as applying it to each event time-of-flight.
 * @param det_in_group Detector-ids that are to be used in this group
 * @param calibration_map Calibration for the entire instrument.
 * @param scale_at_sample Scalar factor to multiply the time-of-flight by to get the time-of-flight of the neutron back
 * at the sample position.
 * @param mask detector ids that exist in the map should not be included.
 */
BankCalibration::BankCalibration(const double time_conversion, const std::vector<detid_t> &det_in_group,
                                 const std::map<detid_t, double> &calibration_map,
                                 const std::map<detid_t, double> &scale_at_sample, const std::set<detid_t> &mask) {
  // determine the range
  detid_t idmin;
  detid_t idmax;
  if (det_in_group.empty()) {
    const auto idrange = getDetidRange(calibration_map);
    idmin = idrange.first;
    idmax = idrange.second;
  } else {
    const auto idrange = getDetidRange(det_in_group);
    idmin = idrange.first;
    idmax = idrange.second;
  }

  m_detid_offset = idmin;

  // get the values copied over for calibration
  copy_values_from_map_to_offset_vector(calibration_map, idmin, idmax, m_calibration);
  // apply time conversion here so it is effectively applied for each detector once rather than on each event
  if (time_conversion != 1.) {
    std::transform(m_calibration.begin(), m_calibration.end(), m_calibration.begin(),
                   [time_conversion](const auto &value) { return std::move(time_conversion * value); });
  }

  // get the values copied over for scale_at_sample
  if (!scale_at_sample.empty())
    copy_values_from_map_to_offset_vector(scale_at_sample, idmin, idmax, m_scale_at_sample);

  // setup the detector mask - this assumes there are not many pixels in the overall mask
  // TODO could benefit from using lower_bound/upper_bound on the input mask rather than all
  for (const auto &detid : mask) {
    if (detid >= idmin && detid <= idmax) {
      m_calibration[detid - m_detid_offset] = IGNORE_PIXEL;
    }
  }

  // mask anything that isn't int the group this assumes both are sorted
  if (!det_in_group.empty()) {
    // find the first and last detector id that is in the range being used
    auto detid_first = std::lower_bound(det_in_group.cbegin(), det_in_group.cend(), idmin);
    auto detid_last = det_in_group.cend();
    for (size_t i = 0; i < m_calibration.size(); ++i) {
      if (m_calibration[i] != IGNORE_PIXEL) {
        const detid_t detid = static_cast<detid_t>(i) + m_detid_offset;
        auto detid_found = std::find(detid_first, detid_last, detid);
        if (detid_found == detid_last)
          m_calibration[i] = IGNORE_PIXEL; // not in the list
        else
          detid_first = detid_found; // next search can start from here
      }
    }
  }
}

const std::pair<detid_t, detid_t> BankCalibration::getDetidRange(const std::vector<detid_t> &det_in_group) {
  detid_t idmin = det_in_group.front();
  detid_t idmax = det_in_group.back();
  return std::make_pair<detid_t, detid_t>(std::move(idmin), std::move(idmax));
}
const std::pair<detid_t, detid_t> BankCalibration::getDetidRange(const std::map<detid_t, double> &calibration_map) {
  detid_t idmin = calibration_map.begin()->first;
  detid_t idmax = calibration_map.rbegin()->first;
  return std::make_pair<detid_t, detid_t>(std::move(idmin), std::move(idmax));
}

/**
 * This assumes that everything is in range. Values that weren't in the calibration map get set to 1. Values that are to
 * be masked will be set to IGNORE_PIXEL.
 */
const double &BankCalibration::value_calibration(const detid_t detid) const {
  return m_calibration[detid - m_detid_offset];
}

double BankCalibration::value_scale_at_sample(const detid_t detid) const {
  return m_scale_at_sample[detid - m_detid_offset];
}

const detid_t &BankCalibration::idmin() const { return m_detid_offset; }
detid_t BankCalibration::idmax() const { return m_detid_offset + static_cast<detid_t>(m_calibration.size()) - 1; }

// ----------------- BankCalibrationFactory implementation
BankCalibrationFactory::BankCalibrationFactory(const std::map<detid_t, double> &calibration_map,
                                               const std::map<detid_t, double> &scale_at_sample,
                                               const std::map<size_t, std::vector<detid_t>> &grouping,
                                               const std::set<detid_t> &mask)
    : m_calibration_map(calibration_map), m_scale_at_sample(scale_at_sample), m_grouping(grouping), m_mask(mask) {}

BankCalibration BankCalibrationFactory::getCalibration(const double time_conversion, const size_t wksp_index) {
  if (m_grouping.empty()) {
    return BankCalibration(time_conversion, std::vector<detid_t>(), m_calibration_map, m_scale_at_sample, m_mask);
  } else {
    return BankCalibration(time_conversion, m_grouping.at(wksp_index), m_calibration_map, m_scale_at_sample, m_mask);
  }
}

} // namespace Mantid::DataHandling::AlignAndFocusPowderSlim
