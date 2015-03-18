#ifndef MANTID_VATES_MDHW_IN_MEMORY_LOADING_PRESENTER
#define MANTID_VATES_MDHW_IN_MEMORY_LOADING_PRESENTER

#include "MantidVatesAPI/MDHWLoadingPresenter.h"
#include <boost/scoped_ptr.hpp>
#include <vector>

class vtkDataSet;
namespace Mantid
{
  namespace VATES
  {
    /** 
    @class MDHWInMemoryLoadingPresenter
    Presenter for loading MDHWs directly from the ADS, does not touch the disk.
    @date 02/12/2011

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
    class WorkspaceProvider;
    class MetaDataExtractorUtils;
    class vtkDataSetFactory;
    
    class DLLExport MDHWInMemoryLoadingPresenter : public MDHWLoadingPresenter
    {
    public:
      MDHWInMemoryLoadingPresenter(MDLoadingView* view, WorkspaceProvider* repository, std::string wsName);
      virtual vtkDataSet* execute(vtkDataSetFactory* factory, ProgressAction& rebinningProgressUpdate, ProgressAction& drawingProgressUpdate);
      virtual void executeLoadMetadata();
      virtual ~MDHWInMemoryLoadingPresenter();
      virtual bool canReadFile() const;
      virtual std::string getWorkspaceTypeName();
      virtual int getSpecialCoordinates();
      std::vector<int> getExtents();
    private:
      /// Repository for accessing workspaces. At this level, does not specify how or where from.
      boost::scoped_ptr<WorkspaceProvider> m_repository;
      /// The name of the workspace.
      const std::string m_wsName;
      std::string m_wsTypeName;
      int m_specialCoords;
    };
  }
}

#endif
