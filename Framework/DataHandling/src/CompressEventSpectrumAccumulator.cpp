// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/CompressEventSpectrumAccumulator.h"
#include "MantidDataObjects/EventList.h"

using Mantid::DataObjects::EventList;

namespace Mantid {
namespace DataHandling {
CompressEventSpectrumAccumulator::CompressEventSpectrumAccumulator(
    std::shared_ptr<std::vector<double>> histogram_bin_edges, const double divisor, CompressBinningMode bin_mode)
    : m_divisor(abs(divisor)), m_histogram_edges(std::move(histogram_bin_edges)) {
  // divisor is applied to make it
  m_offset = static_cast<double>(m_histogram_edges->front()) / m_divisor;

  // create the fine histogram
  // all values start at zero because nothing has been accumulated
  const auto NUM_BINS = static_cast<size_t>(m_histogram_edges->size() - 1);
  m_tof.resize(NUM_BINS, 0.);
  m_count.resize(NUM_BINS, 0.);

  // setup function pointer for finding bins
  if (bin_mode == CompressBinningMode::LINEAR)
    m_findBin = EventList::findLinearBin;
  else
    throw std::runtime_error("Haven't implemented non-linear bins");
}

/**
 * This assumes that the event is within range of the fine histogram
 */
void CompressEventSpectrumAccumulator::addEvent(const float tof) {
  // if (tof < m_histogram_edges->front() || tof >= m_histogram_edges->back())
  // std::cout << "THIS SHOULD NOT GET PRINTED " << tof << "\n";

  const auto optional_bin = m_findBin(*m_histogram_edges.get(), static_cast<double>(tof), m_divisor, m_offset);
  if (optional_bin) {
    const auto bin = optional_bin.get();
    m_tof[bin] += tof;
    m_count[bin] += 1;
  }
}

void CompressEventSpectrumAccumulator::createWeightedEvents(
    std::vector<Mantid::DataObjects::WeightedEventNoTime> *raw_events) const {
  //  raw_events->clear(); // clean out previous version
  raw_events->reserve(this->numberWeightedEvents());

  // loop over bins and create events when appropriate
  const auto NUM_BINS = this->numberHistBins();
  for (size_t i = 0; i < NUM_BINS; ++i) {
    if (const auto counts = m_count[i] > 0) {
      const double weight = static_cast<double>(counts);
      const double tof = static_cast<double>(m_tof[i]) / weight;
      raw_events->emplace_back(tof, weight, weight);
    }
  }
}

/**
 * The number of weighted events that will need to be created in the EventList
 */
std::size_t CompressEventSpectrumAccumulator::numberWeightedEvents() const {
  std::size_t numWgtEvents =
      std::accumulate(m_count.cbegin(), m_count.cend(), 0, [](const auto &current, const auto &value) {
        if (value > 0)
          return current + 1;
        else
          return current;
      });

  return numWgtEvents;
}

std::size_t CompressEventSpectrumAccumulator::numberHistBins() const { return m_count.size(); }

} // namespace DataHandling
} // namespace Mantid
