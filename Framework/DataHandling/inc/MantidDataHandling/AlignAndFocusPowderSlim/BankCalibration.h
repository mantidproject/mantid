// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "MantidDataHandling/DllConfig.h"
#include "MantidGeometry/IDTypes.h"
#include <map>
#include <set>
#include <utility>
#include <vector>

namespace Mantid::DataHandling::AlignAndFocusPowderSlim {

constexpr double IGNORE_PIXEL{1.e6};

/**
 * Class that handles all the calibration constants for a bank of detectors. Accessing values DOES NO RANGE CHECKING so
 * only request values within the range of detector IDs supplied to the constructor.
 */
class MANTID_DATAHANDLING_DLL BankCalibration {
public:
  BankCalibration(const double time_conversion, const std::set<detid_t> &det_in_group,
                  const std::map<detid_t, double> &calibration_map, const std::map<detid_t, double> &scale_at_sample,
                  const std::set<detid_t> &mask);

  const double &value_calibration(const detid_t detid) const;
  /**
   * This returns a value with no bounds checking. If scale_at_sample is not provided, this will have undefined
   * behavior. This value should not be used if the detector is masked.
   */
  double value_scale_at_sample(const detid_t detid) const;
  const detid_t &idmin() const;
  detid_t idmax() const;
  bool empty() const;

private:
  bool detidInRange(const detid_t detid) const;

  std::vector<double> m_calibration;
  std::vector<double> m_scale_at_sample;
  detid_t m_detid_offset;
};

class MANTID_DATAHANDLING_DLL BankCalibrationFactory {
public:
  BankCalibrationFactory(const std::map<detid_t, double> &calibration_map,
                         const std::map<detid_t, double> &scale_at_sample,
                         const std::map<size_t, std::set<detid_t>> &grouping, const std::set<detid_t> &mask,
                         const std::map<size_t, std::set<detid_t>> &bank_detids);

  /**
   * Select which detector ids go into the output group. This resets the internal state of the BankCalibration.
   * @param time_conversion Value to bundle into the calibration constant to account for converting the time-of-flight
   * into microseconds. Applying it here is effectively the same as applying it to each event time-of-flight.
   * @param bank_index Output group for finding grouping information.
   */
  BankCalibration getCalibration(const double time_conversion, const size_t bank_index) const;
  std::vector<BankCalibration> getCalibrations(const double time_conversion, const size_t bank_index) const;

private:
  const std::map<detid_t, double> &m_calibration_map;    ///< detid: difc/difc_focussed
  const std::map<detid_t, double> &m_scale_at_sample;    ///< multiplicative 0<value<1 to move neutron TOF at sample
  const std::map<size_t, std::set<detid_t>> &m_grouping; ///< detector ids for output workspace index
  const std::set<detid_t> &m_mask;
  const std::map<size_t, std::set<detid_t>> &m_bank_detids;
};
} // namespace Mantid::DataHandling::AlignAndFocusPowderSlim
