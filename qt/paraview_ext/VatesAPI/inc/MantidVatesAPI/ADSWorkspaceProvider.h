// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_VATESAPI_ADSWORKSPACEPROVIDER_H
#define MANTID_VATESAPI_ADSWORKSPACEPROVIDER_H

#include "MantidVatesAPI/WorkspaceProvider.h"

namespace Mantid {
namespace VATES {
/**
@class ADSWorkspaceProvider
Type for fetching and disposing of workspaces using the Mantid Analysis Data
Service Instance under-the-hood.

@author Owen Arnold, Tessella plc
@date 22/08/2011
*/
template <typename Workspace_Type>
class DLLExport ADSWorkspaceProvider : public WorkspaceProvider {
public:
  ADSWorkspaceProvider() = default;
  ADSWorkspaceProvider &operator=(const ADSWorkspaceProvider &) = delete;
  ADSWorkspaceProvider(const ADSWorkspaceProvider &) = delete;

  //-------WorkspaceProivder Implementations ------------
  bool canProvideWorkspace(std::string wsName) const override;
  Mantid::API::Workspace_sptr fetchWorkspace(std::string wsName) const override;
  void disposeWorkspace(std::string wsName) const override;

private:
};
} // namespace VATES
} // namespace Mantid

#endif
