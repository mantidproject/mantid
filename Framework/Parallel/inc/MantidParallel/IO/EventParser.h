#ifndef MANTID_PARALLEL_IO_EVENT_PARSER_H
#define MANTID_PARALLEL_IO_EVENT_PARSER_H

#include "MantidParallel/Communicator.h"
#include "MantidParallel/DllConfig.h"
#include "MantidTypes/Event/TofEvent.h"
#include <cstdint>
#include <future>
#include <vector>

namespace Mantid {
namespace Parallel {
namespace IO {
// TODO: replace with Chunker::LoadRange
struct DLLExport LoadRange {
  size_t bankIndex;
  size_t eventOffset;
  size_t eventCount;
};

struct DLLExport EventListEntry {
  int32_t globalIndex; // global spectrum index
  Types::Event::TofEvent tofEvent;
};

/** Distrubuted (MPI) parsing of Nexus events from a data stream. Data is
distributed accross MPI ranks for writing to a event lists.

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
class MANTID_PARALLEL_DLL EventParser {
public:
  EventParser(std::vector<std::vector<int>> rankGroups,
              std::vector<int32_t> bankOffsets,
              std::vector<std::vector<Types::Event::TofEvent> *> &eventLists,
              std::vector<int32_t> globalToLocalSpectrumIndex = {});

  void setPulseInformation(std::vector<IndexType> event_index,
                           std::vector<TimeZeroType> event_time_zero);

  void startAsync(int32_t *event_id_start,
                  TimeOffsetType *event_time_offset_start,
                  const LoadRange &range);

  void redistributeDataMPI(
      std::vector<EventListEntry> &result,
      const std::vector<std::vector<EventListEntry>> &data) const;

  void extractEventsForRanks(std::vector<std::vector<EventListEntry>> &rankData,
                             const int32_t *globalSpectrumIndex,
                             const TimeOffsetType *eventTimeOffset,
                             const LoadRange &range);

  void eventIdToGlobalSpectrumIndex(int32_t *event_id_start, size_t count,
                                    size_t bankIndex) const;

  std::pair<size_t, size_t>
  findStartAndEndPulseIndices(const std::vector<IndexType> &eventIndex,
                              size_t rangeStart, size_t count, size_t &curr);

  void populateEventList(
      std::vector<std::vector<Types::Event::TofEvent> *> &eventList,
      const std::vector<EventListEntry> &events);

  const std::vector<std::vector<EventListEntry>> &rankData() const {
    return m_allRankData;
  }

  const std::vector<std::vector<Types::Event::TofEvent> *> &eventLists() const {
    return m_eventLists;
  }

  void wait() const;

private:
  void doParsing(int32_t *event_id_start,
                 TimeOffsetType *event_time_offset_start,
                 const LoadRange &range);
  std::vector<std::vector<int>> m_rankGroups;
  std::vector<int32_t> m_bankOffsets;
  std::vector<std::vector<Types::Event::TofEvent> *> &m_eventLists;
  std::vector<IndexType> m_eventIndex;
  std::vector<TimeZeroType> m_eventTimeZero;
  std::size_t m_posInEventIndex;
  std::vector<int32_t> m_globalToLocalSpectrumIndex;
  std::vector<std::vector<EventListEntry>> m_allRankData;
  std::vector<EventListEntry> m_thisRankData;
  std::future<void> m_future;
  Communicator m_comm;
};

} // namespace IO
} // namespace Parallel
} // namespace Mantid
#endif // MANTID_PARALLEL_IO_EVENT_PARSER_H