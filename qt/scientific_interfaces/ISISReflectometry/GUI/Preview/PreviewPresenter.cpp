// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "PreviewPresenter.h"
#include "IPreviewModel.h"

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
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
