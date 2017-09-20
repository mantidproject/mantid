#include "MantidParallel/IO/EventParser.h"
#include "MantidTypes/TofEvent.h"
#include <H5Cpp.h>
#include <xmmintrin.h>

using namespace Mantid::Parallel::IO;
using Mantid::Types::TofEvent;

namespace {
template <class IndexType>
std::pair<size_t, size_t>
findStartAndEndPulses(const std::vector<IndexType> &eventIndex,
                      size_t rangeStart, size_t count, size_t &curr) {
  size_t startPulse = 0;
  size_t endPulse = 0;

  size_t pulse = curr;

  const auto rangeEnd = rangeStart + count;

  for (; pulse < eventIndex.size(); ++pulse) {
    size_t icount =
        (pulse != eventIndex.size() - 1 ? eventIndex[pulse + 1] : rangeEnd) -
        eventIndex[pulse];
    if (rangeStart >= static_cast<size_t>(eventIndex[pulse]) &&
        rangeStart < static_cast<size_t>(eventIndex[pulse]) + icount)
      startPulse = pulse;
    if (rangeEnd > static_cast<size_t>(eventIndex[pulse]) &&
        rangeEnd <= static_cast<size_t>(eventIndex[pulse]) + icount)
      endPulse = pulse + 1;
  }

  curr = pulse;

  return {startPulse, endPulse};
}
} // namespace

