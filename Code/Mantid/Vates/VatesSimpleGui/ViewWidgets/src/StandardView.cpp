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
}

void StandardView::destroyView()
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
  this->origSrc = pqActiveObjects::instance().activeSource();
  if (NULL == this->origSrc)
  {
    return;
  }
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();

  if (this->isMDHistoWorkspace(this->origSrc))
  {
    this->ui.rebinButton->setEnabled(false);
  }

  // Show the data
  pqDataRepresentation *drep = builder->createDataRepresentation(\
        this->origSrc->getOutputPort(0), this->view);
  int reptype = VTK_SURFACE;
  if (this->isPeaksWorkspace(this->origSrc))
  {
    reptype = VTK_WIREFRAME;
  }
  vtkSMPropertyHelper(drep->getProxy(), "Representation").Set(reptype);
  drep->getProxy()->UpdateVTKObjects();
  this->origRep = qobject_cast<pqPipelineRepresentation*>(drep);
  this->origRep->colorByArray("signal", vtkDataObject::FIELD_ASSOCIATION_CELLS);

  this->resetDisplay();
  this->onAutoScale();
  emit this->triggerAccept();
}

void StandardView::onCutButtonClicked()
{
  // Apply cut to currently viewed data
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  builder->createFilter("filters", "Cut", this->getPvActiveSrc());
}

void StandardView::onRebinButtonClicked()
{
  if (this->origSrc)
  {
    pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
    this->rebinCut = builder->createFilter("filters", "MDEWRebinningCutter",
                                           this->origSrc);
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

void StandardView::resetCamera()
{
  this->view->resetCamera();
}

/**
 * This function checks if a pqPipelineSource is a MDHistoWorkspace.
 * @return true if the source is a MDHistoWorkspace
 */
bool StandardView::isMDHistoWorkspace(pqPipelineSource *src)
{
  QString wsType(vtkSMPropertyHelper(src->getProxy(),
                                     "WorkspaceTypeName", true).GetAsString());
  // This must be a Mantid rebinner filter if the property is empty.
  if (wsType.isEmpty())
  {
    wsType = src->getSMName();
  }
  return wsType.contains("MDHistoWorkspace");
}

}
}
}
