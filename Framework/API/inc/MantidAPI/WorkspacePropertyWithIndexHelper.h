#ifndef MANTID_API_WORKSPACEWITHINDEXHELPER_H_
#define MANTID_API_WORKSPACEWITHINDEXHELPER_H_

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/WorkspacePropertyWithIndex.h"
#include "MantidAPI/WorkspacePropertyWithIndex.tcc"
#include "MantidKernel/make_unique.h"
#include <memory>

namespace Mantid {
namespace API {

template <typename T>
DLLExport std::unique_ptr<WorkspacePropertyWithIndex<T>>
createWithSpectrumNumbers(
    const std::string &propertyName = "InputWorkspaceWithIndex") {
  return Mantid::Kernel::make_unique<WorkspacePropertyWithIndex<T>>(
      propertyName, IndexType::SpectrumNumber);
}

template <typename T>
DLLExport std::unique_ptr<WorkspacePropertyWithIndex<T>>
createWithWorkspaceIndices(
    const std::string &propertyName = "InputWorkspaceWithIndex") {
  return Mantid::Kernel::make_unique<WorkspacePropertyWithIndex<T>>(
      propertyName, IndexType::WorkspaceIndex);
}

template <typename T>
DLLExport std::unique_ptr<WorkspacePropertyWithIndex<T>>
createWithAllIndexTypes(
    const std::string &propertyName = "InputWorkspaceWithIndex") {
  return Mantid::Kernel::make_unique<WorkspacePropertyWithIndex<T>>(
      propertyName, IndexType::WorkspaceIndex | IndexType::SpectrumNumber);
}

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_WORKSPACEWITHINDEXHELPER_H_ */