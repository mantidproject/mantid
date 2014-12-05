#ifndef MANTID_VATESAPI_ADSWORKSPACEPROVIDER_H
#define MANTID_VATESAPI_ADSWORKSPACEPROVIDER_H

#include "MantidVatesAPI/WorkspaceProvider.h"

namespace Mantid
{
  namespace VATES
  {
     /** 
    @class ADSWorkspaceProvider
    Type for fetching and disposing of workspaces using the Mantid Analysis Data Service Instance under-the-hood.

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
    template<typename Workspace_Type>
    class DLLExport ADSWorkspaceProvider : public WorkspaceProvider
    {
    public:
      ADSWorkspaceProvider();
      ~ADSWorkspaceProvider();

      //-------WorkspaceProivder Implementations ------------
      bool canProvideWorkspace(std::string wsName) const;
      Mantid::API::Workspace_sptr fetchWorkspace(std::string wsName) const;
      void disposeWorkspace(std::string wsName) const;
    private:
      ADSWorkspaceProvider& operator=(const ADSWorkspaceProvider&);
      ADSWorkspaceProvider(const ADSWorkspaceProvider&);
    };
  }
}

#endif
