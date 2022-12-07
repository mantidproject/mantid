// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "PreviewPresenter.h"
#include "Common/Detector.h"
#include "GUI/Batch/IBatchPresenter.h"
#include "GUI/Preview/QtPreviewDockedWidgets.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Strings.h"
#include "MantidQtWidgets/Plotting/AxisID.h"
#include "MantidQtWidgets/RegionSelector/IRegionSelector.h"
#include "MantidQtWidgets/RegionSelector/RegionSelector.h"
#include "ROIType.h"
#include "Reduction/RowExceptions.h"
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
      m_dockedWidgets(std::move(dependencies.dockedWidgets)), m_regionSelector(std::move(dependencies.regionSelector)),
      m_plotPresenter(std::move(dependencies.plotPresenter)), m_stubRegionObserver{new StubRegionObserver} {
  if (!m_dockedWidgets) {
    m_dockedWidgets = std::make_unique<QtPreviewDockedWidgets>(nullptr, m_view->getDockedWidgetsLayout());
  }
  if (!m_regionSelector) {
    m_regionSelector = std::make_unique<RegionSelector>(nullptr, m_dockedWidgets->getRegionSelectorLayout());
  }
  if (!m_plotPresenter) {
    m_plotPresenter = std::make_unique<PlotPresenter>(m_dockedWidgets->getLinePlotView());
  }
  // stub observer subscribes to the region selector
  m_regionSelector->subscribe(m_stubRegionObserver);
  // we subscribe to the stub observer
  m_stubRegionObserver->subscribe(this);

  m_view->subscribe(this);
  m_jobManager->subscribe(this);
  m_dockedWidgets->subscribe(this);

  m_dockedWidgets->setInstViewToolbarEnabled(false);
  m_dockedWidgets->setRegionSelectorEnabled(false);

  m_plotPresenter->setScaleLog(AxisID::YLeft);
  m_plotPresenter->setScaleLog(AxisID::XBottom);
  m_plotPresenter->setPlotErrorBars(true);
}

void PreviewPresenter::acceptMainPresenter(IBatchPresenter *mainPresenter) { m_mainPresenter = mainPresenter; }

void PreviewPresenter::notifyReductionResumed() { updateWidgetEnabledState(); }

void PreviewPresenter::notifyReductionPaused() { updateWidgetEnabledState(); }

void PreviewPresenter::notifyAutoreductionResumed() { updateWidgetEnabledState(); }

void PreviewPresenter::notifyAutoreductionPaused() { updateWidgetEnabledState(); }

void PreviewPresenter::updateWidgetEnabledState() {
  if (m_mainPresenter->isProcessing() || m_mainPresenter->isAutoreducing()) {
    m_view->disableApplyButton();
  } else {
    m_view->enableApplyButton();
  }
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

  // Set the angle so that it has a non-zero value when the reduction is run
  if (auto const theta = m_model->getDefaultTheta()) {
    m_view->setAngle(*theta);
  }

  if (hasLinearDetector(ws)) {
    m_dockedWidgets->resetInstView();
    m_dockedWidgets->setInstViewToolbarEnabled(false);
    m_model->setSummedWs(ws);
    notifySumBanksCompleted();
  } else {
    // Notify the instrument view model that the workspace has changed before we get the surface
    m_instViewModel->updateWorkspace(ws);
    plotInstView();
    // Ensure the toolbar is enabled, and reset the instrument view to zoom mode
    m_dockedWidgets->setInstViewToolbarEnabled(true);
    notifyInstViewZoomRequested();
    // Perform summing banks to update the next plot, if possible
    runSumBanks();
  }
}

void PreviewPresenter::notifyUpdateAngle() { runReduction(); }

void PreviewPresenter::notifySumBanksCompleted() {
  plotRegionSelector();
  m_dockedWidgets->setRegionSelectorEnabled(true);
  // Perform reduction to update the next plot, if possible
  runReduction();
}

void PreviewPresenter::notifyReductionCompleted() {
  // Update the final plot
  plotLinePlot();
}

void PreviewPresenter::notifySumBanksAlgorithmError() {
  clearRegionSelector();
  clearReductionPlot();
}

void PreviewPresenter::notifyReductionAlgorithmError() { clearReductionPlot(); }

void PreviewPresenter::notifyInstViewSelectRectRequested() {
  m_dockedWidgets->setInstViewZoomState(false);
  m_dockedWidgets->setInstViewEditState(false);
  m_dockedWidgets->setInstViewSelectRectState(true);
  m_dockedWidgets->setInstViewSelectRectMode();
}

