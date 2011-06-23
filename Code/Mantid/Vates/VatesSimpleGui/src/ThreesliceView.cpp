#include "ThreesliceView.h"

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqChartValue.h>
#include <pqColorMapModel.h>
#include <pqDataRepresentation.h>
#include <pqObjectBuilder.h>
#include <pqPipelineBrowserWidget.h>
#include <pqPipelineRepresentation.h>
#include <pqPipelineSource.h>
#include <pqProxyTabWidget.h>
#include <pqRenderView.h>
#include <pqScalarsToColors.h>
#include <pqServer.h>
#include <pqServerManagerModel.h>
#include <pqServerManagerSelectionModel.h>
#include <pqSMAdaptor.h>
#include <vtkDataObject.h>
#include <vtkProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>

#include <iostream>

ThreeSliceView::ThreeSliceView(QWidget *parent) : ViewBase(parent)
{
	this->setupUi(this);

	this->mainView = this->createRenderView(this->mainRenderFrame);
	this->xView = this->create2dRenderView(this->xRenderFrame);
	this->yView = this->create2dRenderView(this->yRenderFrame);
	this->zView = this->create2dRenderView(this->zRenderFrame);
  pqActiveObjects::instance().setActiveView(this->mainView);
}

ThreeSliceView::~ThreeSliceView()
{
	pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
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
	//view->setOrientationAxesVisibility(false);
	// Remove roll/rotate interactions from 2D view
	vtkSMPropertyHelper helper(view->getProxy(), "CameraManipulators");
	for (unsigned int cm = 0; cm < helper.GetNumberOfElements(); cm++)
	{
		vtkSMProxy* manip = helper.GetAsProxy(cm);
		if (manip &&
				(strcmp(manip->GetXMLName(), "TrackballRotate") == 0 ||
						strcmp(manip->GetXMLName(), "TrackballRoll") == 0))
		{
			helper.Remove(manip);
			cm--;
		}
	}

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
	this->renderAll();
}

void ThreeSliceView::makeSlice(ViewBase::Direction i, pqRenderView *view,
		pqPipelineSource *cut, pqPipelineRepresentation *repr)
{
	vtkSMProxy *plane = vtkSMPropertyHelper(cut->getProxy(),
			"CutFunction").GetAsProxy();

	repr->colorByArray("signal", vtkDataObject::FIELD_ASSOCIATION_CELLS);

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
  repr->getProxy()->UpdateVTKObjects();

	view->resetViewDirection(orient[0], orient[1], orient[2],
			up[0], up[1], up[2]);
}

void ThreeSliceView::makeThreeSlice()
{
	this->origSource = pqActiveObjects::instance().activeSource();

	pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
	pqDataRepresentation *drep = builder->createDataRepresentation(
			this->origSource->getOutputPort(0), this->mainView);
	vtkSMPropertyHelper(drep->getProxy(), "Representation").Set(VTK_SURFACE);
	drep->getProxy()->UpdateVTKObjects();
	this->originSourceRepr = qobject_cast<pqPipelineRepresentation*>(drep);
	this->originSourceRepr->colorByArray("signal",
			vtkDataObject::FIELD_ASSOCIATION_CELLS);

  // Have to create the cuts and cut representations up here to keep
  // them around

  this->xCut = builder->createFilter("filters", "Cut", this->origSource);
  pqDataRepresentation *trepr = builder->createDataRepresentation(
      this->xCut->getOutputPort(0), this->xView);
  this->xCutRepr = qobject_cast<pqPipelineRepresentation *>(trepr);
	this->makeSlice(ViewBase::X, this->xView, this->xCut, this->xCutRepr);

  this->yCut = builder->createFilter("filters", "Cut", this->origSource);
  trepr = builder->createDataRepresentation(this->yCut->getOutputPort(0),
                                            this->yView);
  this->yCutRepr = qobject_cast<pqPipelineRepresentation *>(trepr);
	this->makeSlice(ViewBase::Y, this->yView, this->yCut, this->yCutRepr);

  this->zCut = builder->createFilter("filters", "Cut", this->origSource);
  trepr = builder->createDataRepresentation(this->zCut->getOutputPort(0),
                                            this->zView);
  this->zCutRepr = qobject_cast<pqPipelineRepresentation *>(trepr);
	this->makeSlice(ViewBase::Z, this->zView, this->zCut, this->zCutRepr);
}

