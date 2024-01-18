// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataHandling/DllConfig.h"
#include "MantidDataObjects/EventList.h"

#include <vector>

namespace Mantid {
namespace DataHandling {

enum class CompressBinningMode { LINEAR, LOGARITHMIC };

/** CompressEventSpectrumAccumulator : TODO: DESCRIPTION
 */
class MANTID_DATAHANDLING_DLL CompressEventSpectrumAccumulator {
public:
  // TODO parameter for expected number of events
  CompressEventSpectrumAccumulator(std::shared_ptr<std::vector<double>> histogram_bin_edges, const double divisor,
                                   CompressBinningMode bin_mode);

  void addEvent(const float tof);
  void createWeightedEvents(std::vector<Mantid::DataObjects::WeightedEventNoTime> *raw_events) const;

  std::size_t numberHistBins() const;
  double totalWeight() const;

private:
  /// Do not allow constructing
  CompressEventSpectrumAccumulator();
  void allocateFineHistogram();

  // offset is applied after division
  // see EventList::findLinearBin for implementation on what that means
  double m_divisor;
  double m_offset;

  /// function pointer on how to find the bin boundaries
  boost::optional<size_t> (*m_findBin)(const MantidVec &, const double, const double, const double);

  /// shared pointer for the histogram bin boundaries
  const std::shared_ptr<std::vector<double>> m_histogram_edges;
  /// sum of all time-of-flight within the bin
  mutable std::vector<float> m_tof;
  /// sum of all events seen in an individual bin
  std::vector<uint32_t> m_count;
  /// track
  bool initialized;

public:
  size_t numevents; // REMOVE
};

} // namespace DataHandling
} // namespace Mantid
