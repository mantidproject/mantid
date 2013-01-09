#include "MantidVatesSimpleGuiViewWidgets/MultisliceView.h"

#include "MantidVatesSimpleGuiQtWidgets/GeometryParser.h"

#include "MantidGeometry/MDGeometry/MDPlaneImplicitFunction.h"
#include "MantidQtSliceViewer/SliceViewerWindow.h"
#include "MantidQtFactory/WidgetFactory.h"
#include "MantidVatesAPI/RebinningKnowledgeSerializer.h"

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqDataRepresentation.h>
#include <pqMultiSliceView.h>
#include <pqObjectBuilder.h>
#include <pqPipelineSource.h>
#include <pqRenderView.h>
#include <pqServerManagerModel.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>

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

  pqDataRepresentation *drep = builder->createDataRepresentation(\
        this->origSrc->getOutputPort(0), this->mainView);
  vtkSMPropertyHelper(drep->getProxy(), "Representation").Set("Slices");
  drep->getProxy()->UpdateVTKObjects();
}

void MultiSliceView::render()
{
  this->origSrc = pqActiveObjects::instance().activeSource();
  this->checkSliceViewCompat();
  this->setupData();
  this->resetDisplay();
  this->onAutoScale();
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
 * This function checks the sources for the WorkspaceName property. If found,
 * the ability to show a given cut in the SliceViewer will be activated.
 */
void MultiSliceView::checkSliceViewCompat()
{
  QString wsName = this->getWorkspaceName();
  if (!wsName.isEmpty())
  {
  }
}

/**
 * This function is responsible for opening the given cut in SliceViewer.
 * It will gather all of the necessary information and create an XML
 * representation of the current dataset and cut parameters. That will then
 * be handed to the SliceViewer.
 * @param name the slice to be opened in SliceViewer
 */
void MultiSliceView::showCutInSliceViewer(const QString &name)
{
  // Get the associated workspace name
  QString wsName = this->getWorkspaceName();

  // Have to jump through some hoops since a rebinner could be used.
  pqServerManagerModel *smModel = pqApplicationCore::instance()->getServerManagerModel();
  QList<pqPipelineSource *> srcs = smModel->findItems<pqPipelineSource *>();
  pqPipelineSource *src1 = NULL;
  foreach (pqPipelineSource *src, srcs)
  {
    const QString name(src->getProxy()->GetXMLName());
    if (name.contains("MDEWRebinningCutter"))
    {
      src1 = src;
    }
  }
  if (NULL == src1)
  {
    src1 = smModel->getItemAtIndex<pqPipelineSource *>(0);
  }

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

  // Get the necessary information from the cut
  pqPipelineSource *cut = smModel->findItem<pqPipelineSource *>(name);
  vtkSMProxy *plane = vtkSMPropertyHelper(cut->getProxy(),
                                          "CutFunction").GetAsProxy();
  double origin[3];
  vtkSMPropertyHelper(plane, "Origin").Get(origin, 3);
  double orient[3];
  vtkSMPropertyHelper(plane, "Normal").Get(orient, 3);

  // Create the XML holder
  VATES::RebinningKnowledgeSerializer rks(VATES::LocationNotRequired);
  rks.setWorkspaceName(wsName.toStdString());
  rks.setGeometryXML(geomXML);

  MDImplicitFunction_sptr impplane(new MDPlaneImplicitFunction(3, orient,
                                                               origin));
  rks.setImplicitFunction(impplane);;
  QString titleAddition = name;

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
