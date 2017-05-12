#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/ISplittersWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"

// WorkspacePropertyWithIndex implementation
#include "MantidAPI/WorkspacePropertyWithIndex.tcc"

namespace Mantid {
namespace API {
///@cond TEMPLATE
template class MANTID_API_DLL
    Mantid::API::WorkspacePropertyWithIndex<Mantid::API::Workspace>;
template class MANTID_API_DLL
    Mantid::API::WorkspacePropertyWithIndex<Mantid::API::IEventWorkspace>;
template class MANTID_API_DLL
    Mantid::API::WorkspacePropertyWithIndex<Mantid::API::IMDEventWorkspace>;
template class MANTID_API_DLL
    Mantid::API::WorkspacePropertyWithIndex<Mantid::API::IMDHistoWorkspace>;
template class MANTID_API_DLL
    Mantid::API::WorkspacePropertyWithIndex<Mantid::API::IMDWorkspace>;
template class MANTID_API_DLL
    Mantid::API::WorkspacePropertyWithIndex<Mantid::API::MatrixWorkspace>;

///@endcond TEMPLATE
} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {
template <>
MANTID_API_DLL IPropertyManager *
IPropertyManager::setTypedProperty<API::MatrixWorkspace_sptr, API::IndexType,
                                   std::vector<int>>(
    const std::string &name,
    const std::tuple<API::MatrixWorkspace_sptr, API::IndexType,
                     std::vector<int>> &value) {
  API::WorkspacePropertyWithIndex<API::MatrixWorkspace> *prop =
      dynamic_cast<API::WorkspacePropertyWithIndex<API::MatrixWorkspace> *>(
          getPointerToProperty(name));
  if (prop) {
    *prop = value;
  } else {
    throw std::invalid_argument("Attempt to assign to property (" + name +
                                ") of incorrect type");
  }
  return this;
}

template <>
MANTID_API_DLL IPropertyManager *
IPropertyManager::setTypedProperty<API::MatrixWorkspace_sptr, API::IndexType,
                                   std::string>(
    const std::string &name,
    const std::tuple<API::MatrixWorkspace_sptr, API::IndexType, std::string>
        &value) {
  API::WorkspacePropertyWithIndex<API::MatrixWorkspace> *prop =
      dynamic_cast<API::WorkspacePropertyWithIndex<API::MatrixWorkspace> *>(
          getPointerToProperty(name));
  if (prop) {
    *prop = value;
  } else {
    throw std::invalid_argument("Attempt to assign to property (" + name +
                                ") of incorrect type");
  }
  return this;
}
} // namespace Kernel
} // namespace Mantid