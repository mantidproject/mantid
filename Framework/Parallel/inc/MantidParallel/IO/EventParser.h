#ifndef MANTID_PARALLEL_IO_EVENT_PARSER_H
#define MANTID_PARALLEL_IO_EVENT_PARSER_H

#include "MantidParallel/Collectives.h"
#include "MantidParallel/Communicator.h"
#include "MantidParallel/Nonblocking.h"
#include "MantidParallel/DllConfig.h"
#include "MantidParallel/IO/Chunker.h"
#include "MantidParallel/IO/PulseTimeGenerator.h"
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
namespace detail {
template <class TimeOffsetType> struct Event {
  int32_t index; // local spectrum index
  TimeOffsetType tof;
  Types::Core::DateAndTime pulseTime;
};

void MANTID_PARALLEL_DLL eventIdToGlobalSpectrumIndex(int32_t *event_id_start,
                                                      size_t count,
                                                      const int32_t bankOffset);

template <class TimeOffsetType>
void redistributeDataMPI(
    Communicator &comm, std::vector<Event<TimeOffsetType>> &result,
    const std::vector<std::vector<Event<TimeOffsetType>>> &data);

template <class TimeOffsetType>
void populateEventLists(
    const std::vector<Event<TimeOffsetType>> &events,
    std::vector<std::vector<Types::Event::TofEvent> *> &eventLists);
}

template <class IndexType, class TimeZeroType, class TimeOffsetType>
class EventParser {
public:
  using Event = detail::Event<TimeOffsetType>;
  EventParser(const Communicator &comm,
              std::vector<std::vector<int>> rankGroups,
              std::vector<int32_t> bankOffsets,
              std::vector<std::vector<Types::Event::TofEvent> *> eventLists);

  void setPulseTimeGenerator(PulseTimeGenerator<IndexType, TimeZeroType> &&gen);

  void startAsync(int32_t *event_id_start,
                  const TimeOffsetType *event_time_offset_start,
                  const Chunker::LoadRange &range);

  void extractEventsForRanks(std::vector<std::vector<Event>> &rankData,
                             const int32_t *globalSpectrumIndex,
                             const TimeOffsetType *eventTimeOffset,
                             const Chunker::LoadRange &range);

  void wait() const;

private:
  void doParsing(int32_t *event_id_start,
                 const TimeOffsetType *event_time_offset_start,
                 const Chunker::LoadRange &range);
  Communicator m_comm;
  std::vector<std::vector<int>> m_rankGroups;
  std::vector<int32_t> m_bankOffsets;
  std::vector<std::vector<Types::Event::TofEvent> *> m_eventLists;
  PulseTimeGenerator<IndexType, TimeZeroType> m_pulseTimes;
  std::vector<std::vector<Event>> m_allRankData;
  std::vector<Event> m_thisRankData;
  std::future<void> m_future;
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
    const Communicator &comm, std::vector<std::vector<int>> rankGroups,
    std::vector<int32_t> bankOffsets,
    std::vector<std::vector<TofEvent> *> eventLists)
    : m_comm(comm), m_rankGroups(std::move(rankGroups)),
      m_bankOffsets(std::move(bankOffsets)),
      m_eventLists(std::move(eventLists)) {}

