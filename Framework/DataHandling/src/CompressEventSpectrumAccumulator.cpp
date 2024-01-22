// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/CompressEventSpectrumAccumulator.h"
#include "MantidDataObjects/EventList.h"

#include <tbb/parallel_sort.h>

using Mantid::DataObjects::EventList;

namespace Mantid {
namespace DataHandling {
CompressEventSpectrumAccumulator::CompressEventSpectrumAccumulator(
    std::shared_ptr<std::vector<double>> histogram_bin_edges, const double divisor, CompressBinningMode bin_mode)
    : m_histogram_edges(std::move(histogram_bin_edges)) {
  // divisor is applied to make it

  m_divisor = abs(divisor);
  m_offset = static_cast<double>(m_histogram_edges->front()) / m_divisor;

  const auto tof_min = static_cast<double>(m_histogram_edges->front());

  // setup function pointer  and parameters for finding bins
  // look in EventList for why
  if (bin_mode == CompressBinningMode::LINEAR) {
    m_findBin = EventList::findLinearBin;
    m_divisor = abs(divisor);
    m_offset = tof_min / m_divisor;
  } else if (bin_mode == CompressBinningMode::LOGARITHMIC) {
    m_findBin = EventList::findLogBin;
    m_divisor = log1p(abs(divisor)); // use this to do change of base
    m_offset = log(tof_min) / m_divisor;
  } else {
    throw std::runtime_error("Haven't implemented this compression binning strategy");
  }

  initialized = false;
  numevents = 0;
}

void CompressEventSpectrumAccumulator::allocateFineHistogram() {
  // create the fine histogram
  // all values start at zero because nothing has been accumulated
  m_tof.reserve(1678); // TODO

  // const auto NUM_BINS = static_cast<size_t>(m_histogram_edges->size() - 1);
  // m_tof.resize(NUM_BINS, 0.);
  // m_count.resize(NUM_BINS, 0.);
}

/**
 * This assumes that the event is within range of the fine histogram
 */
void CompressEventSpectrumAccumulator::addEvent(const float tof) {
  // if (tof < m_histogram_edges->front() || tof >= m_histogram_edges->back())
  // std::cout << "THIS SHOULD NOT GET PRINTED " << tof << "\n";

  if (!initialized) {
    this->allocateFineHistogram();
    initialized = true;
  }
  // add events
  m_tof.push_back(tof);

  /*
 const auto bin_optional = m_findBin(*m_histogram_edges.get(), static_cast<double>(tof), m_divisor, m_offset);
 if (bin_optional) {
   const auto bin = bin_optional.get();
    m_count[bin] += 1;
    m_tof_sparse[bin] += tof;
 }
 */

  numevents++;
}

void CompressEventSpectrumAccumulator::createWeightedEvents(
    std::vector<Mantid::DataObjects::WeightedEventNoTime> *raw_events) const {
  if (m_tof.empty())
    return;

  // code below assumes the tofs are sorted
  tbb::parallel_sort(m_tof.begin(), m_tof.end());

  raw_events->clear();                    // clean out previous version
  raw_events->reserve(m_tof.size() / 10); // blindly assume 10 raw events get compressed on average

  const auto optional_bin =
      m_findBin(*m_histogram_edges.get(), static_cast<double>(m_tof.front()), m_divisor, m_offset);
  size_t lastBin = optional_bin.get();
  double nextTof = m_histogram_edges->at(lastBin + 1);
  double counts = 0;
  double total_tof = 0.;
  for (const auto &tof : m_tof) {
    if (lastBin + 1 >= m_histogram_edges->size()) {
      break; // this should never happen
    }

    if (tof < nextTof) {
      // accumulate information
      counts++;
      total_tof += tof;
    } else {
      // add weighted event
      raw_events->emplace_back(total_tof / counts, counts, counts);
      // reset accumulators to include this new value
      counts = 1;
      total_tof = tof;
      // advance the last bin value
      const auto optional_bin = m_findBin(*m_histogram_edges.get(), static_cast<double>(tof), m_divisor, m_offset);
      lastBin = optional_bin.get();
      nextTof = m_histogram_edges->at(lastBin + 1);
    }
  }
  // add in the last one if it is there
  if (counts > 0.) {
    raw_events->emplace_back(total_tof / counts, counts, counts);
  }

  // drop extra space if there is any
  raw_events->shrink_to_fit();
}

std::size_t CompressEventSpectrumAccumulator::numberHistBins() const { return m_count.size(); }
double CompressEventSpectrumAccumulator::totalWeight() const { return static_cast<double>(m_tof.size()); }

} // namespace DataHandling
} // namespace Mantid
