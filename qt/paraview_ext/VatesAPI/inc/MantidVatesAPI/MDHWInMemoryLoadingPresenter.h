// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_VATES_MDHW_IN_MEMORY_LOADING_PRESENTER
#define MANTID_VATES_MDHW_IN_MEMORY_LOADING_PRESENTER

#include "MantidVatesAPI/MDHWLoadingPresenter.h"
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <vector>

class vtkDataSet;
namespace Mantid {
namespace API {
class IMDHistoWorkspace;
}
namespace VATES {
/**
@class MDHWInMemoryLoadingPresenter
Presenter for loading MDHWs directly from the ADS, does not touch the disk.
@date 02/12/2011
*/

class MDLoadingView;
class WorkspaceProvider;
class MetaDataExtractorUtils;
class vtkDataSetFactory;

class DLLExport MDHWInMemoryLoadingPresenter : public MDHWLoadingPresenter {
public:
  MDHWInMemoryLoadingPresenter(std::unique_ptr<MDLoadingView> view,
                               WorkspaceProvider *repository,
                               std::string wsName);
  vtkSmartPointer<vtkDataSet>
  execute(vtkDataSetFactory *factory, ProgressAction &rebinningProgressUpdate,
          ProgressAction &drawingProgressUpdate) override;
  void executeLoadMetadata() override;
  ~MDHWInMemoryLoadingPresenter() override;
  bool canReadFile() const override;
  std::string getWorkspaceTypeName() override;
  int getSpecialCoordinates() override;
  std::vector<int> getExtents();

private:
  /// Repository for accessing workspaces. At this level, does not specify how
  /// or where from.
  boost::scoped_ptr<WorkspaceProvider> m_repository;
  /// The name of the workspace.
  const std::string m_wsName;
  /// The type name of the workspace
  std::string m_wsTypeName;
  /// The workspace special coordinate system
  int m_specialCoords;
  /// Cached visual histogram workspace. Post transpose. Avoids repeating
  /// transpose.
  boost::shared_ptr<Mantid::API::IMDHistoWorkspace> m_cachedVisualHistoWs;
};
} // namespace VATES
} // namespace Mantid

#endif
