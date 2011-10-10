#include "MantidVatesSimpleGuiViewWidgets/SplatterPlotView.h"

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

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{

SplatterPlotView::SplatterPlotView(QWidget *parent) : ViewBase(parent)
{
  this->ui.setupUi(this);

  // Set the threshold button to create a threshold filter on data
  QObject::connect(this->ui.thresholdButton, SIGNAL(clicked()),
                   this, SLOT(onThresholdButtonClicked()));

  this->view = this->createRenderView(this->ui.renderFrame);
}

SplatterPlotView::~SplatterPlotView()
{
}

void SplatterPlotView::destroyView()
{
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  //this->destroyFilter(builder, QString("Slice"));
  builder->destroy(this->view);
}

pqRenderView* SplatterPlotView::getView()
{
  return this->view.data();
}

void SplatterPlotView::render()
{
  this->origSource = pqActiveObjects::instance().activeSource();
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  this->splatSource = builder->createFilter("filters", "MantidParaViewSplatterPlot", this->origSource);

  // Show the data
  pqDataRepresentation *drep = builder->createDataRepresentation(\
        this->splatSource->getOutputPort(0), this->view);
  vtkSMPropertyHelper(drep->getProxy(), "Representation").Set(VTK_SURFACE);
  drep->getProxy()->UpdateVTKObjects();
  this->splatRepr = qobject_cast<pqPipelineRepresentation*>(drep);
  this->splatRepr->colorByArray("signal", vtkDataObject::FIELD_ASSOCIATION_CELLS);

  this->resetDisplay();
  this->renderAll();

  QPair<double, double> range = this->splatRepr->getColorFieldRange();
  emit this->dataRange(range.first, range.second);
}

void SplatterPlotView::renderAll()
{
  this->view->render();
}

void SplatterPlotView::resetDisplay()
{
  this->view->resetDisplay();
}

void SplatterPlotView::onThresholdButtonClicked()
{
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  this->threshSource = builder->createFilter("filters", "Threshold",
                                             this->splatSource);
}

}
}
}
