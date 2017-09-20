#ifndef MANTID_PARALLEL_IO_EVENT_PARSER_H
#define MANTID_PARALLEL_IO_EVENT_PARSER_H

#include "MantidParallel/DllConfig.h"
#include "MantidTypes/TofEvent.h"
#include <cstdint>
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

  void
  setPulseInformation(const std::vector<IndexType> &event_index,
                      const std::vector<TimeZeroType> &event_time_zero) const;

  void startParsing(int32_t *event_id_start,
                    TimeOffsetType *event_time_offset_start,
                    const LoadRange &range);

  void extractEventsForRanks(const std::vector<int32_t> &globalSpectrumIndex,
                             const TimeOffsetType *eventTimeOffset,
                             size_t offset);

  void eventIdToGlobalSpectrumIndex(int32_t *event_id_start, size_t count,
                                    size_t bankIndex) const;

  void populateEventList(
      std::vector<std::vector<Mantid::Types::TofEvent> *> &eventList,
      const std::vector<Event> &events);

  const std::vector<int32_t> &globalSpectrumIndex() const {
    return m_globalSpectrumIndex;
  }

  const std::vector<std::vector<Event>> &rankData() const {
    return m_allRankData;
  }

  void wait();
  void finalize();

private:
  std::vector<std::vector<int>> m_rankGroups;
  std::vector<int32_t> m_bankOffsets;
  std::vector<std::vector<Mantid::Types::TofEvent> *> &m_eventLists;
  mutable std::vector<IndexType> m_eventIndex;
  mutable std::vector<TimeZeroType> m_eventTimeZero;
  mutable std::vector<int32_t> m_globalSpectrumIndex;
  std::size_t m_posInEventIndex;
  std::vector<std::vector<Event>> m_allRankData;
};

} // namespace IO
} // namespace Parallel
} // namespace Mantid
#endif // MANTID_PARALLEL_IO_EVENT_PARSER_H