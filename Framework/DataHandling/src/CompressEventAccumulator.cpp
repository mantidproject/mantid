// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/CompressEventAccumulator.h"
#include "MantidDataObjects/EventList.h"

#include <algorithm>
#include <tbb/parallel_sort.h>

using Mantid::DataObjects::EventList;

namespace Mantid {
namespace DataHandling {
CompressEventAccumulator::CompressEventAccumulator(std::shared_ptr<std::vector<double>> histogram_bin_edges,
                                                   const double divisor, CompressBinningMode bin_mode)
    : m_histogram_edges(std::move(histogram_bin_edges)), m_initialized(false), m_numevents(0) {
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
}

std::size_t CompressEventAccumulator::numberHistBins() const { return m_histogram_edges->size(); }

template <typename INT_TYPE> double CompressEventAccumulator::getBinCenter(const INT_TYPE bin) const {
  return 0.5 * (m_histogram_edges->operator[](bin) + m_histogram_edges->operator[](bin + 1));
}

boost::optional<size_t> CompressEventAccumulator::findBin(const float tof) const {
  // last parameter being false means don't find exact bin
  // for raw events coming out of the file it
  return m_findBin(*m_histogram_edges.get(), static_cast<double>(tof), m_divisor, m_offset, false);
}

// ------------------------------------------------------------------------
namespace { // anonymous

/**
 * Private class for implementation details of event collection, were bin centers are returned
 */
class CompressSparseInt : public CompressEventAccumulator {
public:
  // pass all arguments to the parent
  CompressSparseInt(std::shared_ptr<std::vector<double>> histogram_bin_edges, const size_t numEvents,
                    const double divisor, CompressBinningMode bin_mode)
      : CompressEventAccumulator(histogram_bin_edges, divisor, bin_mode), m_is_sorted(false) {
    m_tof_bin.reserve(numEvents); // TODO should be based on number of predicted events
    m_initialized = true;
  }

  double totalWeight() const override { return static_cast<double>(m_tof_bin.size()); }

  /**
   * This assumes that the event is within range of the fine histogram
   */
  void addEvent(const float tof) override {
    // if (tof < m_histogram_edges->front() || tof >= m_histogram_edges->back())
    //  std::cout << "THIS SHOULD NOT GET PRINTED " << tof << "\n";

    // add events
    const auto bin_optional = this->findBin(tof);
    if (bin_optional) {
      auto bin = static_cast<uint32_t>(bin_optional.get());
      m_tof_bin.push_back(std::move(bin));
      m_numevents++;
    }
  }

  // this is marked because it is mostly used by createWeightedEvents and m_tof is mutable
  void sort() const {
    if (m_is_sorted)
      return;
    if (m_tof_bin.size() < 2) {
      m_is_sorted = true;
      return;
    }

    // minimum size of a vector to bother doing parallel_sort
    // the grain size shouldn't get too small or the overhead of threading/sync overwhelms everything else
    // TODO this can be tuned
    constexpr size_t MIN_VEC_LENGTH{100000};

    if (m_tof_bin.size() < MIN_VEC_LENGTH)
      std::sort(m_tof_bin.begin(), m_tof_bin.end());
    else
      tbb::parallel_sort(m_tof_bin.begin(), m_tof_bin.end());
    m_is_sorted = true;
  }

  void createWeightedEvents(std::vector<Mantid::DataObjects::WeightedEventNoTime> *raw_events) const override {
    // clean out previous version
    raw_events->clear();

    if (m_tof_bin.empty()) {
      m_is_sorted = true;
      return; // nothing to do
    } else if (m_tof_bin.size() == 1) {
      m_is_sorted = true;
      // don't bother with temporary objects and such if there is only one event
      const auto tof = this->getBinCenter(m_tof_bin.front());
      raw_events->emplace_back(std::move(tof), 1., 1.);
    } else if (m_tof_bin.size() < 1000) {
      // this branch removes events as it accumulates them. It does not assume that the time-of-flight is sorted before
      // starting so the output will not be sorted either
      auto last_end = m_tof_bin.end();
      while (std::distance(m_tof_bin.begin(), last_end) > 1) {
        // start with the last value so there is less movement since remove moves everything to the end of the range
        const auto bin = *(last_end - 1); // one before the last end

        // move everything with this value to the end
        // vector::erase would finally remove the elements, but that takes more time
        auto new_end = std::remove(m_tof_bin.begin(), last_end, bin);

        // number of values moved is the number of counts - which are stored as float
        const auto counts = static_cast<float>(std::distance(new_end, last_end));

        // move the end to the new location
        last_end = new_end;

        // add the event
        const auto tof = this->getBinCenter(bin);
        raw_events->emplace_back(std::move(tof), counts, counts);
      }
    } else {
      // blindly assume 10 raw events are compressed to a single weighed event on average
      raw_events->reserve(m_tof_bin.size() / 10);

      // accumulation method is different for "large" number of events
      // if (m_tof_bin.size() > 1000) { // this number is fairly arbitrary and should be tuned somewhat
      this->sort(); // sort big lists

      // code below assumes the tofs are sorted

      auto iter = m_tof_bin.cbegin();
      const auto iter_end = m_tof_bin.cend();
      while (iter != iter_end) {
        auto range = std::equal_range(iter, iter_end, *iter);
        // distance counts one less than we need for counts - which are stored as float
        const auto counts = static_cast<float>(std::distance(range.first, range.second));
        if (counts > 0.) {
          const auto tof = this->getBinCenter(*iter);
          raw_events->emplace_back(std::move(tof), counts, counts);
        }
        iter = range.second;
      }
    }

    // drop extra space if the capacity is more than 10% of what is needed
    if (static_cast<double>(raw_events->capacity()) > 1.1 * static_cast<double>(raw_events->size()))
      raw_events->shrink_to_fit();
  }

