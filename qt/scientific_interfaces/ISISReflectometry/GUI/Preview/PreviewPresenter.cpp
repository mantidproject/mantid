// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "PreviewPresenter.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Plotting/AxisID.h"
#include "MantidQtWidgets/RegionSelector/IRegionSelector.h"
#include "MantidQtWidgets/RegionSelector/RegionSelector.h"
#include <memory>

using Mantid::API::MatrixWorkspace_sptr;
using MantidQt::MantidWidgets::AxisID;
using MantidQt::MantidWidgets::PlotPresenter;
using MantidQt::Widgets::IRegionSelector;
using MantidQt::Widgets::RegionSelector;

class QLayout;

namespace {
Mantid::Kernel::Logger g_log("Reflectometry Preview Presenter");
} // namespace

namespace MantidQt::CustomInterfaces::ISISReflectometry {
PreviewPresenter::PreviewPresenter(Dependencies dependencies)
    : m_view(dependencies.view), m_model(std::move(dependencies.model)),
      m_jobManager(std::move(dependencies.jobManager)), m_instViewModel(std::move(dependencies.instViewModel)),
      m_regionSelector(std::move(dependencies.regionSelector)),
      m_plotPresenter(std::move(dependencies.plotPresenter)), m_stubRegionObserver{new StubRegionObserver} {

  if (!m_regionSelector) {
    m_regionSelector = std::make_unique<RegionSelector>(nullptr, m_view->getRegionSelectorLayout());
  }
  if (!m_plotPresenter) {
    m_plotPresenter = std::make_unique<PlotPresenter>(m_view->getLinePlotView());
  }
  // stub observer subscribes to the region selector
  m_regionSelector->subscribe(m_stubRegionObserver);
  // we subscribe to the stub observer
  m_stubRegionObserver->subscribe(this);

  m_view->subscribe(this);
  m_jobManager->subscribe(this);

  m_view->setInstViewToolbarEnabled(false);

  m_plotPresenter->setScaleLog(AxisID::YLeft);
  m_plotPresenter->setScaleLog(AxisID::XBottom);
  m_plotPresenter->setPlotErrorBars(true);
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
  plotInstView();
  // Ensure the toolbar is enabled, and reset the instrument view to zoom mode
  m_view->setInstViewToolbarEnabled(true);
  notifyInstViewZoomRequested();
  // Perform summing banks to update the next plot, if possible
  runSumBanks();
}

void PreviewPresenter::notifySumBanksCompleted() {
  plotRegionSelector();
  // Perform reduction to update the next plot, if possible
  runReduction();
}

void PreviewPresenter::notifyReductionCompleted() {
  // Update the final plot
  plotLinePlot();
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
  m_model->setSelectedBanks(indices);
  // Execute summing the selected banks
  runSumBanks();
}

void PreviewPresenter::notifyRegionSelectorExportAdsRequested() { m_model->exportSummedWsToAds(); }

void PreviewPresenter::notifyRectangularROIModeRequested() {
  m_view->setRectangularROIState(true);
  m_regionSelector->addRectangularRegion();
}

void PreviewPresenter::notifyRegionChanged() {
  // Set the selection from the view
  auto roi = m_regionSelector->getRegion();
  m_model->setSelectedRegion(roi);
  runReduction();
}

void PreviewPresenter::notifyLinePlotExportAdsRequested() { m_model->exportReducedWsToAds(); }

void PreviewPresenter::plotInstView() {
  m_view->plotInstView(m_instViewModel->getInstrumentViewActor(), m_instViewModel->getSamplePos(),
                       m_instViewModel->getAxis());
}

void PreviewPresenter::plotRegionSelector() { m_regionSelector->updateWorkspace(m_model->getSummedWs()); }

void PreviewPresenter::plotLinePlot() {
  auto ws = m_model->getReducedWs();
  assert(ws);
  auto const numSpec = ws->getNumberHistograms();
  if (numSpec != 1) {
    g_log.warning("Reduced workspace has " + std::to_string(numSpec) + " spectra; expected 1");
  }
  m_plotPresenter->setSpectrum(ws, 0);
  m_plotPresenter->plot();
}

void PreviewPresenter::runSumBanks() { m_model->sumBanksAsync(*m_jobManager); }

void PreviewPresenter::runReduction() {
  // Ensure the angle is up to date
  m_model->setTheta(m_view->getAngle());
  // Perform the reduction
  m_model->reduceAsync(*m_jobManager);
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
