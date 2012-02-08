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

/**
 * This function sets up the UI components, adds connections for the view's
 * buttons and creates the rendering view.
 * @param parent the parent widget for the standard view
 */
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
  if (this->isPeaksWorkspace(this->origSrc))
  {
    this->ui.rebinButton->setEnabled(false);
    this->ui.cutButton->setEnabled(false);
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
    this->ui.cutButton->setEnabled(false);
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
 * This function enables the cut button for the standard view.
 */
void StandardView::updateUI()
{
  this->ui.cutButton->setEnabled(true);
}

} // SimpleGui
} // Vates
} // Mantid
