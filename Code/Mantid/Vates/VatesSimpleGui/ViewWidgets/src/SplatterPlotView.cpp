#include "MantidVatesSimpleGuiViewWidgets/SplatterPlotView.h"

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
#include <vtkProperty.h>
#include <vtkSMPropertyHelper.h>

#if defined(__INTEL_COMPILER)
  #pragma warning enable 1170
#endif

#include <QMessageBox>

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{

SplatterPlotView::SplatterPlotView(QWidget *parent) : ViewBase(parent)
{
  this->noOverlay = false;
  this->ui.setupUi(this);

  // Set the threshold button to create a threshold filter on data
  QObject::connect(this->ui.thresholdButton, SIGNAL(clicked()),
                   this, SLOT(onThresholdButtonClicked()));

  // Set connection to toggle button for peak coordinate checking
  QObject::connect(this->ui.overridePeakCoordsButton,
                   SIGNAL(toggled(bool)),
                   this,
                   SLOT(onOverridePeakCoordToggled(bool)));

  this->view = this->createRenderView(this->ui.renderFrame);
}

SplatterPlotView::~SplatterPlotView()
{
}

void SplatterPlotView::destroyView()
{
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  if (!this->peaksSource.isEmpty())
  {
    this->destroyPeakSources();
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

  QString renderType = "Surface";
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();

  // Do not allow overplotting of MDWorkspaces
  if (!this->isPeaksWorkspace(src) && NULL != this->splatSource)
  {
    QMessageBox::warning(this, QApplication::tr("Overplotting Warning"),
                         QApplication::tr("SplatterPlot mode does not allow "\
                                          "more that one MDEventWorkspace to "\
                                          "be plotted."));
    // Need to destroy source since we tried to load it and set the active
    // back to something. In this case we'll choose the splatter plot filter.
    builder->destroy(src);
    pqActiveObjects::instance().setActiveSource(this->splatSource);
    this->noOverlay = true;
    return;
  }

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
    this->peaksSource.append(src);
    renderType = "Wireframe";
  }

  // Show the data
  src->updatePipeline();
  pqDataRepresentation *drep = builder->createDataRepresentation(\
           src->getOutputPort(0), this->view);
  vtkSMPropertyHelper(drep->getProxy(), "Representation").Set(renderType.toStdString().c_str());
  drep->getProxy()->UpdateVTKObjects();
  pqPipelineRepresentation *prep = NULL;
  prep = qobject_cast<pqPipelineRepresentation*>(drep);
  prep->colorByArray("signal", vtkDataObject::FIELD_ASSOCIATION_CELLS);

  this->resetDisplay();
  if (this->peaksSource.isEmpty())
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

/**
 * This function checks to see if the Override PC button has been
 * toggled. If the state is unchecked (false), we want to make sure
 * that the coordniates are matched back to the MD workspace.
 * @param state : true is button is checked, false if not
 */
void SplatterPlotView::onOverridePeakCoordToggled(bool state)
{
  if (!state)
  {
    this->checkPeaksCoordinates();
    emit this->triggerAccept();
  }
}

void SplatterPlotView::checkPeaksCoordinates()
{
  if (!this->peaksSource.isEmpty() &&
      !this->ui.overridePeakCoordsButton->isChecked())
  {
    int peakViewCoords = vtkSMPropertyHelper(this->origSrc->getProxy(),
                                             "SpecialCoordinates").GetAsInt();
    // Make commensurate with vtkPeakMarkerFactory
    peakViewCoords--;

    foreach(pqPipelineSource *src, this->peaksSource)
    {
      vtkSMPropertyHelper(src->getProxy(),
                          "Peak Dimensions").Set(peakViewCoords);
      src->getProxy()->UpdateVTKObjects();
    }
  }
}

void SplatterPlotView::onThresholdButtonClicked()
{
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  this->threshSource = builder->createFilter("filters", "Threshold",
                                             this->splatSource);
  emit this->lockColorControls();
}

void SplatterPlotView::checkView()
{
  if (!this->noOverlay && this->peaksSource.isEmpty())
  {
    ViewBase::checkView();
  }
  this->noOverlay = false;
}

void SplatterPlotView::resetCamera()
{
  this->view->resetCamera();
}

void SplatterPlotView::destroyPeakSources()
{
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  for( int i = 0; i < this->peaksSource.size(); ++i )
  {
    builder->destroy(this->peaksSource.at(i));
  }
}

} // SimpleGui
} // Vates
} // Mantid
