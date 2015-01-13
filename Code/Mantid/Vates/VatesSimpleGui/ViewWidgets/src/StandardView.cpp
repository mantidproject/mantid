#include "MantidVatesSimpleGuiViewWidgets/StandardView.h"

// Have to deal with ParaView warnings and Intel compiler the hard way.
#if defined(__INTEL_COMPILER)
  #pragma warning disable 1170
#endif

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqDataRepresentation.h>
#include <pqObjectBuilder.h>
#include <pqPipelineRepresentation.h>
#include <pqPipelineSource.h>
#include <pqRenderView.h>
#include <vtkDataObject.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>

#if defined(__INTEL_COMPILER)
  #pragma warning enable 1170
#endif

#include <QHBoxLayout>
#include <QMessageBox>
#include <QString>

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
  this->cameraReset = false;

  // Set the cut button to create a slice on the data
  QObject::connect(this->ui.cutButton, SIGNAL(clicked()), this,
                   SLOT(onCutButtonClicked()));

  // Set the scale button to create the ScaleWorkspace operator
  QObject::connect(this->ui.scaleButton, SIGNAL(clicked()),
                   this, SLOT(onScaleButtonClicked()));

  // Check for rebinning source being deleted
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  QObject::connect(builder, SIGNAL(destroying(pqPipelineSource*)),
                   this, SLOT(onDestroyingSource(pqPipelineSource*)));

  this->view = this->createRenderView(this->ui.renderFrame);

  QObject::connect(this->view.data(), SIGNAL(endRender()),
                   this, SLOT(onRenderDone()));
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

  if (this->isPeaksWorkspace(this->origSrc))
  {
    this->ui.cutButton->setEnabled(false);
  }

  // Show the data
  pqDataRepresentation *drep = builder->createDataRepresentation(\
        this->origSrc->getOutputPort(0), this->view);
  QString reptype = "Surface";
  if (this->isPeaksWorkspace(this->origSrc))
  {
    reptype = "Wireframe";
  }
  vtkSMPropertyHelper(drep->getProxy(), "Representation").Set(reptype.toStdString().c_str());
  drep->getProxy()->UpdateVTKObjects();
  this->origRep = qobject_cast<pqPipelineRepresentation*>(drep);
  this->origRep->colorByArray("signal", vtkDataObject::FIELD_ASSOCIATION_CELLS);

  this->resetDisplay();
  emit this->triggerAccept();
}

void StandardView::onCutButtonClicked()
{
  // Apply cut to currently viewed data
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  builder->createFilter("filters", "Cut", this->getPvActiveSrc());
}

void StandardView::onScaleButtonClicked()
{
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  this->scaler = builder->createFilter("filters",
                                       "MantidParaViewScaleWorkspace",
                                       this->getPvActiveSrc());
}

/**
 * This function is responsible for calling resetCamera if the internal
 * variable cameraReset has been set to true.
 */
void StandardView::onRenderDone()
{
  if (this->cameraReset)
  {
    this->resetCamera();
    this->cameraReset = false;
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

void StandardView::updateView()
{
  this->cameraReset = true;
}

/**
 * This function checks a pipeline source that ParaView says is being
 * deleted. If the source is a Mantid rebinning filter, the restriction
 * on the SplatterPlot view should be lifted. Also, the cut button can
 * be enabled.
 * @param src : The pipeline source being checked
 */
void StandardView::onDestroyingSource(pqPipelineSource *src)
{
  if (src->getSMName().contains("MantidRebinning"))
  {
    emit this->setViewStatus(ModeControlWidget::SPLATTERPLOT, true);
    this->ui.cutButton->setEnabled(true);
  }
}

} // SimpleGui
} // Vates
} // Mantid
