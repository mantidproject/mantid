// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "GUI/Batch/IBatchView.h"
#include "IPreviewModel.h"
#include "IPreviewPresenter.h"
#include "IPreviewView.h"

#include <memory>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class MANTIDQT_ISISREFLECTOMETRY_DLL PreviewPresenter : public PreviewViewSubscriber,
                                                        public BatchViewSubscriber,
                                                        public IPreviewPresenter {
public:
  PreviewPresenter(IPreviewView *view, std::unique_ptr<IPreviewModel> model);
  virtual ~PreviewPresenter() = default;
  // PreviewViewSubscriber overrides
  void notifyLoadWorkspaceRequested() override;

  // BatchViewSubscriber overrides
  void notifyBatchComplete(bool error) override;
  void notifyBatchCancelled() override;
  void notifyAlgorithmStarted(API::IConfiguredAlgorithm_sptr algorithm) override;
  void notifyAlgorithmComplete(API::IConfiguredAlgorithm_sptr algorithm) override;
  void notifyAlgorithmError(API::IConfiguredAlgorithm_sptr algorithm, std::string const &message) override;

private:
  IPreviewView *m_view{nullptr};
  std::unique_ptr<IPreviewModel> m_model;
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
