#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/WorkspacePropertyWithIndex.h"
#include "MantidKernel/IPropertyManager.h"
#include <MantidIndexing/SpectrumIndexSet.h>

namespace Mantid {

namespace API {

/**
 */
const std::string IEventWorkspace::toString() const {
  std::ostringstream os;
  os << MatrixWorkspace::toString() << "\n";

  os << "Events: " + std::to_string(getNumberEvents());
  switch (getEventType()) {
  case WEIGHTED:
    os << " (weighted)\n";
    break;
  case WEIGHTED_NOTIME:
    os << " (weighted, no times)\n";
    break;
  case TOF:
    os << "\n";
    break;
  }
  return os.str();
}
} // namespace API

///\cond TEMPLATE
/*
 * In order to be able to cast PropertyWithValue classes correctly a definition
 *for the PropertyWithValue<IEventWorkspace> is required
 *
 */
namespace Kernel {

using Mantid::API::IEventWorkspace;
using Mantid::API::IEventWorkspace_sptr;
using Mantid::API::IEventWorkspace_const_sptr;
using Mantid::Indexing::SpectrumIndexSet;
using Mantid::API::WorkspacePropertyWithIndex;

template <>
MANTID_API_DLL IEventWorkspace_sptr
IPropertyManager::getValue<IEventWorkspace_sptr>(
    const std::string &name) const {
  PropertyWithValue<IEventWorkspace_sptr> *prop =
      dynamic_cast<PropertyWithValue<IEventWorkspace_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected shared_ptr<IEventWorkspace>.";
    throw std::runtime_error(message);
  }
}

template <>
MANTID_API_DLL IEventWorkspace_const_sptr
IPropertyManager::getValue<IEventWorkspace_const_sptr>(
    const std::string &name) const {
  PropertyWithValue<IEventWorkspace_sptr> *prop =
      dynamic_cast<PropertyWithValue<IEventWorkspace_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return prop->operator()();
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected const shared_ptr<IEventWorkspace>.";
    throw std::runtime_error(message);
  }
}

template <>
DLLExport std::tuple<IEventWorkspace_const_sptr, SpectrumIndexSet>
IPropertyManager::getValue<
    std::tuple<IEventWorkspace_const_sptr, SpectrumIndexSet>>(
    const std::string &name) const {
  WorkspacePropertyWithIndex<IEventWorkspace> *prop =
      dynamic_cast<WorkspacePropertyWithIndex<IEventWorkspace> *>(
          getPointerToProperty(name));
  if (prop) {
    return std::tuple<IEventWorkspace_const_sptr, SpectrumIndexSet>(*prop);
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected shared_ptr<IEventWorkspace>.";
    throw std::runtime_error(message);
  }
}

template <>
DLLExport std::tuple<IEventWorkspace_sptr, SpectrumIndexSet>
IPropertyManager::getValue<std::tuple<IEventWorkspace_sptr, SpectrumIndexSet>>(
    const std::string &name) const {
  WorkspacePropertyWithIndex<IEventWorkspace> *prop =
      dynamic_cast<WorkspacePropertyWithIndex<IEventWorkspace> *>(
          getPointerToProperty(name));
  if (prop) {
    return std::tuple<IEventWorkspace_sptr, SpectrumIndexSet>(*prop);
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected shared_ptr<IEventWorkspace>.";
    throw std::runtime_error(message);
  }
}

// Enable setTypedProperty for IEventWorkspace
template <>
DLLExport IPropertyManager *
IPropertyManager::setTypedProperty<IEventWorkspace_sptr, API::IndexType,
                                   std::vector<int>>(
    const std::string &name,
    const std::tuple<IEventWorkspace_sptr, API::IndexType, std::vector<int>> &
        value) {
  WorkspacePropertyWithIndex<IEventWorkspace> *prop =
      dynamic_cast<WorkspacePropertyWithIndex<IEventWorkspace> *>(
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
IPropertyManager::setTypedProperty<IEventWorkspace_sptr, API::IndexType,
                                   std::string>(
    const std::string &name,
    const std::tuple<IEventWorkspace_sptr, API::IndexType, std::string> &
        value) {
  WorkspacePropertyWithIndex<IEventWorkspace> *prop =
      dynamic_cast<WorkspacePropertyWithIndex<IEventWorkspace> *>(
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

///\endcond TEMPLATE
