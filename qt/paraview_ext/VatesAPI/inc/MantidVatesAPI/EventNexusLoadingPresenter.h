// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_VATES_EVENT_NEXUS_LOADING_PRESENTER
#define MANTID_VATES_EVENT_NEXUS_LOADING_PRESENTER

#include "MantidVatesAPI/MDEWLoadingPresenter.h"

namespace Mantid {
namespace VATES {
/**
@class EventNexusLoadingPresenter
Presenter for loading conversion of MDEW workspaces into render-able vtk
objects.
@author Owen Arnold, Tessella plc
@date 05/08/2011
*/
class MDLoadingView;
class DLLExport EventNexusLoadingPresenter : public MDEWLoadingPresenter {
public:
  EventNexusLoadingPresenter(std::unique_ptr<MDLoadingView> view,
                             const std::string &fileName);
  vtkSmartPointer<vtkDataSet>
  execute(vtkDataSetFactory *factory, ProgressAction &loadingProgressUpdate,
          ProgressAction &drawingProgressUpdate) override;
  void executeLoadMetadata() override;
  bool hasTDimensionAvailable() const override;
  std::vector<double> getTimeStepValues() const override;
  ~EventNexusLoadingPresenter() override;
  bool canReadFile() const override;
  std::string getWorkspaceTypeName() override;

private:
  const std::string m_filename;
  std::string m_wsTypeName;
};
} // namespace VATES
} // namespace Mantid

#endif