  DataObjects::EventSortType getSortType() const override {
    if (m_is_sorted)
      return DataObjects::TOF_SORT;
    else
      return DataObjects::UNSORTED;
  }

private:
  mutable bool m_is_sorted;

  // time-of-flight bin this data would go into
  mutable std::vector<uint32_t> m_tof_bin;
};

/**
 * Private class for implementation details of dense event collection, when there are more events than bins
 */
class CompressDense : public CompressEventAccumulator {
public:
  // pass all arguments to the parent
  CompressDense(std::shared_ptr<std::vector<double>> histogram_bin_edges, const double divisor,
                CompressBinningMode bin_mode)
      : CompressEventAccumulator(histogram_bin_edges, divisor, bin_mode) {}

  double totalWeight() const override { return std::accumulate(m_count.cbegin(), m_count.cend(), 0.); }

  /**
   * This assumes that the event is within range of the fine histogram
   */
  void addEvent(const float tof) override {
    if (tof < m_histogram_edges->front() || tof >= m_histogram_edges->back())
      std::cout << "THIS SHOULD NOT GET PRINTED " << tof << "\n";

    if (!m_initialized) {
      this->allocateFineHistogram();
      m_initialized = true;
    }

    // add events
    const auto bin_optional = this->findBin(tof);
    if (bin_optional) {
      const auto bin = bin_optional.get();
      m_count[bin] += 1;
      m_numevents++;
    }
  }

  void createWeightedEvents(std::vector<Mantid::DataObjects::WeightedEventNoTime> *raw_events) const override {
    if (m_count.empty())
      return;

    const auto NUM_BINS = m_count.size();
    for (size_t i = 0; i < NUM_BINS; ++i) {
      // underlying weight is stored as float
      const auto counts = static_cast<float>(m_count[i]);
      if (counts > 0.) {
        const auto tof = this->getBinCenter(i);
        raw_events->emplace_back(std::move(tof), counts, counts);
      }
    }
    std::cout << "createWeightedEvents " << m_numevents << " -> " << raw_events->size() << "\n";
  }

  DataObjects::EventSortType getSortType() const override { return DataObjects::TOF_SORT; }

private:
  void allocateFineHistogram() {
    const auto NUM_BINS = static_cast<size_t>(m_histogram_edges->size() - 1);
    m_count.resize(NUM_BINS, 0.);
  }

  /// sum of all events seen in an individual bin
  std::vector<uint8_t> m_count;
};

} // namespace

// ------------------------------------------------------------------------

CompressEventAccumulatorFactory::CompressEventAccumulatorFactory(
    std::shared_ptr<std::vector<double>> histogram_bin_edges, const double divisor, CompressBinningMode bin_mode)
    : m_divisor(divisor), m_bin_mode(bin_mode), m_histogram_edges(std::move(histogram_bin_edges)) {}

std::unique_ptr<CompressEventAccumulator> CompressEventAccumulatorFactory::create(const std::size_t num_events) {
  if (num_events < m_histogram_edges->size()) {
    return std::make_unique<CompressSparseInt>(m_histogram_edges, num_events, m_divisor, m_bin_mode);
  } else {
    return std::make_unique<CompressDense>(m_histogram_edges, m_divisor, m_bin_mode);
  }
}

} // namespace DataHandling
} // namespace Mantid
