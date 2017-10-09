#include <H5Cpp.h>

#include "MantidParallel/IO/EventLoader.h"
#include "MantidParallel/IO/EventLoaderHelpers.h"

namespace Mantid {
namespace Parallel {
namespace IO {
namespace EventLoader {

void load(const std::string &filename, const std::string &groupName,
          const std::vector<std::string> &bankNames,
          const std::vector<int32_t> &bankOffsets,
          std::vector<std::vector<Types::Event::TofEvent> *> eventLists) {
  H5::H5File file(filename, H5F_ACC_RDONLY);
  H5::Group group = file.openGroup(groupName);
  load(readDataType(group, bankNames, "event_index"),
       readDataType(group, bankNames, "event_time_zero"),
       readDataType(group, bankNames, "event_time_offset"), group, bankNames,
       bankOffsets, eventLists);
}
}

} // namespace IO
} // namespace Parallel
} // namespace Mantid
