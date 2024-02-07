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

// ------------------------------------------------------------------------
namespace { // anonymous

/**
 * Private class for implementation details of sparse event collection, when there are less events than bins
 */
class CompressSparse : public CompressEventAccumulator {
public:
  // pass all arguments to the parent
  CompressSparse(std::shared_ptr<std::vector<double>> histogram_bin_edges, const double divisor,
                 CompressBinningMode bin_mode)
      : CompressEventAccumulator(histogram_bin_edges, divisor, bin_mode), m_is_sorted(false) {}

  double totalWeight() const override { return static_cast<double>(m_tof.size()); }

  /**
   * This assumes that the event is within range of the fine histogram
   */
  void addEvent(const float tof) override {
    if (!m_initialized) {
      this->allocateFineHistogram();
      m_initialized = true;
    }
    // add events
    m_tof.push_back(tof);

    m_numevents++;
  }

  // this is marked because it is mostly used by createWeightedEvents and m_tof is mutable
  void sort() const override {
    if (m_is_sorted)
      return;

    // empty events and singles are already sorted
    if (m_tof.size() < 2) {
      m_is_sorted = true;
      return;
    }

    // minimum size of a vector to bother doing parallel_sort
    // the grain size shouldn't get too small or the overhead of threading/sync overwhelms everything else
    // TODO this can be tuned
    constexpr size_t MIN_VEC_LENGTH{10000};

    if (m_tof.size() < MIN_VEC_LENGTH)
      std::sort(m_tof.begin(), m_tof.end());
    else
      tbb::parallel_sort(m_tof.begin(), m_tof.end());
    m_is_sorted = true;
  }

  void createWeightedEvents(std::vector<Mantid::DataObjects::WeightedEventNoTime> *raw_events) const override {
    if (m_tof.empty())
      return;

    raw_events->clear(); // clean out previous version

    // don't bother with temporary objects and such if there is only one event
    if (m_tof.size() == 1) {
      raw_events->emplace_back(m_tof.front(), 1., 1.);
      return;
    }

    // code below assumes the tofs are sorted

    raw_events->reserve(m_tof.size() / 10); // blindly assume 10 raw events get compressed on average

    // ------ equal_range version

    // ------ hand written version

    const auto optional_bin =
        m_findBin(*m_histogram_edges.get(), static_cast<double>(m_tof.front()), m_divisor, m_offset, false);
    size_t lastBin = optional_bin.get();
    double nextTof = m_histogram_edges->at(lastBin + 1);
    double counts = 0;
    // double total_tof = 0.;
    size_t num_next = 0; // TODO REMOVE
    size_t num_same = 0; // TODO REMOVE
    for (const auto &tof : m_tof) {
      if (lastBin + 1 >= m_histogram_edges->size()) {
        break; // this should never happen
      }

      if (tof < nextTof) {
        // accumulate information
        counts++;
        // total_tof += tof;
        num_same++;
      } else {
        // add weighted event
        const auto evtof = this->getBinCenter(lastBin);
        raw_events->emplace_back(evtof, counts, counts);
        // raw_events->emplace_back(total_tof / counts, counts, counts);
        // reset accumulators to include this new value
        counts = 1;
        // total_tof = tof;

        // advance the last bin value
        if (static_cast<double>(tof) < m_histogram_edges->operator[](lastBin + 2)) {
          num_next++;
          // next bin contains the tof
          lastBin += 1;
        } else {
          // find the bin to use
          const auto optional_bin =
              m_findBin(*m_histogram_edges.get(), static_cast<double>(tof), m_divisor, m_offset, false);
          lastBin = optional_bin.get();
        }
        nextTof = m_histogram_edges->at(lastBin + 1);
      }
    }
    // add in the last one if it is there
    if (counts > 0.) {
      const auto evtof = 0.5 * (m_histogram_edges->operator[](lastBin) + m_histogram_edges->operator[](lastBin + 1));
      raw_events->emplace_back(evtof, counts, counts);
    }

    DataObjects::WeightedEventNoTime event(32);
    event.m_weight += 1;
    event.m_errorSquared += 1;

    // drop extra space if there is any
    raw_events->shrink_to_fit();

    // std::cout << "createWeightedEvents " << m_numevents << " -> " << raw_events->size() << " same=" << num_same
    //           << " next=" << num_next << "\n";
  }

private:
  void allocateFineHistogram() {
    m_tof.reserve(1678); // TODO should be based on number of predicted events
  }
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
  CompressSparseInt(std::shared_ptr<std::vector<double>> histogram_bin_edges, const double divisor,
                    CompressBinningMode bin_mode)
      : CompressEventAccumulator(histogram_bin_edges, divisor, bin_mode), m_is_sorted(false) {}

  double totalWeight() const override { return static_cast<double>(m_tof_bin.size()); }

