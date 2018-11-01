// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef SQW_LOADING_PRESENTER_H_
#define SQW_LOADING_PRESENTER_H_

#include "MantidVatesAPI/MDEWLoadingPresenter.h"

namespace Mantid {
namespace VATES {
/**
@class SQWLoadingPresenter
MVP loading presenter for .*sqw file types.
@author Owen Arnold, Tessella plc
@date 16/08/2011
*/
class MDLoadingView;
class DLLExport SQWLoadingPresenter : public MDEWLoadingPresenter {
public:
  SQWLoadingPresenter(std::unique_ptr<MDLoadingView> view,
                      const std::string &fileName);
  vtkSmartPointer<vtkDataSet>
  execute(vtkDataSetFactory *factory, ProgressAction &rebinningProgressUpdate,
          ProgressAction &drawingProgressUpdate) override;
  void extractMetadata(const Mantid::API::IMDEventWorkspace &eventWs) override;
  void executeLoadMetadata() override;
  ~SQWLoadingPresenter() override;
  bool canReadFile() const override;
  std::string getWorkspaceTypeName() override;

private:
  const std::string m_filename;
  std::string m_wsTypeName;
};
} // namespace VATES
} // namespace Mantid

#endif
