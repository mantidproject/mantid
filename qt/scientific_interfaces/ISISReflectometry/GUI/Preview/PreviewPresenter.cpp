// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "PreviewPresenter.h"
#include <memory>

namespace {
Mantid::Kernel::Logger g_log("Reflectometry Preview Presenter");
}

namespace MantidQt::CustomInterfaces::ISISReflectometry {
PreviewPresenter::PreviewPresenter(Dependencies dependencies)
    : m_view(dependencies.view), m_model(std::move(dependencies.model)),
      m_jobManager(std::move(dependencies.jobManager)), m_instViewModel(std::move(dependencies.instViewModel)) {
  m_view->subscribe(this);
  m_jobManager->subscribe(this);
  notifyInstViewZoomRequested();
}

/** Notification received when the user has requested to load a workspace. If it already exists in the ADS
 * then we use that and continue to plot it; otherwise we start an async load.
 */
void PreviewPresenter::notifyLoadWorkspaceRequested() {
  auto const name = m_view->getWorkspaceName();
  if (m_model->loadWorkspaceFromAds(name)) {
    notifyLoadWorkspaceCompleted();
  } else {
    m_model->loadAndPreprocessWorkspaceAsync(name, *m_jobManager);
  }
}

/** Notification received from the job manager when loading has completed.
 */
void PreviewPresenter::notifyLoadWorkspaceCompleted() {
  // The model has already been updated by another callback to contain the loaded workspace. If loading fails
  // then it should bail out early and this method should never be called, so the workspace should
  // always be valid at this point.
  auto ws = m_model->getLoadedWs();
  assert(ws);

  // Notify the instrument view model that the workspace has changed before we get the surface
  m_instViewModel->updateWorkspace(ws);
  auto surface = m_instViewModel->getInstrumentViewSurface();
  m_view->plotInstView(surface);
}

/** Notification received when the user has requested the sele
 */
void PreviewPresenter::notifyInstViewSelectRectRequested() {
  m_view->setInstViewPanState(false);
  m_view->setInstViewZoomState(false);
  m_view->setInstViewSelectRectState(true);
}

void PreviewPresenter::notifyInstViewPanRequested() {
  m_view->setInstViewSelectRectState(false);
  m_view->setInstViewZoomState(false);
  m_view->setInstViewPanState(true);
}

void PreviewPresenter::notifyInstViewZoomRequested() {
  m_view->setInstViewSelectRectState(false);
  m_view->setInstViewPanState(false);
  m_view->setInstViewZoomState(true);
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
