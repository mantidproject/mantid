#include "MantidVatesSimpleGuiViewWidgets/MdViewerWidget.h"

#include "MantidVatesSimpleGuiQtWidgets/ModeControlWidget.h"
#include "MantidVatesSimpleGuiViewWidgets/MultisliceView.h"
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
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxyManager.h>
#include <vtkSMProxy.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMReaderFactory.h>

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
}

MdViewerWidget::MdViewerWidget(QWidget *parent) : VatesViewerInterface(parent)
{
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

void MdViewerWidget::setupUiAndConnections()
{
  this->ui.setupUi(this);
  this->ui.splitter_2->setStretchFactor(1, 1);

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
  //this->originSource = source;

  this->renderAndFinalSetup();
}

void MdViewerWidget::renderWorkspace(QString wsname)
{
  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  if (this->currentView->origSource)
  {
    this->ui.modeControlWidget->setToStandardView();
    builder->destroy(this->currentView->origSource);
  }
  this->currentView->origSource = builder->createSource("sources",
                                                        "MDEW Source",
                                                        pqActiveObjects::instance().activeServer());
  vtkSMPropertyHelper(this->currentView->origSource->getProxy(),
                      "Mantid Workspace Name").Set(wsname.toStdString().c_str());
  this->currentView->origSource->getProxy()->UpdateVTKObjects();

  this->renderAndFinalSetup();
  this->updateTimesteps();
}

void MdViewerWidget::renderAndFinalSetup()
{
  this->currentView->render();
  this->ui.proxyTabWidget->getObjectInspector()->accept();

  const unsigned int val = vtkSMPropertyHelper(\
        this->currentView->origSource->getProxy(),
        "InputGeometryXML", true).GetNumberOfElements();
  if (val > 0)
  {
    emit this->enableMultiSliceViewButton();
  }
  emit this->enableThreeSliceViewButton();
}

void MdViewerWidget::updateTimesteps()
{
  vtkSMSourceProxy *srcProxy1 = vtkSMSourceProxy::SafeDownCast(this->currentView->origSource->getProxy());
  srcProxy1->Modified();
  srcProxy1->UpdatePipelineInformation();
  srcProxy1->UpdatePipeline();
  vtkSMDoubleVectorProperty *tsv = vtkSMDoubleVectorProperty::SafeDownCast(srcProxy1->GetProperty("TimestepValues"));
  if (0 != tsv->GetNumberOfElements())
  {
    double tEnd = tsv->GetElement(tsv->GetNumberOfElements() - 1);
    pqAnimationScene *scene = pqPVApplicationCore::instance()->animationManager()->getActiveScene();
    vtkSMPropertyHelper(scene->getProxy(), "EndTime").Set(tEnd);
  }
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
  if (this->currentView->inherits("ThreeSliceView") ||
      this->currentView->inherits("StandardView"))
  {
    this->ui.proxyTabWidget->getObjectInspector()->accept();
  }
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
