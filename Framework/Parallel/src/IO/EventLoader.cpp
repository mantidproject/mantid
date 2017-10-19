#include "MantidParallel/IO/EventLoader.h"
#include "MantidParallel/IO/EventLoaderHelpers.h"
#include "MantidParallel/IO/NXEventDataLoader.h"

#include <H5Cpp.h>

namespace Mantid {
namespace Parallel {
namespace IO {
namespace EventLoader {

std::vector<boost::optional<int32_t>>
anyEventIdFromBanks(const std::string &filename, const std::string &groupName,
                    const std::vector<std::string> &bankNames) {
  std::vector<boost::optional<int32_t>> eventIds(bankNames.size());
  H5::H5File file(filename, H5F_ACC_RDONLY);
  H5::Group group = file.openGroup(groupName);
  for (size_t i = 0; i < bankNames.size(); ++i) {
    try {
      int32_t eventId;
      detail::read<int32_t>(&eventId, group, bankNames[i] + "/event_id", 0, 1);
      eventIds[i] = eventId;
    } catch (const std::out_of_range &) {
      // No event in file, keep eventIds uninitialized for this bank.
    }
  }
  return eventIds;
}

void load(const std::string &filename, const std::string &groupName,
          const std::vector<std::string> &bankNames,
          const std::vector<int32_t> &bankOffsets,
          std::vector<std::vector<Types::Event::TofEvent> *> eventLists) {
  H5::H5File file(filename, H5F_ACC_RDONLY);
  H5::Group group = file.openGroup(groupName);
  load(readDataType(group, bankNames, "event_index"),
       readDataType(group, bankNames, "event_time_zero"),
       readDataType(group, bankNames, "event_time_offset"), group, bankNames,
       bankOffsets, std::move(eventLists));
}
}

} // namespace IO
} // namespace Parallel
} // namespace Mantid