namespace Mantid {
namespace Parallel {
namespace IO {

template <class IndexType, class TimeZeroType, class TimeOffsetType>
EventParser<IndexType, TimeZeroType, TimeOffsetType>::EventParser(
    std::vector<std::vector<int>> rankGroups,
    const std::vector<int32_t> &bankOffsets,
    std::vector<std::vector<TofEvent> *> &eventLists)
    : m_rankGroups(rankGroups), m_bankOffsets(bankOffsets),
      m_eventLists(eventLists), m_posInEventIndex(0) {
  if (m_bankOffsets.empty())
    throw std::invalid_argument("The bankOffsets vector cannot be empty.");
  if (m_eventLists.empty())
    throw std::invalid_argument("The evenLists vector cannot be empty.");
}

/** Sets the event_index and event_time_zero read from I/O which is used for
 *parsing events from file/event stream.
 *@param event_index
 *@param event_time_zero
 *
 */
template <class IndexType, class TimeZeroType, class TimeOffsetType>
void EventParser<IndexType, TimeZeroType, TimeOffsetType>::setPulseInformation(
    const std::vector<IndexType> &event_index,
    const std::vector<TimeZeroType> &event_time_zero) const {
  m_eventIndex = event_index;
  m_eventTimeZero = event_time_zero;
}

/** Used the detectorIDs supplied to calculate the corresponding global spectrum
 * numbers using the bankOffsets stored at object creation.
 * @param event_id_start Starting position of chunk of data containing detector
 * IDs
 * @param count Number of items in data chunk
 * @param bankIndex Index into the list of bank offsets.
 */
template <class IndexType, class TimeZeroType, class TimeOffsetType>
void EventParser<IndexType, TimeZeroType, TimeOffsetType>::
    eventIdToGlobalSpectrumIndex(int32_t *event_id_start, size_t count,
                                 size_t bankIndex) const {
  m_globalSpectrumIndex.resize(count);

  for (size_t i = 0; i < count; i++)
    m_globalSpectrumIndex[i] = event_id_start[i] - m_bankOffsets[bankIndex];
}

/** Extracts event information from the list of time offsets and global spectrum
 * indices using the event_index and event_time_offset tables provided from
 * file. These events are separated according to MPI ranks.
 * @param rankData vector which stores vectors of data for each mpi rank.
 * @param globalSpectrumIndex list of spectrum indices corresponding to tof data
 * @param eventTimeOffset tof data
 * @param offset File offset (index) for tof data. Used to track event_index and
 * event_time_zero positions
 */
template <class IndexType, class TimeZeroType, class TimeOffsetType>
void EventParser<IndexType, TimeZeroType, TimeOffsetType>::
    extractEventsForRanks(std::vector<std::vector<Event>> &rankData,
                          const std::vector<int32_t> &globalSpectrumIndex,
                          const TimeOffsetType *eventTimeOffset,
                          size_t offset) {
  for (auto &item : rankData)
    item.clear();

  const auto count = globalSpectrumIndex.size();
  // Determine start and end pulse/s for chunk of data provided.
  auto result = findStartAndEndPulses<IndexType>(m_eventIndex, offset, count,
                                                 m_posInEventIndex);

  for (size_t pulse = result.first; pulse < result.second; ++pulse) {
    const auto start =
        std::max(offset, static_cast<hsize_t>(m_eventIndex[pulse])) - offset;
    const auto end =
        std::min(offset + count,
                 static_cast<hsize_t>(pulse != m_eventIndex.size() - 1
                                          ? m_eventIndex[pulse + 1]
                                          : offset + count)) -
        offset;

    for (size_t i = start; i < end; ++i) {
      // TODO: select appropriate ranks round robin?
      // int rank = global_spectrum_index[i] % nrank;
      // rank_data[rank].push_back(...)
      int rank = 0;
      rankData[rank].push_back(
          Event{globalSpectrumIndex[i],
                TofEvent{static_cast<double>(eventTimeOffset[i]),
                         m_eventTimeZero[pulse]}});
    }
  }
}

/** Fills the workspace EventList with extracted events
 * @param eventList Workspace event list to be populated
 * @param events Events extracted from file according to mpi rank.
 */
template <class IndexType, class TimeZeroType, class TimeOffsetType>
void EventParser<IndexType, TimeZeroType, TimeOffsetType>::populateEventList(
    std::vector<std::vector<TofEvent> *> &eventList,
    const std::vector<Event> &events) {
  for (const auto &event : events) {
    // TODO calculate local index
    auto index = event.index;
    eventList[index]->emplace_back(event.tofEvent);
    _mm_prefetch(reinterpret_cast<char *>(&eventList[index]->back() + 1),
                 _MM_HINT_T1);
  }
}

/** Accepts raw data from file which has been pre-treated and sorted into chunks
 * for parsing. The parser extracts event data from the provided buffers,
 * separates then according to MPI ranks and then appends them to the workspace
 * event list.
 * @param event_id_start Buffer containing detector IDs.
 * @param event_time_offset_start Buffer containing TOD.
 * @param range contains information on the detector bank which corresponds to
 * the data in the buffers, the file index offset where data starts and the
 * number of elements in the data array.
 */
template <class IndexType, class TimeZeroType, class TimeOffsetType>
void EventParser<IndexType, TimeZeroType, TimeOffsetType>::startParsing(
    int32_t *event_id_start, TimeOffsetType *event_time_offset_start,
    const LoadRange &range) {

  if (m_eventTimeZero.empty() || m_eventIndex.empty())
    throw std::runtime_error("Both event_time_zero and event_index must be set "
                             "before running the parser.");

  eventIdToGlobalSpectrumIndex(event_id_start, range.eventCount,
                               range.bankIndex);

  // TODO Use MPI Comm to get number of ranks
  int nrank = 1;
  m_allRankData.resize(nrank);

  extractEventsForRanks(m_allRankData, m_globalSpectrumIndex,
                        event_time_offset_start, range.eventOffset);

  // TODO: Redistribute data across MPI Ranks
  // redistributeDataMPI(m_thisRankData, allRankData);
  // populateEventList(m_eventLists, m_thisRankData);
  m_thisRankData = m_allRankData[0]; // TODO Remove

  // TODO: accept something which translates from global to local spectrum index
  populateEventList(m_eventLists, m_thisRankData);
}

template <class IndexType, class TimeZeroType, class TimeOffsetType>
void EventParser<IndexType, TimeZeroType, TimeOffsetType>::wait() {}

template <class IndexType, class TimeZeroType, class TimeOffsetType>
void EventParser<IndexType, TimeZeroType, TimeOffsetType>::finalize() {}

// Template Specialization Exports
template class DLLExport EventParser<int32_t, int64_t, int32_t>;
template class DLLExport EventParser<int64_t, int64_t, double>;
} // namespace IO
} // namespace Parallel
} // namespace Mantid