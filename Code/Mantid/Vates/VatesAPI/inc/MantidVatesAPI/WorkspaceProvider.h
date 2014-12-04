#ifndef MANTID_VATESAPI_WORKSPACE_PROVIDER_H
#define MANTID_VATESAPI_WORKSPACE_PROVIDER_H

#include "MantidKernel/System.h"
#include "MantidAPI/Workspace.h"
#include <string>

namespace Mantid
{
  namespace VATES
  {

    /** 
    @class WorkspaceProvider
    Abstract type for fetching and disposing of workspaces. ADS instance is a
    singleton and therfore very hard to fake in testing. Attempting to test the
    behaviour of types using the ADS directly was causing code-bloat. Use this
    abstract type instead, which can be mocked in testing. Concrete types can
    use the ADS under-the-hood.

    @author Owen Arnold, Tessella plc
    @date 22/08/2011

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class DLLExport WorkspaceProvider
    {
    public:
      virtual bool canProvideWorkspace(std::string wsName) const = 0;
      virtual Mantid::API::Workspace_sptr fetchWorkspace(std::string wsName) const = 0;
      virtual void disposeWorkspace(std::string wsName) const = 0;
      virtual ~WorkspaceProvider(){}
    };
  }
}

#endif
