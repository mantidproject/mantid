#include "threesliceview.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqObjectBuilder.h"
#include "pqPipelineBrowserWidget.h"
#include "pqPipelineRepresentation.h"
#include "pqPipelineSource.h"
#include "pqProxyTabWidget.h"
#include "pqRenderView.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "vtkDataObject.h"
#include "vtkProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"

#include <iostream>

ThreeSliceView::ThreeSliceView(QWidget *parent) : IView(parent)
{
	this->setupUi(this);

	this->mainView = this->createRenderView(this->mainRenderFrame);
	this->xView = this->create2dRenderView(this->xRenderFrame);
	this->yView = this->create2dRenderView(this->yRenderFrame);
	this->zView = this->create2dRenderView(this->zRenderFrame);
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

void ThreeSliceView::makeSlice(IView::Direction i, pqRenderView *view,
		pqPipelineSource *cut, pqPipelineRepresentation *repr)
{
	pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();

	cut = builder->createFilter("filters", "Cut", this->origSource);
	pqDataRepresentation *trepr = builder->createDataRepresentation(
			cut->getOutputPort(0), view);
	repr = qobject_cast<pqPipelineRepresentation *>(trepr);
	vtkSMProxy *plane = vtkSMPropertyHelper(cut->getProxy(),
			"CutFunction").GetAsProxy();

	repr->colorByArray("signal", vtkDataObject::FIELD_ASSOCIATION_CELLS);

	double orient[3], up[3];
	switch(i)
	{
	case IView::X:
		orient[0] = 1.0;
		orient[1] = 0.0;
		orient[2] = 0.0;
		up[0] = 0.0;
		up[1] = 0.0;
		up[2] = 1.0;
		break;
	case IView::Y:
		orient[0] = 0.0;
		orient[1] = 1.0;
		orient[2] = 0.0;
		up[0] = 0.0;
		up[1] = 0.0;
		up[2] = 1.0;
		break;
	case IView::Z:
		orient[0] = 0.0;
		orient[1] = 0.0;
		orient[2] = 1.0;
		up[0] = 1.0;
		up[1] = 0.0;
		up[2] = 0.0;
		break;
	}
	vtkSMPropertyHelper(plane, "Normal").Set(orient, 3);
	trepr->getProxy()->UpdateVTKObjects();

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

	this->makeSlice(IView::X, this->xView, this->xCut, this->xCutRepr);
	this->makeSlice(IView::Y, this->yView, this->yCut, this->yCutRepr);
	this->makeSlice(IView::Z, this->zView, this->zCut, this->zCutRepr);
}

void ThreeSliceView::renderAll()
{
	this->mainView->resetDisplay();
	this->xView->resetDisplay();
	this->yView->resetDisplay();
	this->zView->resetDisplay();

	this->mainView->render();
	this->xView->render();
	this->yView->render();
	this->zView->render();
}
