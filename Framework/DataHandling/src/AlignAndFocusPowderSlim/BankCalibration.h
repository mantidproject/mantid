#pragma once

#include "MantidGeometry/IDTypes.h"
#include <map>
#include <set>
#include <vector>

namespace Mantid::DataHandling::AlignAndFocusPowderSlim {

constexpr double IGNORE_PIXEL{1.e6};

class BankCalibration {
public:
  BankCalibration(const detid_t idmin, const detid_t idmax, const double time_conversion,
                  const std::map<detid_t, double> &calibration_map, const std::set<detid_t> &mask);
  const double &value(const detid_t detid) const;
  const detid_t &idmin() const;
  detid_t idmax() const;

private:
  std::vector<double> m_calibration;
  const detid_t m_detid_offset;
};
} // namespace Mantid::DataHandling::AlignAndFocusPowderSlim
