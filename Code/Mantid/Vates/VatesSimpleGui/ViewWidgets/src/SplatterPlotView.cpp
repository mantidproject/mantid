#include "MantidVatesSimpleGuiViewWidgets/SplatterPlotView.h"
#include "MantidVatesSimpleGuiViewWidgets/CameraManager.h"
#include "MantidVatesSimpleGuiViewWidgets/PeaksTableControllerVsi.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidQtAPI/SelectionNotificationService.h"
#include "MantidVatesAPI/ADSWorkspaceProvider.h"
#include "MantidVatesAPI/vtkPeakMarkerFactory.h"
#include "MantidVatesAPI/ViewFrustum.h"
#include "MantidKernel/Logger.h"
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

// Have to deal with ParaView warnings and Intel compiler the hard way.
#if defined(__INTEL_COMPILER)
  #pragma warning disable 1170
#endif

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqDataRepresentation.h>
#include <pqObjectBuilder.h>
#include <pqPipelineRepresentation.h>
#include <pqPipelineSource.h>
#include <pqRenderView.h>
#include <pqServerManagerModel.h>
#include <vtkDataObject.h>
#include <vtkProperty.h>
#include <vtkPVRenderView.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMPVRepresentationProxy.h>
#include <vtkSMSourceProxy.h>

#if defined(__INTEL_COMPILER)
  #pragma warning enable 1170
#endif

#include <QKeyEvent>
#include <QMessageBox>
#include <QToolButton>
#include <QMenu>
#include <QAction>

