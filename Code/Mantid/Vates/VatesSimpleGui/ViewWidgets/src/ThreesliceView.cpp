#include "MantidVatesSimpleGuiViewWidgets/ThreesliceView.h"

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqChartValue.h>
#include <pqColorMapModel.h>
#include <pqDataRepresentation.h>
#include <pqObjectBuilder.h>
#include <pqPipelineBrowserWidget.h>
#include <pqPipelineRepresentation.h>
#include <pqPipelineSource.h>
#include <pqRenderView.h>
#include <pqScalarsToColors.h>
#include <pqServer.h>
#include <pqServerManagerModel.h>
#include <pqSMAdaptor.h>
#include <vtkDataObject.h>
#include <vtkProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>

#include <QMessageBox>

#include <iostream>

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{

ThreeSliceView::ThreeSliceView(QWidget *parent) : ViewBase(parent)
{
  this->ui.setupUi(this);

  this->mainView = this->createRenderView(this->ui.mainRenderFrame);
  this->xView = this->create2dRenderView(this->ui.xRenderFrame);
  this->yView = this->create2dRenderView(this->ui.yRenderFrame);
  this->zView = this->create2dRenderView(this->ui.zRenderFrame);
  pqActiveObjects::instance().setActiveView(this->mainView);
}

ThreeSliceView::~ThreeSliceView()
{
}

void ThreeSliceView::destroyView()
{
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  // Active source disappers in only this view, so set it from the
  // internal source before destroying view.
  pqActiveObjects::instance().setActiveSource(this->origSrc);
  this->destroyFilter(builder, QString("Slice"));
  builder->destroy(this->mainView);
  builder->destroy(this->xView);
  builder->destroy(this->yView);
  builder->destroy(this->zView);
}

pqRenderView* ThreeSliceView::create2dRenderView(QWidget* widget)
{
  pqRenderView *view = this->createRenderView(widget);
  view->setCenterAxesVisibility(false);
  view->setOrientationAxesInteractivity(false);
  return view;
}

pqRenderView* ThreeSliceView::getView()
{
  return this->mainView.data();
}

void ThreeSliceView::render()
{
  this->makeThreeSlice();
  this->mainView->resetViewDirection(-1, -1, -1, 0, 1, 0);
  this->resetDisplay();
  this->renderAll();
  emit this->triggerAccept();
}

void ThreeSliceView::makeSlice(ViewBase::Direction i, pqRenderView *view,
                               pqPipelineSource *cut, pqPipelineRepresentation *repr)
{
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  cut = builder->createFilter("filters", "Cut", this->origSrc);
  cut->updatePipeline();
  view->updateInteractionMode(cut->getOutputPort(0));
  pqDataRepresentation *trepr = builder->createDataRepresentation(\
        cut->getOutputPort(0), view);
  repr = qobject_cast<pqPipelineRepresentation *>(trepr);

  vtkSMProxy *plane = vtkSMPropertyHelper(cut->getProxy(),
                                          "CutFunction").GetAsProxy();

  repr->colorByArray("signal", vtkDataObject::FIELD_ASSOCIATION_CELLS);
  repr->getProxy()->UpdateVTKObjects();

  double orient[3] = {0.0, 0.0, 0.0};
  double up[3] = {0.0, 0.0, 0.0};
  switch(i)
  {
  case ViewBase::X:
    orient[0] = 1.0;
    orient[1] = 0.0;
    orient[2] = 0.0;
    up[0] = 0.0;
    up[1] = 0.0;
    up[2] = 1.0;
    break;
  case ViewBase::Y:
    orient[0] = 0.0;
    orient[1] = 1.0;
    orient[2] = 0.0;
    up[0] = 0.0;
    up[1] = 0.0;
    up[2] = 1.0;
    break;
  case ViewBase::Z:
    orient[0] = 0.0;
    orient[1] = 0.0;
    orient[2] = 1.0;
    up[0] = 1.0;
    up[1] = 0.0;
    up[2] = 0.0;
    break;
  }
  vtkSMPropertyHelper(plane, "Normal").Set(orient, 3);
  plane->UpdateVTKObjects();

  view->resetViewDirection(orient[0], orient[1], orient[2],
                           up[0], up[1], up[2]);
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
  vtkSMPropertyHelper(drep->getProxy(), "Representation").Set("Surface");
  drep->getProxy()->UpdateVTKObjects();
  this->origRep = qobject_cast<pqPipelineRepresentation*>(drep);
  this->origRep->colorByArray("signal", vtkDataObject::FIELD_ASSOCIATION_CELLS);
  this->origRep->getProxy()->UpdateVTKObjects();

  // Have to create the cuts and cut representations up here to keep
  // them around
  this->makeSlice(ViewBase::X, this->xView, this->xCut, this->xCutRepr);
  this->makeSlice(ViewBase::Y, this->yView, this->yCut, this->yCutRepr);
  this->makeSlice(ViewBase::Z, this->zView, this->zCut, this->zCutRepr);
}

void ThreeSliceView::renderAll()
{
  this->xView->render();
  this->yView->render();
  this->zView->render();
  this->mainView->render();
}

void ThreeSliceView::resetDisplay()
{
  this->xView->resetDisplay();
  this->yView->resetDisplay();
  this->zView->resetDisplay();
  this->mainView->resetDisplay();
}

void ThreeSliceView::correctVisibility(pqPipelineBrowserWidget *pbw)
{
  //pqServerManagerSelectionModel *smsModel = pqApplicationCore::instance()->getSelectionModel();
  //smsModel->setCurrentItem(this->xCut, pqServerManagerSelectionModel::ClearAndSelect);
  //smsModel->setCurrentItem(this->yCut, pqServerManagerSelectionModel::Select);
  //smsModel->setCurrentItem(this->zCut, pqServerManagerSelectionModel::Select);
  pbw->setSelectionVisibility(true);
  //smsModel->setCurrentItem(this->xCut, pqServerManagerSelectionModel::Clear);
  //smsModel->setCurrentItem(this->yCut, pqServerManagerSelectionModel::Clear);
  //smsModel->setCurrentItem(this->zCut, pqServerManagerSelectionModel::Clear);
  this->origRep->setVisible(false);
  this->correctColorScaleRange();
}

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
