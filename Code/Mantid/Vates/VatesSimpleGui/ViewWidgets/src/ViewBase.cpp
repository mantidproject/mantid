#include "MantidVatesSimpleGuiViewWidgets/ViewBase.h"
#include "MantidVatesSimpleGuiViewWidgets/BackgroundRgbProvider.h"

#if defined(__INTEL_COMPILER)
  #pragma warning disable 1170
#endif

#include <pqActiveObjects.h>
#include <pqAnimationManager.h>
#include <pqAnimationScene.h>
#include <pqApplicationCore.h>
#include <pqDataRepresentation.h>
#include <pqObjectBuilder.h>
#include <pqPipelineSource.h>
#include <pqPipelineFilter.h>
#include <pqPipelineRepresentation.h>
#include <pqPVApplicationCore.h>
#include <pqRenderView.h>
#include <pqScalarsToColors.h>
#include <pqServer.h>
#include <pqServerManagerModel.h>
#include <pqView.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMPropertyIterator.h>
#include <vtkSMProxy.h>
#include <vtkSMSourceProxy.h>

#if defined(__INTEL_COMPILER)
  #pragma warning enable 1170
#endif

#include <QDebug>
#include <QHBoxLayout>
#include <QPointer>

#include <stdexcept>

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{

/**
 * Default constructor.
 * @param parent the parent widget for the view
 */
ViewBase::ViewBase(QWidget *parent) : QWidget(parent), m_currentColorMapModel(NULL), m_temporaryWorkspaceIdentifier("tempvsi")
{
}

/**
 * This function creates a single standard ParaView view instance.
 * @param widget the UI widget to associate the view with
 * @param viewName the requested view type, if empty will default to RenderView
 * @return the created view
 */
pqRenderView* ViewBase::createRenderView(QWidget* widget, QString viewName)
{
  QHBoxLayout *hbox = new QHBoxLayout(widget);
  hbox->setMargin(0);

  if (viewName == QString(""))
  {
    viewName = pqRenderView::renderViewType();
  }

  // Create a new render view.
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  pqRenderView *view = qobject_cast<pqRenderView*>(\
        builder->createView(viewName,
                            pqActiveObjects::instance().activeServer()));
  pqActiveObjects::instance().setActiveView(view);

  // Place the widget for the render view in the frame provided.
  hbox->addWidget(view->getWidget());

  /// Make a connection to the view's endRender signal for later checking.
  QObject::connect(view, SIGNAL(endRender()),
                   this, SIGNAL(renderingDone()));

  return view;
}

/**
 * This function removes all filters of a given name: i.e. Slice.
 * @param builder the ParaView object builder
 * @param name the class name of the filters to remove
 */
void ViewBase::destroyFilter(pqObjectBuilder *builder, const QString &name)
{
  pqServer *server = pqActiveObjects::instance().activeServer();
  pqServerManagerModel *smModel = pqApplicationCore::instance()->getServerManagerModel();
  QList<pqPipelineSource *> sources;
  QList<pqPipelineSource *>::Iterator source;
  sources = smModel->findItems<pqPipelineSource *>(server);
  for (source = sources.begin(); source != sources.end(); ++source)
  {
    const QString sourceName = (*source)->getSMName();
    if (sourceName.startsWith(name))
    {
      builder->destroy(*source);
    }
  }
}

/**
 * This function is responsible for setting the color scale range from the
 * full extent of the data.
 * @param colorSelectionWidget Pointer to the color selection widget.
 */
void ViewBase::onAutoScale(ColorSelectionWidget* colorSelectionWidget)
{
  // Update the colorUpdater
  this->colorUpdater.updateState(colorSelectionWidget);

  if (this->colorUpdater.isAutoScale())
  {
    this->setAutoColorScale();
  }
}

/**
 * Set the color scale for auto color scaling.
 *
 */
void ViewBase::setAutoColorScale()
{
  VsiColorScale colorScale;

  try
  {
    colorScale = this->colorUpdater.autoScale();
  }
  catch (std::invalid_argument &)
  {
    // Got a bad proxy or color scale range, so do nothing
    return;
  }

  // Set the color scale widget
  emit this->dataRange(colorScale.minValue, colorScale.maxValue);
  emit this->setLogScale(colorScale.useLogScale);
}


/**
 * This function sets the requested color map on the data.
 * @param model the color map to use
 */
void ViewBase::onColorMapChange(const pqColorMapModel *model)
{
  pqPipelineRepresentation *rep = this->getRep();
  if (NULL == rep)
  {
    return;
  }
  // Work around a "bug" in pqScalarToColors::checkRange() where the lower
  // limit gets lost when log scaling is used. This only happens when
  // changing the color map.
  bool logStateChanged = false;
  if (this->colorUpdater.isLogScale())
  {
    this->colorUpdater.logScale(false);
    logStateChanged = true;
  }
  this->colorUpdater.colorMapChange(rep, model);
  if (logStateChanged)
  {
    this->colorUpdater.logScale(true);
  }
  rep->renderViewEventually();

  if (this->colorUpdater.isAutoScale())
  {
    setAutoColorScale();
  }

  // Workaround for colormap but when changing the visbility of a source
  this->m_currentColorMapModel = model;
}

/**
 * This function sets the data color scale range to the requested bounds.
 * @param min the minimum bound for the color scale
 * @param max the maximum bound for the color scale
 */
void ViewBase::onColorScaleChange(double min, double max)
{
  this->colorUpdater.colorScaleChange(min, max);
}

/**
 * This function sets logarithmic color scaling on the data.
 * @param state Flag to determine whether or not to use log color scaling
 */
void ViewBase::onLogScale(int state)
{
  this->colorUpdater.logScale(state);
}

/**
 * This function passes the color selection widget to the color updater
 * object.
 * @param cs : Reference to the color selection widget
 */
void ViewBase::setColorScaleState(ColorSelectionWidget *cs)
{
  this->colorUpdater.updateState(cs);
}

/**
 * This function checks the current state from the color updater and
 * processes the necessary color changes.
 * @param colorScale A pointer to the colorscale widget.
 */
void ViewBase::setColorsForView(ColorSelectionWidget *colorScale)
{
  // Update the colorupdater with the settings of the colorSelectionWidget
  setColorScaleState(colorScale);

  if (this->colorUpdater.isAutoScale())
  {
    this->onAutoScale(colorScale);
  }
  else
  {
    this->onColorScaleChange(this->colorUpdater.getMinimumRange(),
                             this->colorUpdater.getMaximumRange());
  }
  if (this->colorUpdater.isLogScale())
  {
    this->onLogScale(true);
  }
}

/**
 * This function checks a pqPipelineSource (either from a file or workspace)
 * to see if it is derived from a PeaksWorkspace.
 * @param src the pipeline source to check
 * @return true if the pipeline source is derived from PeaksWorkspace
 */
bool ViewBase::isPeaksWorkspace(pqPipelineSource *src)
{
  if (NULL == src)
  {
    return false;
  }
  QString wsType(vtkSMPropertyHelper(src->getProxy(),
                                     "WorkspaceTypeName", true).GetAsString());

  if (wsType.isEmpty())
  {
    wsType = src->getSMName();
  }
  return wsType.contains("PeaksWorkspace");
}

/**
 * This function retrieves the active pqPipelineRepresentation object according
 * to ParaView's ActiveObjects mechanism.
 * @return the currently active representation
 */
pqPipelineRepresentation *ViewBase::getPvActiveRep()
{
  pqDataRepresentation *drep = pqActiveObjects::instance().activeRepresentation();
  return qobject_cast<pqPipelineRepresentation *>(drep);
}

/**
 * This function creates a ParaView source from a given plugin name and
 * workspace name. This is used in the plugin mode of the simple interface.
 * @param pluginName name of the ParaView plugin
 * @param wsName name of the Mantid workspace to pass to the plugin
 */
pqPipelineSource* ViewBase::setPluginSource(QString pluginName, QString wsName)
{
  // Create the source from the plugin
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  pqServer *server = pqActiveObjects::instance().activeServer();
  pqPipelineSource *src = builder->createSource("sources", pluginName,
                                                server);
  vtkSMPropertyHelper(src->getProxy(),
                      "Mantid Workspace Name").Set(wsName.toStdString().c_str());

  // Update the source so that it retrieves the data from the Mantid workspace
  vtkSMSourceProxy *srcProxy = vtkSMSourceProxy::SafeDownCast(src->getProxy());
  srcProxy->UpdateVTKObjects();
  srcProxy->Modified();
  srcProxy->UpdatePipelineInformation();
  src->updatePipeline();

  return src;
}

/**
 * This function retrieves the active pqPipelineSource object according to ParaView's
 * ActiveObjects mechanism.
 * @return the currently active source
 */
pqPipelineSource *ViewBase::getPvActiveSrc()
{
  return pqActiveObjects::instance().activeSource();
}

/**
 * This function sets the status for the view mode control buttons. This
 * implementation looks at the original source for a view. Views may override
 * this function to provide alternate checks.
 * @param initialView The initial view.
 */
void ViewBase::checkView(ModeControlWidget::Views initialView)
{
  if (this->isMDHistoWorkspace(this->origSrc))
  {
    emit this->setViewsStatus(initialView, true);
    emit this->setViewStatus(ModeControlWidget::SPLATTERPLOT, false);
  }
  else if (this->isPeaksWorkspace(this->origSrc))
  {
    emit this->setViewsStatus(initialView, false);
  }
  else
  {
    emit this->setViewsStatus(initialView, true);
  }
}

/**
 * This metod sets the status of the splatterplot button explictly to a desired value
 * @param visiblity The state of the the splatterplot view button.
 */
void ViewBase::setSplatterplot(bool visibility)
{
    emit this->setViewStatus(ModeControlWidget::SPLATTERPLOT, visibility);
}

/**
 * This metod sets the status of the standard view button explictly to a desired value
 * @param visiblity The state of the the standard view button.
 */
void ViewBase::setStandard(bool visibility)
{
    emit this->setViewStatus(ModeControlWidget::STANDARD, visibility);
}

/**
 * This function sets the status for the view mode control buttons when the
 * view switches.
 */
void ViewBase::checkViewOnSwitch()
{
  if (this->hasWorkspaceType("MDHistoWorkspace"))
  {
    emit this->setViewStatus(ModeControlWidget::SPLATTERPLOT, false);
  }
}

/**
 * This function is responsible for checking if a pipeline source has time
 * step information. If not, it will disable the animation controls. If the
 * pipeline source has time step information, the animation controls will be
 * enabled.
 */
void ViewBase::updateAnimationControls()
{
  pqPipelineSource *src = this->getPvActiveSrc();
  unsigned int numSrcs = this->getNumSources();
  if (this->isPeaksWorkspace(src))
  {
    if (1 == numSrcs)
    {
      emit this->setAnimationControlState(false);
      return;
    }
    if (2 <= numSrcs)
    {
      return;
    }
  }
  vtkSMSourceProxy *srcProxy1 = vtkSMSourceProxy::SafeDownCast(src->getProxy());
  srcProxy1->Modified();
  srcProxy1->UpdatePipelineInformation();
  vtkSMDoubleVectorProperty *tsv = vtkSMDoubleVectorProperty::SafeDownCast(\
                                     srcProxy1->GetProperty("TimestepValues"));
  this->handleTimeInfo(tsv);
}

/**
 * This function looks through the ParaView server manager model and finds
 * those pipeline sources whose server manager group name is "sources". It
 * returns the total count of those present;
 * @return the number of true pipeline sources
 */
unsigned int ViewBase::getNumSources()
{
  unsigned int count = 0;
  pqServer *server = pqActiveObjects::instance().activeServer();
  pqServerManagerModel *smModel = pqApplicationCore::instance()->getServerManagerModel();
  QList<pqPipelineSource *> sources;
  QList<pqPipelineSource *>::Iterator source;
  sources = smModel->findItems<pqPipelineSource *>(server);
  for (source = sources.begin(); source != sources.end(); ++source)
  {
    const QString srcProxyName = (*source)->getProxy()->GetXMLGroup();
    if (srcProxyName == QString("sources"))
    {
      count++;
    }
  }
  return count;
}

/**
 * This function takes the incoming property and determines the number of
 * "time" steps. It enables/disables the animation controls widget based on
 * the number of "time" steps.
 * @param dvp the vector property containing the "time" information
 */
void ViewBase::handleTimeInfo(vtkSMDoubleVectorProperty *dvp)
{
  if (NULL == dvp)
  {
    // This is a normal filter and therefore has no timesteps.
    //qDebug() << "No timestep vector, returning.";
    return;
  }

  const int numTimesteps = static_cast<int>(dvp->GetNumberOfElements());
  //qDebug() << "# timesteps: " << numTimesteps;

  if (1 < numTimesteps)
  {
    double tStart = dvp->GetElement(0);
    double tEnd = dvp->GetElement(dvp->GetNumberOfElements() - 1);
    emit this->setAnimationControlInfo(tStart, tEnd, numTimesteps);
    emit this->setAnimationControlState(true);
  }
  else
  {
    emit this->setAnimationControlState(false);
  }
}

/**
 * This function takes a given set of coordinates and resets the center of
 * rotation of the view to that given point.
 * @param x the x coordinate of the center point
 * @param y the y coordinate of the center point
 * @param z the z coordinate of the center point
 */
void ViewBase::onResetCenterToPoint(double x, double y, double z)
{
  pqRenderView *renderView = this->getPvActiveView();
  pqDataRepresentation* repr = pqActiveObjects::instance().activeRepresentation();
  if (!repr || !renderView)
  {
    //qDebug() << "Active source not shown in active view. Cannot set center.";
    return;
  }
  double center[3];
  center[0] = x;
  center[1] = y;
  center[2] = z;
  renderView->setCenterOfRotation(center);
  renderView->render();
}

/**
 * This function is used to set the current state of the view between a
 * parallel projection and the normal projection.
 * @param state whether or not to use parallel projection
 */
void ViewBase::onParallelProjection(bool state)
{
  pqRenderView *cview = this->getPvActiveView();
  vtkSMProxy *proxy = cview->getProxy();
  vtkSMPropertyHelper(proxy, "CameraParallelProjection").Set(state);
  proxy->UpdateVTKObjects();
  cview->render();
}

/**
 * This function is used to set the LOD threshold for the view.
 * @param state : whether or not to use the LOD threshold
 * @param defVal : default value of LOD threshold
 */
void ViewBase::onLodThresholdChange(bool state, double defVal)
{
  pqRenderView *cview = this->getPvActiveView();
  vtkSMProxy *proxy = cview->getProxy();
  if (state)
  {
    vtkSMPropertyHelper(proxy, "LODThreshold").Set(defVal);
  }
  else
  {
    vtkSMPropertyHelper(proxy, "LODThreshold").Set(VTK_DOUBLE_MAX);
  }
  proxy->UpdateVTKObjects();
  cview->render();
}

/**
 * This function retrieves the active pqRenderView object according to
 * ParaView's ActiveObjects mechanism.
 * @return the currently active view
 */
pqRenderView *ViewBase::getPvActiveView()
{
  return qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
}

/**
 * This function checks the original pipeline object for the WorkspaceName
 * property. This will get an empty string if the simple interface is
 * launched in standalone mode.
 * @return the workspace name for the original pipeline object
 */
QString ViewBase::getWorkspaceName()
{
  pqServerManagerModel *smModel = pqApplicationCore::instance()->getServerManagerModel();
  pqPipelineSource *src = smModel->getItemAtIndex<pqPipelineSource *>(0);
  QString wsName(vtkSMPropertyHelper(src->getProxy(),
                                     "WorkspaceName",
                                     true).GetAsString());
  return wsName;
}

/**
 * This function gets a property iterator from the source proxy and iterates
 * over the properties, printing out the keys.
 * @param src pqPipelineSource to print properties from
 */
void ViewBase::printProxyProps(pqPipelineSource *src)
{
  std::cout << src->getSMName().toStdString() << " Properties:" << std::endl;
  vtkSMPropertyIterator *piter = src->getProxy()->NewPropertyIterator();
  while ( !piter->IsAtEnd() )
  {
    std::cout << piter->GetKey() << std::endl;
    piter->Next();
  }
}

/**
 * This function iterrogates the pqPipelineSource for the TimestepValues
 * property. It then checks to see if the number of timesteps is non-zero.
 * @param src pqPipelineSource to check for timesteps
 * @return true if pqPipelineSource has a non-zero number of timesteps
 */
bool ViewBase::srcHasTimeSteps(pqPipelineSource *src)
{
  vtkSMSourceProxy *srcProxy1 = vtkSMSourceProxy::SafeDownCast(src->getProxy());
  srcProxy1->Modified();
  srcProxy1->UpdatePipelineInformation();
  vtkSMDoubleVectorProperty *tsv = vtkSMDoubleVectorProperty::SafeDownCast(\
                                     srcProxy1->GetProperty("TimestepValues"));
  const unsigned int numTimesteps = tsv->GetNumberOfElements();
  return 0 < numTimesteps;
}

/**
 * This function retrieves the current timestep as determined by ParaView's
 * AnimationManager.
 * @return the current timestep from the animation scene
 */
double ViewBase::getCurrentTimeStep()
{
  pqAnimationManager* mgr = pqPVApplicationCore::instance()->animationManager();
  pqAnimationScene *scene = mgr->getActiveScene();
  return scene->getAnimationTime();
}

/**
 * This function will close view generated sub-windows. Most views will not
 * reimplement this function, so the default is to do nothing.
 */
void ViewBase::closeSubWindows()
{
}

/**
 * This function returns the representation appropriate for the request. It
 * checks the ParaView active representation first. If that can't be found, the
 * fallback is to check the original representation associated with the view.
 * @return the discovered representation
 */
pqPipelineRepresentation *ViewBase::getRep()
{
  pqPipelineRepresentation *rep = this->getPvActiveRep();
  if (NULL == rep)
  {
    rep = this->origRep;
  }
  return rep;
}

/**
 * This function checks if a pqPipelineSource is a MDHistoWorkspace.
 * @return true if the source is a MDHistoWorkspace
 */
bool ViewBase::isMDHistoWorkspace(pqPipelineSource *src)
{
  if (NULL == src)
  {
    return false;
  }
  QString wsType(vtkSMPropertyHelper(src->getProxy(),
                                     "WorkspaceTypeName", true).GetAsString());

  if (wsType.isEmpty())
  {
    wsType = src->getSMName();
  }
  return wsType.contains("MDHistoWorkspace");
}

/**
 * This function checks if a pqPipelineSource is a temporary workspace.
 * @return true if the source is a temporary workspace;
 */
bool ViewBase::isTemporaryWorkspace(pqPipelineSource *src)
{
  if (NULL == src)
  {
    return false;
  }

  QString wsType(vtkSMPropertyHelper(src->getProxy(),
                                     "WorkspaceTypeName", true).GetAsString());

  if (wsType.isEmpty())
  {
    wsType = src->getSMName();
  }
  

  QString wsName(vtkSMPropertyHelper(src->getProxy(),
                                    "WorkspaceName", true).GetAsString());

  if (wsName.contains(m_temporaryWorkspaceIdentifier))
  {
    return true;
  }
  else
  {
    return false;
  }
}

/**
 * This function is where one specifies updates to the UI components for a
 * view.
 */
void ViewBase::updateUI()
{
}

/**
 * This function is where one specifies updates to the held view.
 */
void ViewBase::updateView()
{
}

/// This function is used to update settings, such as background color etc.
void ViewBase::updateSettings()
{
  this->backgroundRgbProvider.update();
}


/**
 * This function checks the current pipeline for a filter with the specified
 * name. The function works for generic filter names only.
 * @param name the name of the filter to search for
 * @return true if the filter is found
 */
bool ViewBase::hasFilter(const QString &name)
{
  pqServer *server = pqActiveObjects::instance().activeServer();
  pqServerManagerModel *smModel = pqApplicationCore::instance()->getServerManagerModel();
  QList<pqPipelineSource *> sources;
  QList<pqPipelineSource *>::Iterator source;
  sources = smModel->findItems<pqPipelineSource *>(server);
  for (source = sources.begin(); source != sources.end(); ++source)
  {
    const QString sourceName = (*source)->getSMName();
    if (sourceName.startsWith(name))
    {
      return true;
    }
  }
  return false;
}

/**
 * This function looks through all pipeline sources for one containing the given
 * workspace name. It hands back a null pointer if that name can't be found.
 * @param name : The workspace name to search for
 * @return : Pointer to the pipeline source if found
 */
pqPipelineSource *ViewBase::hasWorkspace(const QString &name)
{
  pqServer *server = pqActiveObjects::instance().activeServer();
  pqServerManagerModel *smModel = pqApplicationCore::instance()->getServerManagerModel();
  QList<pqPipelineSource *> sources;
  QList<pqPipelineSource *>::Iterator source;
  sources = smModel->findItems<pqPipelineSource *>(server);
  for (source = sources.begin(); source != sources.end(); ++source)
  {
    QString wsName(vtkSMPropertyHelper((*source)->getProxy(),
                                       "WorkspaceName", true).GetAsString());
    if (!wsName.isEmpty())
    {
      if (wsName == name)
      {
        return (*source);
      }
    }
  }
  return NULL;
}

/**
 * This function looks through all pipeline sources for one containing the given
 * workspace typename.
 * @param wsTypeName : The workspace typename (Id) to look for.
 * @return : True if a source is found with the workspace type.
 */
bool ViewBase::hasWorkspaceType(const QString &wsTypeName)
{
  pqServer *server = pqActiveObjects::instance().activeServer();
  pqServerManagerModel *smModel = pqApplicationCore::instance()->getServerManagerModel();
  QList<pqPipelineSource *> sources;
  QList<pqPipelineSource *>::Iterator source;
  sources = smModel->findItems<pqPipelineSource *>(server);
  bool hasWsType = false;
  for (source = sources.begin(); source != sources.end(); ++source)
  {
    QString wsType(vtkSMPropertyHelper((*source)->getProxy(),
                                       "WorkspaceTypeName", true).GetAsString());

    if (wsType.isEmpty())
    {
      wsType = (*source)->getSMName();
    }
    hasWsType = wsType.contains(wsTypeName);
    if (hasWsType)
    {
      break;
    }
  }
  return hasWsType;
}

/**
 * This function sets the default colors for the background and connects a tracker for changes of the background color by the user.
 * @param viewSwitched If the view was switched or created.
 */
void ViewBase::setColorForBackground(bool viewSwitched)
{
  backgroundRgbProvider.setBackgroundColor(this->getView(), viewSwitched);
  backgroundRgbProvider.observe(this->getView());
}

/**
 * React to a change of the visibility of a representation of a source.
 * This can be a change of the status if the "eye" symbol in the PipelineBrowserWidget
 * as well as the addition or removal of a representation. 
 * @param source The pipeleine source assoicated with the call.
 * @param representation The representation associatied with the call 
 */
void ViewBase::onVisibilityChanged(pqPipelineSource*, pqDataRepresentation*)
{
  // Reset the colorscale if it is set to autoscale
  if (colorUpdater.isAutoScale())
  {
    // Workaround: A ParaView bug requires us to reload the ColorMap when the visibility changes.
    if (m_currentColorMapModel) {
      onColorMapChange(m_currentColorMapModel);
    }
    this->setAutoColorScale();
  }
}

/**
 * Initializes the settings of the color scale 
 */
void ViewBase::initializeColorScale()
{
  colorUpdater.initializeColorScale();
}

/**
 * This function reacts to a destroyed source.
 */
void ViewBase::onSourceDestroyed()
{
}

/**
 * Destroy all sources in the view.
 */
void ViewBase::destroyAllSourcesInView() {
  pqServer *server = pqActiveObjects::instance().activeServer();
  pqServerManagerModel *smModel = pqApplicationCore::instance()->getServerManagerModel();
  QList<pqPipelineSource *> sources = smModel->findItems<pqPipelineSource *>(server);

  // Out of all pqPipelineSources, find the "true" sources, which were
  // created by a Source Plugin, i.e. MDEW Source, MDHW Source, PeakSource
  QList<pqPipelineSource*> trueSources;
  for (QList<pqPipelineSource *>::iterator source = sources.begin(); source != sources.end(); ++source) {
    if (!qobject_cast<pqPipelineFilter*>(*source)) {
      trueSources.push_back(*source);
    }
  }

  // For each true source, go to the end of the pipeline and destroy it on the way back
  // to the start. This assumes linear pipelines.
  for (QList<pqPipelineSource *>::iterator trueSource = trueSources.begin(); trueSource != trueSources.end(); ++trueSource) {
    destroySinglePipeline(*trueSource);
  }
}


/**
 * Destroy a single, linear pipeline
 * @param source A true pqPiplineSource, i.e. not a filter.
 */
void ViewBase::destroySinglePipeline(pqPipelineSource * source) {

    pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();

    // Move to the end of the pipeline
    pqPipelineSource *sourceBuffer = source;
    while(sourceBuffer->getNumberOfConsumers() > 0) {
      sourceBuffer = sourceBuffer->getConsumer(0);
    }

    // Now destroy the pipeline coming back again
    pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(sourceBuffer);
    while(filter) {
      sourceBuffer = filter->getInput(0);
      builder->destroy(filter);
      filter = qobject_cast<pqPipelineFilter*>(sourceBuffer);
    }

    builder->destroy(sourceBuffer);
}

/**
 * Set the listener for the visibility of the representations
 */
void ViewBase::setVisibilityListener()
{
  // Set the connection to listen to a visibility change of the representation.
  pqServer *server = pqActiveObjects::instance().activeServer();
  pqServerManagerModel *smModel = pqApplicationCore::instance()->getServerManagerModel();
  QList<pqPipelineSource *> sources;
  sources = smModel->findItems<pqPipelineSource *>(server);

  // Attach the visibilityChanged signal for all sources.
  for (QList<pqPipelineSource *>::iterator source = sources.begin(); source != sources.end(); ++source)
  {
    QObject::connect((*source), SIGNAL(visibilityChanged(pqPipelineSource*, pqDataRepresentation*)),
                     this, SLOT(onVisibilityChanged(pqPipelineSource*, pqDataRepresentation*)),
                     Qt::UniqueConnection);
  }
}

/**
 * Disconnects the visibility listener connection for all sources
 */
void ViewBase::removeVisibilityListener() {
    // Set the connection to listen to a visibility change of the representation.
  pqServer *server = pqActiveObjects::instance().activeServer();
  pqServerManagerModel *smModel = pqApplicationCore::instance()->getServerManagerModel();
  QList<pqPipelineSource *> sources;
  sources = smModel->findItems<pqPipelineSource *>(server);

  // Attach the visibilityChanged signal for all sources.
  for (QList<pqPipelineSource *>::iterator source = sources.begin(); source != sources.end(); ++source)
  {
    QObject::disconnect((*source), SIGNAL(visibilityChanged(pqPipelineSource*, pqDataRepresentation*)),
                         this, SLOT(onVisibilityChanged(pqPipelineSource*, pqDataRepresentation*)));
  }
}



} // namespace SimpleGui
} // namespace Vates
} // namespace Mantid
