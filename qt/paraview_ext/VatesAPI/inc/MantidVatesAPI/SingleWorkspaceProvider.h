#ifndef VATES_API_SINGLE_WORKSPACE_PROVIDER_H_
#define VATES_API_SINGLE_WORKSPACE_PROVIDER_H_

#include "MantidAPI/Workspace_fwd.h"
#include "WorkspaceProvider.h"

namespace Mantid {
namespace VATES {

/** SingleWorkspaceProvider : Provides a single workspace instead of
    serving a client with workspaces from the ADS

Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
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

#endif
