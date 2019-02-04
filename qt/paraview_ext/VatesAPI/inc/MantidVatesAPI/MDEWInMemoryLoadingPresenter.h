// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
