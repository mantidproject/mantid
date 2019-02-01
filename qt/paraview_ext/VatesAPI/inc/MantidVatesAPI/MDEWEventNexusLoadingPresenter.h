// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_VATES_MDEW_EVENT_NEXUS_LOADING_PRESENTER
#define MANTID_VATES_MDEW_EVENT_NEXUS_LOADING_PRESENTER

#include "MantidVatesAPI/MDEWLoadingPresenter.h"

namespace Mantid {
namespace VATES {
/**
@class MDEWEventNexusLoadingPresenter
For loading conversion of MDEW workspaces into render-able vtk objects.
@author Owen Arnold, Tessella plc
@date 09/08/2011
*/

class MDLoadingView;
class DLLExport MDEWEventNexusLoadingPresenter : public MDEWLoadingPresenter {
public:
  MDEWEventNexusLoadingPresenter(std::unique_ptr<MDLoadingView> view,
                                 const std::string &fileName);
  vtkSmartPointer<vtkDataSet>
  execute(vtkDataSetFactory *factory, ProgressAction &rebinningProgressUpdate,
          ProgressAction &drawingProgressUpdate) override;
  void executeLoadMetadata() override;
  ~MDEWEventNexusLoadingPresenter() override;
  bool canReadFile() const override;
  std::string getWorkspaceTypeName() override;

private:
  const std::string m_filename;
  std::string m_wsTypeName;
};
} // namespace VATES
} // namespace Mantid

#endif
