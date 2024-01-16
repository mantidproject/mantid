// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataHandling/CompressEventSpectrumAccumulator.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataObjects/EventList.h"

#include <vector>

namespace Mantid {
namespace DataHandling {

/** CompressEventBankAccumulator : TODO: DESCRIPTION
 */
class MANTID_DATAHANDLING_DLL CompressEventBankAccumulator {
public:
  CompressEventBankAccumulator(detid_t min_detid, detid_t max_detid,
                               std::shared_ptr<std::vector<double>> histogram_bin_edges, const double divisor);

  void addEvent(const detid_t detid, const float tof);
  /// method only intended for testing
  std::size_t numberWeightedEvents() const;

private:
  // disable default constructor
  CompressEventBankAccumulator();

  std::vector<DataHandling::CompressEventSpectrumAccumulator> m_spectra_accum;

  // inclusive
  const detid_t m_detid_min;
  // inclusive
  const detid_t m_detid_max;
  // inclusive
  const float m_tof_min;
  // exclusive
  const float m_tof_max;
};

} // namespace DataHandling
} // namespace Mantid
