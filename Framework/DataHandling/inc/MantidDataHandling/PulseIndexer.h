// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataHandling/DllConfig.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace Mantid {
namespace DataHandling {

/**
 * PulseIndexer contains information for mapping from pulse index/number to event index.
 *
 * The events come in two sets of parallel arrays:
 * * pulse_time[NUM_PULSE] - wall-clock time of pulse - stored in event_time_zero
 * * event_index[NUM_PULSE] - index into the event_detid and event_tof arrays - stored in event_index
 * * event_detid[NUM_EVENT] - detector id of the individual event - stored in event_pixel_id or event_id
 * * event_tof[NUM_EVENT] - time-of-flight of the individual event - stored in event_time_of_flight or event_time_offset
 *
 * In general, NUM_PULSE<NUM_EVENT, but this is not true for "dark count" measurements.
 *
 * Once configured, this allows for the caller to start at the first index into the pulse information,
 * then get the range of event [inclusive, exclusive) detid and tof to iterate through.
 */
class MANTID_DATAHANDLING_DLL PulseIndexer {
public:
  // ----------------------------------------- input iterator allows for read-only access
  struct IteratorValue {
    std::size_t pulseIndex;
    std::size_t eventIndexStart;
    std::size_t eventIndexStop;
  };

  struct MANTID_DATAHANDLING_DLL Iterator {
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = IteratorValue;

    Iterator(const PulseIndexer *indexer, const size_t pulseIndex)
        : m_indexer(indexer), m_lastPulseIndex(m_indexer->getLastPulseIndex()) {
      m_value.pulseIndex = pulseIndex;
      calculateEventRange();
    }

    const IteratorValue &operator*() const;

    // prefix increment ++iter
    Iterator &operator++();
    // postfix increment iter++ is not needed

    bool operator==(const PulseIndexer::Iterator &other) const;
    bool operator!=(const PulseIndexer::Iterator &other) const;

  private:
    const PulseIndexer *m_indexer;
    const size_t m_lastPulseIndex;

    bool calculateEventRange();

    IteratorValue m_value;
  };

  // ----------------------------------------- pulse indexer class start
  PulseIndexer(std::shared_ptr<std::vector<uint64_t>> event_index, const std::size_t firstEventIndex,
               const std::size_t numEvents, const std::string &entry_name, const std::vector<size_t> &pulse_roi);

  /// Which element in the event_index array is the first one to use
  size_t getFirstPulseIndex() const;
  /// Which element in the event_index array is the last one to use
  size_t getLastPulseIndex() const;
  /// The range of event(tof,detid) indices to read for this pulse
  std::pair<size_t, size_t> getEventIndexRange(const size_t pulseIndex) const;
  /**
   * The first event(tof,detid) index to read for this pulse. When this is after the event indices to use, it returns
   * the value of getStopEventIndex. This is only public for testing.
   */
  size_t getStartEventIndex(const size_t pulseIndex) const;
  /**
   * The one past the last event(tof,detid) index to read for this pulse. When this is not a pulse index to use,
   * it returns the value of getStartEventIndex. When the pulseIndex is past the end of the use regions, it returns the
   * m_numEvents. This is only public for testing.
   */
  size_t getStopEventIndex(const size_t pulseIndex) const;

  const Iterator cbegin() const;
  const Iterator cend() const;
  Iterator begin() const;
  Iterator end() const;

private:
  PulseIndexer(); // do not allow empty constructor

  size_t determineFirstPulseIndex() const;
  size_t determineLastPulseIndex() const;
  /// returns true when the roi says the pulse should be used
  bool includedPulse(const size_t pulseIndex) const;

  /// vector of indices (length of # of pulses) into the event arrays
  const std::shared_ptr<std::vector<uint64_t>> m_event_index;

  /**
   * How far into the array of events the tof/detid are already. This is used when data is read in chunks.
   * It is generally taken from the zeroth element of the event_index array, but is also used for chunking by
   * pulse-time.
   *
   * Another way to think of this value is the offset into the tof/detid arrays from disk that are actually in memory.
   * Because of this, all indices into the event arrays (tof/detid) have this value subtracted off.
   */
  std::size_t m_firstEventIndex;

  /**
   * Total number of events tof/detid that should be processed. This can be less than the total number of events in the
   * actual array. This value plus the m_firstEventIndex should be <= the total events in the NXevent_data being
   * processed.
   */
  std::size_t m_numEvents;

  /**
   * Alternating values describe ranges of [use, don't) of pulse index ranges. There will always be a gap between
   * neighboring values.
   */
  std::vector<std::size_t> m_roi;
  /// true when there is more to check than the pulse being between the ends
  bool m_roi_complex;

  /// Total number of pulsetime/pulseindex
  std::size_t m_numPulses;

  /// Name of the NXentry to be used in exceptions
  const std::string m_entry_name;
};

} // namespace DataHandling
} // namespace Mantid
