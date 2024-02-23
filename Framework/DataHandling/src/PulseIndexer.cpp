// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/PulseIndexer.h"
#include <sstream>
#include <utility>

namespace Mantid::DataHandling {
PulseIndexer::PulseIndexer(std::shared_ptr<std::vector<uint64_t>> event_index, const std::size_t firstEventIndex,
                           const std::size_t numEvents, const std::string &entry_name)
    : m_event_index(std::move(event_index)), m_firstEventIndex(firstEventIndex), m_numEvents(numEvents),
      m_entry_name(entry_name) {
  // cache the number of pulses
  m_numPulses = m_event_index->size();

  // determine first useful pulse index
  m_roi.push_back(this->determineFirstPulseIndex());
  // for now, use all pulses up to the end
  m_roi.push_back(this->determineLastPulseIndex());

  /*
  if (firstEventIndex > 0) { // REMOVE
    std::stringstream msg;
    msg << "firstEventIndex=" << firstEventIndex;
    throw std::runtime_error(msg.str());
  }
  */
}

/**
 * This performs a linear search because it is much more likely that the index
 * to look for is at the beginning.
 */
size_t PulseIndexer::determineFirstPulseIndex() const {
  // return early if the number of event indices is too small
  if (1 >= m_event_index->size())
    return 1;

  // special case to stop from setting up temporary objects because the first event is in the first pulse
  if (m_firstEventIndex == 0)
    return 0;

  // linear search is used because it is more likely that the pulse index is earlier in the array.
  // a bisecting search would win if most of the time the first event_index is much after the first quarter of the
  // pulse_index array.
  const auto event_index_end = m_event_index->cend();
  auto event_index_iter = m_event_index->cbegin();

  while ((m_firstEventIndex < *event_index_iter) || (m_firstEventIndex >= *(event_index_iter + 1))) {
    event_index_iter++;

    // make sure not to go past the end
    if (event_index_iter + 1 == event_index_end)
      break;
  }
  return static_cast<size_t>(std::distance(m_event_index->cbegin(), event_index_iter));
}

/**
 * This looks at the event_indexes and number of events to read then determines what is the maximum pulse to use.
 */
size_t PulseIndexer::determineLastPulseIndex() const {
  if (m_firstEventIndex + m_numEvents > m_event_index->back())
    return m_event_index->size();

  const auto eventIndexValue = m_firstEventIndex + m_numEvents;
  // linear search is used because it is more likely that the pulse index is closer to the end of the array.
  // a bisecting search would win if most of the time the first event_index is much after the first quarter of the
  // pulse_index array.
  const auto event_index_end = m_event_index->crend();
  auto event_index_iter = m_event_index->crbegin();
  while ((eventIndexValue > *event_index_iter) || (eventIndexValue <= *(event_index_iter + 1))) {
    event_index_iter++;

    // make sure not to go past the end
    if (event_index_iter + 1 == event_index_end)
      break;
  }

  return static_cast<size_t>(m_event_index->size() - std::distance(m_event_index->crbegin(), event_index_iter));
}

/**
 * This performs a linear search because it is much more likely that the index
 * to look for is at the beginning.
 */
size_t PulseIndexer::getFirstPulseIndex() const { return m_roi.front(); }
size_t PulseIndexer::getLastPulseIndex() const { return m_roi.back(); }

std::pair<size_t, size_t> PulseIndexer::getEventIndexRange(const size_t pulseIndex) const {
  const auto start = this->getStartEventIndex(pulseIndex);
  // return early if the start is too big
  if (start > m_numEvents)
    return std::make_pair(start, m_numEvents);

  // get the end index
  const auto stop = this->getStopEventIndex(pulseIndex);
  if (start > stop) {
    std::stringstream msg;
    msg << "Something went really wrong: " << start << " > " << stop << "| " << m_entry_name
        << " startAt=" << m_firstEventIndex << " numEvents=" << m_event_index->size() << " RAWINDICES=["
        << start + m_firstEventIndex << ",?)"
        << " pulseIndex=" << pulseIndex << " of " << m_event_index->size();
    throw std::runtime_error(msg.str());
  }

  return std::make_pair(start, stop);
}

size_t PulseIndexer::getStartEventIndex(const size_t pulseIndex) const {
  // return the start index to signify not using
  if (pulseIndex >= m_roi.back())
    return this->getStopEventIndex(pulseIndex);

  // determine the correct start index
  size_t eventIndex;
  if (pulseIndex <= m_roi.front()) {
    eventIndex = m_event_index->operator[](m_roi.front());
  } else if (pulseIndex >= m_roi.back()) {
    eventIndex = m_numEvents;
  } else {
    eventIndex = m_event_index->operator[](pulseIndex);
  }

  // return the index with the offset subtracted
  if (eventIndex >= m_firstEventIndex) {
    return eventIndex - m_firstEventIndex;
  } else {
    return 0;
  }
}

size_t PulseIndexer::getStopEventIndex(const size_t pulseIndex) const {
  // return the start index to signify not using
  if (pulseIndex < m_roi.front())
    return this->getStartEventIndex(pulseIndex);

  const auto pulseIndexEnd = pulseIndex + 1;
  // check if the requests have gone past the end - order of if/else matters
  if (pulseIndexEnd >= m_numPulses) // last pulse should read to the last event
    return m_numEvents - m_firstEventIndex;
  else if (pulseIndexEnd >= m_roi.back()) // this can be equal to the number of pulses
    return m_event_index->operator[](m_roi.back()) - m_firstEventIndex;

  const size_t lastEventIndex = m_event_index->operator[](pulseIndexEnd) - m_firstEventIndex;

  return std::min(lastEventIndex, m_numEvents - m_firstEventIndex);
}

} // namespace Mantid::DataHandling
