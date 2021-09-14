// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "PreviewPresenter.h"
#include "GUI/Common/IJobManager.h"
#include "IPreviewModel.h"

#include <memory>

namespace {
Mantid::Kernel::Logger g_log("Reflectometry Preview Presenter");
}

namespace MantidQt::CustomInterfaces::ISISReflectometry {
PreviewPresenter::PreviewPresenter(Dependencies dependencies)
    : m_view(dependencies.view), m_model(std::move(dependencies.model)),
      m_jobManager(std::move(dependencies.jobManager)) {
  m_view->subscribe(this);
  m_jobManager->subscribe(this);
}

void PreviewPresenter::notifyLoadWorkspaceRequested() {
  auto const name = m_view->getWorkspaceName();
  m_model->loadWorkspace(name, *m_jobManager);
}

void PreviewPresenter::notifyLoadWorkspaceCompleted() {
  auto workspace = m_model->getLoadedWs();
  assert(workspace);

  // TODO plot the result

  // m_instViewModel->notifyWorkspaceUpdated(m_model->getLoadedWs()); (update inst actor and create surface)
  // surface = m_instViewModel->getSurface();
  // m_view->plotInstView(surface);
  g_log.notice("Loaded ws pointer");
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
