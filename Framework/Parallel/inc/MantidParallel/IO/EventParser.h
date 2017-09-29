#ifndef MANTID_PARALLEL_IO_EVENT_PARSER_H
#define MANTID_PARALLEL_IO_EVENT_PARSER_H

#include "MantidParallel/DllConfig.h"
#include "MantidTypes/TofEvent.h"
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

struct DLLExport Event {
  int32_t index; // global spectrum index
  Mantid::Types::TofEvent tofEvent;
};

template <class IndexType, class TimeZeroType, class TimeOffsetType>
class MANTID_PARALLEL_DLL EventParser { // TODO
public:
  EventParser(std::vector<std::vector<int>> rankGroups,
              const std::vector<int32_t> &bankOffsets,
              std::vector<std::vector<Mantid::Types::TofEvent> *> &eventLists);

  void setPulseInformation(std::vector<IndexType> event_index,
                           std::vector<TimeZeroType> event_time_zero);

  void startParsing(int32_t *event_id_start,
                    TimeOffsetType *event_time_offset_start,
                    const LoadRange &range);

  void extractEventsForRanks(std::vector<std::vector<Event>> &rankData,
                             const int32_t *globalSpectrumIndex,
                             const TimeOffsetType *eventTimeOffset,
                             const LoadRange &range);

  void eventIdToGlobalSpectrumIndex(int32_t *event_id_start, size_t count,
                                    size_t bankIndex);

  void populateEventList(
      std::vector<std::vector<Mantid::Types::TofEvent> *> &eventList,
      const std::vector<Event> &events);

  const std::vector<std::vector<Event>> &rankData() const {
    return m_allRankData;
  }

  void wait() const;
  void finalize() const;

private:
  void doParsing(int32_t *event_id_start,
                 TimeOffsetType *event_time_offset_start,
                 const LoadRange &range);
  std::vector<std::vector<int>> m_rankGroups;
  std::vector<int32_t> m_bankOffsets;
  std::vector<std::vector<Mantid::Types::TofEvent> *> &m_eventLists;
  std::vector<IndexType> m_eventIndex;
  std::vector<TimeZeroType> m_eventTimeZero;
  std::size_t m_posInEventIndex;
  std::vector<std::vector<Event>> m_allRankData;
  std::vector<Event> m_thisRankData;
  std::future<void> m_future;
};

} // namespace IO
} // namespace Parallel
} // namespace Mantid
#endif // MANTID_PARALLEL_IO_EVENT_PARSER_H