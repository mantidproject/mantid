#include "MantidAPI/ParallelAlgorithm.h"

#include <algorithm>

namespace Mantid {
namespace API {

Parallel::ExecutionMode ParallelAlgorithm::getParallelExecutionMode(
    const std::map<std::string, Parallel::StorageMode> &storageModes) const {
  using namespace Parallel;
  // Match all cloned or empty.
  if (std::all_of(
          storageModes.begin(), storageModes.end(),
          [](const std::pair<std::string, Parallel::StorageMode> &item) {
            return item.second == StorageMode::Cloned;
          }))
    return getCorrespondingExecutionMode(StorageMode::Cloned);
  // Match all master-only (empty handled earlier).
  if (std::all_of(
          storageModes.begin(), storageModes.end(),
          [](const std::pair<std::string, Parallel::StorageMode> &item) {
            return item.second == StorageMode::MasterOnly;
          }))
    return getCorrespondingExecutionMode(StorageMode::MasterOnly);
  return ExecutionMode::Invalid;
}

} // namespace API
} // namespace Mantid
