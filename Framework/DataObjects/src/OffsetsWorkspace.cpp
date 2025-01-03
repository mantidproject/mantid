// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <utility>

#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/OffsetsWorkspace.h"

#include "MantidKernel/IPropertyManager.h"

namespace Mantid::DataObjects {
// Register the workspace
DECLARE_WORKSPACE(OffsetsWorkspace)

//----------------------------------------------------------------------------------------------
/** Constructor, building from an instrument
 *
 * @param inst :: input instrument that is the base for this workspace
 */
OffsetsWorkspace::OffsetsWorkspace(const Geometry::Instrument_const_sptr &inst) : SpecialWorkspace2D(inst) {}

} // namespace Mantid::DataObjects

/// @cond TEMPLATE

namespace Mantid::Kernel {

template <>
DLLExport Mantid::DataObjects::OffsetsWorkspace_sptr
IPropertyManager::getValue<Mantid::DataObjects::OffsetsWorkspace_sptr>(const std::string &name) const {
  auto *prop =
      dynamic_cast<PropertyWithValue<Mantid::DataObjects::OffsetsWorkspace_sptr> *>(getPointerToProperty(name));
  if (prop) {
    return *prop;
  } else {
    std::string message =
        "Attempt to assign property " + name + " to incorrect type. Expected shared_ptr<OffsetsWorkspace>.";
    throw std::runtime_error(message);
  }
}

template <>
DLLExport Mantid::DataObjects::OffsetsWorkspace_const_sptr
IPropertyManager::getValue<Mantid::DataObjects::OffsetsWorkspace_const_sptr>(const std::string &name) const {
  auto *prop =
      dynamic_cast<PropertyWithValue<Mantid::DataObjects::OffsetsWorkspace_sptr> *>(getPointerToProperty(name));
  if (prop) {
    return prop->operator()();
  } else {
    std::string message =
        "Attempt to assign property " + name + " to incorrect type. Expected const shared_ptr<OffsetsWorkspace>.";
    throw std::runtime_error(message);
  }
}

} // namespace Mantid::Kernel

/// @endcond TEMPLATE
