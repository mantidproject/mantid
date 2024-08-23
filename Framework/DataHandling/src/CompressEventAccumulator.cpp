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
    : m_histogram_edges(std::move(histogram_bin_edges)), m_initialized(false) {
  const auto tof_min = static_cast<double>(m_histogram_edges->front());

  // setup function pointer  and parameters for finding bins
  // look in EventList for why
  if (bin_mode == CompressBinningMode::LINEAR) {
    m_findBin = EventList::findLinearBin;
    m_divisor = 1. / abs(divisor);
    m_offset = tof_min * m_divisor;
  } else if (bin_mode == CompressBinningMode::LOGARITHMIC) {
    m_findBin = EventList::findLogBin;
    m_divisor = 1. / log1p(abs(divisor)); // use this to do change of base
    m_offset = log(tof_min) * m_divisor;
  } else {
    throw std::runtime_error("Haven't implemented this compression binning strategy");
  }
}

std::size_t CompressEventAccumulator::numberHistBins() const { return m_histogram_edges->size(); }

template <typename INT_TYPE> double CompressEventAccumulator::getBinCenter(const INT_TYPE bin) const {
  // work with iterators to reduce access time
  const auto &binIter = m_histogram_edges->cbegin() + static_cast<std::vector<double>::difference_type>(bin);
  return 0.5 * ((*binIter) + *(std::next(binIter)));
}

boost::optional<size_t> CompressEventAccumulator::findBin(const float tof) const {
  // last parameter being false means don't find exact bin for raw events coming out of the file
  return m_findBin(*m_histogram_edges.get(), static_cast<double>(tof), m_divisor, m_offset, false);
}

// ------------------------------------------------------------------------
namespace { // anonymous

// minimum size of a vector to bother doing parallel_sort
// the grain size shouldn't get too small or the overhead of threading/sync overwhelms everything else
// TODO this can be tuned
constexpr size_t MIN_VEC_LENGTH_PARALLEL_SORT{5000};

// blindly assume 10 raw events are compressed to a single weighed event on average
constexpr size_t EXP_COMRESS_RATIO{10};

/**
 * Private class for implementation details of sparse event collection, when there are less events than bins
 */
class CompressSparseFloat : public CompressEventAccumulator {
public:
  // pass all arguments to the parent
  CompressSparseFloat(std::shared_ptr<std::vector<double>> histogram_bin_edges, const size_t numEvents,
                      const double divisor, CompressBinningMode bin_mode)
      : CompressEventAccumulator(histogram_bin_edges, divisor, bin_mode), m_is_sorted(false) {
    m_tof.reserve(numEvents);
    m_initialized = true;
  }

  double totalWeight() const override { return static_cast<double>(m_tof.size()); }

  /**
   * This assumes that the event is within range of the fine histogram
   */
  void addEvent(const float tof) override {
    // add events to the end of the list
    m_tof.push_back(tof);
  }

private:
  // this is marked because it is mostly used by createWeightedEvents and m_tof is mutable
  void sort() const {
    if (m_is_sorted)
      return;

    // empty events and singles are already sorted
    if (m_tof.size() < 2) {
      m_is_sorted = true;
      return;
    }

    // calculate smallest tolerance
    const auto &iter = m_histogram_edges->cbegin();
    const auto delta = 1. / static_cast<float>((*(iter + 1)) - (*iter));

    // parallel sort only if this is big enough of a vector
    if (m_tof.size() < MIN_VEC_LENGTH_PARALLEL_SORT)
      std::sort(m_tof.begin(), m_tof.end(), [delta](const auto &left, const auto &right) {
        return std::floor(left * delta) < std::floor(right * delta);
      });
    else
      tbb::parallel_sort(m_tof.begin(), m_tof.end(), [delta](const auto &left, const auto &right) {
        return std::floor(left * delta) < std::floor(right * delta);
      });
    m_is_sorted = true;
  }

public:
  void createWeightedEvents(std::vector<Mantid::DataObjects::WeightedEventNoTime> *raw_events) const override {
    raw_events->clear(); // clean out previous version

    if (m_tof.empty()) {
      m_is_sorted = true;
      return;
    } else if (m_tof.size() == 1) {
      // don't bother with temporary objects and such if there is only one event
      m_is_sorted = true;
      raw_events->emplace_back(m_tof.front(), 1., 1.);
      return;
    } else {
      // code below assumes the tofs are sorted
      this->sort();

      // blindly assume ratio of raw events which are compressed to a single weighed event on average
      raw_events->reserve(m_tof.size() / EXP_COMRESS_RATIO);

      // this finds the bin for the current event, then moves the iterator until an event that is out of range is
      // encountered at that point, an event is added to the output and the iterators are moved
      const auto optional_bin_first = this->findBin(m_tof.front());
      size_t lastBin = optional_bin_first.get();
      double nextTof = m_histogram_edges->at(lastBin + 1);
      uint32_t counts = 0;
      for (const auto &tof : m_tof) {
        if (lastBin + 1 >= m_histogram_edges->size()) {
          break; // this should never happen
        }

        if (tof < nextTof) {
          // accumulate information
          counts++;
        } else {
          // add weighted event
          const auto evtof = this->getBinCenter(lastBin);
          const auto counts_f = static_cast<float>(counts);
          raw_events->emplace_back(evtof, counts_f, counts_f);
          // reset accumulators to include this new value
          counts = 1;
          // total_tof = tof;

          // advance the last bin value
          if (static_cast<double>(tof) < m_histogram_edges->operator[](lastBin + 2)) {
            // next bin contains the tof
            lastBin += 1;
          } else {
            // find the bin to use
            const auto optional_bin = this->findBin(tof);
            lastBin = optional_bin.get();
          }
          nextTof = m_histogram_edges->operator[](lastBin + 1);
        }
      }

      // add in the last one if it is there
      if (counts > 0) {
        const auto evtof = this->getBinCenter(lastBin);
        const auto counts_f = static_cast<float>(counts);
        raw_events->emplace_back(evtof, counts_f, counts_f);
      }
    }
  }

