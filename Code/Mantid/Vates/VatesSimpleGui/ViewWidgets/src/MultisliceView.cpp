#include "MantidVatesSimpleGuiViewWidgets/MultisliceView.h"

#include "MantidVatesSimpleGuiQtWidgets/GeometryParser.h"

#include "MantidGeometry/MDGeometry/MDPlaneImplicitFunction.h"
#include "MantidQtSliceViewer/SliceViewerWindow.h"
#include "MantidQtFactory/WidgetFactory.h"
#include "MantidVatesAPI/RebinningKnowledgeSerializer.h"

// Have to deal with ParaView warnings and Intel compiler the hard way.
#if defined(__INTEL_COMPILER)
  #pragma warning disable 1170
#endif

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqDataRepresentation.h>
#include <pqMultiSliceView.h>
#include <pqObjectBuilder.h>
#include <pqPipelineSource.h>
#include <pqRenderView.h>
#include <pqServerManagerModel.h>
#include <vtkContextMouseEvent.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>

#if defined(__INTEL_COMPILER)
  #pragma warning enable 1170
#endif

#include <QString>

#include <iostream>

using namespace Mantid::Geometry;
using namespace MantidQt::SliceViewer;

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{

MultiSliceView::MultiSliceView(QWidget *parent) : ViewBase(parent)
{
  this->ui.setupUi(this);
  pqRenderView *tmp = this->createRenderView(this->ui.renderFrame,
                                             QString("MultiSlice"));
  this->mainView = qobject_cast<pqMultiSliceView *>(tmp);
  QObject::connect(this->mainView,
                   SIGNAL(sliceClicked(int, double, int, int)),
                   this,
                   SLOT(checkSliceClicked(int,double,int,int)));
}

MultiSliceView::~MultiSliceView()
{
}

void MultiSliceView::destroyView()
{
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  builder->destroy(this->mainView);
}

pqRenderView* MultiSliceView::getView()
{
  return qobject_cast<pqRenderView*>(this->mainView.data());
}


void MultiSliceView::setupData()
{
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();

  // Make sure that origsrc exists
  if (this->origSrc)
  {
    pqDataRepresentation *drep = builder->createDataRepresentation(\
    this->origSrc->getOutputPort(0), this->mainView);
    vtkSMPropertyHelper(drep->getProxy(), "Representation").Set("Slices");
    drep->getProxy()->UpdateVTKObjects();
  }
}

void MultiSliceView::render()
{
  this->origSrc = pqActiveObjects::instance().activeSource();
  this->checkSliceViewCompat();
  this->setupData();
  this->resetDisplay();
}

void MultiSliceView::renderAll()
{
  this->mainView->render();
}

void MultiSliceView::resetDisplay()
{
  this->mainView->resetDisplay();
}

void MultiSliceView::resetCamera()
{
  this->mainView->resetCamera();
}

/**
 * This function checks the signal coming from the MultiSliceView when a slice
 * indicator is clicked. It then calls for the slice to be shown in the
 * SliceViewer.
 * @param axisIndex : index for the axis on which the clicked indicator resides
 * @param sliceOffsetOnAxis : location of slice along axis
 * @param button : which mouse button is being used
 * @param modifier : which modifier key is being used
 */
void MultiSliceView::checkSliceClicked(int axisIndex, double sliceOffsetOnAxis,
                                       int button, int modifier)
{
  if (modifier == vtkContextMouseEvent::SHIFT_MODIFIER &&
      (button == vtkContextMouseEvent::LEFT_BUTTON ||
       button == vtkContextMouseEvent::RIGHT_BUTTON))
  {
    this->showCutInSliceViewer(axisIndex, sliceOffsetOnAxis);
  }
}

/**
 * This function checks the sources for the WorkspaceName property. If not found,
 * the ability to show a given cut in the SliceViewer will be deactivated.
 */
void MultiSliceView::checkSliceViewCompat()
{
  QString wsName = this->getWorkspaceName();
  if (wsName.isEmpty())
  {
    QObject::disconnect(this->mainView, 0, this, 0);
  }
}

/**
 * This function is responsible for opening the given cut in SliceViewer.
 * It will gather all of the necessary information and create an XML
 * representation of the current dataset and cut parameters. That will then
 * be handed to the SliceViewer.
 * @param axisIndex the index of the slice to be opened in SliceViewer
 * @param sliceOffsetOnAxis position of the slice along given axis
 */
void MultiSliceView::showCutInSliceViewer(int axisIndex,
                                          double sliceOffsetOnAxis)
{
  // Get the associated workspace name
  QString wsName = this->getWorkspaceName();

  // Have to jump through some hoops since a rebinner could be used.
  pqServerManagerModel *smModel = pqApplicationCore::instance()->getServerManagerModel();
  QList<pqPipelineSource *> srcs = smModel->findItems<pqPipelineSource *>();
  pqPipelineSource *src1 = NULL;
  pqPipelineSource *src2 = NULL;
  foreach (pqPipelineSource *src, srcs)
  {
    const QString name(src->getProxy()->GetXMLName());

    if (name.contains("ScaleWorkspace"))
    {
      src2 = src;
    }
  }

  src1 = smModel->getItemAtIndex<pqPipelineSource *>(0);

  // Get the current dataset characteristics
  const char *inGeomXML = vtkSMPropertyHelper(src1->getProxy(),
                                             "InputGeometryXML").GetAsString();
  // Check for timesteps and insert the value into the XML if necessary
  std::string geomXML;
  if ( this->srcHasTimeSteps(src1) )
  {
    GeometryParser parser(inGeomXML);
    geomXML = parser.addTDimValue(this->getCurrentTimeStep());
  }
  else
  {
    geomXML = std::string(inGeomXML);
  }

  if (NULL != src2)
  {
    // Need to see if scaling is applied to axis
    QString scalingProperty("Scaling Factor");
    switch (axisIndex)
    {
    case 0:
      scalingProperty.prepend("X ");
      break;
    case 1:
      scalingProperty.prepend("Y ");
      break;
    case 2:
      scalingProperty.prepend("Z ");
      break;
    default:
      break;
    }

    std::vector<double> scaling = vtkSMPropertyHelper(src2->getProxy(),
                                                      scalingProperty,
                                                      true).GetDoubleArray();

    if (!scaling.empty())
    {
      sliceOffsetOnAxis /= scaling[0];
    }
  }
  const double *orient = this->mainView->GetSliceNormal(axisIndex);

  // Construct origin vector from orientation vector
  double origin[3];
  origin[0] = sliceOffsetOnAxis * orient[0];
  origin[1] = sliceOffsetOnAxis * orient[1];
  origin[2] = sliceOffsetOnAxis * orient[2];

  // Create the XML holder
  VATES::RebinningKnowledgeSerializer rks(VATES::LocationNotRequired);
  rks.setWorkspaceName(wsName.toStdString());
  rks.setGeometryXML(geomXML);

  MDImplicitFunction_sptr impplane(new MDPlaneImplicitFunction(3, orient,
                                                               origin));
  rks.setImplicitFunction(impplane);
  QString titleAddition = "";

  // Use the WidgetFactory to create the slice viewer window
  SliceViewerWindow *w = MantidQt::Factory::WidgetFactory::Instance()->createSliceViewerWindow(wsName, titleAddition);
  try
  {
    // Set the slice points, etc, using the XML definition of the plane function
    w->getSlicer()->openFromXML( QString::fromStdString(rks.createXMLString()) );
    w->show();
  }
  catch (std::runtime_error & e)
  {
    QMessageBox::warning(this, tr("MantidPlot"),
                       tr("The slice could not be shown because of the following error:\n"
                          + QString(e.what())),
                       QMessageBox::Ok, QMessageBox::Ok);
    delete w;
  }
}

/**
 * This function closes user requested SliceViewer windows when the view is
 * closed. The function is a no-op (except for factory creation) when no
 * SliceViewer windows were requested.
 */
void MultiSliceView::closeSubWindows()
{
  MantidQt::Factory::WidgetFactory::Instance()->closeAllSliceViewerWindows();
}

} // namespace SimpleGui
} // namespace Vates
} // namespace Mantid
