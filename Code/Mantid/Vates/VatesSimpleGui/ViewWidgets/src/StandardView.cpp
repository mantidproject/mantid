#include "MantidVatesSimpleGuiViewWidgets/StandardView.h"
#include "MantidVatesSimpleGuiViewWidgets/RebinnedSourcesManager.h"
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
#include <pqPipelineFilter.h>
#include <pqRenderView.h>
#include <pqServerManagerModel.h>
#include <pqServer.h>
#include <vtkDataObject.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>
#include <vtkSMPVRepresentationProxy.h>

#if defined(__INTEL_COMPILER)
  #pragma warning enable 1170
#endif

#include <QHBoxLayout>
#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QString>

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{

/**
 * This function sets up the UI components, adds connections for the view's
 * buttons and creates the rendering view.
 * @param parent the parent widget for the standard view
 * @param rebinnedSourcesManager Pointer to a RebinnedSourcesManager
 */
  StandardView::StandardView(QWidget *parent, RebinnedSourcesManager* rebinnedSourcesManager) : ViewBase(parent, rebinnedSourcesManager),
                                                                 m_binMDAction(NULL),
                                                                 m_sliceMDAction(NULL),
                                                                 m_unbinAction(NULL)
{
  this->ui.setupUi(this);
  this->cameraReset = false;

  // Set up the buttons
  setupViewButtons();

  // Set the cut button to create a slice on the data
  QObject::connect(this->ui.cutButton, SIGNAL(clicked()), this,
                   SLOT(onCutButtonClicked()));

  // Listen to a change in the active source, to adapt our rebin buttons
  QObject::connect(&pqActiveObjects::instance(), SIGNAL(sourceChanged(pqPipelineSource*)),
                   this, SLOT(activeSourceChangeListener(pqPipelineSource*)));

  // Set the scale button to create the ScaleWorkspace operator
  QObject::connect(this->ui.scaleButton, SIGNAL(clicked()),
                   this, SLOT(onScaleButtonClicked()));

  this->view = this->createRenderView(this->ui.renderFrame);

  QObject::connect(this->view.data(), SIGNAL(endRender()),
                   this, SLOT(onRenderDone()));


}

StandardView::~StandardView()
{
}

void StandardView::setupViewButtons()
{
  // Populate the rebin button
  QMenu* rebinMenu = new QMenu(this->ui.rebinToolButton);

  m_binMDAction = new QAction("BinMD", rebinMenu);
  m_binMDAction->setIconVisibleInMenu(false);
  
  m_sliceMDAction = new QAction("SliceMD", rebinMenu);
  m_sliceMDAction->setIconVisibleInMenu(false);


  m_unbinAction = new QAction("Remove Rebinning", rebinMenu);
  m_unbinAction->setIconVisibleInMenu(false);

  rebinMenu->addAction(m_binMDAction);
  rebinMenu->addAction(m_sliceMDAction);
  rebinMenu->addAction(m_unbinAction);

  this->ui.rebinToolButton->setPopupMode(QToolButton::InstantPopup);
  this->ui.rebinToolButton->setMenu(rebinMenu);

  QObject::connect(m_binMDAction, SIGNAL(triggered()),
                   this, SLOT(onBinMD()), Qt::QueuedConnection);
  QObject::connect(m_sliceMDAction, SIGNAL(triggered()),
                   this, SLOT(onSliceMD()), Qt::QueuedConnection);
  // Set the unbinbutton to remove the rebinning on a workspace
  // which was binned in the VSI
  QObject::connect(m_unbinAction, SIGNAL(triggered()),
                   this, SIGNAL(unbin()), Qt::QueuedConnection);

}

void StandardView::destroyView()
{
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  this->destroyFilter(builder, QString("Slice"));
  builder->destroy(this->view);
}

pqRenderView* StandardView::getView()
{
  return this->view.data();
}

void StandardView::render()
{
  this->origSrc = pqActiveObjects::instance().activeSource();
  if (NULL == this->origSrc)
  {
    return;
  }
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();

  //setRebinAndUnbinButtons();

  if (this->isPeaksWorkspace(this->origSrc))
  {
    this->ui.cutButton->setEnabled(false);
  }

  // Show the data
  pqDataRepresentation *drep = builder->createDataRepresentation(\
        this->origSrc->getOutputPort(0), this->view);
  QString reptype = "Surface";
  if (this->isPeaksWorkspace(this->origSrc))
  {
    reptype = "Wireframe";
  }
  vtkSMPropertyHelper(drep->getProxy(), "Representation").Set(reptype.toStdString().c_str());
  drep->getProxy()->UpdateVTKObjects();
  this->origRep = qobject_cast<pqPipelineRepresentation*>(drep);
  if (!this->isPeaksWorkspace(this->origSrc))
  {
    vtkSMPVRepresentationProxy::SetScalarColoring(drep->getProxy(), "signal",
                                                  vtkDataObject::FIELD_ASSOCIATION_CELLS);
    //drep->getProxy()->UpdateVTKObjects();
    //vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRange(drep->getProxy(),
    //                                                               "signal",
    //                                                               vtkDataObject::FIELD_ASSOCIATION_CELLS);
    drep->getProxy()->UpdateVTKObjects();
  }

  this->resetDisplay();
  emit this->triggerAccept();
}

void StandardView::onCutButtonClicked()
{
  // Apply cut to currently viewed data
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  builder->createFilter("filters", "Cut", this->getPvActiveSrc());
}

void StandardView::onScaleButtonClicked()
{
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  this->scaler = builder->createFilter("filters",
                                       "MantidParaViewScaleWorkspace",
                                       this->getPvActiveSrc());
}

/**
 * This function is responsible for calling resetCamera if the internal
 * variable cameraReset has been set to true.
 */
void StandardView::onRenderDone()
{
  if (this->cameraReset)
  {
    this->resetCamera();
    this->cameraReset = false;
  }
}

void StandardView::renderAll()
{
  this->view->render();
}

void StandardView::resetDisplay()
{
  this->view->resetDisplay();
  this->view->forceRender();
}

void StandardView::resetCamera()
{
  this->view->resetCamera();
  this->view->forceRender();
}

/**
 * This function enables the cut button for the standard view.
 */
void StandardView::updateUI()
{
  this->ui.cutButton->setEnabled(true);
}

void StandardView::updateView()
{
  this->cameraReset = true;
}

void StandardView::closeSubWindows()
{
}


/**
 * Check if the rebin and unbin buttons should be visible
 * Note that for a rebin button to be visible there may be no
 * MDHisto workspaces present, yet temporary MDHisto workspaces are
 * allowed.
 */
void StandardView::setRebinAndUnbinButtons()
{
  int numberOfTemporaryWorkspaces = 0;
  int numberOfTrueMDHistoWorkspaces = 0;
  int numberOfPeakWorkspaces = 0;

  pqServer *server = pqActiveObjects::instance().activeServer();
  pqServerManagerModel *smModel = pqApplicationCore::instance()->getServerManagerModel();
  QList<pqPipelineSource *> sources = smModel->findItems<pqPipelineSource *>(server);

  for (QList<pqPipelineSource *>::iterator source = sources.begin(); source != sources.end(); ++source)
  {
    if (isTemporaryWorkspace(*source))
    {
      numberOfTemporaryWorkspaces++;
    } else if (isMDHistoWorkspace(*source))
    {
      numberOfTrueMDHistoWorkspaces++;
    }
    else if (isPeaksWorkspace(*source))
    {
      numberOfPeakWorkspaces++;
    }
  }

  // If there are any true MDHisto workspaces then the rebin button should be disabled
  if (numberOfTrueMDHistoWorkspaces > 0 || numberOfPeakWorkspaces > 0)
  {
    this->m_binMDAction->setEnabled(false);
    this->m_sliceMDAction->setEnabled(false);
  }
  else 
  {
    this->m_binMDAction->setEnabled(true);
    this->m_sliceMDAction->setEnabled(true);
  }

  // If there are no temporary workspaces the button should be disabled.
  if (numberOfTemporaryWorkspaces == 0)
  {
    this->m_unbinAction->setEnabled(false);
  }
  else
  {
    this->m_unbinAction->setEnabled(true);
  }
}


/**
 * Reacts to the user selecting the BinMD algorithm
 */ 
void StandardView::onBinMD()
{
  emit rebin("BinMD");
}

/**
 * Reacts to the user selecting the SliceMD algorithm
 */ 
void StandardView::onSliceMD()
{
  emit rebin("SliceMD");
}


/**
  * Listen for a change of the active source in order to check if the the 
  * active source is an MDEventSource for which we allow rebinning.
  */
void StandardView::activeSourceChangeListener(pqPipelineSource* source)
{
  // If there is no active source, then we do not allow rebinning
  if (!source)
  {
    this->m_binMDAction->setEnabled(false);
    this->m_sliceMDAction->setEnabled(false);
    this->m_unbinAction->setEnabled(false);
    return;
  }

  // If it is a filter work your way down
  pqPipelineSource* localSource = source;
  pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(localSource);

  while(filter)
  {
    localSource = filter->getInput(0);
    filter = qobject_cast<pqPipelineFilter*>(localSource);
  }

  // Important to first check the temporary source, then for MDEvent source, 
  // as a temporary source may be an MDEventSource.
  std::string workspaceType(localSource->getProxy()->GetXMLName());
  if (isTemporaryWorkspace(localSource))
  {
    this->m_binMDAction->setEnabled(true);
    this->m_sliceMDAction->setEnabled(true);
    this->m_unbinAction->setEnabled(true);
  }
  else if (workspaceType.find("MDEW Source") != std::string::npos)
  {
    this->m_binMDAction->setEnabled(true);
    this->m_sliceMDAction->setEnabled(true);
    this->m_unbinAction->setEnabled(false);
  }
  else
  {
    this->m_binMDAction->setEnabled(false);
    this->m_sliceMDAction->setEnabled(false);
    this->m_unbinAction->setEnabled(false);
  }
}

} // SimpleGui
} // Vates
} // Mantid
