#ifndef MANTID_VATES_MDEW_IN_MEMORY_LOADING_PRESENTER
#define MANTID_VATES_MDEW_IN_MEMORY_LOADING_PRESENTER

#include "MantidVatesAPI/MDEWLoadingPresenter.h"
#include <boost/scoped_ptr.hpp>

class vtkDataSet;
namespace Mantid {
namespace VATES {
/**
@class MDEWInMemoryLoadingPresenter
Presenter for loading MDEWs directly from the ADS, does not touch the disk.
@author Owen Arnold, Tessella plc
@date 08/09/2011

Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

class MDLoadingView;
class MetaDataExtractorUtils;
class WorkspaceProvider;
class vtkDataSetFactory;

class DLLExport MDEWInMemoryLoadingPresenter : public MDEWLoadingPresenter {
public:
  MDEWInMemoryLoadingPresenter(std::unique_ptr<MDLoadingView> view,
                               WorkspaceProvider *repository,
                               std::string wsName);
  vtkSmartPointer<vtkDataSet>
  execute(vtkDataSetFactory *factory, ProgressAction &rebinningProgressUpdate,
          ProgressAction &drawingProgressUpdate) override;
  void executeLoadMetadata() override;
  ~MDEWInMemoryLoadingPresenter() override;
  bool canReadFile() const override;
  std::string getWorkspaceTypeName() override;
  int getSpecialCoordinates() override;

private:
  /// Repository for accessing workspaces. At this level, does not specify how
  /// or where from.
  boost::scoped_ptr<WorkspaceProvider> m_repository;
  /// The name of the workspace.
  const std::string m_wsName;
  std::string m_wsTypeName;
  int m_specialCoords;
};
} // namespace VATES
} // namespace Mantid

#endif