  DataObjects::EventSortType getSortType() const override {
    if (m_is_sorted)
      return DataObjects::TOF_SORT;
    else
      return DataObjects::UNSORTED;
  }

private:
  /// sum of all time-of-flight within the bin
  mutable std::vector<float> m_tof;
  mutable bool m_is_sorted;
};

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
    // add events
    const auto &bin_optional = this->findBin(tof);
    if (bin_optional) {
      m_tof_bin.push_back(static_cast<uint32_t>(bin_optional.get()));
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

    // parallel sort only if this is big enough of a vector
    if (m_tof_bin.size() < MIN_VEC_LENGTH_PARALLEL_SORT)
      std::sort(m_tof_bin.begin(), m_tof_bin.end());
    else
      tbb::parallel_sort(m_tof_bin.begin(), m_tof_bin.end());
    m_is_sorted = true;
  }

  void createWeightedEvents(std::vector<Mantid::DataObjects::WeightedEventNoTime> *raw_events) const override {
    // clean out previous version
    raw_events->clear();

    if (m_tof_bin.empty()) {
      m_is_sorted = true; // nothing to do
    } else if (m_tof_bin.size() == 1) {
      m_is_sorted = true;
      // don't bother with temporary objects and such if there is only one event
      const auto tof = this->getBinCenter(m_tof_bin.front());
      raw_events->emplace_back(tof, 1., 1.);
    } else if (m_tof_bin.size() < (MAX_EVENTS / 10)) { // 10 was found to give good performance
      // this branch removes events as it accumulates them. It does not assume that the time-of-flight is sorted
      // before starting so the output will not be sorted either
      auto last_end = m_tof_bin.end();
      while (std::distance(m_tof_bin.begin(), last_end) > 0) {
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
        raw_events->emplace_back(tof, counts, counts);
      }
    } else {
      // blindly assume ratio of raw events which are compressed to a single weighed event on average
      raw_events->reserve(m_tof_bin.size() / EXP_COMRESS_RATIO);

      // accumulation method is different for "large" number of events
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
          raw_events->emplace_back(tof, counts, counts);
        }
        iter = range.second;
      }
    }
  }

  DataObjects::EventSortType getSortType() const override {
    if (m_is_sorted)
      return DataObjects::TOF_SORT;
    else
      return DataObjects::UNSORTED;
  }

  static constexpr size_t MAX_EVENTS{20000};

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
    if (!m_initialized) {
      this->allocateFineHistogram();
      m_initialized = true;
    }

    // add events
    const auto &bin_optional = this->findBin(tof);
    if (bin_optional) {
      m_count[bin_optional.get()]++;
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
        raw_events->emplace_back(tof, counts, counts);
      }
    }
  }

  DataObjects::EventSortType getSortType() const override { return DataObjects::TOF_SORT; }

private:
  void allocateFineHistogram() {
    const auto NUM_BINS = static_cast<size_t>(m_histogram_edges->size() - 1);
    m_count.resize(NUM_BINS, 0);
  }

  /// sum of all events seen in an individual bin
  std::vector<uint32_t> m_count;
};

} // namespace

// ------------------------------------------------------------------------

CompressEventAccumulatorFactory::CompressEventAccumulatorFactory(
    std::shared_ptr<std::vector<double>> histogram_bin_edges, const double divisor, CompressBinningMode bin_mode)
    : m_divisor(divisor), m_bin_mode(bin_mode), m_histogram_edges(std::move(histogram_bin_edges)) {}

std::unique_ptr<CompressEventAccumulator> CompressEventAccumulatorFactory::create(const std::size_t num_events) {
  const auto NUM_EDGES = m_histogram_edges->size();

  /*
   * The logic for which accumulator to use is a little selective, a little tuned, and a little arbitrary.
   *
   * When there are "lots" more events than bin edges, the dense case should use less memory. The threshold for this
   * could probably be lowered, but the current place is a sensible mark.
   *
   * When there aren't many events, then the cost of converting each tof to a bin number is fairly cheap and the
   * algorithm for creating a histogram from this is fast.
   *
   * For the rest of the cases, converting each tof to an int, especially for log-compression, is relatively expensive
   * so accumulate the tof/float then sort and histogram the sorted list.
   *
   * The balance is in comparing convert to int + sort + histogram versus sort float + convert some to int + histogram
   */

  if (num_events > NUM_EDGES) {
    // this is a dense array
    return std::make_unique<CompressDense>(m_histogram_edges, m_divisor, m_bin_mode);
  } else if (num_events < CompressSparseInt::MAX_EVENTS) { // somewhat arbitrary value
    return std::make_unique<CompressSparseInt>(m_histogram_edges, num_events, m_divisor, m_bin_mode);
  } else {
    return std::make_unique<CompressSparseFloat>(m_histogram_edges, num_events, m_divisor, m_bin_mode);
  }
}

} // namespace DataHandling
} // namespace Mantid
