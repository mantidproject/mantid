#include "MantidVatesSimpleGuiViewWidgets/StandardView.h"

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

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{

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
  pqDataRepresentation *drep = builder->createDataRepresentation(\
        this->origSource->getOutputPort(0), this->view);
  vtkSMPropertyHelper(drep->getProxy(), "Representation").Set(VTK_SURFACE);
  drep->getProxy()->UpdateVTKObjects();
  this->originSourceRepr = qobject_cast<pqPipelineRepresentation*>(drep);
  this->originSourceRepr->colorByArray("signal",
                                       vtkDataObject::FIELD_ASSOCIATION_CELLS);

  this->resetDisplay();
  this->renderAll();

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

void StandardView::renderAll()
{
  this->view->render();
}

void StandardView::resetDisplay()
{
  this->view->resetDisplay();
}

}
}
}
