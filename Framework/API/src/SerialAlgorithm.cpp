#include "MantidAPI/SerialAlgorithm.h"

namespace Mantid {
namespace API {

Parallel::ExecutionMode SerialAlgorithm::getParallelExecutionMode(
    const std::map<std::string, Parallel::StorageMode> &storageModes) const {
  using namespace Parallel;
  for (const auto &item : storageModes)
    if (item.second != StorageMode::MasterOnly)
      throw std::runtime_error(item.first + " must have " +
                               Parallel::toString(StorageMode::MasterOnly));
  return getCorrespondingExecutionMode(StorageMode::MasterOnly);
}

} // namespace API
} // namespace Mantid
