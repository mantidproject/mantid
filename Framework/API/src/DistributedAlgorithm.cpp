// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/DistributedAlgorithm.h"

#include <algorithm>

namespace Mantid {
namespace API {

Parallel::ExecutionMode
DistributedAlgorithm::getParallelExecutionMode(const std::map<std::string, Parallel::StorageMode> &storageModes) const {
  using namespace Parallel;
  if (std::any_of(storageModes.begin(), storageModes.end(),
                  [](const std::pair<std::string, Parallel::StorageMode> &item) {
                    return item.second == StorageMode::Distributed;
                  })) {
    if (std::any_of(storageModes.begin(), storageModes.end(),
                    [](const std::pair<std::string, Parallel::StorageMode> &item) {
                      return item.second == StorageMode::MasterOnly;
                    }))
      return ExecutionMode::Invalid;
    return ExecutionMode::Distributed;
  }
  if (std::any_of(storageModes.begin(), storageModes.end(),
                  [](const std::pair<std::string, Parallel::StorageMode> &item) {
                    return item.second == StorageMode::MasterOnly;
                  }))
    return ExecutionMode::MasterOnly;
  return ExecutionMode::Identical;
}

} // namespace API
} // namespace Mantid
