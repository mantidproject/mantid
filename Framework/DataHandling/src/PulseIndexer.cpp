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
                           const std::size_t numEvents, const std::string entry_name)
    : m_event_index(std::move(event_index)), m_firstEventIndex(firstEventIndex), m_numEvents(numEvents),
      m_entry_name(entry_name) {
  m_numPulses = m_event_index->size();
}

/**
 * This performs a linear search because it is much more likely that the index
 * to look for is at the beginning.
 */
size_t PulseIndexer::getFirstPulseIndex(const size_t event_index) const {
  // return early if the number of event indices is too small
  if (1 >= m_event_index->size())
    return 1;

  // special case to stop from setting up temporary objects
  if (event_index == 0)
    return 0;

  // linear search is used because it is more likely that the next pulse index
  // is the correct one to use the next one is because we are
  // confirm that the next index is bigger, not the current
  const auto event_index_end = m_event_index->cend();
  auto event_index_iter = m_event_index->cbegin();

  while ((event_index < *event_index_iter) || (event_index >= *(event_index_iter + 1))) {
    event_index_iter++;

    // make sure not to go past the end
    if (event_index_iter + 1 == event_index_end)
      break;
  }
  return static_cast<size_t>(std::distance(m_event_index->cbegin(), event_index_iter));
}

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
  const auto firstEventIndex = m_event_index->operator[](pulseIndex);
  if (firstEventIndex >= m_firstEventIndex)
    return firstEventIndex - m_firstEventIndex;
  else
    return 0;
}

size_t PulseIndexer::getStopEventIndex(const size_t pulseIndex) const {
  if (pulseIndex + 1 >= m_numPulses)
    return m_numEvents;

  const size_t lastEventIndex = m_event_index->operator[](pulseIndex + 1) - m_firstEventIndex;

  return std::min(lastEventIndex, m_numEvents);
}

} // namespace Mantid::DataHandling
