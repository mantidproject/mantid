// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/HistoWorkspace.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceProperty.h"

namespace {

template <typename T>
void setPropertyModeForWorkspaceProperties(Mantid::Kernel::Property *prop,
                                           const Mantid::API::PropertyMode::Type &optional) {
  if (auto workspaceProperty = dynamic_cast<Mantid::API::WorkspaceProperty<T> *>(prop)) {
    workspaceProperty->setPropertyMode(optional);
  }
}

template <typename T, typename... Ts, typename = std::enable_if_t<sizeof...(Ts) != 0>>
void setPropertyModeForWorkspaceProperties(Mantid::Kernel::Property *prop,
                                           const Mantid::API::PropertyMode::Type &optional) {
  setPropertyModeForWorkspaceProperties<T>(prop, optional);
  setPropertyModeForWorkspaceProperties<Ts...>(prop, optional);
}

} // namespace

namespace Mantid {
namespace API {

void MANTID_API_DLL setPropertyModeForWorkspaceProperty(Mantid::Kernel::Property *prop,
                                                        const PropertyMode::Type &optional) {
  setPropertyModeForWorkspaceProperties<API::Workspace, API::MatrixWorkspace, API::HistoWorkspace, API::WorkspaceGroup,
                                        API::IEventWorkspace, API::IMDHistoWorkspace, API::IPeaksWorkspace,
                                        API::ITableWorkspace>(prop, optional);
}

} // namespace API
} // namespace Mantid