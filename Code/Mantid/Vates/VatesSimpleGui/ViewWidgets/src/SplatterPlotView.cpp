#include "MantidVatesSimpleGuiViewWidgets/SplatterPlotView.h"

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidQtAPI/SelectionNotificationService.h"
#include "MantidVatesAPI/ADSWorkspaceProvider.h"
#include "MantidVatesAPI/vtkPeakMarkerFactory.h"

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
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMPVRepresentationProxy.h>
#include <vtkSMSourceProxy.h>

#if defined(__INTEL_COMPILER)
  #pragma warning enable 1170
#endif

#include <QKeyEvent>
#include <QMessageBox>

using namespace MantidQt::API;
using namespace Mantid::VATES;

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{

SplatterPlotView::SplatterPlotView(QWidget *parent) : ViewBase(parent)
{
  this->noOverlay = false;
  this->ui.setupUi(this);

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
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
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
    this->peaksSource.append(src);
    renderType = "Wireframe";
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
    this->onAutoScale();
  }
  else
  {
    this->renderAll();
  }
  emit this->triggerAccept();
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

} // SimpleGui
} // Vates
} // Mantid