void PreviewPresenter::notifyInstViewEditRequested() {
  m_dockedWidgets->setInstViewZoomState(false);
  m_dockedWidgets->setInstViewEditState(true);
  m_dockedWidgets->setInstViewSelectRectState(false);
  m_dockedWidgets->setInstViewEditMode();
}

void PreviewPresenter::notifyInstViewZoomRequested() {
  m_dockedWidgets->setInstViewZoomState(true);
  m_dockedWidgets->setInstViewEditState(false);
  m_dockedWidgets->setInstViewSelectRectState(false);
  m_dockedWidgets->setInstViewZoomMode();
}

void PreviewPresenter::notifyInstViewShapeChanged() {
  // Change to shape editing after a selection has been done to match instrument viewer default behaviour
  notifyInstViewEditRequested();
  // Get the masked workspace indices
  auto indices = m_instViewModel->detIndicesToDetIDs(m_dockedWidgets->getSelectedDetectors());
  auto detIDsStr = Mantid::Kernel::Strings::joinCompress(indices.cbegin(), indices.cend(), ",");
  m_model->setSelectedBanks(ProcessingInstructions{detIDsStr});
  // Execute summing the selected banks
  runSumBanks();
}

void PreviewPresenter::notifyRegionSelectorExportAdsRequested() { m_model->exportSummedWsToAds(); }

void PreviewPresenter::notifyEditROIModeRequested() {
  m_dockedWidgets->setRectangularROIState(false);
  m_dockedWidgets->setEditROIState(true);
  m_regionSelector->cancelDrawingRegion();
}

void PreviewPresenter::notifyRectangularROIModeRequested() {
  auto const regionType = m_dockedWidgets->getRegionType();
  auto const roiType = roiTypeFromString(regionType);
  m_dockedWidgets->setEditROIState(false);
  m_dockedWidgets->setRectangularROIState(true);
  m_regionSelector->addRectangularRegion(regionType, roiTypeToColor(roiType));
}

void PreviewPresenter::notifyRegionChanged() {
  m_dockedWidgets->setRectangularROIState(false);
  m_dockedWidgets->setEditROIState(true);

  runReduction();
}

void PreviewPresenter::notifyLinePlotExportAdsRequested() { m_model->exportReducedWsToAds(); }

void PreviewPresenter::notifyApplyRequested() {
  try {
    m_mainPresenter->notifyPreviewApplyRequested();
  } catch (InvalidTableException const &ex) {
    std::ostringstream msg;
    msg << "Could not update Experiment Settings: ";
    msg << ex.what();
    msg << " Please fix any errors in the Experiment Settings table and try again.";
    g_log.error(msg.str());
  } catch (RowNotFoundException const &ex) {
    std::ostringstream msg;
    msg << "Could not update Experiment Settings: ";
    msg << ex.what();
    msg << " Please add a row for this angle, add a wildcard row, or change the angle.";
    g_log.error(msg.str());
  } catch (MultipleRowsFoundException const &ex) {
    std::ostringstream msg;
    msg << "Could not update Experiment Settings: ";
    msg << ex.what();
    msg << " Applying to multiple rows with the same angle is not supported.";
    g_log.error(msg.str());
  }
}

void PreviewPresenter::plotInstView() {
  m_dockedWidgets->plotInstView(m_instViewModel->getInstrumentViewActor(), m_instViewModel->getSamplePos(),
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
  m_view->setUpdateAngleButtonEnabled(false);
  // Ensure the angle is up to date
  m_model->setTheta(m_view->getAngle());
  // Ensure the selected regions are up to date. Required when Loading new data because an empty run details is created.
  updateSelectedRegionInModelFromView();
  // Perform the reduction
  m_model->reduceAsync(*m_jobManager);
}

PreviewRow const &PreviewPresenter::getPreviewRow() const { return m_model->getPreviewRow(); }

void PreviewPresenter::clearRegionSelector() {
  m_regionSelector->clearWorkspace();
  m_dockedWidgets->setRegionSelectorEnabled(false);
}

void PreviewPresenter::clearReductionPlot() {
  m_plotPresenter->clearModel();
  m_plotPresenter->plot();
}

void PreviewPresenter::updateSelectedRegionInModelFromView() {
  m_model->setSelectedRegion(ROIType::Signal, m_regionSelector->getRegion(roiTypeToString(ROIType::Signal)));
  m_model->setSelectedRegion(ROIType::Background, m_regionSelector->getRegion(roiTypeToString(ROIType::Background)));
  m_model->setSelectedRegion(ROIType::Transmission,
                             m_regionSelector->getRegion(roiTypeToString(ROIType::Transmission)));
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
