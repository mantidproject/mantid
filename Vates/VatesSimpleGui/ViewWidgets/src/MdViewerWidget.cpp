#include <Poco/File.h>
#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/UsageService.h"
#include "MantidQtAPI/InterfaceManager.h"
#include "MantidQtAPI/MantidDesktopServices.h"
#include "MantidQtAPI/MdConstants.h"
#include "MantidQtAPI/MdSettings.h"
#include "MantidQtAPI/TSVSerialiser.h"
#include "MantidVatesSimpleGuiQtWidgets/ModeControlWidget.h"
#include "MantidVatesSimpleGuiQtWidgets/RotationPointDialog.h"
#include "MantidVatesSimpleGuiViewWidgets/ColorMapEditorPanel.h"
#include "MantidVatesSimpleGuiViewWidgets/ColorSelectionWidget.h"
#include "MantidVatesSimpleGuiViewWidgets/MdViewerWidget.h"
#include "MantidVatesSimpleGuiViewWidgets/MultisliceView.h"
#include "MantidVatesSimpleGuiViewWidgets/SaveScreenshotReaction.h"
#include "MantidVatesSimpleGuiViewWidgets/SplatterPlotView.h"
#include "MantidVatesSimpleGuiViewWidgets/StandardView.h"
#include "MantidVatesSimpleGuiViewWidgets/ThreesliceView.h"
#include "MantidVatesSimpleGuiViewWidgets/TimeControlWidget.h"
#include "MantidVatesSimpleGuiViewWidgets/VatesParaViewApplication.h"
#include "MantidVatesSimpleGuiViewWidgets/VsiApplyBehaviour.h"

#include "boost/shared_ptr.hpp"
#include "boost/scoped_ptr.hpp"

// Have to deal with ParaView warnings and Intel compiler the hard way.
#if defined(__INTEL_COMPILER)
#pragma warning disable 1170
#endif
#include <pqApplicationCore.h>
#include <pqActiveObjects.h>
#include <pqAnimationManager.h>
#include <pqAnimationScene.h>
#include <pqApplicationCore.h>
#include <pqApplicationSettingsReaction.h>
#include <pqApplyBehavior.h>
#include <pqDeleteReaction.h>
#include <pqLoadDataReaction.h>
#include <pqObjectBuilder.h>
#include <pqParaViewBehaviors.h>
#include <pqPipelineSource.h>
#include <pqPipelineFilter.h>
#include <pqPVApplicationCore.h>
#include <pqRenderView.h>
#include <pqSettings.h>
#include <pqServer.h>
#include <pqServerManagerModel.h>
#include <pqStatusBar.h>
#include <vtkCamera.h>
#include <vtkMathTextUtilities.h>
#include <vtkPVOrthographicSliceView.h>
#include <vtkPVXMLElement.h>
#include <vtkPVXMLParser.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxyManager.h>
#include <vtkSMProxy.h>
#include <vtkSMReaderFactory.h>
#include <vtkSMRenderViewProxy.h>
#include <vtkSMSessionProxyManager.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMViewProxy.h>
#include <vtksys/SystemTools.hxx>
#include <pqPipelineRepresentation.h>

// Used for plugin mode
#include <pqAlwaysConnectedBehavior.h>
#include <pqAutoLoadPluginXMLBehavior.h>
#include <pqCollaborationBehavior.h>
#include <pqCommandLineOptionsBehavior.h>
#include <pqCrashRecoveryBehavior.h>
#include <pqDataTimeStepBehavior.h>
#include <pqDataRepresentation.h>
#include <pqDefaultViewBehavior.h>
#include <pqFixPathsInStateFilesBehavior.h>
#include <pqInterfaceTracker.h>
#include <pqObjectPickingBehavior.h>
//#include <pqPersistentMainWindowStateBehavior.h>
#include <pqPipelineContextMenuBehavior.h>
#include <pqPipelineSource.h>
//#include <pqPluginActionGroupBehavior.h>
//#include <pqPluginDockWidgetsBehavior.h>
#include <pqPluginManager.h>
#include <pqPluginSettingsBehavior.h>
#include <pqQtMessageHandlerBehavior.h>
#include <pqServer.h>
#include <pqServerManagerModel.h>
#include <pqSpreadSheetVisibilityBehavior.h>
#include <pqStandardPropertyWidgetInterface.h>
#include <pqStandardViewFrameActionsImplementation.h>
#include <pqUndoRedoBehavior.h>
#include <pqView.h>
//#include <pqViewFrameActionsBehavior.h>
#include <pqViewStreamingBehavior.h>
#include <pqVerifyRequiredPluginBehavior.h>
#include <pqSaveDataReaction.h>
#if defined(__INTEL_COMPILER)
#pragma warning enable 1170
#endif

#include <QAction>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QMenuBar>
#include <QModelIndex>
#include <QUrl>
#include <QWidget>
#include <QMessageBox>
#include <QRect>

namespace Mantid {
namespace Vates {
namespace SimpleGui {
using namespace Mantid::API;
using namespace MantidQt::API;

namespace {
Mantid::Kernel::Logger g_log("MdViewerWidget");
}

REGISTER_VATESGUI(MdViewerWidget)

MdViewerWidget::AllVSIViewsState::AllVSIViewsState() { initialize(); }

/**
 * Initializes the views states with new empty values. This can be
 * used to contruct or to re-initialize (forget) the states.
 */
void MdViewerWidget::AllVSIViewsState::initialize() {
  // these will be assigned from vtkSMProxy::SaveXMLState which
  // allocates a new tree with vtkPVXMLElement::New();
  stateStandard = vtkSmartPointer<vtkPVXMLElement>::New();
  stateMulti = vtkSmartPointer<vtkPVXMLElement>::New();
  stateThreeSlice = vtkSmartPointer<vtkPVXMLElement>::New();
  stateSplatter = vtkSmartPointer<vtkPVXMLElement>::New();
}

MdViewerWidget::AllVSIViewsState::~AllVSIViewsState() {}

/**
 * This constructor is used in the plugin mode operation of the VSI.
 */
MdViewerWidget::MdViewerWidget()
    : VatesViewerInterface(), currentView(nullptr),