  /**
   * This assumes that the event is within range of the fine histogram
   */
  void addEvent(const float tof) override {
    // if (tof < m_histogram_edges->front() || tof >= m_histogram_edges->back())
    //  std::cout << "THIS SHOULD NOT GET PRINTED " << tof << "\n";

    if (!m_initialized) {
      this->allocateFineHistogram();
      m_initialized = true;
    }

    // add events
    const auto bin_optional = m_findBin(*m_histogram_edges.get(), static_cast<double>(tof), m_divisor, m_offset, false);
    if (bin_optional) {
      const auto bin = static_cast<uint32_t>(bin_optional.get());
      m_tof_bin.push_back(bin);
      m_numevents++;
    }
  }

  // this is marked because it is mostly used by createWeightedEvents and m_tof is mutable
  void sort() const override {
    if (m_is_sorted)
      return;
    if (m_tof_bin.size() < 2) {
      m_is_sorted = true;
      return;
    }

    // minimum size of a vector to bother doing parallel_sort
    // the grain size shouldn't get too small or the overhead of threading/sync overwhelms everything else
    // TODO this can be tuned
    constexpr size_t MIN_VEC_LENGTH{10000};

    if (m_tof_bin.size() < MIN_VEC_LENGTH)
      std::sort(m_tof_bin.begin(), m_tof_bin.end());
    else
      tbb::parallel_sort(m_tof_bin.begin(), m_tof_bin.end());
    m_is_sorted = true;
  }

  void createWeightedEvents(std::vector<Mantid::DataObjects::WeightedEventNoTime> *raw_events) const override {
    if (m_tof_bin.empty())
      return;

    raw_events->clear(); // clean out previous version

    // don't bother with temporary objects and such if there is only one event
    if (m_tof_bin.size() == 1) {
      const auto tof = this->getBinCenter(m_tof_bin.front());
      raw_events->emplace_back(tof, 1., 1.);

      return;
    }

    // code below assumes the tofs are sorted

    raw_events->reserve(m_tof_bin.size() / 10); // blindly assume 10 raw events get compressed on average

    const auto size_raw = m_tof_bin.size();

    // accumulation method is different for "large" number of events
    if (m_tof_bin.size() > 1000000) { // TODO this number should be tuned somewhat
      // std::cout << "creating weighted events from " << m_tof_bin.size() << "\n";
      this->sort(); // sort big lists

      auto iter = m_tof_bin.cbegin();
      const auto iter_end = m_tof_bin.cend();
      while (iter != iter_end) {
        auto range = std::equal_range(iter, iter_end, *iter);
        // distance counts one less than we need for counts
        const auto counts = static_cast<double>(std::distance(range.first, range.second));
        if (counts > 0.) {
          const auto tof = this->getBinCenter(*iter);
          raw_events->emplace_back(tof, counts, counts);
        } else {
          std::cout << "range " << *range.first << " to " << *range.second
                    << " at index=" << std::distance(m_tof_bin.cbegin(), range.first) << " size=" << m_tof_bin.size()
                    << "\n";
        }
        iter = range.second;
      }

    } else {
      // this branch removes events as it accumulates them. It does not assume that the time-of-flight is sorted before
      // starting so the output will not be sorted either
      while (m_tof_bin.size() > 0) {
        // start with the last value so there is less movement since remove_if moves everything to the end
        const auto bin = m_tof_bin.back();
        const auto tof = this->getBinCenter(bin);

        // move everything with this value to the end
        auto new_end = std::remove(m_tof_bin.begin(), m_tof_bin.end(), bin);
        // number of values moved is the number of counts
        const auto counts = static_cast<double>(std::distance(new_end, m_tof_bin.end()));
        // erase the elements so the next iteration is smaller
        m_tof_bin.erase(new_end, m_tof_bin.end());

        // add the event
        raw_events->emplace_back(tof, counts, counts);
      }
    }

    // TODO REMOVE this check
    const auto size_weighted = std::accumulate(
        raw_events->cbegin(), raw_events->cend(), static_cast<size_t>(0),
        [](const auto &current, const auto &event) { return current + static_cast<size_t>(event.weight()); });
    if (size_raw != size_weighted) {
      std::cout << "started with " << size_raw << " ended with " << size_weighted << "\n";
    }

    if (size_raw > 1000000) // || (size_raw > 1000 && size_raw < 2000))
      std::cout << "converted " << size_raw << " raw events into " << raw_events->size()
                << " weighted events compared to " << m_histogram_edges->size() << " edges\n";
    // TODO REMOVE - end

    // drop extra space if there is any
    raw_events->shrink_to_fit();
  }

private:
  void allocateFineHistogram() {
    m_tof_bin.reserve(1678); // TODO should be based on number of predicted events
  }

  mutable bool m_is_sorted;

