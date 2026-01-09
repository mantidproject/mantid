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
  auto iter = std::find_if(map_values.cbegin(), map_values.cend(),
                           [idmin](const auto &itervalue) { return itervalue.first >= idmin; });
  if (iter == map_values.end())
    throw std::runtime_error("Failed to find idmin=" + std::to_string(idmin) + " in map");
  auto iter_end =
      std::find_if(iter, map_values.cend(), [idmax](const auto itervalue) { return itervalue.first > idmax; });

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
 * @param time_conversion Value to bundle into the calibration constant to account for converting the time-of-flight
 * into microseconds. Applying it here is effectively the same as applying it to each event time-of-flight.
 * @param det_in_group Detector-ids that are to be used in this group
 * @param calibration_map Calibration for the entire instrument.
 * @param scale_at_sample Scalar factor to multiply the time-of-flight by to get the time-of-flight of the neutron back
 * at the sample position.
 * @param mask detector ids that exist in the map should not be included.
 */
BankCalibration::BankCalibration(const double time_conversion, const std::set<detid_t> &det_in_group,
                                 const std::map<detid_t, double> &calibration_map,
                                 const std::map<detid_t, double> &scale_at_sample, const std::set<detid_t> &mask) {
  if (det_in_group.empty())
    return;

  // determine the range
  const detid_t idmin_found = *det_in_group.begin();
  const detid_t idmax_found = *std::prev(det_in_group.end());

  // all the outputs are vectors that are offset by the minimum detid in the bank
  m_detid_offset = idmin_found;

  // get the values copied over for calibration
  copy_values_from_map_to_offset_vector(calibration_map, idmin_found, idmax_found, m_calibration);
  // apply time conversion here so it is effectively applied for each detector once rather than on each event
  if (time_conversion != 1.) {
    std::transform(m_calibration.begin(), m_calibration.end(), m_calibration.begin(),
                   [time_conversion](const auto &value) { return std::move(time_conversion * value); });
  }

  // get the values copied over for scale_at_sample
  if (!scale_at_sample.empty())
    copy_values_from_map_to_offset_vector(scale_at_sample, idmin_found, idmax_found, m_scale_at_sample);

  // mask values that are in the mask or not in the detector group

  // mask anything that isn't in the group this assumes both are sorted
  // find the first and last detector id that is in the range being used
  for (size_t i = 0; i < m_calibration.size(); ++i) {
    if (m_calibration[i] != IGNORE_PIXEL) {
      const detid_t detid = static_cast<detid_t>(i) + m_detid_offset;
      if (!det_in_group.contains(detid) || mask.contains(detid)) {
        m_calibration[i] = IGNORE_PIXEL; // not in the list or masked
      }
    }
  }
}

bool BankCalibration::detidInRange(const detid_t detid) const {
  return (!(detid < this->idmin() || detid > this->idmax())); // doesn't always check both
}

/**
 * This assumes that everything is in range. Values that weren't in the calibration map get set to 1. Values that are to
 * be masked will be set to IGNORE_PIXEL.
 */
const double &BankCalibration::value_calibration(const detid_t detid) const {
  if (this->detidInRange(detid))
    return m_calibration[detid - m_detid_offset];
  else
    return IGNORE_PIXEL;
}

double BankCalibration::value_scale_at_sample(const detid_t detid) const {
  if (this->detidInRange(detid))
    return m_scale_at_sample[detid - m_detid_offset];
  else
    return IGNORE_PIXEL;
}

const detid_t &BankCalibration::idmin() const { return m_detid_offset; }
detid_t BankCalibration::idmax() const { return m_detid_offset + static_cast<detid_t>(m_calibration.size()) - 1; }

bool BankCalibration::empty() const { return m_calibration.empty(); }

// ----------------- BankCalibrationFactory implementation
BankCalibrationFactory::BankCalibrationFactory(const std::map<detid_t, double> &calibration_map,
                                               const std::map<detid_t, double> &scale_at_sample,
                                               const std::map<size_t, std::set<detid_t>> &grouping,
                                               const std::set<detid_t> &mask,
                                               const std::map<size_t, std::set<detid_t>> &bank_detids)
    : m_calibration_map(calibration_map), m_scale_at_sample(scale_at_sample), m_grouping(grouping), m_mask(mask),
      m_bank_detids(bank_detids) {}

BankCalibration BankCalibrationFactory::getCalibration(const double time_conversion, const size_t bank_index) const {
  return BankCalibration(time_conversion, m_bank_detids.at(bank_index), m_calibration_map, m_scale_at_sample, m_mask);
}

std::vector<BankCalibration> BankCalibrationFactory::getCalibrations(const double time_conversion,
                                                                     const size_t bank_index) const {

  // When arbitrary grouping is used, we need to calculate the intersection of each group with the bank detids. The
  // intersection may be empty which means that no detectors from that bank are in this group.
  const auto &bank_detids = m_bank_detids.at(bank_index);

  std::vector<BankCalibration> calibrations;
  calibrations.reserve(m_grouping.size());

  std::transform(m_grouping.begin(), m_grouping.end(), std::back_inserter(calibrations),
                 [this, time_conversion, bank_detids](const auto &pair) {
                   std::set<detid_t> detids_intersection;
                   std::set_intersection(pair.second.begin(), pair.second.end(), bank_detids.begin(), bank_detids.end(),
                                         std::inserter(detids_intersection, detids_intersection.begin()));
                   return BankCalibration(time_conversion, detids_intersection, m_calibration_map, m_scale_at_sample,
                                          m_mask);
                 });

  return calibrations;
}

} // namespace Mantid::DataHandling::AlignAndFocusPowderSlim
