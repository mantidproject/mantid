#include "MantidParallel/IO/EventParser.h"
#include <H5Cpp.h>
#include <xmmintrin.h>

#ifdef MPI_EXPERIMENTAL
#include <mpi.h>
#endif

using namespace Mantid::Parallel::IO;
using Mantid::Types::Event::TofEvent;

namespace Mantid {
namespace Parallel {
namespace IO {

/** Constructor for EventParser.
 *NB there is no range checking for these inputs, clients using the class should
 *ensure they provide sensible data.
 *@param rankGroups rank grouping for banks which determines how work is
 *partitioned. Group ordering must be preserved to ensure pulse time ordering.
 *@param bankOffsets used to convert from detector ID to global spectrum index.
 *@param eventLists workspace eventlists which will be populated by the parser.
 *@param globalToLocalSpectrumIndex lookup table which converts a global
 *spectrum index to a spectrum index local to a given mpi rank
 */
template <class IndexType, class TimeZeroType, class TimeOffsetType>
EventParser<IndexType, TimeZeroType, TimeOffsetType>::EventParser(
    std::vector<std::vector<int>> rankGroups,
    const std::vector<int32_t> &bankOffsets,
    std::vector<std::vector<TofEvent> *> &eventLists,
    std::vector<int32_t> globalToLocalSpectrumIndex)
    : m_rankGroups(rankGroups), m_bankOffsets(bankOffsets),
      m_eventLists(eventLists), m_posInEventIndex(0),
      m_globalToLocalSpectrumIndex(std::move(globalToLocalSpectrumIndex)) {}

/** Sets the event_index and event_time_zero read from I/O which is used for
 *parsing events from file/event stream.
 *@param event_index
 *@param event_time_zero
 *
 */
template <class IndexType, class TimeZeroType, class TimeOffsetType>
void EventParser<IndexType, TimeZeroType, TimeOffsetType>::setPulseInformation(
    std::vector<IndexType> event_index,
    std::vector<TimeZeroType> event_time_zero) {
  m_eventIndex = std::move(event_index);
  m_eventTimeZero = std::move(event_time_zero);
  m_posInEventIndex = 0;
}

/** Uses the detectorIDs supplied to calculate the corresponding global spectrum
 * numbers using the bankOffsets stored at object creation. NB event_id_start is
 * transformed to contain the spectrum indices.
 * @param event_id_start Starting position of chunk of data containing detector
 * IDs. This is transformed in place to save memory bandwidth.
 * @param count Number of items in data chunk
 * @param bankIndex Index into the list of bank offsets.
 */
template <class IndexType, class TimeZeroType, class TimeOffsetType>
void EventParser<IndexType, TimeZeroType, TimeOffsetType>::
    eventIdToGlobalSpectrumIndex(int32_t *event_id_start, size_t count,
                                 size_t bankIndex) {
  for (size_t i = 0; i < count; i++)
    event_id_start[i] -= m_bankOffsets[bankIndex];
}

/** Finds the start and end pulse indices within the event_index given an offset
 * into the event_id/event_time_offset array and a chunk size. Returns the
 * indices which correspond to the start and end pulses covering the data chunk.
 * @param eventIndex index list which is searched
 * @param rangeStart Offset into event_time_offset/event_id
 * @param count Size of data chunk.
 * @param curr Current search position
 */
template <class IndexType, class TimeZeroType, class TimeOffsetType>
std::pair<size_t, size_t> EventParser<IndexType, TimeZeroType, TimeOffsetType>::
    findStartAndEndPulseIndices(const std::vector<IndexType> &eventIndex,
                                size_t rangeStart, size_t count, size_t &curr) {
  size_t startPulse = std::min(rangeStart, curr);
  size_t endPulse = curr;

  size_t pulse = startPulse;

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

  curr = endPulse;

  return {startPulse, endPulse};
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
                          const int32_t *globalSpectrumIndex,
                          const TimeOffsetType *eventTimeOffset,
                          const LoadRange &range) {
  for (auto &item : rankData)
    item.clear();

  auto offset = range.eventOffset;
  auto count = range.eventCount;
  // Determine start and end pulse/s for chunk of data provided.
  auto result = findStartAndEndPulseIndices(m_eventIndex, offset, count,
                                            m_posInEventIndex);

  for (size_t pulse = result.first; pulse <= result.second; ++pulse) {
    const auto start =
        std::max(
            (pulse == 0 ? 0 : static_cast<size_t>(m_eventIndex[pulse - 1])),
            offset) -
        offset;
    const auto end = std::min(static_cast<size_t>(pulse == m_eventIndex.size()
                                                      ? offset + count
                                                      : m_eventIndex[pulse]),
                              offset + count) -
                     offset;

    for (size_t i = static_cast<size_t>(start); i < static_cast<size_t>(end);
         ++i) {
      int rank = globalSpectrumIndex[i] % m_comm.size();
      rankData[rank].push_back(
          Event{globalSpectrumIndex[i],
                TofEvent{static_cast<double>(eventTimeOffset[i]),
                         m_eventTimeZero[pulse]}});
    }
  }
}

/** Uses MPI calls to redistribute chunks which must be processed on certain
 *ranks.
 *@param result output data which must be processed on current rank. (May
 *be updated by other ranks)
 *@param data Data on this rank which belongs to several other ranks.
 */
template <class IndexType, class TimeZeroType, class TimeOffsetType>
void EventParser<IndexType, TimeZeroType, TimeOffsetType>::redistributeDataMPI(
    std::vector<Event> &result, std::vector<std::vector<Event>> &data) {
#ifndef MPI_EXPERIMENTAL
  result = data[m_comm.rank()];
#else
  std::vector<size_t> sizes(data.size());
  std::transform(data.cbegin(), data.cend(), sizes.begin(),
                 [](const std::vector<Event> &vec) { return vec.size(); });
  MPI_Alltoall(sizes.data(), 1, MPI_UNSIGNED_LONG, rec_sizes.data(), 1,
               MPI_UNSIGNED_LONG, MPI_COMM_WORLD);

  auto total_size = std::accumulate(rec_sizes.begin(), rec_sizes.end(),
                                    static_cast<size_t>(0));
  result.resize(total_size);
  size_t offset = 0;
  std::vector<MPI_Request> recv_requests(data.size());
  for (int rank = 0; rank < data.size(); ++rank) {
    int tag = 0;
    MPI_Irecv(result.data() + offset, rec_sizes[rank] * sizeof(T), MPI_CHAR,
              rank2, tag, MPI_COMM_WORLD, &recv_requests[rank]);
    offset += rec_sizes[rank2];
    // TODO:
    // 1. do work between sending and receiving (next range, or insert into
    // workspace after first data was received?).
  }

  std::vector<MPI_Request> send_requests(data.size());
  for (int rank = 0; rank < data.size(); ++rank) {
    const auto &vec = data[rank];
    int tag = 0;
    MPI_Isend(vec.data(), vec.size() * sizeof(T), MPI_CHAR, rank, tag,
              MPI_COMM_WORLD, &send_requests[rank2]);
  }

  MPI_Waitall(data.size(), send_requests.data(), MPI_STATUSES_IGNORE);
  MPI_Waitall(data.size(), recv_requests.data(), MPI_STATUSES_IGNORE);
#endif
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
    // TODO: calculate local index
#ifdef MPI_EXPERIMENTAL
    auto index = m_globalToLocalSpectrumIndex[event.index];
#else
    auto index = event.index;
#endif
    eventList[index]->emplace_back(event.tofEvent);
    // Prefetch data into L1 Cache for faster access
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

  // Wrapped in lambda because std::async is unable to specialize doParsing on
  // its own
  m_future = std::async(std::launch::async, [this, event_id_start,
                                             event_time_offset_start, &range] {
    doParsing(event_id_start, event_time_offset_start, range);
  });
}

template <class IndexType, class TimeZeroType, class TimeOffsetType>
void EventParser<IndexType, TimeZeroType, TimeOffsetType>::doParsing(
    int32_t *event_id_start, TimeOffsetType *event_time_offset_start,
    const LoadRange &range) {
  // change event_id_start in place
  eventIdToGlobalSpectrumIndex(event_id_start, range.eventCount,
                               range.bankIndex);
  int nrank = m_comm.size();
  m_allRankData.resize(nrank);
  // event_id_start now contains globalSpectrumIndex
  extractEventsForRanks(m_allRankData, event_id_start, event_time_offset_start,
                        range);

  redistributeDataMPI(m_thisRankData, m_allRankData);
  // TODO: accept something which translates from global to local spectrum index
  populateEventList(m_eventLists, m_thisRankData);
}

template <class IndexType, class TimeZeroType, class TimeOffsetType>
void EventParser<IndexType, TimeZeroType, TimeOffsetType>::wait() const {
  if (m_future.valid())
    m_future.wait();
}

template <class IndexType, class TimeZeroType, class TimeOffsetType>
void EventParser<IndexType, TimeZeroType, TimeOffsetType>::finalize() const {
  wait();
}

// Template Specialization Exports
template class DLLExport EventParser<int32_t, int32_t, int32_t>;
template class DLLExport EventParser<int32_t, int32_t, int64_t>;
template class DLLExport EventParser<int32_t, int64_t, int32_t>;
template class DLLExport EventParser<int32_t, int64_t, int64_t>;
template class DLLExport EventParser<int64_t, int64_t, double>;
template class DLLExport EventParser<int32_t, int64_t, double>;
template class DLLExport EventParser<int32_t, int32_t, double>;
} // namespace IO
} // namespace Parallel
} // namespace Mantid