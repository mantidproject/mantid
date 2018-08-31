#include "MantidParallel/IO/EventLoader.h"
#include "MantidParallel/IO/EventLoaderHelpers.h"
#include "MantidParallel/IO/MultiProcessEventLoader.h"
#include "MantidParallel/IO/NXEventDataLoader.h"

#include <H5Cpp.h>
#include <thread>

namespace Mantid {
namespace Parallel {
namespace IO {
namespace EventLoader {

/** Return a map from any one event ID in a bank to the bank index.
 *
 * For every bank there is one map entry, i.e., this is NOT a mapping from all
 * IDs in a bank to the bank. The returned map will not contain an entry for
 * banks that contain no events. */
std::unordered_map<int32_t, size_t>
makeAnyEventIdToBankMap(const std::string &filename,
                        const std::string &groupName,
                        const std::vector<std::string> &bankNames) {
  std::unordered_map<int32_t, size_t> idToBank;
  H5::H5File file(filename, H5F_ACC_RDONLY);
  H5::Group group = file.openGroup(groupName);
  for (size_t i = 0; i < bankNames.size(); ++i) {
    try {
      int32_t eventId;
      detail::read<int32_t>(&eventId, group, bankNames[i] + "/event_id", 0, 1);
      idToBank[eventId] = i;
    } catch (const std::out_of_range &) {
      // No event in file, do not add to map.
    }
  }
  return idToBank;
}

/// Load events from given banks into event lists using MPI.
void load(const Communicator &comm, const std::string &filename,
          const std::string &groupName,
          const std::vector<std::string> &bankNames,
          const std::vector<int32_t> &bankOffsets,
          std::vector<std::vector<Types::Event::TofEvent> *> eventLists) {
  H5::H5File file(filename, H5F_ACC_RDONLY);
  H5::Group group = file.openGroup(groupName);
  load(readDataType(group, bankNames, "event_time_offset"), comm, group,
       bankNames, bankOffsets, std::move(eventLists));
}

/// Load events from given banks into event lists.
void load(const std::string &filename, const std::string &groupname,
          const std::vector<std::string> &bankNames,
          const std::vector<int32_t> &bankOffsets,
          std::vector<std::vector<Types::Event::TofEvent> *> eventLists) {
  auto num = std::thread::hardware_concurrency();
  MultiProcessEventLoader loader(static_cast<unsigned>(eventLists.size()), num,
                                 num, "");
  loader.load(filename, groupname, bankNames, bankOffsets, eventLists);
}

} // namespace EventLoader

} // namespace IO
} // namespace Parallel
} // namespace Mantid
