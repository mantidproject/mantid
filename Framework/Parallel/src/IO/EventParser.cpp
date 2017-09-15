#include "MantidParallel/IO/EventParser.h"
#include "MantidTypes/TofEvent.h"

using Mantid::Types::TofEvent;

namespace Mantid {
namespace Parallel {
namespace IO {

template <class IndexType, class TimeZeroType, class TimeOffsetType>
EventParser<IndexType, TimeZeroType, TimeOffsetType>::EventParser(
    std::vector<std::vector<int>> rankGroups,
    std::vector<int32_t> &&bankOffsets,
    std::vector<std::vector<TofEvent> *> &eventLists)
    : m_bankOffsets(std::move(bankOffsets)), m_eventLists(eventLists) {}

template <class IndexType, class TimeZeroType, class TimeOffsetType>
void EventParser<IndexType, TimeZeroType, TimeOffsetType>::setPulseInformation(
    std::vector<IndexType> event_index,
    std::vector<TimeZeroType> event_time_zero) {
  m_event_index = event_index;
  m_event_time_zero = event_time_zero;
}

template <class IndexType, class TimeZeroType, class TimeOffsetType>
void EventParser<IndexType, TimeZeroType, TimeOffsetType>::startParsing(
    int32_t *event_id_start, TimeOffsetType *event_time_offset_start,
    size_t count) {}

template <class IndexType, class TimeZeroType, class TimeOffsetType>
void EventParser<IndexType, TimeZeroType, TimeOffsetType>::wait() {}

template <class IndexType, class TimeZeroType, class TimeOffsetType>
void EventParser<IndexType, TimeZeroType, TimeOffsetType>::finalize() {}
} // namespace IO
} // namespace Parallel
} // namespace Mantid