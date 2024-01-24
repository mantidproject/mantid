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
  PulseIndexer(std::shared_ptr<std::vector<uint64_t>> event_index, const std::size_t firstEventIndex,
               const std::size_t numEvents, const std::string &entry_name);

  /// Which element in the event_index array is the one to use
  size_t getFirstPulseIndex() const;
  /// The range of event(tof,detid) indices to read for this pulse
  std::pair<size_t, size_t> getEventIndexRange(const size_t pulseIndex) const;
  /**
   * The first event(tof,detid) index to read for this pulse.
   * This is only public for testing.
   */
  size_t getStartEventIndex(const size_t pulseIndex) const;
  /**
   * The one past the last event(tof,detid) index to read for this pulse
   * This is only public for testing.
   */
  size_t getStopEventIndex(const size_t pulseIndex) const;

private:
  PulseIndexer(); // do not allow empty constructor

  /// vector of event index (length of # of pulses)
  const std::shared_ptr<std::vector<uint64_t>> m_event_index;

  /**
   * How far into the array of events the tof/detid are already.
   * This is used when data is read in chunks.
   */
  std::size_t m_firstEventIndex;
  /// Total number of pulsetime/pulseindex
  std::size_t m_numPulses;
  /// Total number of events tof/detid
  std::size_t m_numEvents;
  /// Name of the NXentry to be used in exceptions
  const std::string m_entry_name;
};

} // namespace DataHandling
} // namespace Mantid
