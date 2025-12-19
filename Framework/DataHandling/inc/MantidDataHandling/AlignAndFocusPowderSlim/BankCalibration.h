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
#include <vector>

namespace Mantid::DataHandling::AlignAndFocusPowderSlim {

constexpr double IGNORE_PIXEL{1.e6};

/**
 * Class that handles all the calibration constants for a bank of detectors. Accessing values DOES NO RANGE CHECKING so
 * only request values within the range of detector IDs supplied to the constructor.
 */
class MANTID_DATAHANDLING_DLL BankCalibration {
public:
  BankCalibration(const detid_t idmin, const detid_t idmax, const double time_conversion,
                  const std::vector<detid_t> &det_in_group, const std::map<detid_t, double> &calibration_map,
                  const std::map<detid_t, double> &scale_at_sample, const std::set<detid_t> &mask);
  const double &value_calibration(const detid_t detid) const;
  /**
   * This returns a value with no bounds checking. If scale_at_sample is not provided, this will have undefined
   * behavior. This value should not be used if the detector is masked.
   */
  double value_scale_at_sample(const detid_t detid) const;
  const detid_t &idmin() const;
  detid_t idmax() const;

private:
  std::vector<double> m_calibration;
  std::vector<double> m_scale_at_sample;
  const detid_t m_detid_offset;
};
} // namespace Mantid::DataHandling::AlignAndFocusPowderSlim
