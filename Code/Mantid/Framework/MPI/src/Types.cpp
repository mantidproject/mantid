#include "MantidMPI/Types.h"

namespace Mantid {
namespace MPI {

std::string toString(StorageMode mode) {
  switch(mode) {
  case StorageMode::Cloned:
    return "MPI::StorageMode::Cloned";
  case StorageMode::Distributed:
    return "MPI::StorageMode::Distributed";
  case StorageMode::MasterOnly:
    return "MPI::StorageMode::MasterOnly";
  default:
    return "MPI::StorageMode::<undefined>";
  }
}

std::string toString(ExecutionMode mode) {
  switch(mode) {
  case ExecutionMode::Invalid:
    return "MPI::ExecutionMode::Invalid";
  case ExecutionMode::Serial:
    return "MPI::ExecutionMode::Serial";
  case ExecutionMode::Identical:
    return "MPI::ExecutionMode::Identical";
  case ExecutionMode::Distributed:
    return "MPI::ExecutionMode::Distributed";
  case ExecutionMode::MasterOnly:
    return "MPI::ExecutionMode::MasterOnly";
  default:
    return "MPI::ExecutionMode::<undefined>";
  }
}

std::string toString(const std::map<std::string, StorageMode> &map) {
  std::string ret("\n");
  for(const auto &item : map)
    ret.append(item.first + " " + toString(item.second) + "\n");
  return ret;
}

} // namespace MPI
} // namespace Mantid
