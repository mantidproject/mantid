#include "MantidVatesSimpleGuiViewWidgets/ThreesliceView.h"
#include "MantidVatesSimpleGuiViewWidgets/LibHelper.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"
#include <Poco/Path.h>

#if defined(__INTEL_COMPILER)
  #pragma warning disable 1170
#endif

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqDataRepresentation.h>
#include <pqObjectBuilder.h>
#include <pqPipelineRepresentation.h>
#include <pqPipelineSource.h>
#include <pqPluginManager.h>
#include <pqRenderView.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>

#if defined(__INTEL_COMPILER)
  #pragma warning enable 1170
#endif

#include <QMessageBox>

#include <iostream>
#include <string>

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{
namespace
{
  /// Static logger
  Kernel::Logger g_log("ThreeSliceView");
}
ThreeSliceView::ThreeSliceView(QWidget *parent) : ViewBase(parent)
{
  this->ui.setupUi(this);

  // We need to load the QuadView plugin
  QString quadViewLibrary;
#ifdef Q_OS_WIN32
  // Windows requires the full
  // path information. The DLL is located in the apropriate executeable path of paraview.
  const Poco::Path paraviewPath(Mantid::Kernel::ConfigService::Instance().getParaViewPath());
  Poco::Path quadViewFullPath(paraviewPath, QUADVIEW_LIBRARY.toStdString());
  quadViewLibrary = quadViewFullPath.toString().c_str();
#else
  quadViewLibrary = QUADVIEW_LIBRARY;
#endif

  // Need to load plugin
  pqPluginManager* pm = pqApplicationCore::instance()->getPluginManager();
  QString error;
  pm->loadExtension(pqActiveObjects::instance().activeServer(),
                    quadViewLibrary, &error, false);

  g_log.debug() << "Loading QuadView library from " << quadViewLibrary.toStdString() << "\n";

  this->mainView = this->createRenderView(this->ui.mainRenderFrame,
                                          QString("pqQuadView"));
  pqActiveObjects::instance().setActiveView(this->mainView);
}

ThreeSliceView::~ThreeSliceView()
{
}

void ThreeSliceView::destroyView()
{
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  // Active source disappears in only this view, so set it from the
  // internal source before destroying view.
  pqActiveObjects::instance().setActiveSource(this->origSrc);
  builder->destroy(this->mainView);
}

pqRenderView* ThreeSliceView::getView()
{
  return this->mainView.data();
}

void ThreeSliceView::render()
{
  this->makeThreeSlice();
  this->resetDisplay();
  emit this->triggerAccept();
}

void ThreeSliceView::makeThreeSlice()
{
  pqPipelineSource *src = NULL;
  src = pqActiveObjects::instance().activeSource();

  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();

  // Do not allow overplotting PeaksWorkspaces
  if (this->isPeaksWorkspace(src))
  {
    QMessageBox::warning(this, QApplication::tr("Overplotting Warning"),
                         QApplication::tr("Threeslice mode does not allow "\
                                          "overlay of PeaksWorkspaces"));
    // Need to destroy source since we tried to load it and set the active
    // back to something. In this case we'll choose the original source
    builder->destroy(src);
    pqActiveObjects::instance().setActiveSource(this->origSrc);
    return;
  }

  this->origSrc = src;

  pqDataRepresentation *drep = builder->createDataRepresentation(\
        this->origSrc->getOutputPort(0), this->mainView);
  vtkSMPropertyHelper(drep->getProxy(), "Representation").Set("Slices");
  drep->getProxy()->UpdateVTKObjects();
  this->origRep = qobject_cast<pqPipelineRepresentation*>(drep);
}

void ThreeSliceView::renderAll()
{
  this->mainView->render();
}

void ThreeSliceView::resetDisplay()
{
  this->mainView->resetDisplay();
}

/*
void ThreeSliceView::correctVisibility()
{
  //this->correctColorScaleRange();
}
*/
void ThreeSliceView::correctColorScaleRange()
{
  QPair<double, double> range = this->origRep->getColorFieldRange();
  emit this->dataRange(range.first, range.second);
}

void ThreeSliceView::resetCamera()
{
  this->mainView->resetCamera();
}

}
}
}
