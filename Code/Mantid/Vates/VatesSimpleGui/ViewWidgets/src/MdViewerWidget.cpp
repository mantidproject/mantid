#include "MantidVatesSimpleGuiViewWidgets/MdViewerWidget.h"

#include "MantidVatesSimpleGuiQtWidgets/ModeControlWidget.h"
#include "MantidVatesSimpleGuiViewWidgets/MultisliceView.h"
#include "MantidVatesSimpleGuiViewWidgets/SplatterPlotView.h"
#include "MantidVatesSimpleGuiViewWidgets/StandardView.h"
#include "MantidVatesSimpleGuiViewWidgets/ThreesliceView.h"
#include "MantidVatesSimpleGuiViewWidgets/TimeControlWidget.h"

#include "MantidQtAPI/InterfaceManager.h"
#include "MantidKernel/DynamicFactory.h"

#include <pqActiveObjects.h>
#include <pqAnimationManager.h>
#include <pqAnimationScene.h>
#include <pqApplicationCore.h>
#include <pqLoadDataReaction.h>
#include <pqObjectBuilder.h>
#include <pqObjectInspectorWidget.h>
#include <pqParaViewBehaviors.h>
#include <pqPipelineSource.h>
#include <pqPVApplicationCore.h>
#include <pqRenderView.h>
#include <pqStatusBar.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxyManager.h>
#include <vtkSMProxy.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMReaderFactory.h>
#include <vtksys/SystemTools.hxx>

#include <pqPipelineRepresentation.h>

// Used for plugin mode
#include <pqAlwaysConnectedBehavior.h>
#include <pqAutoLoadPluginXMLBehavior.h>
#include <pqCommandLineOptionsBehavior.h>
#include <pqCrashRecoveryBehavior.h>
#include <pqDataTimeStepBehavior.h>
#include <pqDefaultViewBehavior.h>
#include <pqDeleteBehavior.h>
#include <pqFixPathsInStateFilesBehavior.h>
#include <pqObjectPickingBehavior.h>
//#include <pqPersistentMainWindowStateBehavior.h>
#include <pqPipelineContextMenuBehavior.h>
//#include <pqPluginActionGroupBehavior.h>
//#include <pqPluginDockWidgetsBehavior.h>
#include <pqPluginManager.h>
#include <pqPVNewSourceBehavior.h>
#include <pqQtMessageHandlerBehavior.h>
#include <pqSpreadSheetVisibilityBehavior.h>
#include <pqStandardViewModules.h>
#include <pqUndoRedoBehavior.h>
#include <pqViewFrameActionsBehavior.h>
#include <pqVerifyRequiredPluginBehavior.h>

#include <QHBoxLayout>
#include <QMainWindow>
#include <QModelIndex>
#include <QWidget>

