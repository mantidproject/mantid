#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/WorkspacePropertyWithIndex.tcc"
#include "MantidKernel/Exception.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace API {

//----------------------------------------------------------------------------------------------

const std::string IMDHistoWorkspace::toString() const {
  std::ostringstream os;
  os << IMDWorkspace::toString();

  os << MultipleExperimentInfos::toString() << "\n";

  return os.str();
}
} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {

using Mantid::API::IMDHistoWorkspace;
using Mantid::API::IMDHistoWorkspace_sptr;
using Mantid::API::IMDHistoWorkspace_const_sptr;
using Mantid::Indexing::SpectrumIndexSet;
using Mantid::API::WorkspacePropertyWithIndex;

/** In order to be able to cast PropertyWithValue classes correctly a definition
 * for the PropertyWithValue<IMDHistoWorkspace> is required */
template <>
MANTID_API_DLL IMDHistoWorkspace_sptr
IPropertyManager::getValue<IMDHistoWorkspace_sptr>(
    const std::string &name) const {
  PropertyWithValue<IMDHistoWorkspace_sptr> *prop =
      dynamic_cast<PropertyWithValue<IMDHistoWorkspace_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected shared_ptr<IMDHistoWorkspace>.";
    throw std::runtime_error(message);
  }
}

/** In order to be able to cast PropertyWithValue classes correctly a definition
 * for the PropertyWithValue<IMDWorkspace_const_sptr> is required */
template <>
MANTID_API_DLL IMDHistoWorkspace_const_sptr
IPropertyManager::getValue<IMDHistoWorkspace_const_sptr>(
    const std::string &name) const {
  PropertyWithValue<IMDHistoWorkspace_sptr> *prop =
      dynamic_cast<PropertyWithValue<IMDHistoWorkspace_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return prop->operator()();
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected const shared_ptr<IMDHistoWorkspace>.";
    throw std::runtime_error(message);
  }
}

template <>
DLLExport std::tuple<IMDHistoWorkspace_const_sptr, SpectrumIndexSet>
IPropertyManager::getValue<
    std::tuple<IMDHistoWorkspace_const_sptr, SpectrumIndexSet>>(
    const std::string &name) const {
  WorkspacePropertyWithIndex<IMDHistoWorkspace> *prop =
      dynamic_cast<WorkspacePropertyWithIndex<IMDHistoWorkspace> *>(
          getPointerToProperty(name));
  if (prop) {
    return std::tuple<IMDHistoWorkspace_const_sptr, SpectrumIndexSet>(*prop);
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected shared_ptr<IHistoWorkspace>.";
    throw std::runtime_error(message);
  }
}

template <>
DLLExport std::tuple<IMDHistoWorkspace_sptr, SpectrumIndexSet>
IPropertyManager::getValue<
    std::tuple<IMDHistoWorkspace_sptr, SpectrumIndexSet>>(
    const std::string &name) const {
  WorkspacePropertyWithIndex<IMDHistoWorkspace> *prop =
      dynamic_cast<WorkspacePropertyWithIndex<IMDHistoWorkspace> *>(
          getPointerToProperty(name));
  if (prop) {
    return std::tuple<IMDHistoWorkspace_sptr, SpectrumIndexSet>(*prop);
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected shared_ptr<IHistoWorkspace>.";
    throw std::runtime_error(message);
  }
}

// Enable setTypedProperty for IMDHistoWorkspace
template <>
DLLExport IPropertyManager *
IPropertyManager::setTypedProperty<IMDHistoWorkspace_sptr, API::IndexType,
                                   std::vector<int>>(
    const std::string &name,
    const std::tuple<IMDHistoWorkspace_sptr, API::IndexType, std::vector<int>>
        &value) {
  WorkspacePropertyWithIndex<IMDHistoWorkspace> *prop =
      dynamic_cast<WorkspacePropertyWithIndex<IMDHistoWorkspace> *>(
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
DLLExport IPropertyManager *
IPropertyManager::setTypedProperty<IMDHistoWorkspace_sptr, API::IndexType,
                                   std::string>(
    const std::string &name,
    const std::tuple<IMDHistoWorkspace_sptr, API::IndexType, std::string>
        &value) {
  WorkspacePropertyWithIndex<IMDHistoWorkspace> *prop =
      dynamic_cast<WorkspacePropertyWithIndex<IMDHistoWorkspace> *>(
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
