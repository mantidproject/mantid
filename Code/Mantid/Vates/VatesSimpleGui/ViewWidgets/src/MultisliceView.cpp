#include "MantidVatesSimpleGuiViewWidgets/MultisliceView.h"

#include "MantidVatesSimpleGuiQtWidgets/AxisInformation.h"
#include "MantidVatesSimpleGuiQtWidgets/AxisInteractor.h"
#include "MantidVatesSimpleGuiQtWidgets/GeometryParser.h"
#include "MantidVatesSimpleGuiQtWidgets/ScalePicker.h"

#include "MantidGeometry/MDGeometry/MDPlaneImplicitFunction.h"
#include "MantidQtSliceViewer/SliceViewerWindow.h"
#include "MantidVatesAPI/RebinningKnowledgeSerializer.h"

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqChartValue.h>
#include <pqColorMapModel.h>
#include <pqDataRepresentation.h>
#include <pqDisplayPolicy.h>
#include <pqObjectBuilder.h>
#include <pqPipelineBrowserWidget.h>
#include <pqPipelineRepresentation.h>
#include <pqPipelineSource.h>
#include <pqRenderView.h>
#include <pqScalarsToColors.h>
#include <pqServerManagerModel.h>
#include <pqServerManagerSelectionModel.h>
#include <pqSMAdaptor.h>
#include <vtkDataObject.h>
#include <vtkProperty.h>
#include <vtkSMProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>
#include <vtkSMViewProxy.h>

#include <vtkSMPropertyIterator.h>

#include <QModelIndex>
#include <QString>

#include <iostream>
#include "MantidQtFactory/WidgetFactory.h"

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
  this->isOrigSrc = false;
  this->ui.setupUi(this);
  this->ui.xAxisWidget->setScalePosition(AxisInteractor::LeftScale);
  this->ui.yAxisWidget->setScalePosition(AxisInteractor::TopScale);
  this->ui.zAxisWidget->setScalePosition(AxisInteractor::RightScale);

  this->mainView = this->createRenderView(this->ui.renderFrame);

  QObject::connect(this->ui.xAxisWidget->getScalePicker(),
                   SIGNAL(clicked(double)), this, SLOT(makeXcut(double)));
  QObject::connect(this->ui.yAxisWidget->getScalePicker(),
                   SIGNAL(clicked(double)), this, SLOT(makeYcut(double)));
  QObject::connect(this->ui.zAxisWidget->getScalePicker(),
                   SIGNAL(clicked(double)), this, SLOT(makeZcut(double)));

  QObject::connect(this->ui.xAxisWidget->getScalePicker(),
                   SIGNAL(moved(double)), this,
                   SLOT(updateCutPosition(double)));
  QObject::connect(this->ui.yAxisWidget->getScalePicker(),
                   SIGNAL(moved(double)), this,
                   SLOT(updateCutPosition(double)));
  QObject::connect(this->ui.zAxisWidget->getScalePicker(),
                   SIGNAL(moved(double)), this,
                   SLOT(updateCutPosition(double)));

  QObject::connect(this->ui.xAxisWidget,
                   SIGNAL(indicatorSelected(const QString &)), this,
                   SLOT(indicatorSelected(const QString &)));
  QObject::connect(this->ui.yAxisWidget,
                   SIGNAL(indicatorSelected(const QString &)), this,
                   SLOT(indicatorSelected(const QString &)));
  QObject::connect(this->ui.zAxisWidget,
                   SIGNAL(indicatorSelected(const QString &)), this,
                   SLOT(indicatorSelected(const QString &)));

  QObject::connect(this, SIGNAL(sliceNamed(const QString &)),
                   this->ui.xAxisWidget,
                   SLOT(setIndicatorName(const QString &)));
  QObject::connect(this, SIGNAL(sliceNamed(const QString &)),
                   this->ui.yAxisWidget,
                   SLOT(setIndicatorName(const QString &)));
  QObject::connect(this, SIGNAL(sliceNamed(const QString &)),
                   this->ui.zAxisWidget,
                   SLOT(setIndicatorName(const QString &)));

  QObject::connect(this->ui.xAxisWidget,
                   SIGNAL(deleteIndicator(const QString &)), this,
                   SLOT(deleteCut(const QString &)));
  QObject::connect(this->ui.yAxisWidget,
                   SIGNAL(deleteIndicator(const QString &)), this,
                   SLOT(deleteCut(const QString &)));
  QObject::connect(this->ui.zAxisWidget,
                   SIGNAL(deleteIndicator(const QString &)), this,
                   SLOT(deleteCut(const QString &)));

  QObject::connect(this->ui.xAxisWidget,
                   SIGNAL(showOrHideIndicator(bool, const QString &)),
                   this, SLOT(cutVisibility(bool, const QString &)));
  QObject::connect(this->ui.yAxisWidget,
                   SIGNAL(showOrHideIndicator(bool, const QString &)),
                   this, SLOT(cutVisibility(bool, const QString &)));
  QObject::connect(this->ui.zAxisWidget,
                   SIGNAL(showOrHideIndicator(bool, const QString &)),
                   this, SLOT(cutVisibility(bool, const QString &)));

  QObject::connect(this->ui.xAxisWidget,
                   SIGNAL(showInSliceView(const QString &)),
                   this,
                   SLOT(showCutInSliceViewer(const QString &)));
  QObject::connect(this->ui.yAxisWidget,
                   SIGNAL(showInSliceView(const QString &)),
                   this,
                   SLOT(showCutInSliceViewer(const QString &)));
  QObject::connect(this->ui.zAxisWidget,
                   SIGNAL(showInSliceView(const QString &)),
                   this,
                   SLOT(showCutInSliceViewer(const QString &)));

  this->ui.xAxisWidget->installEventFilter(this);
  this->ui.yAxisWidget->installEventFilter(this);
  this->ui.zAxisWidget->installEventFilter(this);
}

