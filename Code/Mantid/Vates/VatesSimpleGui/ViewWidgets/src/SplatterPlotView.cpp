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
  if (this->peaksSource)
  {
    builder->destroy(this->peaksSource);
    pqActiveObjects::instance().setActiveSource(this->origSrc);
  }
  if (this->threshSource)
  {
    builder->destroy(this->threshSource);
  }
  if (this->splatSource)
  {
    builder->destroy(this->splatSource);
  }
  builder->destroy(this->view);
}

pqRenderView* SplatterPlotView::getView()
{
  return this->view.data();
}

void SplatterPlotView::render()
{
  pqPipelineSource *src = NULL;
  src = pqActiveObjects::instance().activeSource();

  int renderType = VTK_SURFACE;
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();

  if (!this->isPeaksWorkspace(src))
  {
    this->origSrc = src;
    this->splatSource = builder->createFilter("filters",
                                              "MantidParaViewSplatterPlot",
                                              this->origSrc);
    src = this->splatSource;
  }
  else
  {
    this->peaksSource = src;
    renderType = VTK_WIREFRAME;
  }

  // Show the data
  pqDataRepresentation *drep = builder->createDataRepresentation(\
           src->getOutputPort(0), this->view);
  vtkSMPropertyHelper(drep->getProxy(), "Representation").Set(renderType);
  drep->getProxy()->UpdateVTKObjects();
  pqPipelineRepresentation *prep = NULL;
  prep = qobject_cast<pqPipelineRepresentation*>(drep);
  prep->colorByArray("signal", vtkDataObject::FIELD_ASSOCIATION_CELLS);

  this->resetDisplay();
  if (NULL == this->peaksSource)
  {
    this->onAutoScale();
  }
  else
  {
    this->renderAll();
  }
  emit this->triggerAccept();
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

void SplatterPlotView::checkView()
{
  if (NULL == this->peaksSource)
  {
    ViewBase::checkView();
  }
}

void SplatterPlotView::resetCamera()
{
  this->view->resetCamera();
}

}
}
}
