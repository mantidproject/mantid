#include "MantidAPI/DistributedAlgorithm.h"

namespace Mantid {
namespace API {

Parallel::ExecutionMode DistributedAlgorithm::getParallelExecutionMode(
    const std::map<std::string, Parallel::StorageMode> &storageModes) const {
  return Parallel::getCorrespondingExecutionMode(storageModes.begin()->second);
}

} // namespace API
} // namespace Mantid
