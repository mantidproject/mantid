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

  m_view->setInstViewToolbarEnabled(false);
}

/** Notification received when the user has requested to load a workspace. If it already exists in the ADS
 * then we use that and continue to plot it; otherwise we start an async load.
 */
void PreviewPresenter::notifyLoadWorkspaceRequested() {
  auto const name = m_view->getWorkspaceName();
  try {
    if (m_model->loadWorkspaceFromAds(name)) {
      notifyLoadWorkspaceCompleted();
    } else {
      m_model->loadAndPreprocessWorkspaceAsync(name, *m_jobManager);
    }
  } catch (std::runtime_error const &ex) {
    g_log.error(ex.what());
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
  m_view->plotInstView(m_instViewModel->getInstrumentViewActor(), m_instViewModel->getSamplePos(),
                       m_instViewModel->getAxis());
  // Ensure the toolbar is enabled, and reset the instrument view to zoom mode
  m_view->setInstViewToolbarEnabled(true);
  notifyInstViewZoomRequested();
  // TODO reset the other plots (or perhaps re-run the reduction with the new data?)
}

void PreviewPresenter::notifySumBanksCompleted() {
  g_log.debug("Sum banks completed");
  // TODO Implement plotting of the summed workspace
  // m_view->plotSliceView(m_model->getSummedWs())
}

void PreviewPresenter::notifyInstViewSelectRectRequested() {
  m_view->setInstViewZoomState(false);
  m_view->setInstViewEditState(false);
  m_view->setInstViewSelectRectState(true);
  m_view->setInstViewSelectRectMode();
}

void PreviewPresenter::notifyInstViewEditRequested() {
  m_view->setInstViewZoomState(false);
  m_view->setInstViewEditState(true);
  m_view->setInstViewSelectRectState(false);
  m_view->setInstViewEditMode();
}

void PreviewPresenter::notifyInstViewZoomRequested() {
  m_view->setInstViewZoomState(true);
  m_view->setInstViewEditState(false);
  m_view->setInstViewSelectRectState(false);
  m_view->setInstViewZoomMode();
}

void PreviewPresenter::notifyInstViewShapeChanged() {
  // Change to shape editing after a selection has been done to match instrument viewer default behaviour
  notifyInstViewEditRequested();
  // Get the masked workspace indices
  auto indices = m_instViewModel->detIndicesToDetIDs(m_view->getSelectedDetectors());
  auto selectionStr = m_model->detIDsToString(indices);
  g_log.debug(selectionStr);

  m_model->setSelectedBanks(indices);
  m_model->sumBanksAsync(*m_jobManager);
}

void PreviewPresenter::notifyContourExportAdsRequested() { m_model->exportSummedWsToAds(); }
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