#include <iostream>

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{
using namespace MantidQt::API;

REGISTER_VATESGUI(MdViewerWidget)

MdViewerWidget::MdViewerWidget() : VatesViewerInterface()
{
  this->isPluginInitialized = false;
  this->wsType = VatesViewerInterface::MDEW;
}

MdViewerWidget::MdViewerWidget(QWidget *parent) : VatesViewerInterface(parent)
{
  this->wsType = VatesViewerInterface::MDEW;
  this->checkEnvSetup();
  // We're in the standalone application mode
  this->isPluginInitialized = false;
  this->setupUiAndConnections();
  // FIXME: This doesn't allow a clean split of the classes. I will need
  //        to investigate creating the individual behaviors to see if that
  //        eliminates the dependence on the QMainWindow.
  if (parent->inherits("QMainWindow"))
  {
    QMainWindow *mw = qobject_cast<QMainWindow *>(parent);
    new pqParaViewBehaviors(mw, mw);
  }
  this->setupMainView();
}

MdViewerWidget::~MdViewerWidget()
{
}

void MdViewerWidget::checkEnvSetup()
{
  QString pv_plugin_path = vtksys::SystemTools::GetEnv("PV_PLUGIN_PATH");
  if (pv_plugin_path.isEmpty())
  {
    throw std::runtime_error("PV_PLUGIN_PATH not setup.\nVates plugins will not be available.\n"
                             "Further use will cause the program to crash.\nPlease exit and "
                             "set this variable.");
  }
}

void MdViewerWidget::setupUiAndConnections()
{
  this->ui.setupUi(this);
  this->ui.splitter_2->setStretchFactor(1, 1);
  this->ui.statusBar->setSizeGripEnabled(false);

  // Unset the connections since the views aren't up yet.
  this->removeProxyTabWidgetConnections();

  QObject::connect(this->ui.modeControlWidget,
                   SIGNAL(executeSwitchViews(ModeControlWidget::Views)),
                   this, SLOT(switchViews(ModeControlWidget::Views)));
}

void MdViewerWidget::setupMainView()
{
  // Commented this out to only use Mantid supplied readers
  // Initialize all readers available to ParaView. Now our application can load
  // all types of datasets supported by ParaView.
  //vtkSMProxyManager::GetProxyManager()->GetReaderFactory()->RegisterPrototypes("sources");

  // Set the standard view as the default
  this->currentView = this->setMainViewWidget(this->ui.viewWidget,
                                              ModeControlWidget::STANDARD);

  // Create a layout to manage the view properly
  this->viewLayout = new QHBoxLayout(this->ui.viewWidget);
  this->viewLayout->setMargin(0);
  this->viewLayout->setStretch(0, 1);
  this->viewLayout->addWidget(this->currentView);

  this->setParaViewComponentsForView();
}

void MdViewerWidget::setupPluginMode()
{
  this->createAppCoreForPlugin();
  this->checkEnvSetup();
  this->setupUiAndConnections();
  if (!this->isPluginInitialized)
  {
    this->setupParaViewBehaviors();
  }
  this->setupMainView();
}

void MdViewerWidget::createAppCoreForPlugin()
{
  if (!pqApplicationCore::instance())
  {
    int argc = 1;
    char *argv[] = {"/tmp/MantidPlot"};
    new pqPVApplicationCore(argc, argv);
  }
  else
  {
    this->isPluginInitialized = true;
  }
}

void MdViewerWidget::setupParaViewBehaviors()
{
  // Register ParaView interfaces.
  pqPluginManager* pgm = pqApplicationCore::instance()->getPluginManager();

  // * adds support for standard paraview views.
  pgm->addInterface(new pqStandardViewModules(pgm));

  // Load plugins distributed with application.
  pqApplicationCore::instance()->loadDistributedPlugins();

  // Define application behaviors.
  new pqQtMessageHandlerBehavior(this);
  new pqDataTimeStepBehavior(this);
  new pqViewFrameActionsBehavior(this);
  new pqSpreadSheetVisibilityBehavior(this);
  new pqPipelineContextMenuBehavior(this);
  new pqDefaultViewBehavior(this);
  new pqAlwaysConnectedBehavior(this);
  new pqPVNewSourceBehavior(this);
  new pqDeleteBehavior(this);
  new pqUndoRedoBehavior(this);
  new pqCrashRecoveryBehavior(this);
  new pqAutoLoadPluginXMLBehavior(this);
  //new pqPluginDockWidgetsBehavior(mainWindow);
  new pqVerifyRequiredPluginBehavior(this);
  //new pqPluginActionGroupBehavior(mainWindow);
  new pqFixPathsInStateFilesBehavior(this);
  new pqCommandLineOptionsBehavior(this);
  //new pqPersistentMainWindowStateBehavior(mainWindow);
  new pqObjectPickingBehavior(this);
}

void MdViewerWidget::connectLoadDataReaction(QAction *action)
{
  // We want the actionLoad to result in the showing up the ParaView's OpenData
  // dialog letting the user pick from one of the supported file formats.
  this->dataLoader = new pqLoadDataReaction(action);
  QObject::connect(this->dataLoader, SIGNAL(loadedData(pqPipelineSource*)),
                   this, SLOT(onDataLoaded(pqPipelineSource*)));
}

void MdViewerWidget::removeProxyTabWidgetConnections()
{
  QObject::disconnect(&pqActiveObjects::instance(), 0,
                      this->ui.proxyTabWidget, 0);
}

ViewBase* MdViewerWidget::setMainViewWidget(QWidget *container,
                                            ModeControlWidget::Views v)
{
  ViewBase *view;
  switch(v)
  {
  case ModeControlWidget::STANDARD:
  {
    view = new StandardView(container);
  }
  break;
  case ModeControlWidget::THREESLICE:
  {
    view = new ThreeSliceView(container);
  }
  break;
  case ModeControlWidget::MULTISLICE:
  {
    view = new MultiSliceView(container);
  }
  break;
  case ModeControlWidget::SPLATTERPLOT:
  {
    view = new SplatterPlotView(container);
  }
  break;
  default:
    view = NULL;
    break;
  }
  return view;
}

void MdViewerWidget::setParaViewComponentsForView()
{
  // Extra setup stuff to hook up view to other items
  this->ui.proxyTabWidget->setupDefaultConnections();
  this->ui.proxyTabWidget->setView(this->currentView->getView());
  this->ui.proxyTabWidget->setShowOnAccept(true);
  this->ui.pipelineBrowser->setActiveView(this->currentView->getView());
  QObject::connect(this->ui.proxyTabWidget->getObjectInspector(),
                   SIGNAL(postaccept()),
                   this, SLOT(checkForUpdates()));
  QObject::connect(this->currentView, SIGNAL(triggerAccept()),
                   this->ui.proxyTabWidget->getObjectInspector(),
                   SLOT(accept()));
  if (this->currentView->inherits("MultiSliceView"))
  {
    QObject::connect(this->ui.pipelineBrowser,
                     SIGNAL(clicked(const QModelIndex &)),
                     static_cast<MultiSliceView *>(this->currentView),
                     SLOT(selectIndicator()));
    QObject::connect(this->ui.proxyTabWidget->getObjectInspector(),
                     SIGNAL(accepted()),
                     static_cast<MultiSliceView *>(this->currentView),
                     SLOT(updateSelectedIndicator()));
  }
  if (this->currentView->inherits("StandardView"))
  {
    QObject::connect(static_cast<StandardView *>(this->currentView),
                     SIGNAL(enableMultiSliceViewButton()),
                     this, SIGNAL(enableMultiSliceViewButton()));
  }
  QObject::connect(this->ui.colorSelectionWidget,
                   SIGNAL(colorMapChanged(const pqColorMapModel *)),
                   this->currentView,
                   SLOT(onColorMapChange(const pqColorMapModel *)));
  QObject::connect(this->ui.colorSelectionWidget,
                   SIGNAL(colorScaleChanged(double, double)),
                   this->currentView,
                   SLOT(onColorScaleChange(double, double)));
  QObject::connect(this->currentView, SIGNAL(dataRange(double, double)),
                   this->ui.colorSelectionWidget,
                   SLOT(setColorScaleRange(double, double)));
  QObject::connect(this->ui.colorSelectionWidget, SIGNAL(autoScale()),
                   this->currentView, SLOT(onAutoScale()));
  QObject::connect(this->ui.colorSelectionWidget, SIGNAL(logScale(int)),
                   this->currentView, SLOT(onLogScale(int)));
}

void MdViewerWidget::onDataLoaded(pqPipelineSource* source)
{
  UNUSED_ARG(source);
  if (this->currentView->origSource)
  {
    pqApplicationCore::instance()->getObjectBuilder()->destroy(this->currentView->origSource);
  }
  if (QString("PeaksReader") == source->getProxy()->GetXMLName())
  {
    this->wsType = VatesViewerInterface::PEAKS;
  }
  else
  {
    this->wsType = VatesViewerInterface::MDEW;
  }

  this->renderAndFinalSetup();
  this->checkForTimesteps();
}

void MdViewerWidget::checkForTimesteps()
{
  vtkSMSourceProxy *srcProxy1 = vtkSMSourceProxy::SafeDownCast(this->currentView->origSource->getProxy());
  vtkSMDoubleVectorProperty *tsv = vtkSMDoubleVectorProperty::SafeDownCast(srcProxy1->GetProperty("TimestepValues"));
  if (NULL != tsv && 0 != tsv->GetNumberOfElements())
  {
    this->ui.timeControlWidget->setEnabled(true);
  }
  else
  {
    this->ui.timeControlWidget->setEnabled(false);
  }
}

void MdViewerWidget::renderWorkspace(QString wsname, int wstype)
{
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  if (this->currentView->origSource)
  {
    this->ui.modeControlWidget->setToStandardView();
    builder->destroySources();
  }
  QString sourcePlugin = "";
  if (VatesViewerInterface::PEAKS == wstype)
  {
    this->wsType = VatesViewerInterface::PEAKS;
    sourcePlugin = "Peaks Source";
  }
  else
  {
    this->wsType = VatesViewerInterface::MDEW;
    sourcePlugin = "MDEW Source";
  }

  this->currentView->origSource = builder->createSource("sources", sourcePlugin,
                                                        pqActiveObjects::instance().activeServer());
  vtkSMPropertyHelper(this->currentView->origSource->getProxy(),
                      "Mantid Workspace Name").Set(wsname.toStdString().c_str());
  this->currentView->origSource->getProxy()->UpdateVTKObjects();

  this->renderAndFinalSetup();
  this->setTimesteps();
}

void MdViewerWidget::renderAndFinalSetup()
{
  this->currentView->render();
  this->currentView->onAutoScale();
  this->ui.proxyTabWidget->getObjectInspector()->accept();

  if (VatesViewerInterface::MDEW == this->wsType)
  {
    const unsigned int val = vtkSMPropertyHelper(\
                               this->currentView->origSource->getProxy(),
                               "InputGeometryXML", true).GetNumberOfElements();
    if (val > 0)
    {
      emit this->enableMultiSliceViewButton();
    }
    emit this->enableThreeSliceViewButton();
    emit this->enableSplatterPlotViewButton();
  }
}

void MdViewerWidget::checkForUpdates()
{
  vtkSMProxy *proxy = pqActiveObjects::instance().activeSource()->getProxy();
  if (strcmp(proxy->GetXMLName(), "MDEWRebinningCutter") == 0)
  {
    this->currentView->resetDisplay();
    this->currentView->onAutoScale();
    this->updateTimesteps();
  }
}

void MdViewerWidget::updateAnimationControls(vtkSMDoubleVectorProperty *dvp)
{
  const int numTimesteps = static_cast<int>(dvp->GetNumberOfElements());
  if (0 != numTimesteps)
  {
    double tStart = dvp->GetElement(0);
    double tEnd = dvp->GetElement(dvp->GetNumberOfElements() - 1);
    pqAnimationScene *scene = pqPVApplicationCore::instance()->animationManager()->getActiveScene();
    vtkSMPropertyHelper(scene->getProxy(), "StartTime").Set(tStart);
    vtkSMPropertyHelper(scene->getProxy(), "EndTime").Set(tEnd);
    vtkSMPropertyHelper(scene->getProxy(), "NumberOfFrames").Set(numTimesteps);
    this->ui.timeControlWidget->setEnabled(true);
  }
  else
  {
    this->ui.timeControlWidget->setEnabled(false);
  }
}

void MdViewerWidget::setTimesteps()
{
  if (VatesViewerInterface::PEAKS == this->wsType)
  {
    return;
  }
  vtkSMSourceProxy *srcProxy1 = vtkSMSourceProxy::SafeDownCast(this->currentView->origSource->getProxy());
  srcProxy1->Modified();
  srcProxy1->UpdatePipelineInformation();
  //srcProxy1->UpdatePipeline();
  vtkSMDoubleVectorProperty *tsv = vtkSMDoubleVectorProperty::SafeDownCast(srcProxy1->GetProperty("TimestepValues"));
  this->updateAnimationControls(tsv);
}

void MdViewerWidget::updateTimesteps()
{
  vtkSMSourceProxy *rbcProxy = vtkSMSourceProxy::SafeDownCast(pqActiveObjects::instance().activeSource()->getProxy());
  rbcProxy->Modified();
  rbcProxy->UpdatePipelineInformation();
  vtkSMDoubleVectorProperty *tsv = vtkSMDoubleVectorProperty::SafeDownCast(rbcProxy->GetProperty("TimestepValues"));
  const int numTimesteps = static_cast<int>(tsv->GetNumberOfElements());
  vtkSMSourceProxy *srcProxy = vtkSMSourceProxy::SafeDownCast(this->currentView->origSource->getProxy());
  vtkSMPropertyHelper(srcProxy, "TimestepValues").Set(tsv->GetElements(),
                                                      numTimesteps);
  this->updateAnimationControls(tsv);
}

void MdViewerWidget::switchViews(ModeControlWidget::Views v)
{
  this->removeProxyTabWidgetConnections();
  this->hiddenView = this->setMainViewWidget(this->ui.viewWidget, v);
  this->hiddenView->hide();
  this->viewLayout->removeWidget(this->currentView);
  this->swapViews();
  this->viewLayout->addWidget(this->currentView);
  this->currentView->show();
  this->hiddenView->hide();
  this->setParaViewComponentsForView();
  this->hiddenView->close();
  this->hiddenView->destroyView();
  delete this->hiddenView;
  this->currentView->render();
  if (this->currentView->inherits("ThreeSliceView"))
  {
    static_cast<ThreeSliceView *>(this->currentView)->correctVisibility(this->ui.pipelineBrowser);
  }
}

void MdViewerWidget::swapViews()
{
  ViewBase *temp;
  temp = this->currentView;
  this->currentView = this->hiddenView;
  this->hiddenView = temp;
}

}
}
}
