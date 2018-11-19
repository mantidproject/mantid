// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_VATES_MDHW_NEXUS_LOADING_PRESENTER
#define MANTID_VATES_MDHW_NEXUS_LOADING_PRESENTER

#include "MantidVatesAPI/MDHWLoadingPresenter.h"
#include <vector>

namespace Mantid {
namespace VATES {
/**
    @class MDHWNexusLoadingPresenter
    For loading conversion of MDHW workspaces into render-able vtk objects.

    @date 08/04/2013
 */

class MDLoadingView;
class DLLExport MDHWNexusLoadingPresenter : public MDHWLoadingPresenter {
public:
  MDHWNexusLoadingPresenter(std::unique_ptr<MDLoadingView> view,
                            const std::string &fileName);
  vtkSmartPointer<vtkDataSet>
  execute(vtkDataSetFactory *factory, ProgressAction &rebinningProgressUpdate,
          ProgressAction &drawingProgressUpdate) override;
  void executeLoadMetadata() override;
  ~MDHWNexusLoadingPresenter() override;
  bool canReadFile() const override;
  std::string getWorkspaceTypeName() override;
  std::vector<int> getExtents();

private:
  void loadWorkspace();
  void loadWorkspace(ProgressAction &rebinningProgressUpdate);
  const std::string m_filename;
  std::string m_wsTypeName;
  Mantid::API::IMDHistoWorkspace_sptr m_histoWs;
};
} // namespace VATES
} // namespace Mantid

#endif
