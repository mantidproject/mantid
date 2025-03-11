// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/PulseIndexer.h"
#include "MantidKernel/TimeROI.h"
#include <sstream>
#include <utility>

namespace Mantid::DataHandling {
PulseIndexer::PulseIndexer(std::shared_ptr<std::vector<uint64_t>> event_index, const std::size_t firstEventIndex,
                           const std::size_t numEvents, const std::string &entry_name,
                           const std::vector<size_t> &pulse_roi)
    : m_event_index(std::move(event_index)), m_firstEventIndex(firstEventIndex), m_numEvents(numEvents),
      m_roi_complex(false), m_entry_name(entry_name) {
  // cache the number of pulses
  m_numPulses = m_event_index->size();

  // determine first useful pulse index
  m_roi.push_back(this->determineFirstPulseIndex());
  // for now, use all pulses up to the end
  m_roi.push_back(this->determineLastPulseIndex());

  // update based on the information in the pulse_roi
  if (!pulse_roi.empty()) {
    if (pulse_roi.size() % 2 != 0)
      throw std::runtime_error("Invalid size for pulsetime roi, must be even or empty");

    // new roi is the intersection of these two
    auto roi_combined = Mantid::Kernel::ROI::calculate_intersection(m_roi, pulse_roi);
    m_roi.clear();
    if (roi_combined.empty()) {
      // if roi_combined is empty then no pulses should be included, just set range to 0
      m_roi.push_back(0);
      m_roi.push_back(0);
      m_roi_complex = false;
      return;
    } else
      m_roi.assign(roi_combined.cbegin(), roi_combined.cend());
    m_roi_complex = bool(m_roi.size() > 2);
  }

  // determine if should trim the front end to remove empty pulses
  auto firstPulseIndex = m_roi.front();
  auto eventRange = this->getEventIndexRange(firstPulseIndex);
  while (eventRange.first == eventRange.second && eventRange.first < m_numEvents) {
    ++firstPulseIndex;
    eventRange = this->getEventIndexRange(firstPulseIndex);
  }

  // determine if should trim the back end to remove empty pulses
  auto lastPulseIndex = m_roi.back();
  eventRange = this->getEventIndexRange(lastPulseIndex - 1);
  while (eventRange.first == eventRange.second && eventRange.second > 0) {
    --lastPulseIndex;
    eventRange = this->getEventIndexRange(lastPulseIndex - 1);
  }

  // update the value if it has changed
  if ((firstPulseIndex != m_roi.front()) || (lastPulseIndex != m_roi.back())) {
    auto roi_combined = Mantid::Kernel::ROI::calculate_intersection(m_roi, {firstPulseIndex, lastPulseIndex});
    m_roi.clear();
    if (roi_combined.empty()) {
      // if roi_combined is empty then no pulses should be included, just set range to 0
      m_roi.push_back(0);
      m_roi.push_back(0);
    } else
      m_roi.assign(roi_combined.cbegin(), roi_combined.cend());
  }

  // after the updates, recalculate if the roi is more than a single region
  m_roi_complex = bool(m_roi.size() > 2);
}

/**
 * This performs a linear search because it is much more likely that the index
 * to look for is at the beginning.
 */
size_t PulseIndexer::determineFirstPulseIndex() const {
  // return early if the number of event indices is too small
  if (1 >= m_event_index->size())
    return 1;

  size_t firstPulseIndex = 0;

  // special case to stop from setting up temporary objects because the first event is in the first pulse
  if (m_firstEventIndex != 0) {
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
    firstPulseIndex = static_cast<size_t>(std::distance(m_event_index->cbegin(), event_index_iter));
  }

  // verify that there isn't a repeat right after the found value
  if (firstPulseIndex + 1 != m_event_index->size()) {
    for (; firstPulseIndex < m_event_index->size() - 1; ++firstPulseIndex) {
      if ((*m_event_index)[firstPulseIndex] != (*m_event_index)[firstPulseIndex + 1]) {
        break;
      }
    }
  }

  return firstPulseIndex;
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
  while ((eventIndexValue < *event_index_iter)) {
    event_index_iter++;

    // make sure not to go past the end
    if (event_index_iter + 1 == event_index_end)
      break;
  }

  return m_event_index->size() - static_cast<size_t>(std::distance(m_event_index->crbegin(), event_index_iter));
}

size_t PulseIndexer::getFirstPulseIndex() const { return m_roi.front(); }
size_t PulseIndexer::getLastPulseIndex() const { return m_roi.back(); }

std::pair<size_t, size_t> PulseIndexer::getEventIndexRange(const size_t pulseIndex) const {
  const auto start = this->getStartEventIndex(pulseIndex);
  // return early if the start is too big
  if (start >= m_numEvents)
    return std::make_pair(start, m_numEvents);

  // get the end index
  const auto stop = this->getStopEventIndex(pulseIndex);
  if (start > stop) {
    std::stringstream msg;
    msg << "Something went really wrong with pulseIndex=" << pulseIndex << ": " << start << " > " << stop << "| "
        << m_entry_name << " startAt=" << m_firstEventIndex << " numEvents=" << m_event_index->size() << " RAWINDICES=["
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
    eventIndex = (*m_event_index)[m_roi.front()];
  } else {
    eventIndex = (*m_event_index)[pulseIndex];
  }

  // return the index with the offset subtracted
  if (eventIndex >= m_firstEventIndex) {
    return eventIndex - m_firstEventIndex;
  } else {
    return 0;
  }
}

bool PulseIndexer::includedPulse(const size_t pulseIndex) const {
  if (pulseIndex >= m_roi.back()) {
    return false; // case is already handled in getStopEventIndex and shouldn't be called
  } else if (pulseIndex < m_roi.front()) {
    return false;
  } else {
    if (m_roi_complex) {
      // get the first ROI time boundary greater than the input time. Note that an ROI is a series of alternating
      // ROI_USE and ROI_IGNORE values.
      const auto iterUpper = std::upper_bound(m_roi.cbegin(), m_roi.cend(), pulseIndex);

      return (std::distance(m_roi.cbegin(), iterUpper) % 2 != 0);
    } else {
      return true; // value is past m_roi.front()
    }
  }
}

size_t PulseIndexer::getStopEventIndex(const size_t pulseIndex) const {
  if (pulseIndex >= m_roi.back()) // indicates everything has already been read
    return m_numEvents;

  // return the start index to signify not using
  if (!includedPulse(pulseIndex))
    return this->getStartEventIndex(pulseIndex);

  const auto pulseIndexEnd = pulseIndex + 1;

  // check if the requests have gone past the end - order of if/else matters
  size_t eventIndex = (pulseIndexEnd >= m_event_index->size()) ? m_numEvents : (*m_event_index)[pulseIndexEnd];
  if (pulseIndexEnd == m_roi.back()) {
    eventIndex = std::min(m_numEvents, eventIndex);
    if (pulseIndexEnd == m_event_index->size())
      eventIndex = m_numEvents;
  }

  if (eventIndex > m_firstEventIndex)
    return eventIndex - m_firstEventIndex;
  else
    return m_numEvents;
}

// ----------------------------------------- range for iteration
const PulseIndexer::Iterator PulseIndexer::cbegin() const {
  return PulseIndexer::Iterator(this, this->getFirstPulseIndex());
}

const PulseIndexer::Iterator PulseIndexer::cend() const {
  return PulseIndexer::Iterator(this, this->getLastPulseIndex());
}

PulseIndexer::Iterator PulseIndexer::begin() const { return PulseIndexer::Iterator(this, this->getFirstPulseIndex()); }

PulseIndexer::Iterator PulseIndexer::end() const { return PulseIndexer::Iterator(this, this->getLastPulseIndex()); }

// ----------------------------------------- input iterator implementation

/// returns true if the range is empty
bool PulseIndexer::Iterator::calculateEventRange() {
  const auto eventRange = m_indexer->getEventIndexRange(m_value.pulseIndex);
  m_value.eventIndexStart = eventRange.first;
  m_value.eventIndexStop = eventRange.second;

  return m_value.eventIndexStart == m_value.eventIndexStop;
}

const PulseIndexer::IteratorValue &PulseIndexer::Iterator::operator*() const { return m_value; }

PulseIndexer::Iterator &PulseIndexer::Iterator::operator++() {
  ++m_value.pulseIndex;
  // cache the final pulse index to use
  const auto lastPulseIndex = m_indexer->m_roi.back();

  // advance to the next included pulse
  while ((m_value.pulseIndex < lastPulseIndex) && (!m_indexer->includedPulse(m_value.pulseIndex)))
    ++m_value.pulseIndex;

  // return early if this has advanced to the end
  if (m_value.pulseIndex >= lastPulseIndex)
    return *this;

  while (this->calculateEventRange() && (m_value.pulseIndex < lastPulseIndex)) {
    ++m_value.pulseIndex; // move forward a pulse while there is
  }

  return *this;
}

bool PulseIndexer::Iterator::operator==(const PulseIndexer::Iterator &other) const {
  if (this->m_indexer != other.m_indexer)
    return false;
  else
    return this->m_value.pulseIndex == other.m_value.pulseIndex;
}

bool PulseIndexer::Iterator::operator!=(const PulseIndexer::Iterator &other) const { return !(*this == other); }

} // namespace Mantid::DataHandling
