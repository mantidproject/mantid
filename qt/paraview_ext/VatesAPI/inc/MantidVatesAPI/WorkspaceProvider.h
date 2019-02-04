// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_VATESAPI_WORKSPACE_PROVIDER_H
#define MANTID_VATESAPI_WORKSPACE_PROVIDER_H

#include "MantidAPI/Workspace_fwd.h"
#include "MantidKernel/System.h"
#include <string>

namespace Mantid {
namespace VATES {

/**
@class WorkspaceProvider
Abstract type for fetching and disposing of workspaces. ADS instance is a
singleton and therfore very hard to fake in testing. Attempting to test the
behaviour of types using the ADS directly was causing code-bloat. Use this
abstract type instead, which can be mocked in testing. Concrete types can
use the ADS under-the-hood.

@author Owen Arnold, Tessella plc
@date 22/08/2011
*/
class DLLExport WorkspaceProvider {
public:
  virtual bool canProvideWorkspace(std::string wsName) const = 0;
  virtual Mantid::API::Workspace_sptr
  fetchWorkspace(std::string wsName) const = 0;
  virtual void disposeWorkspace(std::string wsName) const = 0;
  virtual ~WorkspaceProvider() {}
};
} // namespace VATES
} // namespace Mantid

#endif
