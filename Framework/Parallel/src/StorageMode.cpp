#include "MantidParallel/StorageMode.h"

namespace Mantid {
namespace Parallel {

/// Returns a human-readable string representation of a StorageMode.
std::string toString(StorageMode mode) {
  switch (mode) {
  case StorageMode::Cloned:
    return "Parallel::StorageMode::Cloned";
  case StorageMode::Distributed:
    return "Parallel::StorageMode::Distributed";
  case StorageMode::MasterOnly:
    return "Parallel::StorageMode::MasterOnly";
  default:
    return "Parallel::StorageMode::<undefined>";
  }
}

/// Returns a human-readable string representation of a StorageMode map.
std::string toString(const std::map<std::string, StorageMode> &map) {
  std::string ret("\n");
  for (const auto &item : map)
    ret.append(item.first + " " + toString(item.second) + "\n");
  return ret;
}

} // namespace Parallel
} // namespace Mantid
