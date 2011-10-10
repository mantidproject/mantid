#include "MantidVatesSimpleGuiViewWidgets/SplatterPlotView.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqObjectBuilder.h"
#include "pqPipelineSource.h"
#include "pqRenderView.h"

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{

SplatterPlotView::SplatterPlotView(QWidget *parent) : ViewBase(parent)
{
  this->ui.setupUi(this);

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
  //pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();

  /*
  // Show the data
  pqDataRepresentation *drep = builder->createDataRepresentation(\
        this->origSource->getOutputPort(0), this->view);
  int reptype = VTK_SURFACE;
  char *xmlName = this->origSource->getProxy()->GetXMLName();
  if (QString("PeaksReader") == QString(xmlName) || QString("Peaks Source") == QString(xmlName))
  {
    reptype = VTK_WIREFRAME;
  }
  vtkSMPropertyHelper(drep->getProxy(), "Representation").Set(reptype);
  drep->getProxy()->UpdateVTKObjects();
  this->originSourceRepr = qobject_cast<pqPipelineRepresentation*>(drep);
  this->originSourceRepr->colorByArray("signal",
                                       vtkDataObject::FIELD_ASSOCIATION_CELLS);

  */
  this->resetDisplay();
  this->renderAll();
  //QPair<double, double> range = this->originSourceRepr->getColorFieldRange();
  //emit this->dataRange(range.first, range.second);
}

void SplatterPlotView::renderAll()
{
  this->view->render();
}

void SplatterPlotView::resetDisplay()
{
  this->view->resetDisplay();
}

}
}
}
