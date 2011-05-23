#include "MultisliceView.h"

#include "AxisInformation.h"
#include "GeometryParser.h"
#include "ScalePicker.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqObjectBuilder.h"
#include "pqPipelineBrowserWidget.h"
#include "pqPipelineRepresentation.h"
#include "pqPipelineSource.h"
#include "pqRenderView.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerSelectionModel.h"
#include "vtkDataObject.h"
#include "vtkProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMViewProxy.h"

#include <QModelIndex>
#include <QString>

#include <iostream>
MultiSliceView::MultiSliceView(QWidget *parent) : IView(parent)
{
	this->ui.setupUi(this);

	this->mainView = this->createRenderView(this->ui.renderFrame);

	QObject::connect(this->ui.xAxisWidget->getScalePicker(),
			SIGNAL(clicked(double)), this, SLOT(makeXcut(double)));
	QObject::connect(this->ui.yAxisWidget->getScalePicker(),
			SIGNAL(clicked(double)), this, SLOT(makeYcut(double)));
	QObject::connect(this->ui.zAxisWidget->getScalePicker(),
			SIGNAL(clicked(double)), this, SLOT(makeZcut(double)));

	QObject::connect(this, SIGNAL(sliceNamed(const QString &)),
			this->ui.xAxisWidget, SLOT(setIndicatorName(const QString &)));
	QObject::connect(this, SIGNAL(sliceNamed(const QString &)),
			this->ui.yAxisWidget, SLOT(setIndicatorName(const QString &)));
	QObject::connect(this, SIGNAL(sliceNamed(const QString &)),
			this->ui.zAxisWidget, SLOT(setIndicatorName(const QString &)));
}

MultiSliceView::~MultiSliceView()
{
	pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
	this->destroyFilter(builder, QString("Slice"));
	builder->destroy(this->mainView);
}

pqRenderView* MultiSliceView::getView()
{
	return this->mainView.data();
}

void MultiSliceView::clearIndicatorSelections()
{
	this->ui.xAxisWidget->clearSelections();
	this->ui.yAxisWidget->clearSelections();
	this->ui.zAxisWidget->clearSelections();
}

void MultiSliceView::setupData()
{
	pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();

	pqDataRepresentation *drep = builder->createDataRepresentation(
			this->origSource->getOutputPort(0), this->mainView);
	vtkSMPropertyHelper(drep->getProxy(), "Representation").Set(VTK_SURFACE);
	drep->getProxy()->UpdateVTKObjects();
	this->originSourceRepr = qobject_cast<pqPipelineRepresentation*>(drep);
	this->originSourceRepr->colorByArray("signal",
			vtkDataObject::FIELD_ASSOCIATION_CELLS);
}

void MultiSliceView::setupAxisInfo()
{
	const char *geomXML = vtkSMPropertyHelper(this->origSource->getProxy(),
			"InputGeometryXML").GetAsString();
	GeometryParser parser(geomXML);
	AxisInformation *xinfo = parser.getAxisInfo("XDimension");
	AxisInformation *yinfo = parser.getAxisInfo("YDimension");
	AxisInformation *zinfo = parser.getAxisInfo("ZDimension");

	this->ui.xAxisWidget->setInformation(QString(xinfo->getTitle().c_str()),
				xinfo->getMinimum(), xinfo->getMaximum());
	this->ui.yAxisWidget->setInformation(QString(yinfo->getTitle().c_str()),
				yinfo->getMinimum(), yinfo->getMaximum());
	this->ui.zAxisWidget->setInformation(QString(zinfo->getTitle().c_str()),
			zinfo->getMinimum(), zinfo->getMaximum());

	delete xinfo;
	delete yinfo;
	delete zinfo;
}

void MultiSliceView::render()
{
	this->origSource = pqActiveObjects::instance().activeSource();
	this->setupData();
	this->setupAxisInfo();
	this->mainView->resetDisplay();
	this->mainView->render();
}

void MultiSliceView::makeXcut(double value)
{
	double origin[3], orient[3];
	origin[0] = value;
	origin[1] = 0.0;
	origin[2] = 0.0;
	orient[0] = 1.0;
	orient[1] = 0.0;
	orient[2] = 0.0;
	this->makeCut(origin, orient);
}

void MultiSliceView::makeYcut(double value)
{
	double origin[3], orient[3];
	origin[0] = 0.0;
	origin[1] = value;
	origin[2] = 0.0;
	orient[0] = 0.0;
	orient[1] = 1.0;
	orient[2] = 0.0;
	this->makeCut(origin, orient);
}

void MultiSliceView::makeZcut(double value)
{
	double origin[3], orient[3];
	origin[0] = 0.0;
	origin[1] = 0.0;
	origin[2] = value;
	orient[0] = 0.0;
	orient[1] = 0.0;
	orient[2] = 1.0;
	this->makeCut(origin, orient);
}

void MultiSliceView::makeCut(double origin[], double orient[])
{
	this->clearIndicatorSelections();
	pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();

	pqPipelineSource *cut = builder->createFilter("filters", "Cut",
			this->origSource);
	emit sliceNamed(cut->getSMName());
	pqDataRepresentation *trepr = builder->createDataRepresentation(
			cut->getOutputPort(0),this->mainView);
	pqPipelineRepresentation *repr = qobject_cast<pqPipelineRepresentation *>(trepr);
	vtkSMProxy *plane = vtkSMPropertyHelper(cut->getProxy(),
			"CutFunction").GetAsProxy();

	repr->colorByArray("signal", vtkDataObject::FIELD_ASSOCIATION_CELLS);

	vtkSMPropertyHelper(plane, "Origin").Set(origin, 3);
	vtkSMPropertyHelper(plane, "Normal").Set(orient, 3);
	trepr->getProxy()->UpdateVTKObjects();
}

void MultiSliceView::selectIndicator()
{
	pqServerManagerSelectionModel *smsModel = pqApplicationCore::instance()->getSelectionModel();
	pqPipelineSource *source = qobject_cast<pqPipelineSource *>(smsModel->currentItem());
	QString name = source->getSMName();
	this->ui.xAxisWidget->selectIndicator(name);
	this->ui.yAxisWidget->selectIndicator(name);
	this->ui.zAxisWidget->selectIndicator(name);
}

void MultiSliceView::updateSelectedIndicator()
{
	pqServerManagerSelectionModel *smsModel = pqApplicationCore::instance()->getSelectionModel();
	pqPipelineSource *cut = qobject_cast<pqPipelineSource *>(smsModel->currentItem());
	vtkSMProxy *plane = vtkSMPropertyHelper(cut->getProxy(),
			"CutFunction").GetAsProxy();
	double origin[3];
	vtkSMPropertyHelper(plane, "Origin").Get(origin, 3);
	if (this->ui.xAxisWidget->hasIndicator())
	{
		this->ui.xAxisWidget->updateIndicator(origin[0]);
	}
	if (this->ui.yAxisWidget->hasIndicator())
	{
		this->ui.yAxisWidget->updateIndicator(origin[1]);
	}
	if (this->ui.zAxisWidget->hasIndicator())
	{
		this->ui.zAxisWidget->updateIndicator(origin[2]);
	}
}
