#pragma once

#include <H5Cpp.h>
#include <memory>
#include <stack>
#include <string>
#include <vector>

namespace Mantid::DataHandling::AlignAndFocusPowderSlim {

namespace NxsFieldNames {
const std::string TIME_OF_FLIGHT("event_time_offset"); // float32 in ORNL nexus files
const std::string DETID("event_id");                   // uint32 in ORNL nexus files
const std::string INDEX_ID("event_index");
} // namespace NxsFieldNames

class NexusLoader {
public:
  NexusLoader(const bool is_time_filtered, const std::vector<std::pair<size_t, size_t>> &pulse_indices);
  template <typename Type>
  void loadData(H5::DataSet &SDS, std::unique_ptr<std::vector<Type>> &data, const std::vector<size_t> &offsets,
                const std::vector<size_t> &slabsizes);
  std::stack<std::pair<uint64_t, uint64_t>> getEventIndexRanges(H5::Group &event_group, const uint64_t number_events);

private:
  const bool m_is_time_filtered;
  const std::vector<std::pair<size_t, size_t>> m_pulse_indices;
  void loadEventIndex(H5::Group &event_group, std::unique_ptr<std::vector<uint64_t>> &data);
};

} // namespace Mantid::DataHandling::AlignAndFocusPowderSlim
