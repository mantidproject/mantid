// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidVatesSimpleGuiViewWidgets/SplatterPlotView.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/Common/MdConstants.h"
#include "MantidQtWidgets/Common/SelectionNotificationService.h"
#include "MantidVatesAPI/ADSWorkspaceProvider.h"
#include "MantidVatesAPI/ViewFrustum.h"
#include "MantidVatesAPI/vtkPeakMarkerFactory.h"
#include "MantidVatesSimpleGuiViewWidgets/CameraManager.h"
#include "MantidVatesSimpleGuiViewWidgets/PeaksTableControllerVsi.h"
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

// Have to deal with ParaView warnings and Intel compiler the hard way.
#if defined(__INTEL_COMPILER)
#pragma warning disable 1170
#endif

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqDataRepresentation.h>
#include <pqObjectBuilder.h>
#include <pqPipelineFilter.h>
#include <pqPipelineRepresentation.h>
#include <pqPipelineSource.h>
#include <pqRenderView.h>
#include <pqServerManagerModel.h>
#include <vtkDataObject.h>
#include <vtkPVRenderView.h>
#include <vtkProperty.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMPVRepresentationProxy.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMSourceProxy.h>

#if defined(__INTEL_COMPILER)
#pragma warning enable 1170
#endif

#include <QAction>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QToolButton>

using namespace MantidQt::API;
using namespace Mantid::VATES;

