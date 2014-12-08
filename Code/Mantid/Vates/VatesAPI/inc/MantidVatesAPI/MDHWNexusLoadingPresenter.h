#ifndef MANTID_VATES_MDHW_NEXUS_LOADING_PRESENTER
#define MANTID_VATES_MDHW_NEXUS_LOADING_PRESENTER

#include "MantidVatesAPI/MDHWLoadingPresenter.h"

namespace Mantid
{
namespace VATES
{
/**
    @class MDHWNexusLoadingPresenter
    For loading conversion of MDHW workspaces into render-able vtk objects.

    @date 08/04/2013

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
class DLLExport MDHWNexusLoadingPresenter : public MDHWLoadingPresenter
{
public:
  MDHWNexusLoadingPresenter(MDLoadingView* view,
                            const std::string fileName);
  virtual vtkDataSet* execute(vtkDataSetFactory* factory,
                              ProgressAction& rebinningProgressUpdate,
                              ProgressAction& drawingProgressUpdate);
  virtual void executeLoadMetadata();
  virtual ~MDHWNexusLoadingPresenter();
  virtual bool canReadFile() const;
  virtual std::string getWorkspaceTypeName();
private:
  const std::string m_filename;
  std::string m_wsTypeName;
};
}
}

#endif