MultiSliceView::~MultiSliceView()
{
}

/**
 * This function sets an event filter for the AxisInteractor widgets. This
 * will listen and check for resize events. If a resize event is being issued,
 * the AxisInteractor needs to update it's scene rectangle and the correct
 * positions of any indicators.
 * @param ob the QObject associated with the event
 * @param ev the QEvent being issued
 * @return true if this function handles the event
 */
bool MultiSliceView::eventFilter(QObject *ob, QEvent *ev)
{
  if (ev->type() == QEvent::Resize)
  {
    AxisInteractor *axis = static_cast<AxisInteractor *>(ob);
    QString name = axis->objectName();
    int coord = -1;
    if (name == "xAxisWidget")
    {
      coord = 0;
    }
    if (name == "yAxisWidget")
    {
      coord = 1;
    }
    if (name == "zAxisWidget")
    {
      coord = 2;
    }
    axis->updateSceneRect();
    this->resetOrDeleteIndicators(axis, coord);
    return true;
  }
  return QObject::eventFilter(ob, ev);
}

void MultiSliceView::destroyView()
{
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  this->destroyFilter(builder, QString("Slice"));
  builder->destroy(this->mainView);
}

pqRenderView* MultiSliceView::getView()
{
  return this->mainView.data();
}

void MultiSliceView::clearIndicatorSelections()
{
  this->ui.xAxisWidget->clearSelections();
  this->ui.yAxisWidget->clearSelections();
  this->ui.zAxisWidget->clearSelections();
}

void MultiSliceView::setupData()
{
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();

  pqDataRepresentation *drep = builder->createDataRepresentation(\
        this->origSrc->getOutputPort(0), this->mainView);
  vtkSMPropertyHelper(drep->getProxy(), "Representation").Set(VTK_SURFACE);
  drep->getProxy()->UpdateVTKObjects();
  this->origRep = qobject_cast<pqPipelineRepresentation*>(drep);
  this->origRep->colorByArray("signal",
                                       vtkDataObject::FIELD_ASSOCIATION_CELLS);
}

