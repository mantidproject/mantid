#include "MantidVatesSimpleGuiViewWidgets/ViewBase.h"

#include <pqActiveObjects.h>
#include <pqAnimationManager.h>
#include <pqAnimationScene.h>
#include <pqApplicationCore.h>
#include <pqDataRepresentation.h>
#include <pqObjectBuilder.h>
#include <pqPipelineSource.h>
#include <pqPipelineRepresentation.h>
#include <pqPVApplicationCore.h>
#include <pqRenderView.h>
#include <pqServer.h>
#include <pqServerManagerModel.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>
#include <vtkSMSourceProxy.h>

#include <QHBoxLayout>
#include <QPointer>

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{

ViewBase::ViewBase(QWidget *parent) : QWidget(parent)
{
}

pqRenderView* ViewBase::createRenderView(QWidget* widget)
{
  QHBoxLayout *hbox = new QHBoxLayout(widget);
  hbox->setMargin(0);

  // Create a new render view.
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  pqRenderView *view = qobject_cast<pqRenderView*>(\
        builder->createView(pqRenderView::renderViewType(),
                            pqActiveObjects::instance().activeServer()));
  pqActiveObjects::instance().setActiveView(view);

  // Place the widget for the render view in the frame provided.
  hbox->addWidget(view->getWidget());
  return view;
}

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

void ViewBase::onAutoScale()
{
  QPair <double, double> range = this->colorUpdater.autoScale(this->getPvActiveRep());
  this->renderAll();
  emit this->dataRange(range.first, range.second);
}

void ViewBase::onColorMapChange(const pqColorMapModel *model)
{
  this->colorUpdater.colorMapChange(this->getPvActiveRep(), model);
  this->renderAll();
}

void ViewBase::onColorScaleChange(double min, double max)
{
  this->colorUpdater.colorScaleChange(this->getPvActiveRep(), min, max);
  this->renderAll();
}

void ViewBase::onLogScale(int state)
{
  this->colorUpdater.logScale(this->getPvActiveRep(), state);
  this->renderAll();
}

void ViewBase::correctVisibility(pqPipelineBrowserWidget *pbw)
{
  UNUSED_ARG(pbw);
}

/**
 * This function checks a pqPipelineSource (either from a file or workspace)
 * to see if it is derived from a PeaksWorkspace.
 * @param src the pipeline source to check
 * @return true if the pipeline source is derived from PeaksWorkspace
 */
bool ViewBase::isPeaksWorkspace(pqPipelineSource *src)
{
  QString wsType(vtkSMPropertyHelper(src->getProxy(),
                                     "WorkspaceTypeName").GetAsString());
  return wsType.contains("PeaksWorkspace");
}

pqPipelineRepresentation *ViewBase::getPvActiveRep()
{
  pqDataRepresentation *drep = pqActiveObjects::instance().activeRepresentation();
  return qobject_cast<pqPipelineRepresentation *>(drep);
}

/**
 * Create a ParaView source from a given plugin name and workspace name. This is
 * used in the plugin mode of the simple interface.
 * @param pluginName name of the ParaView plugin
 * @param wsName name of the Mantid workspace to pass to the plugin
 */
void ViewBase::setPluginSource(QString pluginName, QString wsName)
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
  srcProxy->UpdatePipelineInformation();;
}

/**
 * Retrieve the active pqPipelineSource object according to ParaView's
 * ActiveObjects mechanism.
 * @return the currently active source
 */
pqPipelineSource *ViewBase::getPvActiveSrc()
{
  return pqActiveObjects::instance().activeSource();
}

/**
 * Function that sets the status for the view mode control buttons. This
 * implementation looks at the original source for a view. Views may override
 * this function to provide alternate checks.
 */
void ViewBase::checkView()
{
  emit this->setViewsStatus(!this->isPeaksWorkspace(this->origSrc));
}

/**
 * This function is responsible for checking if a pipeline source has time
 * step information. If not, it will disable the animation controls. If the
 * pipeline source has time step information, the animation controls will be
 * enabled and the start, stop and number of time steps updated for the
 * animation scene. If the withUpdate flag is used (default off), then the
 * original pipeline source is updated with the number of "time" steps.
 * @param withUpdate update the original source with "time" step info
 */
void ViewBase::setTimeSteps(bool withUpdate)
{
  pqPipelineSource *src = this->getPvActiveSrc();
  unsigned int numSrcs = this->getNumSources();
  if (!withUpdate && this->isPeaksWorkspace(src))
  {
    if (1 == numSrcs)
    {
      emit this->setAnimationControlState(false);
      return;
    }
    if (2 == numSrcs)
    {
      return;
    }
  }
  vtkSMSourceProxy *srcProxy1 = vtkSMSourceProxy::SafeDownCast(src->getProxy());
  srcProxy1->Modified();
  srcProxy1->UpdatePipelineInformation();
  vtkSMDoubleVectorProperty *tsv = vtkSMDoubleVectorProperty::SafeDownCast(\
                                     srcProxy1->GetProperty("TimestepValues"));
  this->handleTimeInfo(tsv, withUpdate);
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
 * This function takes the incoming property and determines the start "time",
 * end "time" and the number of "time" steps. It also enables/disables the
 * animation controls widget based on the number of "time" steps.
 * @param dvp the vector property containing the "time" information
 * @param doUpdate flag to update original source with "time" step info
 */
void ViewBase::handleTimeInfo(vtkSMDoubleVectorProperty *dvp, bool doUpdate)
{
  const int numTimesteps = static_cast<int>(dvp->GetNumberOfElements());
  if (0 != numTimesteps)
  {
    if (doUpdate)
    {
      vtkSMSourceProxy *srcProxy = vtkSMSourceProxy::SafeDownCast(\
                                     this->origSrc->getProxy());
      vtkSMPropertyHelper(srcProxy, "TimestepValues").Set(dvp->GetElements(),
                                                          numTimesteps);
    }
    double tStart = dvp->GetElement(0);
    double tEnd = dvp->GetElement(dvp->GetNumberOfElements() - 1);
    pqAnimationScene *scene = pqPVApplicationCore::instance()->animationManager()->getActiveScene();
    vtkSMPropertyHelper(scene->getProxy(), "StartTime").Set(tStart);
    vtkSMPropertyHelper(scene->getProxy(), "EndTime").Set(tEnd);
    vtkSMPropertyHelper(scene->getProxy(), "NumberOfFrames").Set(numTimesteps);
    emit this->setAnimationControlState(true);
    emit this->setAnimationControlInfo(tStart, tEnd, numTimesteps);
  }
  else
  {
    emit this->setAnimationControlState(false);
  }
}

/**
 * This function is lifted directly from ParaView. It allows the center of
 * rotation of the view to be placed at the center of the mesh associated
 * with the visualized data.
 */
void ViewBase::onResetCenterToData()
{
  pqRenderView* renderView =
      qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  pqDataRepresentation* repr = pqActiveObjects::instance().activeRepresentation();
  if (!repr || !renderView)
  {
    //qDebug() << "Active source not shown in active view. Cannot set center.";
    return;
  }

  double bounds[6];
  if (repr->getDataBounds(bounds))
  {
    double center[3];
    center[0] = (bounds[1]+bounds[0])/2.0;
    center[1] = (bounds[3]+bounds[2])/2.0;
    center[2] = (bounds[5]+bounds[4])/2.0;
    renderView->setCenterOfRotation(center);
    renderView->render();
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
  pqRenderView* renderView =
      qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
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

}
}
}
