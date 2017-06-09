#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspacePropertyWithIndex.tcc"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/System.h"

using Mantid::API::SpectraAxis;

namespace Mantid {
namespace DataObjects {
// Register the workspace
DECLARE_WORKSPACE(OffsetsWorkspace)

//----------------------------------------------------------------------------------------------
/** Constructor, building from an instrument
 *
 * @param inst :: input instrument that is the base for this workspace
 * @return created OffsetsWorkspace
 */
OffsetsWorkspace::OffsetsWorkspace(Geometry::Instrument_const_sptr inst)
    : SpecialWorkspace2D(inst) {}

} // namespace DataObjects
} // namespace Mantid

/// @cond TEMPLATE

namespace Mantid {
namespace Kernel {

using Mantid::DataObjects::OffsetsWorkspace;
using Mantid::DataObjects::OffsetsWorkspace_sptr;
using Mantid::DataObjects::OffsetsWorkspace_const_sptr;
using Mantid::Indexing::SpectrumIndexSet;
using Mantid::API::WorkspacePropertyWithIndex;

template <>
DLLExport OffsetsWorkspace_sptr
IPropertyManager::getValue<OffsetsWorkspace_sptr>(
    const std::string &name) const {
  PropertyWithValue<OffsetsWorkspace_sptr> *prop =
      dynamic_cast<PropertyWithValue<OffsetsWorkspace_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected shared_ptr<OffsetsWorkspace>.";
    throw std::runtime_error(message);
  }
}

template <>
DLLExport OffsetsWorkspace_const_sptr
IPropertyManager::getValue<OffsetsWorkspace_const_sptr>(
    const std::string &name) const {
  PropertyWithValue<OffsetsWorkspace_sptr> *prop =
      dynamic_cast<PropertyWithValue<OffsetsWorkspace_sptr> *>(
          getPointerToProperty(name));
  if (prop) {
    return prop->operator()();
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected const shared_ptr<OffsetsWorkspace>.";
    throw std::runtime_error(message);
  }
}

template <>
DLLExport std::tuple<OffsetsWorkspace_sptr &, SpectrumIndexSet &>
IPropertyManager::getValue<
    std::tuple<OffsetsWorkspace_sptr &, SpectrumIndexSet &>>(
    const std::string &name) const {
  WorkspacePropertyWithIndex<OffsetsWorkspace> *prop =
      dynamic_cast<WorkspacePropertyWithIndex<OffsetsWorkspace> *>(
          getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected shared_ptr<IOffsetsWorkspace>.";
    throw std::runtime_error(message);
  }
}

template <>
DLLExport std::tuple<OffsetsWorkspace_const_sptr &, SpectrumIndexSet &>
IPropertyManager::getValue<
    std::tuple<OffsetsWorkspace_const_sptr &, SpectrumIndexSet &>>(
    const std::string &name) const {
  WorkspacePropertyWithIndex<OffsetsWorkspace> *prop =
      dynamic_cast<WorkspacePropertyWithIndex<OffsetsWorkspace> *>(
          getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message =
        "Attempt to assign property " + name +
        " to incorrect type. Expected shared_ptr<IOffsetsWorkspace>.";
    throw std::runtime_error(message);
  }
}

// Enable setTypedProperty for OffsetsWorkspace
template <>
DLLExport IPropertyManager *
IPropertyManager::setTypedProperty<OffsetsWorkspace_sptr, API::IndexType,
                                   std::vector<int>>(
    const std::string &name,
    const std::tuple<OffsetsWorkspace_sptr, API::IndexType, std::vector<int>>
        &value) {
  WorkspacePropertyWithIndex<OffsetsWorkspace> *prop =
      dynamic_cast<WorkspacePropertyWithIndex<OffsetsWorkspace> *>(
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
IPropertyManager::setTypedProperty<OffsetsWorkspace_sptr, API::IndexType,
                                   std::string>(
    const std::string &name,
    const std::tuple<OffsetsWorkspace_sptr, API::IndexType, std::string>
        &value) {
  WorkspacePropertyWithIndex<OffsetsWorkspace> *prop =
      dynamic_cast<WorkspacePropertyWithIndex<OffsetsWorkspace> *>(
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

/// @endcond TEMPLATE
