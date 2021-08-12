// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "PreviewPresenter.h"
#include "GUI/Batch/IBatchJobAlgorithm.h"
#include "IPreviewModel.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "Reduction/Item.h"

#include <memory>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
PreviewPresenter::PreviewPresenter(IPreviewView *view, std::unique_ptr<IPreviewModel> model)
    : m_view(view), m_model(std::move(model)) {
  m_view->subscribe(this);
}

void PreviewPresenter::notifyLoadWorkspaceRequested() {
  auto const name = m_view->getWorkspaceName();
  m_model->loadWorkspace(name);
}

void PreviewPresenter::notifyBatchComplete(bool) {}

void PreviewPresenter::notifyBatchCancelled() {}

void PreviewPresenter::notifyAlgorithmStarted(API::IConfiguredAlgorithm_sptr) {}

void PreviewPresenter::notifyAlgorithmComplete(API::IConfiguredAlgorithm_sptr algorithm) {
  auto jobAlgorithm = std::dynamic_pointer_cast<IBatchJobAlgorithm>(algorithm);
  auto item = jobAlgorithm->item();
  if (!item || !item->isPreview())
    return;

  auto loadedWs = m_model->getLoadedWs();
  // TODO plot the result
}

void PreviewPresenter::notifyAlgorithmError(API::IConfiguredAlgorithm_sptr, std::string const &) {}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
