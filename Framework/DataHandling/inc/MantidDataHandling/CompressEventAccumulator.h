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

/** CompressEventAccumulator : TODO: DESCRIPTION
 */
class MANTID_DATAHANDLING_DLL CompressEventAccumulator {
public:
  // TODO parameter for expected number of events
  CompressEventAccumulator(std::shared_ptr<std::vector<double>> histogram_bin_edges, const double divisor,
                           CompressBinningMode bin_mode);

  virtual void addEvent(const float tof) = 0;
  virtual void createWeightedEvents(std::vector<Mantid::DataObjects::WeightedEventNoTime> *raw_events) const = 0;
  virtual void sort() const = 0;

  std::size_t numberHistBins() const;
  virtual double totalWeight() const = 0;

protected:
  template <typename INT_TYPE> double getBinCenter(const INT_TYPE bin) const;
  /// shared pointer for the histogram bin boundaries
  const std::shared_ptr<std::vector<double>> m_histogram_edges;
  /// keep track if the m_tof is already sorted
  // offset is applied after division
  // see EventList::findLinearBin for implementation on what that means
  double m_divisor;
  double m_offset;
  /// function pointer on how to find the bin boundaries
  boost::optional<size_t> (*m_findBin)(const MantidVec &, const double, const double, const double, const bool);

  /// track
  bool m_initialized;

public:
  size_t m_numevents; // REMOVE
};

/**
 * @brief The CompressEventAccumulatorFactory Factory object that will create the correct type of
 * CompressEventAccumulator based on configuration information.
 */
class MANTID_DATAHANDLING_DLL CompressEventAccumulatorFactory {
public:
  CompressEventAccumulatorFactory(std::shared_ptr<std::vector<double>> histogram_bin_edges, const double divisor,
                                  CompressBinningMode bin_mode);
  std::unique_ptr<CompressEventAccumulator> create();

private:
  double m_divisor;
  CompressBinningMode m_bin_mode;
  const std::shared_ptr<std::vector<double>> m_histogram_edges;
};

} // namespace DataHandling
} // namespace Mantid