  // time-of-flight bin this data would go into
  mutable std::vector<uint32_t> m_tof_bin;
};

/**
 * Private class for implementation details of dense event collection, when there are more events than bins
 */
class CompressIntMap : public CompressEventAccumulator {
public:
  // pass all arguments to the parent
  CompressIntMap(std::shared_ptr<std::vector<double>> histogram_bin_edges, const double divisor,
                 CompressBinningMode bin_mode)
      : CompressEventAccumulator(histogram_bin_edges, divisor, bin_mode) {}

  double totalWeight() const override { return 0.; } // TODO
  void sort() const override {}                      // intentionally does nothing

  /**
   * This assumes that the event is within range of the fine histogram
   */
  void addEvent(const float tof) override {
    // if (tof < m_histogram_edges->front() || tof >= m_histogram_edges->back())
    //  std::cout << "THIS SHOULD NOT GET PRINTED " << tof << "\n";

    if (!m_initialized) {
      // this->allocateFineHistogram();
      m_initialized = true;
    }

    // add events
    const auto bin_optional = m_findBin(*m_histogram_edges.get(), static_cast<double>(tof), m_divisor, m_offset, false);
    if (bin_optional) {
      const auto bin = static_cast<uint32_t>(bin_optional.get());

      // from https://stackoverflow.com/questions/1409454/c-map-find-to-possibly-insert-how-to-optimize-operations
      auto iter = m_data.insert({bin, static_cast<uint32_t>(1)});
      if (!iter.second) {
        // update the value
        iter.first->second++;
      }
      m_numevents++;
    }
  }

  void createWeightedEvents(std::vector<Mantid::DataObjects::WeightedEventNoTime> *raw_events) const override {
    if (m_data.empty())
      return;

    raw_events->clear(); // clean out previous version

    // don't bother with temporary objects and such if there is only one event
    if (m_data.size() == 1) {
      const auto [bin, counts_i] = *m_data.begin();
      const auto counts = static_cast<double>(counts_i);
      const auto tof = this->getBinCenter(bin);
      raw_events->emplace_back(tof, counts, counts);

      return;
    }
  }

private:
  // bin number to number of counts
  std::unordered_map<uint32_t, uint32_t> m_data;
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
    const auto bin_optional = m_findBin(*m_histogram_edges.get(), static_cast<double>(tof), m_divisor, m_offset, false);
    if (bin_optional) {
      const auto bin = bin_optional.get();
      m_count[bin] += 1;
      // m_tof[bin] += tof;
      m_numevents++;
    }
  }

  // method is no-op to keep the interface consistent
  // this is necessary to have TBB sort all of these at once for the sparse implmentation
  void sort() const override {}

  void createWeightedEvents(std::vector<Mantid::DataObjects::WeightedEventNoTime> *raw_events) const override {
    if (m_count.empty())
      return;

    const auto NUM_BINS = m_tof.size();
    for (size_t i = 0; i < NUM_BINS; ++i) {
      if (m_count[i] > 0) {
        const auto counts = static_cast<double>(m_count[i]);
        // const auto tof = static_cast<double>(m_tof[i]) / counts;
        const auto tof = 0.5 * (m_histogram_edges->operator[](i) + m_histogram_edges->operator[](i + 1));
        raw_events->emplace_back(tof, counts, counts);
      }
    }
    std::cout << "createWeightedEvents " << m_numevents << " -> " << raw_events->size() << "\n";
  }

private:
  void allocateFineHistogram() {
    const auto NUM_BINS = static_cast<size_t>(m_histogram_edges->size() - 1);
    // m_tof.resize(NUM_BINS, 0.);
    m_count.resize(NUM_BINS, 0.);
  }

  /// sum of all events seen in an individual bin
  std::vector<uint8_t> m_count;
  mutable std::vector<float> m_tof;
};

} // namespace

// ------------------------------------------------------------------------

CompressEventAccumulatorFactory::CompressEventAccumulatorFactory(
    std::shared_ptr<std::vector<double>> histogram_bin_edges, const double divisor, CompressBinningMode bin_mode)
    : m_divisor(divisor), m_bin_mode(bin_mode), m_histogram_edges(std::move(histogram_bin_edges)) {}

std::unique_ptr<CompressEventAccumulator> CompressEventAccumulatorFactory::create() {
  // return std::make_unique<CompressSparse>(m_histogram_edges, m_divisor, m_bin_mode);
  return std::make_unique<CompressSparseInt>(m_histogram_edges, m_divisor, m_bin_mode);
  // return std::make_unique<CompressIntMap>(m_histogram_edges, m_divisor, m_bin_mode);
  // return std::make_unique<CompressDense>(m_histogram_edges, m_divisor, m_bin_mode);
}

} // namespace DataHandling
} // namespace Mantid
