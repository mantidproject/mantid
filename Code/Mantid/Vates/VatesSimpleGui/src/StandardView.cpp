#include "standardview.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqObjectBuilder.h"
#include "pqPipelineRepresentation.h"
#include "pqPipelineSource.h"
#include "pqRenderView.h"
#include "vtkDataObject.h"
#include "vtkProperty.h"
#include "vtkSMPropertyHelper.h"

#include <QHBoxLayout>

StandardView::StandardView(QWidget *parent) : IView(parent)
{
	this->setupUi(this);

	// Set the cut button to create a slice on the data
	QObject::connect(this->cutButton, SIGNAL(clicked()), this,
			SLOT(onCutButtonClicked()));

	// Set the rebin button to create the RebinCutter operator
	QObject::connect(this->rebinButton, SIGNAL(clicked()), this,
			SLOT(onRebinButtonClicked()));

	this->view = this->createRenderView(this->renderFrame);
}

StandardView::~StandardView()
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
	this->origSource = pqActiveObjects::instance().activeSource();
	pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();

	// Show the data
	pqDataRepresentation *drep = builder->createDataRepresentation(
			this->origSource->getOutputPort(0), this->view);
	vtkSMPropertyHelper(drep->getProxy(), "Representation").Set(VTK_SURFACE);
	drep->getProxy()->UpdateVTKObjects();
	this->originSourceRepr = qobject_cast<pqPipelineRepresentation*>(drep);
	this->originSourceRepr->colorByArray("signal",
			vtkDataObject::FIELD_ASSOCIATION_CELLS);

	this->view->resetDisplay();
	this->view->render();
}

void StandardView::onCutButtonClicked()
{
    // Apply cut to currently viewed data
    pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
    builder->createFilter("filters", "Cut", pqActiveObjects::instance().activeSource());
}

void StandardView::onRebinButtonClicked()
{
    pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
    this->rebinCut = builder->createFilter("filters", "RebinningCutter",
    		this->origSource);
}
