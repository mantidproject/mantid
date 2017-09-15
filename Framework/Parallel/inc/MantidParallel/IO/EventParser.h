#ifndef MANTID_PARALLEL_IO_EVENT_PARSER_H
#define MANTID_PARALLEL_IO_EVENT_PARSER_H

#include <cstdint>
#include <vector>

namespace Mantid {
namespace Types {
class TofEvent;
}

namespace Parallel {
namespace IO {

class EventParser { // TODO
public:
  EventParser(std::vector<std::vector<int>> rankGroups,
              std::vector<int32_t> &&bankOffsets,
              std::vector<std::vector<Mantid::Types::TofEvent> *> &eventLists);

  void setPulseInformation(std::vector<int32_t> event_index,
                           std::vector<int64_t> event_time_zero);

  void startParsing(int32_t *event_id_start, int32_t *event_time_offset_start,
                    size_t count);
  void wait();
  void finalize();

private:
  std::vector<int32_t> m_bankOffsets;
  std::vector<std::vector<Mantid::Types::TofEvent> *> &m_eventLists;
  std::vector<int32_t> m_event_index;
  std::vector<int64_t> m_event_time_zero;
};

} // namespace IO
} // namespace Parallel
} // namespace Mantid
#endif // MANTID_PARALLEL_IO_EVENT_PARSER_H