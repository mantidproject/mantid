// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/SerialAlgorithm.h"

namespace Mantid {
namespace API {

Parallel::ExecutionMode SerialAlgorithm::getParallelExecutionMode(
    const std::map<std::string, Parallel::StorageMode> &storageModes) const {
  using namespace Parallel;
  const auto nonMasterOnly = std::find_if(
      storageModes.cbegin(), storageModes.cend(), [](const auto &element) {
        return element.second != StorageMode::MasterOnly;
      });
  if (nonMasterOnly != storageModes.cend()) {
    throw std::runtime_error(nonMasterOnly->first + " must have " +
                             Parallel::toString(StorageMode::MasterOnly));
  }
  return getCorrespondingExecutionMode(StorageMode::MasterOnly);
}

} // namespace API
} // namespace Mantid
