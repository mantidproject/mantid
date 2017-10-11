#ifndef MANTID_PARALLEL_IO_EVENT_PARSER_H
#define MANTID_PARALLEL_IO_EVENT_PARSER_H

#include "MantidParallel/Collectives.h"
#include "MantidParallel/Communicator.h"
#include "MantidParallel/Nonblocking.h"
#include "MantidParallel/DllConfig.h"
#include "MantidParallel/IO/Chunker.h"
#include "MantidParallel/IO/EventDataSink.h"
#include "MantidTypes/Event/TofEvent.h"

#include <cstdint>
#include <future>
#include <numeric>
#include <vector>
#include <xmmintrin.h>

using Mantid::Types::Event::TofEvent;

namespace Mantid {
namespace Parallel {
namespace IO {

struct MANTID_PARALLEL_DLL EventListEntry {
  int32_t globalIndex; // global spectrum index
  Types::Event::TofEvent tofEvent;
};

/** Distributed (MPI) parsing of Nexus events from a data stream. Data is
distributed accross MPI ranks for writing to event lists on the correct target
rank.

@author Lamar Moore
@date 2017

Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
template <class IndexType, class TimeZeroType, class TimeOffsetType>
class EventParser
    : public EventDataSink<IndexType, TimeZeroType, TimeOffsetType> {
public:
  EventParser(std::vector<std::vector<int>> rankGroups,
              std::vector<int32_t> bankOffsets,
              std::vector<std::vector<Types::Event::TofEvent> *> eventLists);

  void setPulseInformation(std::vector<IndexType> event_index,
                           std::vector<TimeZeroType> event_time_zero,
                           const int64_t event_time_zero_offset) override;

  void startAsync(int32_t *event_id_start,
                  const TimeOffsetType *event_time_offset_start,
                  const Chunker::LoadRange &range) override;

  void redistributeDataMPI(
      std::vector<EventListEntry> &result,
      const std::vector<std::vector<EventListEntry>> &data) const;

  void extractEventsForRanks(std::vector<std::vector<EventListEntry>> &rankData,
                             const int32_t *globalSpectrumIndex,
                             const TimeOffsetType *eventTimeOffset,
                             const Chunker::LoadRange &range);

  void eventIdToGlobalSpectrumIndex(int32_t *event_id_start, size_t count,
                                    size_t bankIndex) const;

  std::pair<size_t, size_t> findStartAndEndPulseIndices(size_t rangeStart,
                                                        size_t count);

  void populateEventList(const std::vector<EventListEntry> &events);

  const std::vector<std::vector<EventListEntry>> &rankData() const {
    return m_allRankData;
  }

  void wait() const override;

private:
  void doParsing(int32_t *event_id_start,
                 const TimeOffsetType *event_time_offset_start,
                 const Chunker::LoadRange &range);
  std::vector<std::vector<int>> m_rankGroups;
  std::vector<int32_t> m_bankOffsets;
  std::vector<std::vector<Types::Event::TofEvent> *> m_eventLists;
  std::vector<IndexType> m_eventIndex;
  std::vector<TimeZeroType> m_eventTimeZero;
  Types::Core::DateAndTime m_eventTimeZeroOffset;
  std::size_t m_posInEventIndex{0};
  std::vector<std::vector<EventListEntry>> m_allRankData;
  std::vector<EventListEntry> m_thisRankData;
  std::future<void> m_future;
  Communicator m_comm;
};

/** Constructor for EventParser.
 *
 * @param rankGroups rank grouping for banks which determines how work is
 * partitioned. The EventParser guarantees to process data obtained from ranks
 * in the same group in-order to ensure pulse time ordering.
 * @param bankOffsets used to convert from event ID to global spectrum index.
 * This assumes that all event IDs within a bank a contiguous.
 * @param eventLists workspace event lists which will be populated by the
 * parser. The parser assumes that there always is a matching event list for any
 * event ID that will be passed in via `startAsync`.
 * @param globalToLocalSpectrumIndex lookup table which converts a global
 * spectrum index to a spectrum index local to a given mpi rank
 */
template <class IndexType, class TimeZeroType, class TimeOffsetType>
EventParser<IndexType, TimeZeroType, TimeOffsetType>::EventParser(
    std::vector<std::vector<int>> rankGroups, std::vector<int32_t> bankOffsets,
    std::vector<std::vector<TofEvent> *> eventLists)
    : m_rankGroups(std::move(rankGroups)),
      m_bankOffsets(std::move(bankOffsets)),
      m_eventLists(std::move(eventLists)) {}

/** Sets the event_index and event_time_zero read from I/O which is used for
 * parsing events from file/event stream.
 *
 * @param event_index The event_index entry from the NXevent_data group.
 * @param event_time_zero The event_time_zero entry from the NXevent_data group.
 */
template <class IndexType, class TimeZeroType, class TimeOffsetType>
void EventParser<IndexType, TimeZeroType, TimeOffsetType>::setPulseInformation(
    std::vector<IndexType> event_index,
    std::vector<TimeZeroType> event_time_zero,
    const int64_t event_time_zero_offset) {
  m_eventIndex = std::move(event_index);
  m_eventTimeZero = std::move(event_time_zero);
  m_eventTimeZeroOffset = Types::Core::DateAndTime(event_time_zero_offset);
  m_posInEventIndex = 0;
}

/** Transform event IDs to global spectrum numbers using the bankOffsets stored
 * at object creation.
 *
 * The transformation is in-place to save memory bandwidth and modifies the
 * range pointed to by `event_id_start`.
 * @param event_id_start Starting position of chunk of data containing event
 * IDs.
 * @param count Number of items in data chunk
 * @param bankIndex Index into the list of bank offsets.
 */
template <class IndexType, class TimeZeroType, class TimeOffsetType>
void EventParser<IndexType, TimeZeroType, TimeOffsetType>::
    eventIdToGlobalSpectrumIndex(int32_t *event_id_start, size_t count,
                                 size_t bankIndex) const {
  for (size_t i = 0; i < count; ++i)
    event_id_start[i] -= m_bankOffsets[bankIndex];
}

/** Finds the start and end pulse indices within the event_index given an offset
 * into the event_id/event_time_offset array and a chunk size.
 *
 * Returns the indices which correspond to the first and last pulse covering the
 * data chunk.
 * @param rangeStart Offset into event_time_offset/event_id
 * @param count Size of data chunk.
 */
template <class IndexType, class TimeZeroType, class TimeOffsetType>
std::pair<size_t, size_t>
EventParser<IndexType, TimeZeroType,
            TimeOffsetType>::findStartAndEndPulseIndices(size_t rangeStart,
                                                         size_t count) {
  size_t startPulse = m_posInEventIndex;
  size_t endPulse = startPulse;
  const auto rangeEnd = rangeStart + count;

  for (size_t pulse = startPulse; pulse < m_eventIndex.size(); ++pulse) {
    size_t icount = (pulse != m_eventIndex.size() - 1 ? m_eventIndex[pulse + 1]
                                                      : rangeEnd) -
                    static_cast<size_t>(m_eventIndex[pulse]);
    fprintf(stderr, "%lu %lu\n", pulse, icount);
    if (rangeStart >= static_cast<size_t>(m_eventIndex[pulse]) &&
        rangeStart < static_cast<size_t>(m_eventIndex[pulse]) + icount)
      startPulse = pulse;
    if (rangeEnd > static_cast<size_t>(m_eventIndex[pulse]) &&
        rangeEnd <= static_cast<size_t>(m_eventIndex[pulse]) + icount) {
      endPulse = pulse + 1;
      break;
    }
  }

  m_posInEventIndex = endPulse - 1;

  return {startPulse, endPulse};
}

template <class TimeZeroType>
Types::Core::DateAndTime getPulseTime(const Types::Core::DateAndTime &offset,
                                      const TimeZeroType &eventTimeZero) {
  return offset + eventTimeZero;
}

template <>
inline Types::Core::DateAndTime getPulseTime<uint64_t>(const Types::Core::DateAndTime &offset,
                                      const uint64_t &eventTimeZero) {
  return offset + static_cast<int64_t>(eventTimeZero);
}

template <>
inline Types::Core::DateAndTime
getPulseTime<int32_t>(const Types::Core::DateAndTime &offset,
                      const int32_t &eventTimeZero) {
  return offset + static_cast<int64_t>(eventTimeZero);
}

template <>
inline Types::Core::DateAndTime getPulseTime<uint32_t>(const Types::Core::DateAndTime &offset,
                                      const uint32_t &eventTimeZero) {
  return offset + static_cast<int64_t>(eventTimeZero);
}

/** Extracts event information from the list of time offsets and global spectrum
 * indices using the event_index and event_time_offset tables provided from
 * file. These events are separated according to MPI ranks.
 * @param rankData vector which stores vectors of data for each MPI rank.
 * @param globalSpectrumIndex list of spectrum indices corresponding to tof data
 * @param eventTimeOffset tof data
 * @param offset File offset (index) for tof data. Used to track event_index and
 * event_time_zero positions
 */
template <class IndexType, class TimeZeroType, class TimeOffsetType>
void EventParser<IndexType, TimeZeroType, TimeOffsetType>::
    extractEventsForRanks(std::vector<std::vector<EventListEntry>> &rankData,
                          const int32_t *globalSpectrumIndex,
                          const TimeOffsetType *eventTimeOffset,
                          const Chunker::LoadRange &range) {
  for (auto &item : rankData)
    item.clear();

  rankData.resize(m_comm.size());

  auto offset = range.eventOffset;
  auto count = range.eventCount;
  auto result = findStartAndEndPulseIndices(offset, count);

  size_t actual{0};
  for (size_t pulse = result.first; pulse < result.second; ++pulse) {
    const auto start =
        std::max(offset, static_cast<size_t>(m_eventIndex[pulse])) - offset;
    const auto end =
        std::min(offset + count,
                 static_cast<size_t>(pulse != m_eventIndex.size() - 1
                                         ? m_eventIndex[pulse + 1]
                                         : count)) -
        offset;
    //fprintf(stderr, "pulse %lu start %lu end %lu\n", pulse, start, end);

    const auto pulseTime =
        getPulseTime(m_eventTimeZeroOffset, m_eventTimeZero[pulse]);
    for (size_t i = static_cast<size_t>(start); i < static_cast<size_t>(end);
         ++i) {
      ++actual;
      int rank = globalSpectrumIndex[i] % m_comm.size();
      rankData[rank].emplace_back(EventListEntry{
          globalSpectrumIndex[i],
          TofEvent{static_cast<double>(eventTimeOffset[i]), pulseTime}});
    }
  }
  if(actual != count)
    fprintf(stderr, "aaaa\n");
}

/** Uses MPI calls to redistribute chunks which must be processed on certain
 * ranks.
 * @param result output data which must be processed on current rank. (May
 * be updated by other ranks)
 * @param data Data on this rank which belongs to several other ranks.
 */
template <class IndexType, class TimeZeroType, class TimeOffsetType>
void EventParser<IndexType, TimeZeroType, TimeOffsetType>::redistributeDataMPI(
    std::vector<EventListEntry> &result,
    const std::vector<std::vector<EventListEntry>> &data) const {
  if (m_comm.size() == 1) {
    result = data.front();
    return;
  }

  std::vector<int> sizes(data.size());
  std::transform(data.cbegin(), data.cend(), sizes.begin(),
                 [](const std::vector<EventListEntry> &vec) {
                   return static_cast<int>(vec.size());
                 });
  std::vector<int> recv_sizes(data.size());
  Parallel::all_to_all(m_comm, sizes, recv_sizes);

  auto total_size = std::accumulate(recv_sizes.begin(), recv_sizes.end(), 0);
  result.resize(total_size);
  size_t offset = 0;
  std::vector<Parallel::Request> recv_requests;
  for (int rank = 0; rank < m_comm.size(); ++rank) {
    int tag = 0;
    auto buffer = reinterpret_cast<char *>(result.data() + offset);
    int size = recv_sizes[rank] * static_cast<int>(sizeof(EventListEntry));
    recv_requests.emplace_back(m_comm.irecv(rank, tag, buffer, size));
    offset += recv_sizes[rank];
  }

  std::vector<Parallel::Request> send_requests;
  for (int rank = 0; rank < m_comm.size(); ++rank) {
    const auto &vec = data[rank];
    int tag = 0;
    send_requests.emplace_back(
        m_comm.isend(rank, tag, reinterpret_cast<const char *>(vec.data()),
                     static_cast<int>(vec.size() * sizeof(EventListEntry))));
  }

  Parallel::wait_all(send_requests.begin(), send_requests.end());
  Parallel::wait_all(recv_requests.begin(), recv_requests.end());
}

/** Fills the workspace EventList with extracted events
 * @param events Events extracted from file according to mpi rank.
 */
template <class IndexType, class TimeZeroType, class TimeOffsetType>
void EventParser<IndexType, TimeZeroType, TimeOffsetType>::populateEventList(
    const std::vector<EventListEntry> &events) {
  for (const auto &event : events) {
    // Currently this supports only a hard-code round-robin partitioning.
    auto index = event.globalIndex / m_comm.size();
    m_eventLists[index]->emplace_back(event.tofEvent);
    // In general `index` is random so this loop suffers from frequent cache
    // misses (probably because the hardware prefetchers cannot keep up with the
    // number of different memory locations that are getting accessed). We
    // manually prefetch into L2 cache to reduce the amount of misses.
    _mm_prefetch(reinterpret_cast<char *>(&m_eventLists[index]->back() + 1),
                 _MM_HINT_T1);
  }
}

/** Accepts raw data from file which has been pre-treated and sorted into chunks
 * for parsing. The parser extracts event data from the provided buffers,
 * separates then according to MPI ranks and then appends them to the workspace
 * event list. Asynchronously starts parsing wait() must be called before
 * attempting to invoke this method subsequently.
 * @param event_id_start Buffer containing event IDs.
 * @param event_time_offset_start Buffer containing TOD.
 * @param range contains information on the detector bank which corresponds to
 * the data in the buffers, the file index offset where data starts and the
 * number of elements in the data array.
 */
template <class IndexType, class TimeZeroType, class TimeOffsetType>
void EventParser<IndexType, TimeZeroType, TimeOffsetType>::startAsync(
    int32_t *event_id_start, const TimeOffsetType *event_time_offset_start,
    const Chunker::LoadRange &range) {
  fprintf(stderr, "parsing %lu events from bank %lu\n", range.eventCount, range.bankIndex);

  if (m_eventTimeZero.empty() || m_eventIndex.empty())
    throw std::runtime_error("Both event_time_zero and event_index must be set "
                             "before running the parser.");

  // Wrapped in lambda because std::async is unable to specialize doParsing on
  // its own
  m_future =
      std::async(std::launch::async,
                 [this, event_id_start, event_time_offset_start, &range] {
                   doParsing(event_id_start, event_time_offset_start, range);
                 });
}

template <class IndexType, class TimeZeroType, class TimeOffsetType>
void EventParser<IndexType, TimeZeroType, TimeOffsetType>::doParsing(
    int32_t *event_id_start, const TimeOffsetType *event_time_offset_start,
    const Chunker::LoadRange &range) {
  // change event_id_start in place
  eventIdToGlobalSpectrumIndex(event_id_start, range.eventCount,
                               range.bankIndex);

  // event_id_start now contains globalSpectrumIndex
  extractEventsForRanks(m_allRankData, event_id_start, event_time_offset_start,
                        range);

  redistributeDataMPI(m_thisRankData, m_allRankData);
  // TODO: accept something which translates from global to local spectrum index
  populateEventList(m_thisRankData);
}

template <class IndexType, class TimeZeroType, class TimeOffsetType>
void EventParser<IndexType, TimeZeroType, TimeOffsetType>::wait() const {
  if (m_future.valid())
    m_future.wait();
}

} // namespace IO
} // namespace Parallel
} // namespace Mantid
#endif // MANTID_PARALLEL_IO_EVENT_PARSER_H
