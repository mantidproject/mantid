#ifndef MANTID_PARALLEL_IO_EVENT_PARSER_H
#define MANTID_PARALLEL_IO_EVENT_PARSER_H

#include "MantidParallel/Collectives.h"
#include "MantidParallel/Communicator.h"
#include "MantidParallel/DllConfig.h"
#include "MantidParallel/IO/Chunker.h"
#include "MantidParallel/IO/EventDataPartitioner.h"
#include "MantidParallel/Nonblocking.h"
#include "MantidTypes/Event/TofEvent.h"

#include <chrono>
#include <cstdint>
#include <numeric>
#include <thread>
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
void MANTID_PARALLEL_DLL eventIdToGlobalSpectrumIndex(int32_t *event_id_start,
                                                      size_t count,
                                                      const int32_t bankOffset);
}

template <class TimeOffsetType> class EventParser {
public:
  using Event = detail::Event<TimeOffsetType>;
  EventParser(const Communicator &comm,
              std::vector<std::vector<int>> rankGroups,
              std::vector<int32_t> bankOffsets,
              std::vector<std::vector<Types::Event::TofEvent> *> eventLists);

  void setEventDataPartitioner(
      std::unique_ptr<AbstractEventDataPartitioner<TimeOffsetType>>
          partitioner);
  void setEventTimeOffsetUnit(const std::string &unit);

  void startAsync(int32_t *event_id_start,
                  const TimeOffsetType *event_time_offset_start,
                  const Chunker::LoadRange &range);

  void wait();

private:
  void doParsing(int32_t *event_id_start,
                 const TimeOffsetType *event_time_offset_start,
                 const Chunker::LoadRange &range);

  void redistributeDataMPI();
  void populateEventLists();

  // Default to 0 such that failure to set unit is easily detected.
  double m_timeOffsetScale{0.0};
  Communicator m_comm;
  std::vector<std::vector<int>> m_rankGroups;
  std::vector<int32_t> m_bankOffsets;
  std::vector<std::vector<Types::Event::TofEvent> *> m_eventLists;
  std::unique_ptr<AbstractEventDataPartitioner<TimeOffsetType>> m_partitioner;
  std::vector<std::vector<Event>> m_allRankData;
  std::vector<Event> m_thisRankData;
  std::thread m_thread;
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
template <class TimeOffsetType>
EventParser<TimeOffsetType>::EventParser(
    const Communicator &comm, std::vector<std::vector<int>> rankGroups,
    std::vector<int32_t> bankOffsets,
    std::vector<std::vector<TofEvent> *> eventLists)
    : m_comm(comm), m_rankGroups(std::move(rankGroups)),
      m_bankOffsets(std::move(bankOffsets)),
      m_eventLists(std::move(eventLists)) {}

/// Set the EventDataPartitioner to use for parsing subsequent events.
template <class TimeOffsetType>
void EventParser<TimeOffsetType>::setEventDataPartitioner(
    std::unique_ptr<AbstractEventDataPartitioner<TimeOffsetType>> partitioner) {
  // We hold (and use) the PulseTimeGenerator via a virtual base class to avoid
  // the need of having IndexType and TimeZeroType as templates for the whole
  // class.
  m_partitioner = std::move(partitioner);
}

/** Set the unit of the values in `event_time_offset`.
 *
 * The unit is used to initialize a scale factor needed for conversion of
 * time-of-flight to microseconds, the unit used by TofEvent. */
template <class TimeOffsetType>
void EventParser<TimeOffsetType>::setEventTimeOffsetUnit(
    const std::string &unit) {
  constexpr char second[] = "second";
  constexpr char microsecond[] = "microsecond";
  constexpr char nanosecond[] = "nanosecond";

  if (unit == second) {
    m_timeOffsetScale = 1e6;
    return;
  }
  if (unit == microsecond) {
    m_timeOffsetScale = 1.0;
    return;
  }
  if (unit == nanosecond) {
    m_timeOffsetScale = 1e-3;
    return;
  }
  throw std::runtime_error("EventParser: unsupported unit `" + unit +
                           "` for event_time_offset");
}

/// Convert m_allRankData into m_thisRankData by means of redistribution via
/// MPI.
template <class TimeOffsetType>
void EventParser<TimeOffsetType>::redistributeDataMPI() {
  if (m_comm.size() == 1) {
    m_thisRankData = m_allRankData.front();
    return;
  }

  std::vector<int> sizes(m_allRankData.size());
  std::transform(m_allRankData.cbegin(), m_allRankData.cend(), sizes.begin(),
                 [](const std::vector<Event> &vec) {
                   return static_cast<int>(vec.size());
                 });
  std::vector<int> recv_sizes(m_allRankData.size());
  Parallel::all_to_all(m_comm, sizes, recv_sizes);

  auto total_size = std::accumulate(recv_sizes.begin(), recv_sizes.end(), 0);
  m_thisRankData.resize(total_size);
  size_t offset = 0;
  std::vector<Parallel::Request> recv_requests;
  for (int rank = 0; rank < m_comm.size(); ++rank) {
    if (recv_sizes[rank] == 0)
      continue;
    int tag = 0;
    auto buffer = reinterpret_cast<char *>(m_thisRankData.data() + offset);
    int size = recv_sizes[rank] * static_cast<int>(sizeof(Event));
    recv_requests.emplace_back(m_comm.irecv(rank, tag, buffer, size));
    offset += recv_sizes[rank];
  }

  std::vector<Parallel::Request> send_requests;
  for (int rank = 0; rank < m_comm.size(); ++rank) {
    const auto &vec = m_allRankData[rank];
    if (vec.size() == 0)
      continue;
    int tag = 0;
    send_requests.emplace_back(
        m_comm.isend(rank, tag, reinterpret_cast<const char *>(vec.data()),
                     static_cast<int>(vec.size() * sizeof(Event))));
  }

  Parallel::wait_all(send_requests.begin(), send_requests.end());
  Parallel::wait_all(recv_requests.begin(), recv_requests.end());
}

/// Append events in m_thisRankData to m_eventLists.
template <class TimeOffsetType>
void EventParser<TimeOffsetType>::populateEventLists() {
  for (const auto &event : m_thisRankData) {
    m_eventLists[event.index]->emplace_back(
        m_timeOffsetScale * static_cast<double>(event.tof), event.pulseTime);
    // In general `index` is random so this loop suffers from frequent cache
    // misses (probably because the hardware prefetchers cannot keep up with the
    // number of different memory locations that are getting accessed). We
    // manually prefetch into L2 cache to reduce the amount of misses.
    _mm_prefetch(
        reinterpret_cast<char *>(&m_eventLists[event.index]->back() + 1),
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
template <class TimeOffsetType>
void EventParser<TimeOffsetType>::startAsync(
    int32_t *event_id_start, const TimeOffsetType *event_time_offset_start,
    const Chunker::LoadRange &range) {
  // Wrapped in lambda because std::thread is unable to specialize doParsing on
  // its own
  m_thread =
      std::thread([this, event_id_start, event_time_offset_start, &range] {
        doParsing(event_id_start, event_time_offset_start, range);
      });
}

template <class TimeOffsetType>
void EventParser<TimeOffsetType>::doParsing(
    int32_t *event_id_start, const TimeOffsetType *event_time_offset_start,
    const Chunker::LoadRange &range) {
  // change event_id_start in place
  detail::eventIdToGlobalSpectrumIndex(event_id_start, range.eventCount,
                                       m_bankOffsets[range.bankIndex]);

  // event_id_start now contains globalSpectrumIndex
  m_partitioner->partition(m_allRankData, event_id_start,
                           event_time_offset_start, range);

  redistributeDataMPI();
  populateEventLists();
}

template <class TimeOffsetType> void EventParser<TimeOffsetType>::wait() {
  m_thread.join();
}

} // namespace IO
} // namespace Parallel
} // namespace Mantid
#endif // MANTID_PARALLEL_IO_EVENT_PARSER_H
