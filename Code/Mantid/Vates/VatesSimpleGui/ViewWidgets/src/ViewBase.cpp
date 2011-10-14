#include "MantidVatesSimpleGuiViewWidgets/ViewBase.h"

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqDataRepresentation.h>
#include <pqObjectBuilder.h>
#include <pqPipelineSource.h>
#include <pqPipelineRepresentation.h>
#include <pqRenderView.h>
#include <pqServer.h>
#include <pqServerManagerModel.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMSourceProxy.h>

#include <QHBoxLayout>
#include <QPointer>

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{

ViewBase::ViewBase(QWidget *parent) : QWidget(parent),
  pluginMode(false)
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
  QPair <double, double> range = this->colorUpdater.autoScale(this->origRep);
  this->renderAll();
  emit this->dataRange(range.first, range.second);
}

void ViewBase::onColorMapChange(const pqColorMapModel *model)
{
  this->colorUpdater.colorMapChange(this->origRep, model);
  this->renderAll();
}

void ViewBase::onColorScaleChange(double min, double max)
{
  this->colorUpdater.colorScaleChange(this->origRep, min, max);
  this->renderAll();
}

void ViewBase::onLogScale(int state)
{
  this->colorUpdater.logScale(this->origRep, state);
  this->renderAll();
}

void ViewBase::correctVisibility(pqPipelineBrowserWidget *pbw)
{
  UNUSED_ARG(pbw);
}

bool ViewBase::isPeaksWorkspace(pqPipelineSource *src)
{
  QString wsType(vtkSMPropertyHelper(src->getProxy(),
                                     "WorkspaceTypeName").GetAsString());
  std::cout << "WS Type: " << wsType.toStdString() << std::endl;
  return wsType.contains("PeaksWorkspace");
}

pqPipelineRepresentation *ViewBase::getPvActiveRep()
{
  pqDataRepresentation *drep = pqActiveObjects::instance().activeRepresentation();
  return qobject_cast<pqPipelineRepresentation *>(drep);
}

void ViewBase::setSource(pqPipelineSource *src, bool pluginMode)
{
  this->pluginMode = pluginMode;
  if (this->pluginMode)
  {
    vtkSMSourceProxy *srcProxy = vtkSMSourceProxy::SafeDownCast(src->getProxy());
    srcProxy->UpdateVTKObjects();
    srcProxy->Modified();
    srcProxy->UpdatePipelineInformation();;
  }

  if (this->isPeaksWorkspace(src) && !this->origSrc)
  {
    emit this->disableViews();
  }
}


}
}
}
