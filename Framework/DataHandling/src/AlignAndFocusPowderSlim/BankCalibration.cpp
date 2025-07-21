#include "BankCalibration.h"
#include <algorithm>
#include <stdexcept>

namespace Mantid::DataHandling {

// ------------------------ BankCalibration object
/**
 * Calibration of a subset of pixels as requested in the constructor. This is used because a vector is faster lookup
 * than a map for dense array of values.
 *
 * @param idmin Minimum detector id to include in the calibration
 * @param idmax Maximum detector id to include in the calibration
 * @param time_conversion Value to bundle into the calibration constant to account for converting the time-of-flight
 * into microseconds. Applying it here is effectively the same as applying it to each event time-of-flight.
 * @param calibration_map Calibration for the entire instrument.
 * @param mask detector ids that exist in the map should not be included.
 */
BankCalibration::BankCalibration(const detid_t idmin, const detid_t idmax, const double time_conversion,
                                 const std::map<detid_t, double> &calibration_map, const std::set<detid_t> &mask)
    : m_detid_offset(idmin) {
  // error check the id-range
  if (idmax < idmin)
    throw std::runtime_error("BAD!"); // TODO better message

  // allocate memory and set the default value to 1
  m_calibration.assign(static_cast<size_t>(idmax - idmin + 1), 1.);

  // set up iterators for copying data
  auto iter = calibration_map.find(idmin);
  if (iter == calibration_map.end())
    throw std::runtime_error("ALSO BAD!");
  auto iter_end = calibration_map.find(idmax);
  if (iter_end != calibration_map.end())
    ++iter_end;

  // copy over values that matter
  for (; iter != iter_end; ++iter) {
    const auto index = static_cast<size_t>(iter->first - m_detid_offset);
    m_calibration[index] = iter->second;
  }

  // apply time conversion here so it is effectively applied for each detector once rather than on each event
  if (time_conversion != 1.) {
    std::transform(m_calibration.begin(), m_calibration.end(), m_calibration.begin(),
                   [time_conversion](const auto &value) { return std::move(time_conversion * value); });
  }

  // setup the detector mask - this assumes there are not many pixels in the overall mask
  // TODO could benefit from using lower_bound/upper_bound on the input mask rather than all
  for (const auto &detid : mask) {
    if (detid >= idmin && detid <= idmax) {
      m_calibration[detid - m_detid_offset] = IGNORE_PIXEL;
    }
  }
}

/**
 * This assumes that everything is in range. Values that weren't in the calibration map get set to 1.
 */
const double &BankCalibration::value(const detid_t detid) const { return m_calibration[detid - m_detid_offset]; }

const detid_t &BankCalibration::idmin() const { return m_detid_offset; }
detid_t BankCalibration::idmax() const { return m_detid_offset + static_cast<detid_t>(m_calibration.size()) - 1; }
} // namespace Mantid::DataHandling