/// Set the PulseTimeGenerator to use for parsing subsequent events.
template <class IndexType, class TimeZeroType, class TimeOffsetType>
void EventParser<IndexType, TimeZeroType, TimeOffsetType>::
    setPulseTimeGenerator(PulseTimeGenerator<IndexType, TimeZeroType> &&gen) {
  m_pulseTimes = std::move(gen);
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
    extractEventsForRanks(std::vector<std::vector<Event>> &rankData,
                          const int32_t *globalSpectrumIndex,
                          const TimeOffsetType *eventTimeOffset,
                          const Chunker::LoadRange &range) {
  for (auto &item : rankData)
    item.clear();

  rankData.resize(m_comm.size());

  m_pulseTimes.seek(range.eventOffset);
  int ranks = m_comm.size();
  for (size_t event = 0; event < range.eventCount; ++event) {
    // Currently this supports only a hard-coded round-robin partitioning.
    int rank = globalSpectrumIndex[event] % ranks;
    auto index = globalSpectrumIndex[event] / ranks;
    rankData[rank].emplace_back(
        Event{index, eventTimeOffset[event], m_pulseTimes.next()});
  }
}

namespace detail {
/** Uses MPI calls to redistribute chunks which must be processed on certain
 * ranks.
 * @param comm MPI communicator.
 * @param result output data which must be processed on current rank. (May
 * be updated by other ranks)
 * @param data Data on this rank which belongs to several other ranks.
 */
template <class TimeOffsetType>
void redistributeDataMPI(
    Communicator &comm, std::vector<Event<TimeOffsetType>> &result,
    const std::vector<std::vector<Event<TimeOffsetType>>> &data) {
  using Event = Event<TimeOffsetType>;
  if (comm.size() == 1) {
    result = data.front();
    return;
  }

  std::vector<int> sizes(data.size());
  std::transform(data.cbegin(), data.cend(), sizes.begin(),
                 [](const std::vector<Event> &vec) {
                   return static_cast<int>(vec.size());
                 });
  std::vector<int> recv_sizes(data.size());
  Parallel::all_to_all(comm, sizes, recv_sizes);

  auto total_size = std::accumulate(recv_sizes.begin(), recv_sizes.end(), 0);
  result.resize(total_size);
  size_t offset = 0;
  std::vector<Parallel::Request> recv_requests;
  for (int rank = 0; rank < comm.size(); ++rank) {
    if (recv_sizes[rank] == 0)
      continue;
    int tag = 0;
    auto buffer = reinterpret_cast<char *>(result.data() + offset);
    int size = recv_sizes[rank] * static_cast<int>(sizeof(Event));
    recv_requests.emplace_back(comm.irecv(rank, tag, buffer, size));
    offset += recv_sizes[rank];
  }

  std::vector<Parallel::Request> send_requests;
  for (int rank = 0; rank < comm.size(); ++rank) {
    const auto &vec = data[rank];
    if (vec.size() == 0)
      continue;
    int tag = 0;
    send_requests.emplace_back(
        comm.isend(rank, tag, reinterpret_cast<const char *>(vec.data()),
                   static_cast<int>(vec.size() * sizeof(Event))));
  }

  Parallel::wait_all(send_requests.begin(), send_requests.end());
  Parallel::wait_all(recv_requests.begin(), recv_requests.end());
}

/** Fills the workspace EventList with extracted events
 * @param events Events extracted from file according to mpi rank.
 */
template <class TimeOffsetType>
void populateEventLists(
    const std::vector<Event<TimeOffsetType>> &events,
    std::vector<std::vector<Types::Event::TofEvent> *> &eventLists) {
  for (const auto &event : events) {
    eventLists[event.index]->emplace_back(event.tof, event.pulseTime);
    // In general `index` is random so this loop suffers from frequent cache
    // misses (probably because the hardware prefetchers cannot keep up with the
    // number of different memory locations that are getting accessed). We
    // manually prefetch into L2 cache to reduce the amount of misses.
    _mm_prefetch(reinterpret_cast<char *>(&eventLists[event.index]->back() + 1),
                 _MM_HINT_T1);
  }
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
  detail::eventIdToGlobalSpectrumIndex(event_id_start, range.eventCount,
                                       m_bankOffsets[range.bankIndex]);

  // event_id_start now contains globalSpectrumIndex
  extractEventsForRanks(m_allRankData, event_id_start, event_time_offset_start,
                        range);

  detail::redistributeDataMPI(m_comm, m_thisRankData, m_allRankData);
  // TODO: accept something which translates from global to local spectrum index
  populateEventLists(m_thisRankData, m_eventLists);
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
