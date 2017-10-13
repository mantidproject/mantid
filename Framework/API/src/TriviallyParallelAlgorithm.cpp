#include "MantidAPI/TriviallyParallelAlgorithm.h"

namespace Mantid {
namespace API {

Parallel::ExecutionMode TriviallyParallelAlgorithm::getParallelExecutionMode(
    const std::map<std::string, Parallel::StorageMode> &storageModes) const {
  return Parallel::getCorrespondingExecutionMode(storageModes.begin()->second);
}

} // namespace API
} // namespace Mantid