      hiddenView(nullptr), viewSwitched(false), dataLoader(nullptr),
      lodAction(nullptr), screenShot(nullptr), viewLayout(nullptr),
      viewSettings(nullptr), useCurrentColorSettings(false),
      initialView(ModeControlWidget::STANDARD),
      m_rebinAlgorithmDialogProvider(this),
      m_rebinnedWorkspaceIdentifier("_tempvsi"), m_colorMapEditorPanel(nullptr),
      m_gridAxesStartUpOn(true), m_allViews() {
  // this will initialize the ParaView application if needed.
  VatesParaViewApplication::instance();

  // Calling workspace observer functions.
  observeAfterReplace();
  observePreDelete();
  observeADSClear();

  this->internalSetup(true);

  setAcceptDrops(true);
  // Connect the rebinned sources manager
  QObject::connect(&m_rebinnedSourcesManager,
                   SIGNAL(switchSources(std::string, std::string)), this,
                   SLOT(onSwitchSources(std::string, std::string)));
}

/**
 * This constructor is used in the standalone mode operation of the VSI.
 * @param parent the parent widget for the main window
 */
MdViewerWidget::MdViewerWidget(QWidget *parent)
    : VatesViewerInterface(parent), m_rebinAlgorithmDialogProvider(this) {

  // this will initialize the ParaView application if needed.
  VatesParaViewApplication::instance();

  // We're in the standalone application mode
  this->internalSetup(false);
  this->setupUiAndConnections();
  this->setupMainView(ModeControlWidget::STANDARD);
}

MdViewerWidget::~MdViewerWidget() {}

/**
 * This function consolidates setting up some of the internal members between
 * the standalone and plugin modes.
 * @param pMode flag to set the plugin mode
 */
void MdViewerWidget::internalSetup(bool pMode) {
  static int widgetNumber = 0;
  this->m_widgetName = QString("MdViewerWidget%1").arg(widgetNumber++);
  this->pluginMode = pMode;
  this->rotPointDialog = nullptr;
  this->lodThreshold = 5.0;
  this->viewSwitched = false;
}

/**
 * This function sets up the UI components and connects some of the main
 * window's control buttons.
 */
void MdViewerWidget::setupUiAndConnections() {
  this->ui.setupUi(this);
  this->ui.splitter_2->setStretchFactor(1, 1);
  this->ui.splitter_3->setStretchFactor(0, 1);
  this->ui.statusBar->setSizeGripEnabled(false);

  QObject::connect(this->ui.modeControlWidget,
                   SIGNAL(executeSwitchViews(ModeControlWidget::Views)), this,
                   SLOT(switchViews(ModeControlWidget::Views)));

  // Setup rotation point button
  QObject::connect(this->ui.resetViewsStateToAllData, SIGNAL(released()), this,
                   SLOT(onResetViewsStateToAllData()));

  // Setup rotation point button
  QObject::connect(this->ui.resetCenterToPointButton, SIGNAL(clicked()), this,
                   SLOT(onRotationPoint()));

  /// Provide access to the color-editor panel for the application.
  if (!m_colorMapEditorPanel) {
    m_colorMapEditorPanel = new ColorMapEditorPanel(this);
    m_colorMapEditorPanel->setUpPanel();
  }

  QAction *temp = new QAction(this);
  pqDeleteReaction *deleteHandler = new pqDeleteReaction(temp);
  deleteHandler->connect(this->ui.propertiesPanel,
                         SIGNAL(deleteRequested(pqPipelineSource *)),
                         SLOT(deleteSource(pqPipelineSource *)));

  VsiApplyBehaviour *applyBehavior =
      new VsiApplyBehaviour(&m_colorScaleLock, this);

  applyBehavior->registerPanel(this->ui.propertiesPanel);
  VatesParaViewApplication::instance()->setupParaViewBehaviors();
  g_log.warning("Annotation Name: " + m_widgetName.toStdString());

  // Connect the rebinned sources manager
  QObject::connect(&m_rebinnedSourcesManager,
                   SIGNAL(triggerAcceptForNewFilters()),
                   this->ui.propertiesPanel, SLOT(apply()));

  // Add the color scale lock to the ColoSelectionWidget, which should
  // now be initialized
  ui.colorSelectionWidget->setColorScaleLock(&m_colorScaleLock);
}

void MdViewerWidget::panelChanged() { this->currentView->renderAll(); }

/**
 * This function places the standard view to the main window, installs an
 * event filter, tweaks the UI layout for the view and calls the routine that
 * sets up connections between ParaView and the main window widgets.
 */
void MdViewerWidget::setupMainView(ModeControlWidget::Views viewType) {
  // Commented this out to only use Mantid supplied readers
  // Initialize all readers available to ParaView. Now our application can load
  // all types of datasets supported by ParaView.
  // vtkSMProxyManager::GetProxyManager()->GetReaderFactory()->RegisterPrototypes("sources");

  // Set the view at startup to view, the view will be changed, depending on
  // the workspace
  this->currentView =
      this->createAndSetMainViewWidget(this->ui.viewWidget, viewType);
  this->initialView = viewType;
  this->currentView->installEventFilter(this);

  // Create a layout to manage the view properly
  this->viewLayout = new QHBoxLayout(this->ui.viewWidget);
  this->viewLayout->setMargin(0);
  this->viewLayout->setStretch(0, 1);
  this->viewLayout->addWidget(this->currentView);

  this->setParaViewComponentsForView();
}

/**
 * This function connects ParaView's data loader the given action.
 * @param action the action to connect data loading to
 */
void MdViewerWidget::connectLoadDataReaction(QAction *action) {
  // We want the actionLoad to result in the showing up the ParaView's OpenData
  // dialog letting the user pick from one of the supported file formats.
  this->dataLoader = new pqLoadDataReaction(action);
  QObject::connect(this->dataLoader, SIGNAL(loadedData(pqPipelineSource *)),
                   this, SLOT(onDataLoaded(pqPipelineSource *)));
}

/**
 * This function creates the requested view on the main window. It also
 * registers a usage of the view with the UsageService.
 * @param container the UI widget to associate the view mode with
 * @param v the view mode to set on the main window
 * @param createRenderProxy :: whether to create a default render proxy for
 * the view
 * @return the requested view
 */
ViewBase *MdViewerWidget::createAndSetMainViewWidget(QWidget *container,
                                                     ModeControlWidget::Views v,
                                                     bool createRenderProxy) {
  ViewBase *view;
  std::string featureName("VSI:");
  switch (v) {
  case ModeControlWidget::STANDARD: {
    view = new StandardView(container, &m_rebinnedSourcesManager,
                            createRenderProxy);
    featureName += "StandardView";
  } break;
  case ModeControlWidget::THREESLICE: {
    view = new ThreeSliceView(container, &m_rebinnedSourcesManager,
                              createRenderProxy);
    featureName += "ThreeSliceView";
  } break;
  case ModeControlWidget::MULTISLICE: {
    view = new MultiSliceView(container, &m_rebinnedSourcesManager,
                              createRenderProxy);
    featureName += "MultiSliceView";
  } break;
  case ModeControlWidget::SPLATTERPLOT: {
    view = new SplatterPlotView(container, &m_rebinnedSourcesManager,
                                createRenderProxy);
    featureName += "SplatterPlotView";
  } break;
  default:
    view = nullptr;
    break;
  }

  // Set the colorscale lock
  view->setColorScaleLock(&m_colorScaleLock);

  using Mantid::Kernel::UsageService;
  UsageService::Instance().registerFeatureUsage("Interface", featureName,
                                                false);
  return view;
}

/**
 * This function is responsible for setting up all the connections between
 * ParaView's pqPipelineBrowser and pqProxyTabWidget and cetatin main window
 * widgets.
 */
void MdViewerWidget::setParaViewComponentsForView() {
  // Extra setup stuff to hook up view to other items
  // this->ui.propertiesPanel->setView(this->currentView->getView());
  this->ui.pipelineBrowser->setActiveView(this->currentView->getView());

  pqActiveObjects *activeObjects = &pqActiveObjects::instance();
  QObject::connect(activeObjects, SIGNAL(portChanged(pqOutputPort *)),
                   this->ui.propertiesPanel,
                   SLOT(setOutputPort(pqOutputPort *)));

  QObject::connect(activeObjects,
                   SIGNAL(representationChanged(pqDataRepresentation *)),
                   this->ui.propertiesPanel,
                   SLOT(setRepresentation(pqDataRepresentation *)));

  QObject::connect(activeObjects, SIGNAL(viewChanged(pqView *)),
                   this->ui.propertiesPanel, SLOT(setView(pqView *)));

  QObject::connect(this->currentView, SIGNAL(triggerAccept()),
                   this->ui.propertiesPanel, SLOT(apply()));
  QObject::connect(this->ui.propertiesPanel, SIGNAL(applied()), this,
                   SLOT(checkForUpdates()));

  QObject::connect(this->currentView, SIGNAL(renderingDone()), this,
                   SLOT(renderingDone()));

  SplatterPlotView *spv = dynamic_cast<SplatterPlotView *>(this->currentView);
  if (spv) {
    QObject::connect(this->ui.propertiesPanel, SIGNAL(applied()), spv,
                     SLOT(checkPeaksCoordinates()));
    QObject::connect(spv, SIGNAL(toggleOrthographicProjection(bool)),
                     this->ui.parallelProjButton, SLOT(setChecked(bool)));
    QObject::connect(spv, SIGNAL(resetToStandardView()),
                     this->ui.modeControlWidget, SLOT(setToStandardView()));
  }

  QObject::connect(this->currentView,
                   SIGNAL(setViewsStatus(ModeControlWidget::Views, bool)),
                   this->ui.modeControlWidget,
                   SLOT(enableViewButtons(ModeControlWidget::Views, bool)));
  // note the diff: Buttons <> Button
  QObject::connect(this->currentView,
                   SIGNAL(setViewStatus(ModeControlWidget::Views, bool)),
                   this->ui.modeControlWidget,
                   SLOT(enableViewButton(ModeControlWidget::Views, bool)));

  this->connectColorSelectionWidget();

  // Connect the reset view state button, which is between the color selection
  // widget and the ParaQ toolbars
  QObject::connect(this->ui.resetViewsStateToAllData, SIGNAL(released()), this,
                   SLOT(onResetViewsStateToAllData()));

  // Set animation (time) control widget <-> view signals/slots.
  QObject::connect(this->currentView, SIGNAL(setAnimationControlState(bool)),
                   this->ui.timeControlWidget,
                   SLOT(enableAnimationControls(bool)));
  QObject::connect(this->currentView,
                   SIGNAL(setAnimationControlInfo(double, double, int)),
                   this->ui.timeControlWidget,
                   SLOT(updateAnimationControls(double, double, int)));

  // Set the connection for the parallel projection button
  QObject::connect(this->ui.parallelProjButton, SIGNAL(toggled(bool)),
                   this->currentView, SLOT(onParallelProjection(bool)));

  // Start listening to a rebinning event
  QObject::connect(this->currentView, SIGNAL(rebin(std::string)), this,
                   SLOT(onRebin(std::string)), Qt::UniqueConnection);

  // Start listening to an unbinning event
  QObject::connect(this->currentView, SIGNAL(unbin()), this, SLOT(onUnbin()),
                   Qt::UniqueConnection);
}

/**
 * Reaction for a rebin event
 * @param algorithmType The type of rebinning algorithm
 */
void MdViewerWidget::onRebin(const std::string &algorithmType) {
  pqPipelineSource *source = pqActiveObjects::instance().activeSource();

  std::string inputWorkspaceName;
  std::string outputWorkspaceName;
  m_rebinnedSourcesManager.checkSource(source, inputWorkspaceName,
                                       outputWorkspaceName, algorithmType);
  m_rebinAlgorithmDialogProvider.showDialog(inputWorkspaceName,
                                            outputWorkspaceName, algorithmType);
}

/**
 * Switch a source.
 * @param rebinnedWorkspaceName The name of the rebinned  workspace.
 * @param sourceType The type of the source.
 */
void MdViewerWidget::onSwitchSources(std::string rebinnedWorkspaceName,
                                     std::string sourceType) {
  // Create the rebinned workspace
  pqPipelineSource *rebinnedSource = prepareRebinnedWorkspace(
      std::move(rebinnedWorkspaceName), std::move(sourceType));

  try {
    // Repipe the filters to the rebinned source
    m_rebinnedSourcesManager.repipeRebinnedSource();

    // Update the animation controls in order to get the correct time slice
    this->currentView->updateAnimationControls();

    // Update the color scale
    this->currentView->onAutoScale(this->ui.colorSelectionWidget);

    // Set the splatterplot button explicitly
    this->currentView->setSplatterplot(true);

    pqActiveObjects::instance().setActiveSource(nullptr);
    pqActiveObjects::instance().setActiveSource(rebinnedSource);
  } catch (const std::runtime_error &error) {
    g_log.warning() << error.what();
  }
}

/**
 * This gives the user a simple way to reset the state and forget all
 * the interactions to start anew. Resets to all data and resets the
 * center point and direction/rotation/angle.
 */
void MdViewerWidget::onResetViewsStateToAllData() {
  // forget alll the view saved states
  m_allViews.initialize();

  if (!this->currentView)
    return;

  // reset direction/rotation
  pqRenderView *pqv = this->currentView->getView();
  if (!pqv) {
    g_log.warning() << "Serious inconsistency found: could not retrieve a "
                       "pqRenderView while "
                       "trying to reset the state of the views.";
    return;
  }
  pqv->resetViewDirection(0, 0, -1, 0, 0, 0);

  // reset current view
  this->currentView->resetDisplay(); // includes a resetCamera() or more
  this->currentView->render();
}

/**
 * Creates and renders a rebinned workspace source
 * @param rebinnedWorkspaceName The name of the rebinned workspace.
 * @param sourceType The name of the source plugin.
 */
pqPipelineSource *MdViewerWidget::prepareRebinnedWorkspace(
    const std::string &rebinnedWorkspaceName, const std::string &sourceType) {
  // Load a new source plugin
  auto gridAxesOn = areGridAxesOn();
  pqPipelineSource *newRebinnedSource = this->currentView->setPluginSource(
      QString::fromStdString(sourceType),
      QString::fromStdString(rebinnedWorkspaceName), gridAxesOn);

  // It seems that the new source gets set as active before it is fully
  // constructed. We therefore reset it.
  pqActiveObjects::instance().setActiveSource(nullptr);
  pqActiveObjects::instance().setActiveSource(newRebinnedSource);

  this->renderAndFinalSetup();

  this->currentView->onAutoScale(this->ui.colorSelectionWidget);

  // Register the source
  m_rebinnedSourcesManager.registerRebinnedSource(newRebinnedSource);

  return newRebinnedSource;
}

/**
 * Creates and renders back to the original source
 * @param originalWorkspaceName The name of the original workspace
 */
pqPipelineSource *MdViewerWidget::renderOriginalWorkspace(
    const std::string &originalWorkspaceName) {
  // Load a new source plugin
  QString sourcePlugin = "MDEW Source";
  auto gridAxesOn = areGridAxesOn();
  pqPipelineSource *source = this->currentView->setPluginSource(
      sourcePlugin, QString::fromStdString(originalWorkspaceName), gridAxesOn);

  // Render and final setup
  this->renderAndFinalSetup();

  return source;
}

/**
 * Gets triggered by an unbin event. It removes the rebinning on a workspace
 * which has been rebinned from within the VSI.
 */
void MdViewerWidget::onUnbin() {
  // Force the removal of the rebinning
  pqPipelineSource *activeSource = pqActiveObjects::instance().activeSource();

  removeRebinning(activeSource, true);
}

/**
 * Remove the rebinning.
 * @param source The pipeline source for which the rebinning will be removed.
 * @param forced If it should be removed under all circumstances.
 * @param view If switched, to which view is it being switched
 */
void MdViewerWidget::removeRebinning(pqPipelineSource *source, bool forced,
                                     ModeControlWidget::Views view) {
  if (forced || view == ModeControlWidget::SPLATTERPLOT) {
    std::string originalWorkspaceName;
    std::string rebinnedWorkspaceName;
    m_rebinnedSourcesManager.getStoredWorkspaceNames(
        source, originalWorkspaceName, rebinnedWorkspaceName);

    // If the active source has not been rebinned, then send a reminder to the
    // user that only rebinned sources
    // can be unbinned
    if (originalWorkspaceName.empty() || rebinnedWorkspaceName.empty()) {
      if (forced == true) {
        QMessageBox::warning(
            this, QApplication::tr("Unbin Warning"),
            QApplication::tr(
                "You cannot unbin a source which has not be rebinned. \n"
                "To unbin, select a rebinned source and \n"
                "press Remove Rebinning again"));
      }
      return;
    }

    // We need to check that the rebinned workspace name has still a source
    // associated to it
    if (!m_rebinnedSourcesManager.isRebinnedSourceBeingTracked(source)) {
      return;
    }

    // Create the original source
    pqPipelineSource *originalSource =
        renderOriginalWorkspace(originalWorkspaceName);

    // Repipe the filters to the original source
    try {
      m_rebinnedSourcesManager.repipeOriginalSource(source, originalSource);
    } catch (const std::runtime_error &error) {
      g_log.warning() << error.what();
    }

    // Render and final setup
    pqActiveObjects::instance().activeView()->forceRender();

    // Set the buttons correctly if we switch to splatterplot
    if (view == ModeControlWidget::SPLATTERPLOT) {
      this->currentView->setSplatterplot(false);
      this->currentView->setStandard(true);
    }
  }
}

/**
 * Remove rebinning from all rebinned sources
 * @param view The view mode.
 */
void MdViewerWidget::removeAllRebinning(ModeControlWidget::Views view) {
  // Iterate over all rebinned sources and remove them
  pqServer *server = pqActiveObjects::instance().activeServer();
  pqServerManagerModel *smModel =
      pqApplicationCore::instance()->getServerManagerModel();
  const QList<pqPipelineSource *> sources =
      smModel->findItems<pqPipelineSource *>(server);

  // We need to record all true sources, The filters will be removed in the
  // removeRebinning step
  // Hence the iterator will not point to a valid object anymore.
  QList<pqPipelineSource *> sourcesToAlter;
  foreach (pqPipelineSource *source, sources) {
    const QString srcProxyName = source->getProxy()->GetXMLGroup();
    if (srcProxyName == QString("sources")) {
      sourcesToAlter.push_back(source);
    }
  }

  foreach (pqPipelineSource *source, sourcesToAlter) {
    removeRebinning(source, false, view);
  }
}

/**
 * This function loads and renders data from the given source for the
 * standalone mode.
 * @param source a ParaView compatible source
 */
void MdViewerWidget::onDataLoaded(pqPipelineSource *source) {
  source->updatePipeline();
  this->renderAndFinalSetup();
}

/**
 * This function is responsible for carrying out actions when ParaView
 * says the rendering is completed. It currently handles making sure the
 * color selection widget state is passed between views.
 */
void MdViewerWidget::renderingDone() {
  if (this->viewSwitched) {
    Mantid::VATES::ColorScaleLockGuard colorScaleLockGuard(&m_colorScaleLock);
    this->setColorMap(); // Load the default color map
    this->currentView->setColorsForView(this->ui.colorSelectionWidget);
    this->viewSwitched = false;
  }
}

/**
 * This function determines the type of source plugin and sets the workspace
 * name so that the data can be retrieved and rendered.
 * @param workspaceName The workspace name for the data.
 * @param workspaceType A numeric indicator of the workspace type.
 * @param instrumentName The name of the instrument which measured the workspace
 * data.
 */
void MdViewerWidget::renderWorkspace(QString workspaceName, int workspaceType,
                                     std::string instrumentName) {
  Mantid::VATES::ColorScaleLockGuard colorScaleLockGuard(&m_colorScaleLock);
  // Workaround: Note that setting to the standard view was part of the
  // eventFilter. This causes the
  //             VSI window to not close properly. Moving it here ensures that
  //             we have the switch, but
  //             after the window is started again.
  if (this->currentView->getNumSources() == 0) {
    this->setColorForBackground();
    this->setColorMap();

    if (VatesViewerInterface::PEAKS != workspaceType) {
      resetCurrentView(workspaceType, instrumentName);
    }

    this->currentView->hide();
    // Set the auto log scale state
    this->currentView->initializeColorScale();

    // Set the grid axs to on. This should be set whenever we have 0 sources in
    // the view.
    m_gridAxesStartUpOn = true;
  }

  // Set usage of current color settings to true, since we have loade the VSI
  if (!this->useCurrentColorSettings) {
    this->useCurrentColorSettings = true;
  }

  QString sourcePlugin = "";
  if (VatesViewerInterface::PEAKS == workspaceType) {
    sourcePlugin = "Peaks Source";
  } else if (VatesViewerInterface::MDHW == workspaceType) {
    sourcePlugin = "MDHW Source";
  } else {
    sourcePlugin = "MDEW Source";
  }

  // Load a new source plugin
  auto gridAxesOn = areGridAxesOn();
  pqPipelineSource *source = this->currentView->setPluginSource(
      sourcePlugin, workspaceName, gridAxesOn);
  source->getProxy()->SetAnnotation(this->m_widgetName.toLatin1().data(), "1");
  this->renderAndFinalSetup();
  this->currentView->show();
}

/**
 * Reset the current view if this is required
 * @param workspaceType The type of workspace.
 * @param instrumentName The name of the instrument.
 */
void MdViewerWidget::resetCurrentView(int workspaceType,
                                      const std::string &instrumentName) {
  // Check if the current view is the correct initial view for the workspace
  // type and the instrument
  ModeControlWidget::Views initialView =
      getInitialView(workspaceType, instrumentName);
  auto currentViewType = currentView->getViewType();

  if (initialView != currentViewType) {
    this->ui.modeControlWidget->setToSelectedView(initialView);
  } else {
    this->currentView->show();
  }

  this->initialView = initialView;
}

/*
 * Provides an initial view. This view is specified either in the
 * Mantid.user.properties file or by the most common technique of the
 * instrument which is associated with the workspace data.
 * @param workspaceType The work space type.
 * @param instrumentName The name of the instrument with which the workspace
 *                       data was measured.
 * @returns An initial view.
*/
ModeControlWidget::Views
MdViewerWidget::getInitialView(int workspaceType,
                               const std::string &instrumentName) {
  // Get the possible initial views
  QString initialViewFromUserProperties =
      mdSettings.getUserSettingInitialView();
  QString initialViewFromTechnique = getViewForInstrument(instrumentName);

  // The user-properties-defined default view takes precedence over the
  // technique-defined default view
  QString initialView;
  if (initialViewFromUserProperties == mdConstants.getTechniqueDependence()) {
    initialView = initialViewFromTechnique;
  } else {
    initialView = initialViewFromUserProperties;
  }

  ModeControlWidget::Views view =
      this->ui.modeControlWidget->getViewFromString(initialView);

  // Make sure that the default view is compatible with the current workspace,
  // e.g. a a histo workspace cannot have a splatter plot
  return checkViewAgainstWorkspace(view, workspaceType);
}

/**
 * Get the view which is adequat for a specified machine
 * @param instrumentName The name of the instrument with which the workspace
 *                       data was measured.
 * @returns A view.
 */
QString
MdViewerWidget::getViewForInstrument(const std::string &instrumentName) const {
  // If nothing is specified the standard view is chosen
  if (instrumentName.empty()) {
    return mdConstants.getStandardView();
  }

  // Check for techniques
  // Precedence is 1. Single Crystal Diffraction -->SPLATTERPLOT
  //               2. Neutron Diffraction --> SPLATTERPLOT
  //               3. *Spectroscopy* --> MULTISLICE
  //               4. Other --> STANDARD
  QString associatedView;
  try {
    const auto techniques = Mantid::Kernel::ConfigService::Instance()
                                .getInstrument(instrumentName)
                                .techniques();

    if (techniques.count("Single Crystal Diffraction") > 0) {
      associatedView = mdConstants.getSplatterPlotView();
    } else if (checkIfTechniqueContainsKeyword(techniques, "Spectroscopy")) {
      associatedView = mdConstants.getMultiSliceView();
    } else if (techniques.count("Neutron Diffraction") > 0) {
      associatedView = mdConstants.getSplatterPlotView();
    } else {
      associatedView = mdConstants.getStandardView();
    }
  } catch (...) {
    associatedView = mdConstants.getStandardView();
  }
  return associatedView;
}

/**
 * Check if a set of techniques contains a technique which matches specified
 * keyword
 * @param techniques A set of techniques
 * @param keyword A keyword
 * @returns True if the keyword is contained in at least one technique else
 * false.
 */
bool MdViewerWidget::checkIfTechniqueContainsKeyword(
    const std::set<std::string> &techniques, const std::string &keyword) const {
  boost::regex pattern("(.*)" + keyword + "(.*)");

  for (auto const &technique : techniques) {
    if (boost::regex_match(technique, pattern)) {
      return true;
    }
  }

  return false;
}

/*
  * Check that the selected default view is compatible with the workspace type
  *
  * @param view An initial view.
  * @param workspaceType The type of workspace.
  * @returns A user-specified inital view or the standard view.
*/
ModeControlWidget::Views
MdViewerWidget::checkViewAgainstWorkspace(ModeControlWidget::Views view,
                                          int workspaceType) {
  ModeControlWidget::Views selectedView;

  if (VatesViewerInterface::MDHW == workspaceType) {
    // Histo workspaces cannot have a splatter plot,
    if (view == ModeControlWidget::SPLATTERPLOT) {
      g_log.notice("The preferred initial view favours the splatterplot "
                   "as initial view, but an MDHisto workspace is being "
                   "loaded. A MDHisto workspace cannot be loaded into a "
                   "splatterplot view. Defaulted to MultiSlice view.");

      selectedView = ModeControlWidget::MULTISLICE;
    } else {
      selectedView = view;
    }
  } else {
    selectedView = view;
  }

  return selectedView;
}

/**
 * This function performs setup for the plugin mode of the Vates Simple
 * Interface. It calls a number of defined functions to complete the process.
 */
void MdViewerWidget::setupPluginMode(int WsType,
                                     const std::string &instrumentName) {
  // Don't use the current color map at start up.
  this->useCurrentColorSettings = false;
  this->setupUiAndConnections();
  this->createMenus();
  ModeControlWidget::Views initialView =
      this->getInitialView(WsType, instrumentName);
  this->setupMainView(initialView);
}

/**
 * Load the state of the Vates window from a Mantid project file
 *
 * @param lines :: a string representing the state of the Vates window
 */
void MdViewerWidget::loadFromProject(const std::string &lines) {
  this->useCurrentColorSettings = false;
  this->setupUiAndConnections();
  this->createMenus();

  TSVSerialiser tsv(lines);

  int viewType;
  std::string viewName, sourceName, originalSourceName, originalRepName;
  tsv.selectLine("ViewName");
  tsv >> viewName;
  tsv.selectLine("SourceName");
  tsv >> sourceName;
  tsv.selectLine("OriginalSourceName");
  tsv >> originalSourceName;
  tsv.selectLine("OriginalRepresentationName");
  tsv >> originalRepName;
  tsv.selectLine("ViewType");
  tsv >> viewType;

  auto vtype = static_cast<ModeControlWidget::Views>(viewType);

  // Set the view type on the widget
  this->ui.modeControlWidget->blockSignals(true);
  this->ui.modeControlWidget->setToSelectedView(vtype);
  this->ui.modeControlWidget->blockSignals(false);

  // Load the state of VSI from the XML dump
  QSettings settings;
  auto workingDir = settings.value("Project/WorkingDirectory", "").toString();
  auto fileName = workingDir.toStdString() + "/VSI.xml";
  Poco::File file(fileName);
  if (!file.exists())
    return; // file path invalid.

  auto success = loadVSIState(fileName);
  if (!success)
    return; // state could not be loaded. There's nothing left to do!

  auto &activeObjects = pqActiveObjects::instance();
  auto proxyManager = activeObjects.activeServer()->proxyManager();
  auto viewProxy = proxyManager->GetProxy("views", viewName.c_str());
  auto sourceProxy = proxyManager->GetProxy("sources", sourceName.c_str());

  if (!viewProxy || !sourceProxy)
    return; // could not find the active view/source from last session.

  // Get the active objects from the last session
  auto model = pqApplicationCore::instance()->getServerManagerModel();
  auto view = model->findItem<pqRenderView *>(viewProxy);
  auto source = model->findItem<pqPipelineSource *>(sourceProxy);

  if (!view || !source)
    return; // could not find the active PV view/source from last session.

  setActiveObjects(view, source);
  setupViewFromProject(vtype);
  auto origSrcProxy =
      proxyManager->GetProxy("sources", originalSourceName.c_str());
  auto origSrc = model->findItem<pqPipelineSource *>(origSrcProxy);
  this->currentView->origSrc = qobject_cast<pqPipelineSource *>(origSrc);

  // Work around to force the update of the shader preset.
  auto rep = pqActiveObjects::instance().activeRepresentation()->getProxy();
  rep->UpdateProperty("ShaderPreset", 1);

  if (tsv.selectSection("colormap")) {
    std::string colorMapLines;
    tsv >> colorMapLines;
    ui.colorSelectionWidget->loadFromProject(colorMapLines);
  }

  if (tsv.selectSection("rebinning")) {
    std::string rebinningLines;
    tsv >> rebinningLines;
    m_rebinnedSourcesManager.loadFromProject(rebinningLines);
  }

  currentView->show();

  // Don't call render on ViewBase here as that will reset the camera.
  // Instead just directly render the view proxy using render all.
  this->currentView->renderAll();
  this->currentView->updateAnimationControls();
  this->setDestroyedListener();
  this->currentView->setVisibilityListener();
}

/**
 * Load the state of VSI from an XML file.
 *
 * @param fileName
 * @return true if the state was successfully loaded
 */
bool MdViewerWidget::loadVSIState(const std::string &fileName) {
  auto proxyManager =
      pqActiveObjects::instance().activeServer()->proxyManager();

  try {
    proxyManager->LoadXMLState(fileName.c_str());
  } catch (...) {
    return false;
  }

  // Update all registered proxies.
  // Some things may have been incorrectly setup during the loading step
  // due to the load order.
  proxyManager->UpdateRegisteredProxiesInOrder(0);
  return true;
}

/**
 * Setup the current view from a project file.
 *
 * There's need to create an entirely new view as it already exists in the
 * loaded project state.
 *
 * @param vtype :: the type of view to be created (e.g. Multislice)
 */
void MdViewerWidget::setupViewFromProject(ModeControlWidget::Views vtype) {
  // Initilise the current view to something and setup
  currentView = createAndSetMainViewWidget(ui.viewWidget, vtype, false);
  initialView = vtype;
  currentView->installEventFilter(this);
  viewLayout = new QHBoxLayout(ui.viewWidget);
  viewLayout->setMargin(0);
  viewLayout->setStretch(0, 1);
  viewLayout->addWidget(currentView);

  auto source = pqActiveObjects::instance().activeSource();
  auto view = pqActiveObjects::instance().activeView();
  // Swap out the existing view for the newly loaded source and representation
  currentView->origSrc = qobject_cast<pqPipelineSource *>(source);
  currentView->setView(qobject_cast<pqRenderView *>(view));
  setParaViewComponentsForView();
}

/**
 * Set the active objects. This will trigger events to update the properties
 * panel and the pipeline viewer.
 *
 * @param view :: the view to set as currently active
 * @param source :: the source to set as currently active
 */
void MdViewerWidget::setActiveObjects(pqView *view, pqPipelineSource *source) {

  auto &activeObjects = pqActiveObjects::instance();
  activeObjects.setActiveView(view);
  activeObjects.setActiveSource(source);
  activeObjects.setActivePort(source->getOutputPort(0));
}

/**
 * Save the state of the Vates window to a Mantid project file
 *
 * @param app :: handle to the main application window instance
 * @return a string representing the current state of the window
 */
std::string MdViewerWidget::saveToProject(ApplicationWindow *app) {
  UNUSED_ARG(app);
  TSVSerialiser tsv, contents;

  // save window position & size
  contents.writeLine("geometry") << parentWidget()->geometry();

  QSettings settings;
  auto workingDir = settings.value("Project/WorkingDirectory", "").toString();
  auto fileName = workingDir.toStdString() + "/VSI.xml";

  // Dump the state of VSI to a XML file
  auto session = pqActiveObjects::instance().activeServer()->proxyManager();
  session->SaveXMLState(fileName.c_str());
  contents.writeLine("FileName") << fileName;

  // Save the view type. e.g. Splatterplot, Multislice...
  auto vtype = currentView->getViewType();
  contents.writeLine("ViewType") << static_cast<int>(vtype);

  auto &activeObjects = pqActiveObjects::instance();
  auto proxyManager = activeObjects.activeServer()->proxyManager();
  auto view = activeObjects.activeView()->getProxy();
  auto source = activeObjects.activeSource()->getProxy();

  auto viewName = proxyManager->GetProxyName("views", view);
  auto sourceName = proxyManager->GetProxyName("sources", source);

  contents.writeLine("ViewName") << viewName;
  contents.writeLine("SourceName") << sourceName;

  if (this->currentView->origRep) {
    auto repName = proxyManager->GetProxyName(
        "representations", this->currentView->origRep->getProxy());
    contents.writeLine("OriginalRepresentationName") << repName;
  }

  if (this->currentView->origSrc) {
    auto srcName = proxyManager->GetProxyName(
        "sources", this->currentView->origSrc->getProxy());
    contents.writeLine("OriginalSourceName") << srcName;
  }

  contents.writeSection("colormap", ui.colorSelectionWidget->saveToProject());
  contents.writeSection("rebinning", m_rebinnedSourcesManager.saveToProject());
  tsv.writeSection("vsi", contents.outputLines());

  return tsv.outputLines();
}

std::vector<std::string> MdViewerWidget::getWorkspaceNames() {
  auto server = pqActiveObjects::instance().activeServer();
  auto model = pqApplicationCore::instance()->getServerManagerModel();
  const auto sources = model->findItems<pqPipelineSource *>(server);

  std::vector<std::string> workspaceNames;
  for (auto source : sources) {
    const auto proxy = source->getProxy();
    const auto srcProxyName = proxy->GetXMLGroup();
    if (srcProxyName == QString("sources")) {
      std::string wsName(
          vtkSMPropertyHelper(proxy, "WorkspaceName", true).GetAsString());
      workspaceNames.push_back(wsName);
    }
  }

  return workspaceNames;
}

std::string MdViewerWidget::getWindowName() {
  return m_widgetName.toStdString();
}

std::string MdViewerWidget::getWindowType() { return "VSIWindow"; }

/**
 * This function tells the current view to render the data, perform any
 * necessary checks on the view given the workspace type and update the
 * animation controls if necessary.
 */
void MdViewerWidget::renderAndFinalSetup() {
  Mantid::VATES::ColorScaleLockGuard colorScaleLockGuard(&m_colorScaleLock);
  this->setColorForBackground();
  this->currentView->render();
  this->setColorMap();
  this->currentView->setColorsForView(this->ui.colorSelectionWidget);
  this->currentView->checkView(this->initialView);
  this->currentView->updateAnimationControls();
  this->setDestroyedListener();
  this->currentView->setVisibilityListener();
  this->currentView->onAutoScale(this->ui.colorSelectionWidget);
}

/**
 * Set the background color for this view.
 */
void MdViewerWidget::setColorForBackground() {
  this->currentView->setColorForBackground(this->useCurrentColorSettings);
}

/**
 * This function is used during the post-apply process of particular pipeline
 * filters to check for updates to anything that relies on information from the
 * rendered data.
 */
void MdViewerWidget::checkForUpdates() {
  Mantid::VATES::ColorScaleLockGuard colorScaleLockGuard(&m_colorScaleLock);
  pqPipelineSource *src = pqActiveObjects::instance().activeSource();
  if (!src) {
    return;
  }
  vtkSMProxy *proxy = src->getProxy();

  if (QString(proxy->GetXMLName()).contains("Threshold")) {
    this->ui.colorSelectionWidget->enableControls(true);
    vtkSMDoubleVectorProperty *range = vtkSMDoubleVectorProperty::SafeDownCast(
        proxy->GetProperty("ThresholdBetween"));
    this->ui.colorSelectionWidget->setColorScaleRange(range->GetElement(0),
                                                      range->GetElement(1));
  }
  if (QString(proxy->GetXMLName()).contains("ScaleWorkspace")) {
    this->currentView->resetDisplay();
  }

  // Make sure that the color scale is calculated
  if (this->ui.colorSelectionWidget->getAutoScaleState()) {
    this->currentView->onAutoScale(this->ui.colorSelectionWidget);
  }
}

/**
 * This function executes the logic for switching views on the main level
 * window.
 *
 * @param v the view mode to switch to
 */
void MdViewerWidget::switchViews(ModeControlWidget::Views v) {
  this->removeAllRebinning(v);
  this->viewSwitched = true;

  // normally it will just close child SliceView windows
  auto axesGridOn = areGridAxesOn();
  this->currentView->closeSubWindows();
  this->disconnectDialogs();
  this->hiddenView = this->createAndSetMainViewWidget(this->ui.viewWidget, v);
  this->ui.colorSelectionWidget->ignoreColorChangeCallbacks(true);
  this->hiddenView->setColorScaleState(this->ui.colorSelectionWidget);
  auto viewSize = this->hiddenView->size();
  this->hiddenView->hide();
  this->viewLayout->removeWidget(this->currentView);

  // save all the current visual state before switching.
  saveViewState(this->currentView);
  this->swapViews();

  this->viewLayout->addWidget(this->currentView);
  this->currentView->installEventFilter(this);
  this->currentView->show();
  this->hiddenView->hide();
  this->setParaViewComponentsForView();
  this->connectDialogs();

  this->hiddenView->close();
  this->hiddenView->destroyView();
  this->hiddenView->deleteLater();
  this->currentView->setAxesGrid(axesGridOn);
  // Currently this render will do one or more resetCamera() and even
  // resetDisplay() for different views, see for example
  // StandardView::onRenderDone().
  this->currentView->render();
  // so because of that, after render() we restore the view state =>
  // visual glitch from initial/reset camera to restored camera. This
  // should improved in the future but requires reorganization of
  // ViewBase and the specialized VSI view classes (trac ticket #11739).
  restoreViewState(this->currentView, v);
  this->currentView->setColorsForView(this->ui.colorSelectionWidget);
  this->setColorForBackground();

  this->currentView->checkViewOnSwitch();
  this->updateAppState();
  this->initialView = v;

  this->setDestroyedListener();
  this->currentView->setVisibilityListener();

  // A workaround to make the view redraw itself properly
  // after switching from a resized view
  this->currentView->resize(viewSize);

  // ignore callbacks until as late as possible to keep desired state
  // regardless of what the Paraview widgets are doing
  this->ui.colorSelectionWidget->ignoreColorChangeCallbacks(false);
}

/**
 * This function performs a standard pointer swap for the view switching.
 */
void MdViewerWidget::swapViews() {
  if (!this->currentView)
    g_log.error(
        "Inconsistency found when swapping views, the current view is NULL");
  if (!this->hiddenView)
    g_log.error(
        "Inconsistency found when swapping views, the next view is NULL");
  std::swap(this->currentView, this->hiddenView);
}

/**
 * This function allows one to filter the Qt events and look for a hide
 * event. As long as the event does not come from the system (minimize VSI
 * window or switch virtual desktops), it then executes source cleanup and
 * view mode switch if the viewer is in plugin mode.
 * @param obj the subject of the event
 * @param ev the actual event
 * @return true if the event was handled
 */
bool MdViewerWidget::eventFilter(QObject *obj, QEvent *ev) {
  if (this->currentView == obj) {
    if (this->pluginMode && QEvent::Hide == ev->type() && !ev->spontaneous()) {
      if (this->ui.parallelProjButton->isChecked()) {
        this->ui.parallelProjButton->toggle();
      }

      this->ui.colorSelectionWidget->reset();
      this->currentView->setColorScaleState(this->ui.colorSelectionWidget);
      this->currentView->destroyAllSourcesInView();
      this->currentView->updateSettings();
      this->currentView->hide();
      this->useCurrentColorSettings = false;

      this->m_colorMapEditorPanel->hide();

      return true;
    }
  }
// IMPORTANT FOR MULTIPLE VSI INSTANCES:
// The following code block seems to be intended for use with multiple instances
// but it introduces an undesired behaviour in its current form. This is
// especially visible when we are dealing with source-filter chains (the active
// source is more likely to be the filter than the underlying source).
// Instead of setting oricSrc as the active source we need to devise an
// alternative
// strategy, e.g. keep track of the last active source of the particlar
// VSI instance and set this as the active source.
#if 0
  if(ev->type() == QEvent::WindowActivate)
  {
    if(this->currentView)
    {
      pqView* view = this->currentView->getView();
      pqActiveObjects::instance().setActiveView(view);
      pqActiveObjects::instance().setActiveSource(this->currentView->origSrc);
    }
  }
#endif
  return VatesViewerInterface::eventFilter(obj, ev);
}

/**
 * This function performs shutdown procedures when MantidPlot is shut down,
 */
void MdViewerWidget::shutdown() {
  // This seems to cure a XInitThreads error.
  pqPVApplicationCore::instance()->deleteLater();
  // Ensure that the MathText utilties are cleaned up as they call Python
  // cleanup code
  // and we need to make sure this can happen before MantidPlot shuts down the
  // interpreter
  vtkMathTextUtilitiesCleanup();
}

/**
 * This function creates the main view widget specific menu items.
 */
void MdViewerWidget::createMenus() {
  QMenuBar *menubar;
  if (this->pluginMode) {
    menubar = new QMenuBar(this->parentWidget());
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    menubar->setSizePolicy(policy);
  } else {
    menubar = qobject_cast<QMainWindow *>(this->parentWidget())->menuBar();
  }

  QMenu *viewMenu = menubar->addMenu(QApplication::tr("&View"));

  this->lodAction =
      new QAction(QApplication::tr("Level-of-Detail (LOD...)"), this);
  this->lodAction->setShortcut(QKeySequence::fromString("Ctrl+Shift+L"));
  this->lodAction->setStatusTip(
      QApplication::tr("Enable/disable level-of-detail threshold."));
  this->lodAction->setCheckable(true);
  this->lodAction->setChecked(true);
  QObject::connect(this->lodAction, SIGNAL(toggled(bool)), this,
                   SLOT(onLodToggled(bool)));
  viewMenu->addAction(this->lodAction);

  QAction *screenShotAction =
      new QAction(QApplication::tr("Save Screenshot"), this);
  screenShotAction->setShortcut(QKeySequence::fromString("Ctrl+Shift+R"));
  screenShotAction->setStatusTip(
      QApplication::tr("Save a screenshot of the current view."));
  this->screenShot = new SaveScreenshotReaction(screenShotAction);
  viewMenu->addAction(screenShotAction);

  QAction *settingsAction = new QAction(QApplication::tr("Settings..."), this);
  settingsAction->setShortcut(QKeySequence::fromString("Ctrl+Shift+S"));
  settingsAction->setStatusTip(
      QApplication::tr("Show the settings for the current view."));
  this->viewSettings = new pqApplicationSettingsReaction(settingsAction);
  viewMenu->addAction(settingsAction);

  QMenu *helpMenu = menubar->addMenu(QApplication::tr("&Help"));

  QAction *wikiHelpAction =
      new QAction(QApplication::tr("Show Wiki Help"), this);
  wikiHelpAction->setShortcut(QKeySequence::fromString("Ctrl+Shift+H"));
  wikiHelpAction->setStatusTip(
      QApplication::tr("Show the wiki help page in a browser."));
  QObject::connect(wikiHelpAction, SIGNAL(triggered()), this,
                   SLOT(onWikiHelp()));
  helpMenu->addAction(wikiHelpAction);

  if (this->pluginMode) {
    this->ui.verticalLayout_4->insertWidget(0, menubar);
  }
}

/**
 * This function adds the menus defined here to a QMainWindow menu bar.
 * This must be done after the setup of the standalone application so that
 * the MdViewerWidget menus aren't added before the standalone ones.
 */
void MdViewerWidget::addMenus() { this->createMenus(); }

/**
 * This function intercepts the LOD menu action checking and calls the
 * correct slot on the current view.
 * @param state : whether the action is checked or not
 */
void MdViewerWidget::onLodToggled(bool state) {
  this->currentView->onLodThresholdChange(state, this->lodThreshold);
}

/**
 * This function handles creating the rotation point input dialog box and
 * setting the communication between it and the current view.
 */
void MdViewerWidget::onRotationPoint() {
  if (!this->rotPointDialog) {
    this->rotPointDialog = new RotationPointDialog(this);
    this->connectRotationPointDialog();
  }
  this->rotPointDialog->show();
  this->rotPointDialog->raise();
  this->rotPointDialog->activateWindow();
}

/**
 * This function shows the wiki help page for the simple interface in a
 * browser.
 */
void MdViewerWidget::onWikiHelp() {
  MantidDesktopServices::openUrl(QUrl(QString("http://www.mantidproject.org/") +
                                      "VatesSimpleInterface_v2"));
}

/**
 * This function disconnects the present instances of the color options and the
 * point rotation dialog boxes from the current view. This is necessary on
 * switch view since the connection to the current view is destroyed.
 */
void MdViewerWidget::disconnectDialogs() {
  if (this->rotPointDialog) {
    this->rotPointDialog->close();
    QObject::disconnect(this->rotPointDialog, nullptr, this->currentView,
                        nullptr);
  }
}

/**
 * This function sets up the connections between the color selection widget
 * items and the current view.
 */
void MdViewerWidget::connectColorSelectionWidget() {
  // Set the color selection widget signal -> view slot connection
  QObject::connect(this->ui.colorSelectionWidget,
                   SIGNAL(colorMapChanged(const Json::Value &)),
                   this->currentView,
                   SLOT(onColorMapChange(const Json::Value &)));
  QObject::connect(this->ui.colorSelectionWidget,
                   SIGNAL(colorScaleChanged(double, double)), this->currentView,
                   SLOT(onColorScaleChange(double, double)));

  // Set the view signal -> color selection widget slot connection
  QObject::connect(this->currentView, SIGNAL(dataRange(double, double)),
                   this->ui.colorSelectionWidget,
                   SLOT(setColorScaleRange(double, double)));
  QObject::connect(this->ui.colorSelectionWidget,
                   SIGNAL(autoScale(ColorSelectionWidget *)), this->currentView,
                   SLOT(onAutoScale(ColorSelectionWidget *)));
  QObject::connect(this->ui.colorSelectionWidget, SIGNAL(logScale(int)),
                   this->currentView, SLOT(onLogScale(int)));
  QObject::connect(this->currentView, SIGNAL(lockColorControls(bool)),
                   this->ui.colorSelectionWidget, SLOT(enableControls(bool)));

  QObject::connect(this->currentView, SIGNAL(setLogScale(bool)),
                   this->ui.colorSelectionWidget, SLOT(onSetLogScale(bool)));
}

/**
 * This function sets up the connections between the rotation point dialog and
 * the current view.
 */
void MdViewerWidget::connectRotationPointDialog() {
  if (this->rotPointDialog) {
    QObject::connect(
        this->rotPointDialog, SIGNAL(sendCoordinates(double, double, double)),
        this->currentView, SLOT(onResetCenterToPoint(double, double, double)));
  }
}

/**
 * This function sets up the connections for all the dialogs associated with
 * the MdViewerWidget.
 */
void MdViewerWidget::connectDialogs() { this->connectRotationPointDialog(); }

/**
 * This function handles any update to the state of application components
 * like menus, menu items, buttons, views etc.
 */
void MdViewerWidget::updateAppState() {
  auto type = currentView->getViewType();
  if (type == ModeControlWidget::THREESLICE ||
      type == ModeControlWidget::SPLATTERPLOT) {
    this->currentView->onLodThresholdChange(false, this->lodThreshold);
    this->lodAction->setChecked(false);
  } else {
    this->currentView->onLodThresholdChange(true, this->lodThreshold);
    this->lodAction->setChecked(true);
  }
}

/**
 * This function responds to the replacement of a workspace. It does not
 * handle workspace renaming. Also, by default it replaces the original
 * representation with a new one, deleting the old one first.
 * @param wsName : Name of workspace changing
 * @param ws : Pointer to changing workspace
 */
void MdViewerWidget::afterReplaceHandle(
    const std::string &wsName,
    const boost::shared_ptr<Mantid::API::Workspace> ws) {
  UNUSED_ARG(ws);
  pqPipelineSource *src = this->currentView->hasWorkspace(wsName.c_str());
  if (src) {
    // Have to mark the filter as modified to get it to update. Do this by
    // changing the requested workspace name to a dummy name and then change
    // back. However, push the change all the way down for it to work.
    vtkSMProxy *proxy = src->getProxy();
    vtkSMPropertyHelper(proxy, "Mantid Workspace Name").Set("ChangeMe!");
    proxy->UpdateVTKObjects();

    vtkSMPropertyHelper(proxy, "Mantid Workspace Name").Set(wsName.c_str());
    // Update the source so that it retrieves the data from the Mantid workspace
    proxy->UpdateVTKObjects();
    src->updatePipeline();

    this->currentView->setColorsForView(this->ui.colorSelectionWidget);
    this->currentView->renderAll();
  }
}

/**
 * This function responds to a workspace being deleted. If there are one or
 * more PeaksWorkspaces present, the requested one will be deleted.
 * Otherwise, if it is an IMDWorkspace, everything goes!
 * @param wsName : Name of workspace being deleted
 * @param ws : Pointer to workspace being deleted
 */
void MdViewerWidget::preDeleteHandle(const std::string &wsName,
                                     const boost::shared_ptr<Workspace> ws) {
  UNUSED_ARG(ws);

  pqPipelineSource *src = this->currentView->hasWorkspace(wsName.c_str());
  if (src) {
    long long numSources = this->currentView->getNumSources();
    if (numSources > 1) {
      pqObjectBuilder *builder =
          pqApplicationCore::instance()->getObjectBuilder();
      if (this->currentView->isPeaksWorkspace(src)) {
        builder->destroy(src);
        return;
      }
    }

    // Remove all visibility listeners
    this->currentView->removeVisibilityListener();

    emit this->requestClose();
  }
}

/**
* Set the listener for when sources are being destroyed
*/
void MdViewerWidget::setDestroyedListener() {
  pqServer *server = pqActiveObjects::instance().activeServer();
  pqServerManagerModel *smModel =
      pqApplicationCore::instance()->getServerManagerModel();
  const QList<pqPipelineSource *> sources =
      smModel->findItems<pqPipelineSource *>(server);

  // Attach the destroyd signal of all sources to the viewbase.
  foreach (pqPipelineSource *source, sources) {
    QObject::connect(source, SIGNAL(destroyed()), this->currentView,
                     SLOT(onSourceDestroyed()), Qt::UniqueConnection);
  }
}

/**
 * Dectect when a PeaksWorkspace is dragged into the VSI.
 * @param e A drag event.
 */
void MdViewerWidget::dragEnterEvent(QDragEnterEvent *e) {
  QString name = e->mimeData()->objectName();
  if (name == "MantidWorkspace") {
    QString text = e->mimeData()->text();
    QStringList wsNames;
    handleDragAndDropPeaksWorkspaces(e, text, wsNames);
  } else {
    e->ignore();
  }
}

/**
 * React to dropping a PeaksWorkspace onto the VSI.
 * @param e Drop event.
 */
void MdViewerWidget::dropEvent(QDropEvent *e) {
  QString name = e->mimeData()->objectName();
  if (name == "MantidWorkspace") {
    QString text = e->mimeData()->text();
    QStringList wsNames;
    handleDragAndDropPeaksWorkspaces(e, text, wsNames);
    if (!wsNames.empty()) {
      // We render the first workspace name, it is a peak workspace and the
      // instrument is not relevant
      renderWorkspace(wsNames[0], 1, "");
    }
  }
}

/* Verify that at least one source other than a "Peaks Source" has been loaded
 * in the VSI.
 * @return true if something other than a Peaks Source is found.
 */
bool otherWorkspacePresent() {
  pqServer *server = pqActiveObjects::instance().activeServer();
  pqServerManagerModel *smModel =
      pqApplicationCore::instance()->getServerManagerModel();
  auto sources = smModel->findItems<pqPipelineSource *>(server);
  auto result = std::find_if(
      sources.begin(), sources.end(), [](const pqPipelineSource *src) {
        return strcmp(src->getProxy()->GetXMLName(), "Peaks Source") != 0;
      });
  return result != sources.end();
}

/**
  * Handle the drag and drop events of peaks workspaces.
  * @param e The event.
  * @param text String containing information regarding the workspace name.
  * @param wsNames  Reference to a list of workspaces names, which are being
 * extracted.
  */
void MdViewerWidget::handleDragAndDropPeaksWorkspaces(QEvent *e,
                                                      const QString &text,
                                                      QStringList &wsNames) {
  int endIndex = 0;
  while (text.indexOf("[\"", endIndex) > -1) {
    int startIndex = text.indexOf("[\"", endIndex) + 2;
    endIndex = text.indexOf("\"]", startIndex);
    QString candidate = text.mid(startIndex, endIndex - startIndex);
    // Only append the candidate if SplattorPlotView is selected and an
    // MDWorkspace is loaded.
    if (currentView->getViewType() == ModeControlWidget::Views::SPLATTERPLOT &&
        otherWorkspacePresent()) {
      if (boost::dynamic_pointer_cast<IPeaksWorkspace>(
              AnalysisDataService::Instance().retrieve(
                  candidate.toStdString()))) {
        wsNames.append(candidate);
        e->accept();
      } else {
        e->ignore();
      }
    } else {
      e->ignore();
    }
  }
}

/**
 * Set the color map
 */
void MdViewerWidget::setColorMap() {
  // If it is not the first startup of the color map, then we want to use the
  // current color map
  this->ui.colorSelectionWidget->loadColorMap(this->useCurrentColorSettings);
}

/**
 * Save the state of a view. Normally you use this to save the state
 * of the current view before switching to a different view, so when
 * the user switches back to the original view its state can be
 * restored. @see restoreViewState(). This class saves one state for
 * every type of view. The state of a view includes the main
 * properties of the camera (vtkCamera) namely Position, FocalPoint,
 * ViewUp, ViewAngle, ClippingRange) and also properties of the
 * pqRenderView (CenterOfRotation, CenterAxesVisibility, etc.). And
 * for the slice views you'll also need to save the slices status.
 *
 * The state is saved using vtkSMProxy's saveXMLState which returns
 * the XML tree that can be reloaded to reproduce the same state in
 * another (render view) proxy. Note that the alternative
 * GetFullState()/ methods do not seem to be usable in this way but is
 * meant to support undo/do in ParaView.
 *
 * @param view A VSI view (subclass of ViewBase) which can be for
 * example of types: standard, multislice, threeslice, splatter, its
 * state will be saved
 */
void MdViewerWidget::saveViewState(ViewBase *view) {
  if (!view)
    return;

  auto vtype = currentView->getViewType();
  switch (vtype) {
  case ModeControlWidget::Views::STANDARD: {
    m_allViews.stateStandard.TakeReference(
        view->getView()->getRenderViewProxy()->SaveXMLState(nullptr));
  } break;
  case ModeControlWidget::Views::THREESLICE: {
    m_allViews.stateThreeSlice.TakeReference(
        view->getView()->getRenderViewProxy()->SaveXMLState(nullptr));
  } break;
  case ModeControlWidget::Views::MULTISLICE: {
    m_allViews.stateMulti.TakeReference(
        view->getView()->getRenderViewProxy()->SaveXMLState(nullptr));
  } break;
  case ModeControlWidget::Views::SPLATTERPLOT: {
    m_allViews.stateSplatter.TakeReference(
        view->getView()->getRenderViewProxy()->SaveXMLState(nullptr));
  } break;
  default:
    view = nullptr;
    break;
  }
}

/**
 * Restores the state of a view (if there's a saved state for this
 * type of view, which should happen if the user has been in that view
 * before and is switching back to it. @see saveViewState().
 *
 * @param view View where we want to restore the previous state
 * @param vtype Type of view (standard, multislice, etc.)
 */
void MdViewerWidget::restoreViewState(ViewBase *view,
                                      ModeControlWidget::Views vtype) {
  if (!view)
    return;

  int loaded = 0;

  switch (vtype) {
  case ModeControlWidget::STANDARD: {
    if (m_allViews.stateStandard)
      loaded = view->getView()->getRenderViewProxy()->LoadXMLState(
          m_allViews.stateStandard.GetPointer(), nullptr);
  } break;
  case ModeControlWidget::THREESLICE: {
    if (m_allViews.stateThreeSlice)
      loaded = view->getView()->getRenderViewProxy()->LoadXMLState(
          m_allViews.stateThreeSlice.GetPointer(), nullptr);
  } break;
  case ModeControlWidget::MULTISLICE: {
    if (m_allViews.stateMulti)
      loaded = view->getView()->getRenderViewProxy()->LoadXMLState(
          m_allViews.stateMulti.GetPointer(), nullptr);
  } break;
  case ModeControlWidget::SPLATTERPLOT: {
    if (m_allViews.stateSplatter)
      loaded = view->getView()->getRenderViewProxy()->LoadXMLState(
          m_allViews.stateSplatter.GetPointer(), nullptr);
  } break;
  default:
    view = nullptr;
    break;
  }

  if (!loaded)
    g_log.warning() << "Failed to restore the state of the current view even "
                       "though I thought I had "
                       "a state saved from before. The current state may not "
                       "be consistent.";
}

/**
 * Get the current grid axes setting
 * @returns the true if the grid axes are on, else false
 */
bool MdViewerWidget::areGridAxesOn() {
  // If we start up then we want to have the grid axes on
  if (m_gridAxesStartUpOn) {
    m_gridAxesStartUpOn = false;
    return true;
  }

  // Get the state of the Grid Axes
  auto renderView = this->currentView->getView();
  vtkSMProxy *gridAxes3DActor =
      vtkSMPropertyHelper(renderView->getProxy(), "AxesGrid", true)
          .GetAsProxy();
  auto gridAxesSetting =
      vtkSMPropertyHelper(gridAxes3DActor, "Visibility").GetAsInt();
  if (gridAxesSetting == 0) {
    return false;
  } else {
    return true;
  }
}

} // namespace SimpleGui
} // namespace Vates
} // namespace Mantid