using namespace MantidQt::API;
using namespace Mantid::VATES;

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{

namespace
{
  Mantid::Kernel::Logger g_log("SplatterPlotView");
}


SplatterPlotView::SplatterPlotView(QWidget *parent) : ViewBase(parent),
                                                      m_cameraManager(boost::make_shared<CameraManager>()),
                                                      m_peaksTableController(NULL),
                                                      m_peaksWorkspaceNameDelimiter(";")                         
{
  this->noOverlay = false;
  this->ui.setupUi(this);

 // Setup the peaks viewer
  m_peaksTableController = new PeaksTableControllerVsi(m_cameraManager, this);
  m_peaksTableController->setMaximumHeight(150);
  //this->ui.tableLayout->addWidget(m_peaksTableController);
  this->ui.verticalLayout->addWidget(m_peaksTableController);
  m_peaksTableController->setVisible(true);
  QObject::connect(m_peaksTableController, SIGNAL(setRotationToPoint(double, double, double)),
                   this, SLOT(onResetCenterToPoint(double, double, double)));

  // Set the threshold button to create a threshold filter on data
  QObject::connect(this->ui.thresholdButton, SIGNAL(clicked()),
                   this, SLOT(onThresholdButtonClicked()));

  // Set connection to toggle button for peak coordinate checking
  QObject::connect(this->ui.overridePeakCoordsButton,
                   SIGNAL(toggled(bool)),
                   this,
                   SLOT(onOverridePeakCoordToggled(bool)));

  // Set connection to toggle button for pick mode checking
  QObject::connect(this->ui.pickModeButton,
                   SIGNAL(toggled(bool)),
                   this,
                   SLOT(onPickModeToggled(bool)));

  this->view = this->createRenderView(this->ui.renderFrame);
  this->installEventFilter(this);

  setupVisiblePeaksButtons();
  
}

SplatterPlotView::~SplatterPlotView()
{
}

/**
 * This function is an event filter for handling pick mode. The release
 * of the p key triggers the automatic accept feature and then calls the
 * read and send function.
 * @param obj : Object causing event
 * @param ev : Event object
 * @return true if the event is handled
 */
bool SplatterPlotView::eventFilter(QObject *obj, QEvent *ev)
{
  if (this->ui.pickModeButton->isChecked())
  {
    this->setFocus();
    if (QEvent::KeyRelease == ev->type() && this == obj)
    {
      QKeyEvent *kev = static_cast<QKeyEvent *>(ev);
      if (Qt::Key_P == kev->key())
      {
        emit this->triggerAccept();
        this->readAndSendCoordinates();
        return true;
      }
    }
    return false;
  }
  return false;
}

void SplatterPlotView::destroyView()
{
  destroyFiltersForSplatterPlotView();

  // Destroy the view.
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  builder->destroy(this->view);
  pqActiveObjects::instance().setActiveSource(this->origSrc);
}

pqRenderView* SplatterPlotView::getView()
{
  return this->view.data();
}

void SplatterPlotView::render()
{
  pqPipelineSource *src = NULL;
  src = pqActiveObjects::instance().activeSource();

  QString renderType = "Points";
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();

  // Do not allow overplotting of MDWorkspaces
  if (!this->isPeaksWorkspace(src) && NULL != this->splatSource)
  {
    QMessageBox::warning(this, QApplication::tr("Overplotting Warning"),
                         QApplication::tr("SplatterPlot mode does not allow "\
                                          "more that one MDEventWorkspace to "\
                                          "be plotted."));
    // Need to destroy source since we tried to load it and set the active
    // back to something. In this case we'll choose the splatter plot filter.
    builder->destroy(src);
    pqActiveObjects::instance().setActiveSource(this->splatSource);
    this->noOverlay = true;
    return;
  }

  bool isPeaksWorkspace = this->isPeaksWorkspace(src);
  if (!isPeaksWorkspace)
  {
    this->origSrc = src;
    this->splatSource = builder->createFilter("filters",
                                              "MantidParaViewSplatterPlot",
                                              this->origSrc);
    src = this->splatSource;
  }
  else
  {
      // We don't want to load the same peak workspace twice into the splatterplot mode
    if (checkIfPeaksWorkspaceIsAlreadyBeingTracked(src)) {
      QMessageBox::warning(this, QApplication::tr("Duplicate Peaks Workspace"),
                           QApplication::tr("You cannot load the same "\
                                          "Peaks Workpsace multiple times."));
      builder->destroy(src);
      pqActiveObjects::instance().setActiveSource(this->splatSource);
      return;
    }

    this->peaksSource.append(src);
    setPeakSourceFrame(src);
    renderType = "Wireframe";
    // Start listening if the source was destroyed
    QObject::connect(src, SIGNAL(destroyed()), 
                     this, SLOT(onPeakSourceDestroyed()));
    setPeakButton(true);
  }

  // Show the data
  src->updatePipeline();
  pqDataRepresentation *drep = builder->createDataRepresentation(\
           src->getOutputPort(0), this->view);
  vtkSMPropertyHelper(drep->getProxy(), "Representation").Set(renderType.toStdString().c_str());
  if (!isPeaksWorkspace)
  {
    vtkSMPropertyHelper(drep->getProxy(), "PointSize").Set(1);
  }
  drep->getProxy()->UpdateVTKObjects();
  if (!isPeaksWorkspace)
  {
    vtkSMPVRepresentationProxy::SetScalarColoring(drep->getProxy(), "signal",
                                                  vtkDataObject::FIELD_ASSOCIATION_CELLS);
    drep->getProxy()->UpdateVTKObjects();
  }

  this->resetDisplay();
  if (this->peaksSource.isEmpty())
  {
    //this->setAutoColorScale();
  }
  else
  {
    this->renderAll();
  }

  // Add peaksSource to the peak controller and the peak filter
  if (isPeaksWorkspace)
  {
    try
    {
      m_peaksTableController->updatePeaksWorkspaces(this->peaksSource, this->splatSource);
      
      if (m_peaksFilter)
      {
       updatePeaksFilter(m_peaksFilter);
      }
    }
    catch (...)
    {
      setPeakButton(false);
    }
  }

  emit this->triggerAccept();
  if (vtkSMProxy* viewProxy = this->getView()->getProxy())
    {
    vtkSMPropertyHelper helper(viewProxy, "InteractionMode");
    if (helper.GetAsInt() == vtkPVRenderView::INTERACTION_MODE_2D)
      {
      helper.Set(vtkPVRenderView::INTERACTION_MODE_3D);
      viewProxy->UpdateProperty("InteractionMode",1);
      this->resetCamera();
      }
    }
}

void SplatterPlotView::renderAll()
{
  this->view->render();
}

void SplatterPlotView::resetDisplay()
{
  this->view->resetDisplay();
}

/**
 * This function checks to see if the Override PC button has been
 * toggled. If the state is unchecked (false), we want to make sure
 * that the coordniates are matched back to the MD workspace.
 * @param state : true is button is checked, false if not
 */
void SplatterPlotView::onOverridePeakCoordToggled(bool state)
{
  if (!state)
  {
    this->checkPeaksCoordinates();
    emit this->triggerAccept();
  }
}

void SplatterPlotView::checkPeaksCoordinates()
{
  if (!this->peaksSource.isEmpty() &&
      !this->ui.overridePeakCoordsButton->isChecked())
  {

    int peakViewCoords = vtkSMPropertyHelper(this->origSrc->getProxy(),
                                             "SpecialCoordinates").GetAsInt();
    // Make commensurate with vtkPeakMarkerFactory
    peakViewCoords--;

    foreach(pqPipelineSource *src, this->peaksSource)
    {
      vtkSMPropertyHelper(src->getProxy(),
                          "Peak Dimensions").Set(peakViewCoords);
      src->getProxy()->UpdateVTKObjects();
    }
  }
}

void SplatterPlotView::onThresholdButtonClicked()
{
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  this->threshSource = builder->createFilter("filters", "Threshold",
                                             this->splatSource);
  emit this->lockColorControls();
}

void SplatterPlotView::checkView(ModeControlWidget::Views initialView)
{
  if (!this->noOverlay && this->peaksSource.isEmpty())
  {
    ViewBase::checkView(initialView);
  }
  this->noOverlay = false;
}

/**
 * This function is responsible for setting up and tearing down the VTK
 * probe filter for use in pick mode.
 * @param state : True if button is toggled, false if not
 */
void SplatterPlotView::onPickModeToggled(bool state)
{
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  if (state)
  {
    pqPipelineSource *src = NULL;
    if (NULL != this->threshSource)
    {
      src = this->threshSource;
    }
    else
    {
      src = this->splatSource;
    }
    this->probeSource = builder->createFilter("filters", "ProbePoint", src);
    emit this->triggerAccept();
  }
  else
  {
    builder->destroy(this->probeSource);
  }
  emit this->toggleOrthographicProjection(state);
  this->onParallelProjection(state);
}

void SplatterPlotView::resetCamera()
{
  this->view->resetCamera();
}

void SplatterPlotView::destroyPeakSources()
{
  // First remove the peaks table, since it makes use of the peaks workspace.
  onRemovePeaksTable();

  pqServer *server = pqActiveObjects::instance().activeServer();
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  pqServerManagerModel *smModel = pqApplicationCore::instance()->getServerManagerModel();
  QList<pqPipelineSource *> sources;
  QList<pqPipelineSource *>::Iterator source;
  sources = smModel->findItems<pqPipelineSource *>(server);
  for (source = sources.begin(); source != sources.end(); ++source)
  {
    if (this->isPeaksWorkspace(*source))
    {
      builder->destroy(*source);
    }
  }
  this->peaksSource.clear();
}


/**
 * This function reads the coordinates from the probe point plugin and
 * passes them on to a listening serivce that will handle them in the
 * appropriate manner.
 */
void SplatterPlotView::readAndSendCoordinates()
{
  QList<vtkSMProxy *> pList = this->probeSource->getHelperProxies("Source");
  vtkSMDoubleVectorProperty *coords = vtkSMDoubleVectorProperty::SafeDownCast(\
        pList[0]->GetProperty("Center"));

  if (NULL != coords)
  {
    // Get coordinate type
    int peakViewCoords = vtkSMPropertyHelper(this->origSrc->getProxy(),
                                             "SpecialCoordinates").GetAsInt();
    // Make commensurate with vtkPeakMarkerFactory
    peakViewCoords--;

    if (peakViewCoords < vtkPeakMarkerFactory::Peak_in_HKL)
    {
      // For Qlab and Qsample coordinate data
      // Qlab needs to be true, but enum is 0
      bool coordType = !(static_cast<bool>(peakViewCoords));
      SelectionNotificationService::Instance().sendQPointSelection(coordType,
                                                                   coords->GetElement(0),
                                                                   coords->GetElement(1),
                                                                   coords->GetElement(2));
    }
  }
  else
  {
    return;
  }
}

/**
 * Set up the buttons for the visible peaks.
 */
void SplatterPlotView::setupVisiblePeaksButtons()
{
  // Populate the rebin button
  QMenu* peaksMenu = new QMenu(this->ui.peaksButton);

  m_allPeaksAction = new QAction("Show all peaks in table", peaksMenu);
  m_allPeaksAction->setIconVisibleInMenu(false);

  m_removePeaksAction = new QAction("Remove table", peaksMenu);
  m_removePeaksAction->setIconVisibleInMenu(false);

  peaksMenu->addAction(m_allPeaksAction);
  peaksMenu->addAction(m_removePeaksAction);

  this->ui.peaksButton->setPopupMode(QToolButton::InstantPopup);
  this->ui.peaksButton->setMenu(peaksMenu);
  setPeakButton(false);

  QObject::connect(m_allPeaksAction, SIGNAL(triggered()),
                   this, SLOT(onShowAllPeaksTable()), Qt::QueuedConnection);

  QObject::connect(m_removePeaksAction, SIGNAL(triggered()),
                   this, SLOT(onRemovePeaksTable()), Qt::QueuedConnection);
}


/**
 * On show all peaks 
 */
void SplatterPlotView::onShowAllPeaksTable()
{
  createPeaksFilter();

  if (m_peaksTableController->hasPeaks())
  {
     m_peaksTableController->showFullTable();
     m_peaksTableController->show();
  }
}


/**
 * Remove the visible peaks table.
 */
void SplatterPlotView::onRemovePeaksTable()
{
  if (m_peaksTableController->hasPeaks())
  {
    m_peaksTableController->removeTable();
  }

  if (m_peaksFilter)
  {
    pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
    builder->destroy(m_peaksFilter);
  }
}


/**
 * Create the peaks filter
 */
void SplatterPlotView::createPeaksFilter()
{
  // If the peaks filter already exists, then stay idle.
  if (m_peaksFilter)
  {
    return;
  }

  // If the there is no peaks workspace, then stay idle.
  if (peaksSource.isEmpty())
  {
    return;
  }

  // Create the peak filter
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();

  // Set the peaks workspace name. We need to trigger accept in order to log the workspace in the filter
  try
  {
    m_peaksFilter = builder->createFilter("filters","MantidParaViewPeaksFilter", this->splatSource);
    QObject::connect(m_peaksFilter, SIGNAL(destroyed()),
                     this, SLOT(onPeaksFilterDestroyed()));

    // Setup the peaks filter
    updatePeaksFilter(m_peaksFilter);

    // Create point representation of the source and set the point size 
    const double pointSize = 4;
    pqDataRepresentation *dataRepresentation  = m_peaksFilter->getRepresentation(this->view);
    vtkSMPropertyHelper(dataRepresentation->getProxy(), "Representation").Set("Points");
    vtkSMPropertyHelper(dataRepresentation->getProxy(), "PointSize").Set(pointSize);
    dataRepresentation->getProxy()->UpdateVTKObjects();

    if (!this->isPeaksWorkspace(this->origSrc))
    {
      vtkSMPVRepresentationProxy::SetScalarColoring(dataRepresentation->getProxy(), "signal",
                                                  vtkDataObject::FIELD_ASSOCIATION_CELLS);
      dataRepresentation->getProxy()->UpdateVTKObjects();
    }
    this->resetDisplay();
    this->setVisibilityListener();
    this->renderAll();
  } catch(std::runtime_error &ex)
  {
    // Destroy peak filter
    if (m_peaksFilter)
    {
      builder->destroy(m_peaksFilter);
    }
    g_log.warning() << ex.what();
  }
}

/* On peaks source destroyed
 * @param source The reference to the destroyed source
 */
void SplatterPlotView::onPeakSourceDestroyed()
{
  // For each peak Source check if there is a "true" source available.
  // If it is not availble then remove it from the peakSource storage.
  for (QList<QPointer<pqPipelineSource>>::Iterator it = peaksSource.begin(); it != peaksSource.end();) {
    pqServer *server = pqActiveObjects::instance().activeServer();
    pqServerManagerModel *smModel = pqApplicationCore::instance()->getServerManagerModel();
    QList<pqPipelineSource *> sources;
    sources = smModel->findItems<pqPipelineSource *>(server);

    bool foundSource = false;
    for (QList<pqPipelineSource *>::iterator src = sources.begin(); src != sources.end(); ++src) {
      if ((*src) == (*it)) {
        foundSource = true;
      }
    }

    if (!foundSource) {
      it = peaksSource.erase(it); 
    } 
    else {
      ++it;
    }
  }

  if (peaksSource.isEmpty())
  {
    setPeakButton(false);
  }

  // Update the availbale peaksTableController with the available workspaces 
  m_peaksTableController->updatePeaksWorkspaces(peaksSource, splatSource);
  
  // Update the peaks filter
  try
  {  
    updatePeaksFilter(m_peaksFilter);
  }
  catch(std::runtime_error &ex)
  {
    g_log.warning() << ex.what();
  }

  // Set an active source
  if (peaksSource.isEmpty()) {
    pqActiveObjects::instance().setActiveSource(this->splatSource);
  }
  else {
    pqActiveObjects::instance().setActiveSource(this->peaksSource[0]);
  }
}

/**
 * Sets the visibility of the peak button.
 * @param state The visibility state of the peak button.
 */
void SplatterPlotView::setPeakButton(bool state)
{
  this->ui.peaksButton->setEnabled(state);
}

/**
 * Set the frame of the peak source
 * @param source The peak source
 */
void SplatterPlotView::setPeakSourceFrame(pqPipelineSource* source)
{
  int peakViewCoords = vtkSMPropertyHelper(this->origSrc->getProxy(), "SpecialCoordinates").GetAsInt();
  peakViewCoords--;
  vtkSMPropertyHelper(source->getProxy(), "Peak Dimensions").Set(peakViewCoords);
}

/**
 * Check if a peaks workspace is already tracked by the peaksSource list.
 */
bool SplatterPlotView::checkIfPeaksWorkspaceIsAlreadyBeingTracked(pqPipelineSource* source) {
  bool isContained = false;
  std::string sourceName(vtkSMPropertyHelper(source->getProxy(), "WorkspaceName").GetAsString());
  for (QList<QPointer<pqPipelineSource>>::Iterator it = peaksSource.begin(); it != peaksSource.end(); ++it) {
    std::string trackedName(vtkSMPropertyHelper((*it)->getProxy(), "WorkspaceName").GetAsString());
    if ((*it == source) || (sourceName == trackedName)) {
      isContained = true;
      break;
    }
  }
  return isContained;
}

/**
 * Updates the peaks filter, i.e. supplies the filter with a list of peaks workspaces and delimiter
 * @param filter The peaks filter.
 */
void SplatterPlotView::updatePeaksFilter(pqPipelineSource* filter) {
  if (!filter){
    return;
  }

  // If there are no peaks, then destroy the filter, else update it.
  if (peaksSource.isEmpty()) {
    pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
    builder->destroy(filter);
  }
  else {
    std::string workspaceNamesConcatentated = m_peaksTableController->getConcatenatedWorkspaceNames(m_peaksWorkspaceNameDelimiter);
    if (workspaceNamesConcatentated.empty())
    {
      throw std::runtime_error("The peaks viewer does not contain a valid peaks workspace.");
    }

    vtkSMPropertyHelper(filter->getProxy(), "PeaksWorkspace").Set(workspaceNamesConcatentated.c_str());
    vtkSMPropertyHelper(filter->getProxy(), "Delimiter").Set(m_peaksWorkspaceNameDelimiter.c_str());
    emit this->triggerAccept();
    filter->updatePipeline();
    this->resetCamera();
  }  
}

/**
 * Reacts to a destroyed peaks filter, mainly for setting the peak filter pointer to NULL.
 * We need to do this, since PV can destroy the filter in a general destorySources command.
 */
void SplatterPlotView::onPeaksFilterDestroyed() {
  m_peaksFilter = NULL;
}

/**
 * Destroy all sources in the splatterplot view. We need to delete the filters before 
 * we can delete the underlying sources
 */
void SplatterPlotView::destroyAllSourcesInView() {
  destroyFiltersForSplatterPlotView();

  // Destroy the remaning sources and filters
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  builder->destroySources();
}

void SplatterPlotView::destroyFiltersForSplatterPlotView(){
   pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  if (this->m_peaksFilter)
  {
    builder->destroy(this->m_peaksFilter);
  }
  if (!this->peaksSource.isEmpty())
  {
    this->destroyPeakSources();
    pqActiveObjects::instance().setActiveSource(this->origSrc);
  }
  if (this->probeSource)
  {
    builder->destroy(this->probeSource);
  }
  if (this->threshSource)
  {
    builder->destroy(this->threshSource);
  }
  if (this->splatSource)
  {
    builder->destroy(this->splatSource);
  }
}

} // SimpleGui
} // Vates
} // Mantid
