#include "StandardView.h"

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqChartValue.h>
#include <pqColorMapModel.h>
#include <pqDataRepresentation.h>
#include <pqObjectBuilder.h>
#include <pqPipelineRepresentation.h>
#include <pqPipelineSource.h>
#include <pqRenderView.h>
#include <pqScalarsToColors.h>
#include <pqSMAdaptor.h>
#include <vtkDataObject.h>
#include <vtkProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>

#include <QHBoxLayout>

StandardView::StandardView(QWidget *parent) : ViewBase(parent)
{
  this->ui.setupUi(this);

	// Set the cut button to create a slice on the data
  QObject::connect(this->ui.cutButton, SIGNAL(clicked()), this,
			SLOT(onCutButtonClicked()));

	// Set the rebin button to create the RebinCutter operator
  QObject::connect(this->ui.rebinButton, SIGNAL(clicked()), this,
			SLOT(onRebinButtonClicked()));

  this->view = this->createRenderView(this->ui.renderFrame);
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

  QPair<double, double> range = this->originSourceRepr->getColorFieldRange();
  emit this->dataRange(range.first, range.second);
}

void StandardView::onCutButtonClicked()
{
  // Apply cut to currently viewed data
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  builder->createFilter("filters", "Cut", pqActiveObjects::instance().activeSource());
}

void StandardView::onRebinButtonClicked()
{
  if (this->origSource)
  {
    pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
    this->rebinCut = builder->createFilter("filters", "MDEWRebinningCutter",
    this->origSource);
    emit this->enableMultiSliceViewButton();
  }
}

void StandardView::onColorMapChange(const pqColorMapModel *model)
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
  this->view->render();
}

void StandardView::onColorScaleChange(double min, double max)
{
  this->originSourceRepr->getLookupTable()->setScalarRange(min, max);
  this->originSourceRepr->getProxy()->UpdateVTKObjects();
  this->view->render();
}

void StandardView::onAutoScale()
{
  QPair<double, double> val = this->originSourceRepr->getColorFieldRange();
  double min = val.first;
  double max = val.second;
  this->originSourceRepr->getLookupTable()->setScalarRange(min, max);
  this->originSourceRepr->getProxy()->UpdateVTKObjects();
  this->view->render();
  emit this->dataRange(min, max);
}

void StandardView::onLogScale(int state)
{
  pqScalarsToColors *lut = this->originSourceRepr->getLookupTable();
  pqSMAdaptor::setElementProperty(lut->getProxy()->GetProperty("UseLogScale"),
                                  state);
  lut->getProxy()->UpdateVTKObjects();
  this->view->render();
}