namespace Mantid {
namespace Vates {
namespace SimpleGui {

namespace {
Mantid::Kernel::Logger g_log("SplatterPlotView");
const char *g_defaultRepresentation = "Point Gaussian";
const double g_defaultOpacity = 0.5;
const double g_defaultRadius = 0.005;
} // namespace

SplatterPlotView::SplatterPlotView(
    QWidget *parent, RebinnedSourcesManager *rebinnedSourcesManager,
    bool createRenderProxy)
    : ViewBase(parent, rebinnedSourcesManager),
      m_cameraManager(boost::make_shared<CameraManager>()),
      m_peaksTableController(nullptr), m_peaksWorkspaceNameDelimiter(";") {
  this->m_noOverlay = false;
  this->m_ui.setupUi(this);

  // Setup the peaks viewer
  m_peaksTableController = new PeaksTableControllerVsi(m_cameraManager, this);
  m_peaksTableController->setMaximumHeight(150);
  // this->m_ui.tableLayout->addWidget(m_peaksTableController);
  this->m_ui.verticalLayout->addWidget(m_peaksTableController);
  m_peaksTableController->setVisible(true);
  QObject::connect(m_peaksTableController,
                   SIGNAL(setRotationToPoint(double, double, double)), this,
                   SLOT(onResetCenterToPoint(double, double, double)));

  this->m_ui.thresholdButton->setToolTip(
      "Create a threshold filter on the data");
  // Set the threshold button to create a threshold filter on data
  QObject::connect(this->m_ui.thresholdButton, SIGNAL(clicked()), this,
                   SLOT(onThresholdButtonClicked()));

  // Set connection to toggle button for peak coordinate checking
  QObject::connect(this->m_ui.overridePeakCoordsButton, SIGNAL(toggled(bool)),
                   this, SLOT(onOverridePeakCoordToggled(bool)));

  // Set connection to toggle button for pick mode checking
  QObject::connect(this->m_ui.pickModeButton, SIGNAL(toggled(bool)), this,
                   SLOT(onPickModeToggled(bool)));

  if (createRenderProxy)
    this->m_view = this->createRenderView(this->m_ui.renderFrame);

  this->installEventFilter(this);

  setupVisiblePeaksButtons();
}

SplatterPlotView::~SplatterPlotView() {}

/**
 * This function is an event filter for handling pick mode. The release
 * of the p key triggers the automatic accept feature and then calls the
 * read and send function.
 * @param obj : Object causing event
 * @param ev : Event object
 * @return true if the event is handled
 */
bool SplatterPlotView::eventFilter(QObject *obj, QEvent *ev) {
  if (this->m_ui.pickModeButton->isChecked()) {
    this->setFocus();
    if (QEvent::KeyRelease == ev->type() && this == obj) {
      QKeyEvent *kev = static_cast<QKeyEvent *>(ev);
      if (Qt::Key_P == kev->key()) {
        emit this->triggerAccept();
        this->readAndSendCoordinates();
        return true;
      }
    }
    return false;
  }
  return false;
}

void SplatterPlotView::destroyView() {
  destroyFiltersForSplatterPlotView();

  // Destroy the view.
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  builder->destroy(this->m_view);
  pqActiveObjects::instance().setActiveSource(this->origSrc);
}

pqRenderView *SplatterPlotView::getView() { return this->m_view.data(); }

void SplatterPlotView::render() {
  pqPipelineSource *src = pqActiveObjects::instance().activeSource();
  bool isPeaksWorkspace = this->isPeaksWorkspace(src);
  // Hedge for two things.
  // 1. If there is no active source
  // 2. If we are loading a peak workspace without haveing
  //    a splatterplot source in place
  bool isBadInput = !src || (isPeaksWorkspace && !this->m_splatSource);
  if (isBadInput) {
    g_log.warning() << "SplatterPlotView: Could not render source. You are "
                       "either loading an active source "
                    << "or you are loading a peak source without having a "
                       "splatterplot source in place.\n";
    return;
  }

  const char *renderType = g_defaultRepresentation;
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();

  // Do not allow overplotting of MDWorkspaces
  if (!this->isPeaksWorkspace(src) && this->m_splatSource) {
    QMessageBox::warning(this, QApplication::tr("Overplotting Warning"),
                         QApplication::tr("SplatterPlot mode does not allow "
                                          "more that one MDEventWorkspace to "
                                          "be plotted."));
    // Need to destroy source since we tried to load it and set the active
    // back to something. In this case we'll choose the splatter plot filter.
    builder->destroy(src);
    pqActiveObjects::instance().setActiveSource(this->m_splatSource);
    this->m_noOverlay = true;
    return;
  }

  if (!isPeaksWorkspace) {
    this->origSrc = src;
    this->m_splatSource = builder->createFilter(
        "filters", MantidQt::API::MdConstants::MantidParaViewSplatterPlot,
        this->origSrc);
  } else {
    // We don't want to load the same peak workspace twice into the splatterplot
    // mode
    if (checkIfPeaksWorkspaceIsAlreadyBeingTracked(src)) {
      QMessageBox::warning(this, QApplication::tr("Duplicate Peaks Workspace"),
                           QApplication::tr("You cannot load the same "
                                            "Peaks Workpsace multiple times."));
      builder->destroy(src);
      pqActiveObjects::instance().setActiveSource(this->m_splatSource);
      return;
    }

    this->m_peaksSource.append(src);
    setPeakSourceFrame(src);
    renderType = "Wireframe";
    // Start listening if the source was destroyed
    QObject::connect(src, SIGNAL(destroyed()), this,
                     SLOT(onPeakSourceDestroyed()));
    setPeakButton(true);
  }

  // Show the data
  src->updatePipeline();
  pqDataRepresentation *drep =
      builder->createDataRepresentation(src->getOutputPort(0), this->m_view);
  vtkSMPropertyHelper(drep->getProxy(), "Representation").Set(renderType);
  if (!isPeaksWorkspace) {
    vtkSMPropertyHelper(drep->getProxy(), "Opacity").Set(g_defaultOpacity);
    vtkSMPropertyHelper(drep->getProxy(), "GaussianRadius")
        .Set(g_defaultRadius);
  } else {
    vtkSMPropertyHelper(drep->getProxy(), "LineWidth").Set(2);
  }
  drep->getProxy()->UpdateVTKObjects();
  if (!isPeaksWorkspace) {
    vtkSMPVRepresentationProxy::SetScalarColoring(
        drep->getProxy(), "signal", vtkDataObject::FIELD_ASSOCIATION_POINTS);
    drep->getProxy()->UpdateVTKObjects();
  }

  this->resetDisplay();
  if (this->m_peaksSource.isEmpty()) {
    // this->setAutoColorScale();
  } else {
    this->renderAll();
  }

  // Add m_peaksSource to the peak controller and the peak filter
  if (isPeaksWorkspace) {
    try {
      m_peaksTableController->updatePeaksWorkspaces(this->m_peaksSource,
                                                    this->m_splatSource);

      if (m_peaksFilter) {
        updatePeaksFilter(m_peaksFilter);
      }
    } catch (...) {
      setPeakButton(false);
    }
  }

  emit this->triggerAccept();
  if (vtkSMProxy *viewProxy = this->getView()->getProxy()) {
    vtkSMPropertyHelper helper(viewProxy, "InteractionMode");
    if (helper.GetAsInt() == vtkPVRenderView::INTERACTION_MODE_2D) {
      helper.Set(vtkPVRenderView::INTERACTION_MODE_3D);
      viewProxy->UpdateProperty("InteractionMode", 1);
      this->resetCamera();
    }
  }
}

void SplatterPlotView::renderAll() { this->m_view->render(); }

void SplatterPlotView::resetDisplay() { this->m_view->resetDisplay(); }

/**
 * This function checks to see if the Override PC button has been
 * toggled. If the state is unchecked (false), we want to make sure
 * that the coordniates are matched back to the MD workspace.
 * @param state : true is button is checked, false if not
 */
void SplatterPlotView::onOverridePeakCoordToggled(bool state) {
  if (!state) {
    this->checkPeaksCoordinates();
    emit this->triggerAccept();
  }
}

void SplatterPlotView::checkPeaksCoordinates() {
  if (!this->m_peaksSource.isEmpty() &&
      !this->m_ui.overridePeakCoordsButton->isChecked()) {

    int peakViewCoords =
        vtkSMPropertyHelper(
            this->origSrc->getProxy(),
            MantidQt::API::MdConstants::MantidParaViewSpecialCoordinates
                .toLatin1()
                .constData())
            .GetAsInt();
    // Make commensurate with vtkPeakMarkerFactory
    peakViewCoords--;

    foreach (pqPipelineSource *src, this->m_peaksSource) {
      vtkSMPropertyHelper(
          src->getProxy(),
          MantidQt::API::MdConstants::PeakDimensions.toLatin1().constData())
          .Set(peakViewCoords);
      src->getProxy()->UpdateVTKObjects();
    }
  }
}

void SplatterPlotView::onThresholdButtonClicked() {
  if (!this->m_threshSource) {
    // prevent trying to create a filter for empty data (nullptr)
    // which causes VSI to crash
    return;
  }
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  this->m_threshSource = builder->createFilter(
      "filters", MantidQt::API::MdConstants::Threshold, this->m_splatSource);
  auto filterProxy =
      builder
          ->createDataRepresentation(this->m_threshSource->getOutputPort(0),
                                     this->m_view)
          ->getProxy();
  vtkSMPropertyHelper(filterProxy, "Representation")
      .Set(g_defaultRepresentation);
  vtkSMPropertyHelper(filterProxy, "Opacity").Set(g_defaultOpacity);
  vtkSMPropertyHelper(filterProxy, "GaussianRadius").Set(g_defaultRadius);
  emit this->lockColorControls();
}

void SplatterPlotView::checkView(ModeControlWidget::Views initialView) {
  if (!this->m_noOverlay && this->m_peaksSource.isEmpty()) {
    ViewBase::checkView(initialView);
  }
  this->m_noOverlay = false;
}

/**
 * This function is responsible for setting up and tearing down the VTK
 * probe filter for use in pick mode.
 * @param state : True if button is toggled, false if not
 */
void SplatterPlotView::onPickModeToggled(bool state) {
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();

  if (state) {
    pqPipelineSource *src = nullptr;
    if (this->m_threshSource) {
      src = this->m_threshSource;
    } else if (this->m_splatSource) {
      src = this->m_splatSource;
    } else {
      // no sources are present in the view -> don't do anything
      return;
    }
    this->m_probeSource = builder->createFilter(
        "filters", MantidQt::API::MdConstants::ProbePoint, src);
    emit this->triggerAccept();
  } else {
    builder->destroy(this->m_probeSource);
  }
  emit this->toggleOrthographicProjection(state);
  this->onParallelProjection(state);
}

void SplatterPlotView::resetCamera() { this->m_view->resetCamera(); }

void SplatterPlotView::destroyPeakSources() {
  // First remove the peaks table, since it makes use of the peaks workspace.
  onRemovePeaksTable();

  pqServer *server = pqActiveObjects::instance().activeServer();
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  pqServerManagerModel *smModel =
      pqApplicationCore::instance()->getServerManagerModel();
  const QList<pqPipelineSource *> sources =
      smModel->findItems<pqPipelineSource *>(server);

  foreach (pqPipelineSource *source, sources) {
    if (this->isPeaksWorkspace(source)) {
      builder->destroy(source);
    }
  }

  this->m_peaksSource.clear();
}

/**
 * This function reads the coordinates from the probe point plugin and
 * passes them on to a listening serivce that will handle them in the
 * appropriate manner.
 */
void SplatterPlotView::readAndSendCoordinates() {
  QList<vtkSMProxy *> pList = this->m_probeSource->getHelperProxies("Source");
  vtkSMDoubleVectorProperty *coords =
      vtkSMDoubleVectorProperty::SafeDownCast(pList[0]->GetProperty("Center"));

  if (coords) {
    // Get coordinate type
    int peakViewCoords =
        vtkSMPropertyHelper(
            this->origSrc->getProxy(),
            MantidQt::API::MdConstants::MantidParaViewSpecialCoordinates
                .toLatin1()
                .constData())
            .GetAsInt();
    // Make commensurate with vtkPeakMarkerFactory
    peakViewCoords--;

    if (peakViewCoords < vtkPeakMarkerFactory::Peak_in_HKL) {
      // For Qlab and Qsample coordinate data
      // Qlab needs to be true, but enum is 0
      bool coordType = !(static_cast<bool>(peakViewCoords));
      SelectionNotificationService::Instance().sendQPointSelection(
          coordType, coords->GetElement(0), coords->GetElement(1),
          coords->GetElement(2));
    }
  } else {
    return;
  }
}

/**
 * Set up the buttons for the visible peaks.
 */
void SplatterPlotView::setupVisiblePeaksButtons() {
  // Populate the rebin button
  QMenu *peaksMenu = new QMenu(this->m_ui.peaksButton);

  m_allPeaksAction = new QAction("Show all peaks in table", peaksMenu);
  m_allPeaksAction->setIconVisibleInMenu(false);

  m_removePeaksAction = new QAction("Remove table", peaksMenu);
  m_removePeaksAction->setIconVisibleInMenu(false);

  peaksMenu->addAction(m_allPeaksAction);
  peaksMenu->addAction(m_removePeaksAction);

  this->m_ui.peaksButton->setPopupMode(QToolButton::InstantPopup);
  this->m_ui.peaksButton->setMenu(peaksMenu);
  setPeakButton(false);

  QObject::connect(m_allPeaksAction, SIGNAL(triggered()), this,
                   SLOT(onShowAllPeaksTable()), Qt::QueuedConnection);

  QObject::connect(m_removePeaksAction, SIGNAL(triggered()), this,
                   SLOT(onRemovePeaksTable()), Qt::QueuedConnection);
}

/**
 * On show all peaks
 */
void SplatterPlotView::onShowAllPeaksTable() {
  createPeaksFilter();

  if (m_peaksTableController->hasPeaks()) {
    m_peaksTableController->showFullTable();
    m_peaksTableController->show();
  }
}

/**
 * Remove the visible peaks table.
 */
void SplatterPlotView::onRemovePeaksTable() {
  if (m_peaksTableController->hasPeaks()) {
    m_peaksTableController->removeTable();
  }

  if (m_peaksFilter) {
    this->destroyFilter(MantidQt::API::MdConstants::MDPeaksFilter);
  }
}

/**
 * Create the peaks filter
 */
void SplatterPlotView::createPeaksFilter() {
  // If the peaks filter already exists, then stay idle.
  if (m_peaksFilter) {
    return;
  }

  // If the there is no peaks workspace, then stay idle.
  if (m_peaksSource.isEmpty()) {
    return;
  }

  // Create the peak filter
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();

  // Set the peaks workspace name. We need to trigger accept in order to log the
  // workspace in the filter
  try {
    m_peaksFilter = builder->createFilter(
        "filters", MantidQt::API::MdConstants::MantidParaViewPeaksFilter,
        this->m_splatSource);
    QObject::connect(m_peaksFilter, SIGNAL(destroyed()), this,
                     SLOT(onPeaksFilterDestroyed()));

    // Setup the peaks filter
    updatePeaksFilter(m_peaksFilter);

    // Create point representation of the source and set the point size
    pqDataRepresentation *dataRepresentation =
        m_peaksFilter->getRepresentation(this->m_view);
    vtkSMPropertyHelper(dataRepresentation->getProxy(), "Representation")
        .Set(g_defaultRepresentation);
    vtkSMPropertyHelper(dataRepresentation->getProxy(), "GaussianRadius")
        .Set(g_defaultRadius);
    vtkSMPropertyHelper(dataRepresentation->getProxy(), "Opacity")
        .Set(g_defaultOpacity);
    dataRepresentation->getProxy()->UpdateVTKObjects();

    if (!this->isPeaksWorkspace(this->origSrc)) {
      vtkSMPVRepresentationProxy::SetScalarColoring(
          dataRepresentation->getProxy(), "signal",
          vtkDataObject::FIELD_ASSOCIATION_POINTS);
      dataRepresentation->getProxy()->UpdateVTKObjects();
    }
    this->resetDisplay();
    this->setVisibilityListener();
    this->renderAll();
  } catch (std::runtime_error &ex) {
    // Destroy peak filter
    if (m_peaksFilter) {
      this->destroyFilter(MantidQt::API::MdConstants::MDPeaksFilter);
    }
    g_log.warning() << ex.what();
  }
}

/* On peaks source destroyed
 * @param source The reference to the destroyed source
 */
void SplatterPlotView::onPeakSourceDestroyed() {
  // For each peak Source check if there is a "true" source available.
  // If it is not availble then remove it from the peakSource storage.
  for (auto it = m_peaksSource.begin(); it != m_peaksSource.end();) {
    pqServer *server = pqActiveObjects::instance().activeServer();
    pqServerManagerModel *smModel =
        pqApplicationCore::instance()->getServerManagerModel();
    const QList<pqPipelineSource *> sources =
        smModel->findItems<pqPipelineSource *>(server);

    auto foundSource = std::find(sources.begin(), sources.end(), *it);

    if (foundSource == sources.end()) {
      it = m_peaksSource.erase(it);
    } else {
      ++it;
    }
  }

  if (m_peaksSource.isEmpty()) {
    setPeakButton(false);
  }

  // Update the availbale peaksTableController with the available workspaces
  m_peaksTableController->updatePeaksWorkspaces(m_peaksSource, m_splatSource);

  // Update the peaks filter
  try {
    updatePeaksFilter(m_peaksFilter);
  } catch (std::runtime_error &ex) {
    g_log.warning() << ex.what();
  }

  // Set an active source
  if (m_peaksSource.isEmpty()) {
    pqActiveObjects::instance().setActiveSource(this->m_splatSource);
  } else {
    pqActiveObjects::instance().setActiveSource(this->m_peaksSource[0]);
  }
}

/**
 * Sets the visibility of the peak button.
 * @param state The visibility state of the peak button.
 */
void SplatterPlotView::setPeakButton(bool state) {
  this->m_ui.peaksButton->setEnabled(state);
}

/**
 * Set the frame of the peak source
 * @param source The peak source
 */
void SplatterPlotView::setPeakSourceFrame(pqPipelineSource *source) {
  int peakViewCoords =
      vtkSMPropertyHelper(
          this->origSrc->getProxy(),
          MantidQt::API::MdConstants::MantidParaViewSpecialCoordinates
              .toLatin1()
              .constData())
          .GetAsInt();
  peakViewCoords--;
  vtkSMPropertyHelper(
      source->getProxy(),
      MantidQt::API::MdConstants::PeakDimensions.toLatin1().constData())
      .Set(peakViewCoords);
}

/**
 * Check if a peaks workspace is already tracked by the m_peaksSource list.
 */
bool SplatterPlotView::checkIfPeaksWorkspaceIsAlreadyBeingTracked(
    pqPipelineSource *source) {
  bool isContained = false;
  std::string sourceName(
      vtkSMPropertyHelper(
          source->getProxy(),
          MantidQt::API::MdConstants::WorkspaceName.toLatin1().constData())
          .GetAsString());
  for (QList<QPointer<pqPipelineSource>>::Iterator it = m_peaksSource.begin();
       it != m_peaksSource.end(); ++it) {
    std::string trackedName(
        vtkSMPropertyHelper(
            (*it)->getProxy(),
            MantidQt::API::MdConstants::WorkspaceName.toLatin1().constData())
            .GetAsString());
    if ((*it == source) || (sourceName == trackedName)) {
      isContained = true;
      break;
    }
  }
  return isContained;
}

/**
 * Updates the peaks filter, i.e. supplies the filter with a list of peaks
 * workspaces and delimiter
 * @param filter The peaks filter.
 */
void SplatterPlotView::updatePeaksFilter(pqPipelineSource *filter) {
  if (!filter) {
    return;
  }

  // If there are no peaks, then destroy the filter, else update it.

  if (m_peaksSource.isEmpty()) {
    this->destroyFilter(MantidQt::API::MdConstants::MDPeaksFilter);
  } else {
    std::string workspaceNamesConcatentated =
        m_peaksTableController->getConcatenatedWorkspaceNames(
            m_peaksWorkspaceNameDelimiter);
    if (workspaceNamesConcatentated.empty()) {
      throw std::runtime_error(
          "The peaks viewer does not contain a valid peaks workspace.");
    }

    vtkSMPropertyHelper(
        filter->getProxy(),
        MantidQt::API::MdConstants::PeaksWorkspace.toLatin1().constData())
        .Set(0, workspaceNamesConcatentated.c_str());
    vtkSMPropertyHelper(
        filter->getProxy(),
        MantidQt::API::MdConstants::PeaksWorkspace.toLatin1().constData())
        .Set(1, m_peaksWorkspaceNameDelimiter.c_str());
    emit this->triggerAccept();
    filter->updatePipeline();
    this->resetCamera();
  }
}

/**
 * Reacts to a destroyed peaks filter, mainly for setting the peak filter
 * pointer to NULL.
 * We need to do this, since PV can destroy the filter in a general
 * destorySources command.
 */
void SplatterPlotView::onPeaksFilterDestroyed() { m_peaksFilter = nullptr; }

/**
 * Destroy all sources in the splatterplot view. We need to delete the filters
 * before
 * we can delete the underlying sources
 */
void SplatterPlotView::destroyAllSourcesInView() {
  destroyFiltersForSplatterPlotView();

  // Destroy the remaning sources and filters
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  builder->destroySources();
}

void SplatterPlotView::setView(pqRenderView *view) {
  clearRenderLayout(this->m_ui.renderFrame);

  auto &activeObjects = pqActiveObjects::instance();
  this->m_view = view;

  auto server = activeObjects.activeServer();
  auto model = pqApplicationCore::instance()->getServerManagerModel();
  auto filters = model->findItems<pqPipelineFilter *>(server);
  this->m_splatSource = findFilter(
      filters, MantidQt::API::MdConstants::MantidParaViewSplatterPlot);
  this->m_peaksFilter = findFilter(
      filters, MantidQt::API::MdConstants::MantidParaViewPeaksFilter);
  this->m_threshSource =
      findFilter(filters, MantidQt::API::MdConstants::Threshold);
  this->m_probeSource =
      findFilter(filters, MantidQt::API::MdConstants::ProbePoint);

  QHBoxLayout *hbox = new QHBoxLayout(this->m_ui.renderFrame);
  hbox->setMargin(0);
  hbox->addWidget(m_view->widget());
}

ModeControlWidget::Views SplatterPlotView::getViewType() {
  return ModeControlWidget::Views::SPLATTERPLOT;
}

void SplatterPlotView::destroyFiltersForSplatterPlotView() {
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  if (this->m_peaksFilter) {
    builder->destroy(this->m_peaksFilter);
  }
  if (!this->m_peaksSource.isEmpty()) {
    this->destroyPeakSources();
    pqActiveObjects::instance().setActiveSource(this->origSrc);
  }
  if (this->m_probeSource) {
    builder->destroy(this->m_probeSource);
  }
  if (this->m_threshSource) {
    builder->destroy(this->m_threshSource);
  }
  if (this->m_splatSource) {
    builder->destroy(this->m_splatSource);
  }
}

/* Find a pqPipelineFilter using the XML name of the proxy
 *
 * If there is more than one match only the first found is returned. If no items
 * are matched a nullptr is returned.
 *
 * @param filters :: a list of filters to search
 * @param name :: the XML name of the item to find
 * @return pointer to the pqPipelineFilter found in filters
 */
pqPipelineFilter *
SplatterPlotView::findFilter(const QList<pqPipelineFilter *> &filters,
                             const QString &name) const {
  auto result = std::find_if(filters.begin(), filters.end(),
                             [&name](const pqPipelineFilter *src) {
                               return strcmp(src->getProxy()->GetXMLName(),
                                             name.toStdString().c_str()) == 0;
                             });

  if (result != filters.end()) {
    return result[0];
  } else {
    return nullptr;
  }
}

} // namespace SimpleGui
} // namespace Vates
} // namespace Mantid
