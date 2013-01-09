#include "MantidVatesSimpleGuiViewWidgets/ThreesliceView.h"

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqChartValue.h>
#include <pqColorMapModel.h>
#include <pqDataRepresentation.h>
#include <pqObjectBuilder.h>
#include <pqPipelineBrowserWidget.h>
#include <pqPipelineRepresentation.h>
#include <pqPipelineSource.h>
#include <pqPluginManager.h>
#include <pqRenderView.h>
#include <pqScalarsToColors.h>
#include <pqServer.h>
#include <pqServerManagerModel.h>
#include <vtkDataObject.h>
#include <vtkProperty.h>
#include <vtkPVPluginsInformation.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>

#include <QMessageBox>

#include <iostream>

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{

ThreeSliceView::ThreeSliceView(QWidget *parent) : ViewBase(parent)
{
  this->ui.setupUi(this);
  // Need to load plugin
  pqPluginManager* pm = pqApplicationCore::instance()->getPluginManager();
  QString error;
  vtkPVPluginsInformation *pvi = pm->loadedExtensions(pqActiveObjects::instance().activeServer(),
                                                      false);
  for(unsigned int i = 0; i < pvi->GetNumberOfPlugins(); ++i)
  {
    std::cout << "D: " << pvi->GetPluginName(i) << std::endl;
  }

  QStringList list = pm->pluginPaths(pqActiveObjects::instance().activeServer(),
                                     false);
  std::cout << "C: " << list.size() << std::endl;
  foreach(QString path, list)
  {
    std::cout << "B: " << path.toStdString() << std::endl;
  }

  pm->loadExtension(pqActiveObjects::instance().activeServer(),
                    "libQuadView.so", &error, false);
  std::cout << "A: " << error.toStdString() << std::endl;

  this->mainView = this->createRenderView(this->ui.mainRenderFrame,
                                          QString("QuadView"));
  pqActiveObjects::instance().setActiveView(this->mainView);
}

ThreeSliceView::~ThreeSliceView()
{
}

void ThreeSliceView::destroyView()
{
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  // Active source disappears in only this view, so set it from the
  // internal source before destroying view.
  pqActiveObjects::instance().setActiveSource(this->origSrc);
  builder->destroy(this->mainView);
}

pqRenderView* ThreeSliceView::getView()
{
  return this->mainView.data();
}

void ThreeSliceView::render()
{
  this->makeThreeSlice();
  this->resetDisplay();
  this->renderAll();
  emit this->triggerAccept();
}

void ThreeSliceView::makeThreeSlice()
{
  pqPipelineSource *src = NULL;
  src = pqActiveObjects::instance().activeSource();

  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();

  // Do not allow overplotting PeaksWorkspaces
  if (this->isPeaksWorkspace(src))
  {
    QMessageBox::warning(this, QApplication::tr("Overplotting Warning"),
                         QApplication::tr("Threeslice mode does not allow "\
                                          "overlay of PeaksWorkspaces"));
    // Need to destroy source since we tried to load it and set the active
    // back to something. In this case we'll choose the original source
    builder->destroy(src);
    pqActiveObjects::instance().setActiveSource(this->origSrc);
    return;
  }

  this->origSrc = src;

  pqDataRepresentation *drep = builder->createDataRepresentation(\
        this->origSrc->getOutputPort(0), this->mainView);
  vtkSMPropertyHelper(drep->getProxy(), "Representation").Set("Surface");
  drep->getProxy()->UpdateVTKObjects();
  this->origRep = qobject_cast<pqPipelineRepresentation*>(drep);
  this->origRep->colorByArray("signal", vtkDataObject::FIELD_ASSOCIATION_CELLS);
  this->origRep->getProxy()->UpdateVTKObjects();
}

void ThreeSliceView::renderAll()
{
  this->mainView->render();
}

void ThreeSliceView::resetDisplay()
{
  this->mainView->resetDisplay();
}

void ThreeSliceView::correctVisibility(pqPipelineBrowserWidget *pbw)
{
  //pqServerManagerSelectionModel *smsModel = pqApplicationCore::instance()->getSelectionModel();
  //smsModel->setCurrentItem(this->xCut, pqServerManagerSelectionModel::ClearAndSelect);
  //smsModel->setCurrentItem(this->yCut, pqServerManagerSelectionModel::Select);
  //smsModel->setCurrentItem(this->zCut, pqServerManagerSelectionModel::Select);
  pbw->setSelectionVisibility(true);
  //smsModel->setCurrentItem(this->xCut, pqServerManagerSelectionModel::Clear);
  //smsModel->setCurrentItem(this->yCut, pqServerManagerSelectionModel::Clear);
  //smsModel->setCurrentItem(this->zCut, pqServerManagerSelectionModel::Clear);
  this->origRep->setVisible(false);
  this->correctColorScaleRange();
}

void ThreeSliceView::correctColorScaleRange()
{
  QPair<double, double> range = this->origRep->getColorFieldRange();
  emit this->dataRange(range.first, range.second);
}

void ThreeSliceView::resetCamera()
{
  this->mainView->resetCamera();
}

}
}
}