void ThreeSliceView::renderAll(bool resetDisplay)
{
  if (resetDisplay)
  {
    this->mainView->resetDisplay();
    this->xView->resetDisplay();
    this->yView->resetDisplay();
    this->zView->resetDisplay();
  }
	this->mainView->render();
	this->xView->render();
	this->yView->render();
	this->zView->render();
}

void ThreeSliceView::onColorScaleChange(double min, double max)
{
  this->originSourceRepr->getLookupTable()->setScalarRange(min, max);
  this->originSourceRepr->getProxy()->UpdateVTKObjects();
  this->renderAll(false);
}

void ThreeSliceView::onAutoScale()
{
  QPair<double, double> val = this->originSourceRepr->getColorFieldRange();
  double min = val.first;
  double max = val.second;
  this->originSourceRepr->getLookupTable()->setScalarRange(min, max);
  this->originSourceRepr->getProxy()->UpdateVTKObjects();
  this->renderAll(false);
  emit this->dataRange(min, max);
}

void ThreeSliceView::onColorMapChange(const pqColorMapModel *model)
{
  pqScalarsToColors *lut = this->originSourceRepr->getLookupTable();
  // Need the scalar bounds to calculate the color point settings
  QPair<double, double> bounds = lut->getScalarRange();

  vtkSMProxy *lutProxy = lut->getProxy();

  // Set the ColorSpace
  pqSMAdaptor::setElementProperty(lutProxy->GetProperty("ColorSpace"),
                                  model->getColorSpace());
  // Set the NaN color
  QList<QVariant> values;
  QColor nanColor;
  model->getNanColor(nanColor);
  values << nanColor.redF() << nanColor.greenF() << nanColor.blueF();
  pqSMAdaptor::setMultipleElementProperty(lutProxy->GetProperty("NanColor"),
                                          values);

  // Set the RGB points
  QList<QVariant> rgbPoints;
  for(int i = 0; i < model->getNumberOfPoints(); i++)
  {
    QColor rgbPoint;
    pqChartValue fraction;
    model->getPointColor(i, rgbPoint);
    model->getPointValue(i, fraction);
    rgbPoints << fraction.getDoubleValue() * bounds.second << rgbPoint.redF()
              << rgbPoint.greenF() << rgbPoint.blueF();
  }
  pqSMAdaptor::setMultipleElementProperty(lutProxy->GetProperty("RGBPoints"),
                                          rgbPoints);

  lutProxy->UpdateVTKObjects();
  this->renderAll(false);
}

void ThreeSliceView::correctVisibility(pqPipelineBrowserWidget *pbw)
{
  pqServerManagerSelectionModel *smsModel = pqApplicationCore::instance()->getSelectionModel();
  smsModel->setCurrentItem(this->xCut, pqServerManagerSelectionModel::ClearAndSelect);
  smsModel->setCurrentItem(this->yCut, pqServerManagerSelectionModel::Select);
  smsModel->setCurrentItem(this->zCut, pqServerManagerSelectionModel::Select);
  pbw->setSelectionVisibility(true);
  smsModel->setCurrentItem(this->xCut, pqServerManagerSelectionModel::Clear);
  smsModel->setCurrentItem(this->yCut, pqServerManagerSelectionModel::Clear);
  smsModel->setCurrentItem(this->zCut, pqServerManagerSelectionModel::Clear);
  this->originSourceRepr->setVisible(false);
  this->correctColorScaleRange();
}

void ThreeSliceView::correctColorScaleRange()
{
  QPair<double, double> range = this->originSourceRepr->getColorFieldRange();
  emit this->dataRange(range.first, range.second);
}

void ThreeSliceView::onLogScale(int state)
{
  pqScalarsToColors *lut = this->originSourceRepr->getLookupTable();
  pqSMAdaptor::setElementProperty(lut->getProxy()->GetProperty("UseLogScale"),
                                  state);
  lut->getProxy()->UpdateVTKObjects();
  this->renderAll(false);
}
