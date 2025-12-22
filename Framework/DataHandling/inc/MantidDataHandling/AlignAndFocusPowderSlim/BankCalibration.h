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
  BankCalibration(const double time_conversion, const std::vector<detid_t> &det_in_group,
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

private:
  const std::pair<detid_t, detid_t> getDetidRange(const std::vector<detid_t> &det_in_group);
  const std::pair<detid_t, detid_t> getDetidRange(const std::map<detid_t, double> &calibration_map);
  std::vector<double> m_calibration;
  std::vector<double> m_scale_at_sample;
  detid_t m_detid_offset;
};

class BankCalibrationFactory {
public:
  BankCalibrationFactory(const std::map<detid_t, double> &calibration_map,
                         const std::map<detid_t, double> &scale_at_sample,
                         const std::map<size_t, std::vector<detid_t>> &grouping, const std::set<detid_t> &mask);

  /**
   * Select which detector ids go into the output group. This resets the internal state of the BankCalibration.
   * @param det_in_group Specifying this effectively reinitializes all of the internal data for what pixels will be used
   */
  BankCalibration getCalibration(const double time_conversion, const size_t wksp_index);

private:
  const std::map<detid_t, double> &m_calibration_map;
  const std::map<detid_t, double> &m_scale_at_sample;
  const std::map<size_t, std::vector<detid_t>> &m_grouping; ///< detector ids for output workspace index
  const std::set<detid_t> &m_mask;
};
} // namespace Mantid::DataHandling::AlignAndFocusPowderSlim
