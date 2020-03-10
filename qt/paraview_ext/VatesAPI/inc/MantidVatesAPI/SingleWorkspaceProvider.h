// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Workspace_fwd.h"
#include "WorkspaceProvider.h"

namespace Mantid {
namespace VATES {

/** SingleWorkspaceProvider : Provides a single workspace instead of
    serving a client with workspaces from the ADS
*/
class DLLExport SingleWorkspaceProvider : public WorkspaceProvider {
public:
  SingleWorkspaceProvider(Mantid::API::Workspace_sptr workspace);
  bool canProvideWorkspace(std::string wsName) const override;
  Mantid::API::Workspace_sptr fetchWorkspace(std::string wsName) const override;
  void disposeWorkspace(std::string wsName) const override;

private:
  Mantid::API::Workspace_sptr m_workspace;
};
} // namespace VATES
} // namespace Mantid
