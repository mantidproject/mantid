#include "MantidAPI/ParallelAlgorithm.h"

namespace Mantid {
namespace API {

Parallel::ExecutionMode ParallelAlgorithm::getParallelExecutionMode(
    const std::map<std::string, Parallel::StorageMode> &storageModes) const {
  return Parallel::getCorrespondingExecutionMode(storageModes.begin()->second);
}

} // namespace API
} // namespace Mantid
