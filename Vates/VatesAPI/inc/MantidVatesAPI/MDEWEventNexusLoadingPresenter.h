#ifndef MANTID_VATES_MDEW_EVENT_NEXUS_LOADING_PRESENTER
#define MANTID_VATES_MDEW_EVENT_NEXUS_LOADING_PRESENTER

#include "MantidVatesAPI/MDEWLoadingPresenter.h"

namespace Mantid
{
  namespace VATES
  {
    /** 
    @class MDEWEventNexusLoadingPresenter
    For loading conversion of MDEW workspaces into render-able vtk objects.
    @author Owen Arnold, Tessella plc
    @date 09/08/2011

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    class MDLoadingView;
    class DLLExport MDEWEventNexusLoadingPresenter : public MDEWLoadingPresenter
    {
    public:
      MDEWEventNexusLoadingPresenter(MDLoadingView* view, const std::string fileName);
      virtual vtkDataSet* execute(vtkDataSetFactory* factory, ProgressAction& rebinningProgressUpdate, ProgressAction& drawingProgressUpdate);
      virtual void executeLoadMetadata();
      virtual ~MDEWEventNexusLoadingPresenter();
      virtual bool canReadFile() const;
      virtual std::string getWorkspaceTypeName();
    private:
      const std::string m_filename;
      std::string m_wsTypeName;
    };
  }
}

#endif