void MultiSliceView::setupAxisInfo()
{
  const char *geomXML = vtkSMPropertyHelper(this->origSrc->getProxy(),
                                            "InputGeometryXML").GetAsString();

  GeometryParser parser(geomXML);
  AxisInformation *xinfo = parser.getAxisInfo("XDimension");
  AxisInformation *yinfo = parser.getAxisInfo("YDimension");
  AxisInformation *zinfo = parser.getAxisInfo("ZDimension");

  this->ui.xAxisWidget->setInformation(xinfo);
  this->ui.yAxisWidget->setInformation(yinfo);
  this->ui.zAxisWidget->setInformation(zinfo);

  delete xinfo;
  delete yinfo;
  delete zinfo;
}

void MultiSliceView::render()
{
  this->origSrc = pqActiveObjects::instance().activeSource();
  this->checkSliceViewCompat();
  this->setupData();
  this->setupAxisInfo();
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

void MultiSliceView::makeXcut(double value)
{
  double origin[3], orient[3];
  origin[0] = value;
  origin[1] = 0.0;
  origin[2] = 0.0;
  orient[0] = 1.0;
  orient[1] = 0.0;
  orient[2] = 0.0;
  this->makeCut(origin, orient);
}

void MultiSliceView::makeYcut(double value)
{
  double origin[3], orient[3];
  origin[0] = 0.0;
  origin[1] = value;
  origin[2] = 0.0;
  orient[0] = 0.0;
  orient[1] = 1.0;
  orient[2] = 0.0;
  this->makeCut(origin, orient);
}

void MultiSliceView::makeZcut(double value)
{
  double origin[3], orient[3];
  origin[0] = 0.0;
  origin[1] = 0.0;
  origin[2] = value;
  orient[0] = 0.0;
  orient[1] = 0.0;
  orient[2] = 1.0;
  this->makeCut(origin, orient);
}

void MultiSliceView::makeCut(double origin[], double orient[])
{
  this->clearIndicatorSelections();
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();

  pqPipelineSource *cut = builder->createFilter("filters", "Cut",
                                                this->origSrc);
  emit sliceNamed(cut->getSMName());
  pqDataRepresentation *trepr = builder->createDataRepresentation(\
        cut->getOutputPort(0),this->mainView);
  pqPipelineRepresentation *repr = qobject_cast<pqPipelineRepresentation *>(trepr);
  vtkSMProxy *plane = vtkSMPropertyHelper(cut->getProxy(),
                                          "CutFunction").GetAsProxy();

  repr->colorByArray("signal", vtkDataObject::FIELD_ASSOCIATION_CELLS);

  vtkSMPropertyHelper(plane, "Origin").Set(origin, 3);
  vtkSMPropertyHelper(plane, "Normal").Set(orient, 3);
  trepr->getProxy()->UpdateVTKObjects();
}

void MultiSliceView::selectIndicator()
{
  pqServerManagerSelectionModel *smsModel = pqApplicationCore::instance()->getSelectionModel();
  pqPipelineSource *source = qobject_cast<pqPipelineSource *>(smsModel->currentItem());
  QString name = source->getSMName();
  this->ui.xAxisWidget->selectIndicator(name);
  this->ui.yAxisWidget->selectIndicator(name);
  this->ui.zAxisWidget->selectIndicator(name);
}

void MultiSliceView::updateSelectedIndicator()
{
  pqServerManagerSelectionModel *smsModel = pqApplicationCore::instance()->getSelectionModel();
  pqPipelineSource *cut = qobject_cast<pqPipelineSource *>(smsModel->currentItem());
  if (cut->getSMName().contains("Slice"))
  {
    vtkSMProxy *plane = vtkSMPropertyHelper(cut->getProxy(),
                                            "CutFunction").GetAsProxy();
    double origin[3];
    vtkSMPropertyHelper(plane, "Origin").Get(origin, 3);
    if (this->ui.xAxisWidget->hasIndicator())
    {
      this->ui.xAxisWidget->updateIndicator(origin[0]);
    }
    if (this->ui.yAxisWidget->hasIndicator())
    {
      this->ui.yAxisWidget->updateIndicator(origin[1]);
    }
    if (this->ui.zAxisWidget->hasIndicator())
    {
      this->ui.zAxisWidget->updateIndicator(origin[2]);
    }
  }
}

void MultiSliceView::indicatorSelected(const QString &name)
{
  pqServerManagerModel *smModel = pqApplicationCore::instance()->getServerManagerModel();
  pqPipelineSource *cut = smModel->findItem<pqPipelineSource *>(name);
  pqServerManagerSelectionModel *smsModel = pqApplicationCore::instance()->getSelectionModel();
  smsModel->setCurrentItem(cut, pqServerManagerSelectionModel::ClearAndSelect);
}

void MultiSliceView::updateCutPosition(double position)
{
  pqServerManagerSelectionModel *smsModel = pqApplicationCore::instance()->getSelectionModel();
  const pqServerManagerSelection *list = smsModel->selectedItems();
  pqPipelineSource *cut = qobject_cast<pqPipelineSource *>(list->at(0));

  vtkSMProxy *plane = vtkSMPropertyHelper(cut->getProxy(),
                                          "CutFunction").GetAsProxy();
  double origin[3] = {0.0, 0.0, 0.0};
  if (this->ui.xAxisWidget->hasIndicator())
  {
    origin[0] = position;
  }
  if (this->ui.yAxisWidget->hasIndicator())
  {
    origin[1] = position;
  }
  if (this->ui.zAxisWidget->hasIndicator())
  {
    origin[2] = position;
  }
  vtkSMPropertyHelper(plane, "Origin").Set(origin, 3);
  cut->getProxy()->UpdateVTKObjects();
}

void MultiSliceView::deleteCut(const QString &name)
{
  pqServerManagerModel *smModel = pqApplicationCore::instance()->getServerManagerModel();
  pqPipelineSource *cut = smModel->findItem<pqPipelineSource *>(name);
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  builder->destroy(cut);
  this->origRep->setVisible(this->noIndicatorsLeft());
}

void MultiSliceView::cutVisibility(bool isVisible, const QString &name)
{
  pqDisplayPolicy* display_policy = pqApplicationCore::instance()->getDisplayPolicy();
  pqServerManagerModel *smModel = pqApplicationCore::instance()->getServerManagerModel();
  pqPipelineSource *cut = smModel->findItem<pqPipelineSource *>(name);
  pqOutputPort *port = cut->getOutputPort(0);
  display_policy->setRepresentationVisibility(port, this->mainView, isVisible);
  this->renderAll();
}

bool MultiSliceView::noIndicatorsLeft()
{
  int count = 0;
  count += this->ui.xAxisWidget->numIndicators();
  count += this->ui.yAxisWidget->numIndicators();
  count += this->ui.zAxisWidget->numIndicators();
  return count == 0;
}

/**
 * This function is responsible for resetting all of the axis scale information
 * when the rebinner is used. All cuts will be deleted if the axis labeling
 * has changed. If the bounds have been changed, those will be updated. If a
 * cut is outside the new bounds, it will be deleted. Nothing will be done if
 * only the number of bins has been changed.
 */
void MultiSliceView::setAxisScales()
{
  pqPipelineSource *src = this->getPvActiveSrc();
  const char *geomXML = vtkSMPropertyHelper(src->getProxy(),
                                            "InputGeometryXML").GetAsString();
  GeometryParser parser(geomXML);
  AxisInformation *xinfo = parser.getAxisInfo("XDimension");
  AxisInformation *yinfo = parser.getAxisInfo("YDimension");
  AxisInformation *zinfo = parser.getAxisInfo("ZDimension");

  // Check to see if axis mapping has changed and update if necessary.
  bool isXChanged = this->checkTitles(xinfo, this->ui.xAxisWidget);
  bool isYChanged = this->checkTitles(yinfo, this->ui.yAxisWidget);
  bool isZChanged = this->checkTitles(zinfo, this->ui.zAxisWidget);
  bool haveAxesChanged = isXChanged || isYChanged || isZChanged;

  if (haveAxesChanged)
  {
    if (isXChanged)
    {
      this->ui.xAxisWidget->deleteAllIndicators();
      this->ui.xAxisWidget->setInformation(xinfo, true);
    }
    if (isYChanged)
    {
      this->ui.yAxisWidget->deleteAllIndicators();
      this->ui.yAxisWidget->setInformation(yinfo, true);
    }
    if (isZChanged)
    {
      this->ui.zAxisWidget->deleteAllIndicators();
      this->ui.zAxisWidget->setInformation(zinfo, true);
    }
  }

  // Axis mapping not changed, so check if boundaries changed.
  bool xBoundsChanged = this->checkBounds(xinfo, this->ui.xAxisWidget);
  bool yBoundsChanged = this->checkBounds(yinfo, this->ui.yAxisWidget);
  bool zBoundsChanged = this->checkBounds(zinfo, this->ui.zAxisWidget);
  bool haveBoundsChanged = xBoundsChanged || yBoundsChanged || zBoundsChanged;

  if (haveBoundsChanged)
  {
    if (xBoundsChanged)
    {
      this->ui.xAxisWidget->setBounds(xinfo, true);
      this->resetOrDeleteIndicators(this->ui.xAxisWidget, 0);
    }
    if (yBoundsChanged)
    {
      this->ui.yAxisWidget->setBounds(yinfo, true);
      this->resetOrDeleteIndicators(this->ui.yAxisWidget, 1);
    }
    if (zBoundsChanged)
    {
      this->ui.zAxisWidget->setBounds(zinfo, true);
      this->resetOrDeleteIndicators(this->ui.zAxisWidget, 2);
    }
  }

  delete xinfo;
  delete yinfo;
  delete zinfo;
}

/**
 * This function compares the axis title from the requested one to the title of
 * the currently viewed one.
 * @param info the information from the incoming axis
 * @param axis the information from the current axis
 * @return true if the titles are not equal
 */
bool MultiSliceView::checkTitles(AxisInformation *info, AxisInteractor *axis)
{
  return QString(info->getTitle().c_str()) != axis->getTitle();
}

/**
 * This function compares the axis bounds from the requested one to the
 * bounds of the currently viewed one.
 * @param info the information from the incoming axis
 * @param axis the information from the current axis
 * @return true if the bounds are not equal
 */
bool MultiSliceView::checkBounds(AxisInformation *info, AxisInteractor *axis)
{
  bool upperChanged = info->getMaximum() != axis->getMaximum();
  bool lowerChanged = info->getMinimum() != axis->getMinimum();
  return upperChanged || lowerChanged;
}

/**
 * This function handles either resetting or deleting cuts based on a new
 * set of axis boundaries. If the cut is outside the bounds, it is deleted.
 * Those cuts inside the boundaries need to be reset as the scale widget
 * redraws itself for the bounds change as the graphical indicators do not
 * handle this one their own.
 * @param axis the axis containing the information to check
 * @param pos the integer value for either x, y or z
 */
void MultiSliceView::resetOrDeleteIndicators(AxisInteractor *axis, int pos)
{
  pqServerManagerModel *smModel = pqApplicationCore::instance()->getServerManagerModel();
  QList<pqPipelineSource *> cuts = smModel->findItems<pqPipelineSource *>();
  double axis_min = axis->getMinimum();
  double axis_max = axis->getMaximum();
  foreach (pqPipelineSource *cut, cuts)
  {
    const QString name = cut->getSMName();
    if (name.contains("Slice"))
    {
      vtkSMProxy *plane = vtkSMPropertyHelper(cut->getProxy(),
                                              "CutFunction").GetAsProxy();
      double origin[3];
      vtkSMPropertyHelper(plane, "Origin").Get(origin, 3);
      double value = origin[pos];
      if (value >= axis_min && value <= axis_max)
      {
        axis->updateRequestedIndicator(name, value);
      }
      else
      {
        axis->deleteRequestedIndicator(name);
      }
    }
  }
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
    this->ui.xAxisWidget->setShowSliceView(true);
    this->ui.yAxisWidget->setShowSliceView(true);
    this->ui.zAxisWidget->setShowSliceView(true);
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
  coord_t origin[3];
  vtkSMPropertyHelper(plane, "Origin").Get(origin, 3);
  coord_t orient[3];
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
  // Set the slice points, etc, using the XML definition of the plane function
  w->getSlicer()->openFromXML( QString::fromStdString(rks.createXMLString()) );
  w->show();
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
