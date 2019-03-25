// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FitPropertyBrowser.h"
#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidQtWidgets/Common/MantidDesktopServices.h"
#include "MantidQtWidgets/Common/MultifitSetupDialog.h"
#include "MantidQtWidgets/Common/PropertyHandler.h"
#include "MantidQtWidgets/Common/SequentialFitDialog.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidAPI/ICostFunction.h"
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"

#include "MantidQtWidgets/Common/QtPropertyBrowser/FilenameDialogEditor.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/FormulaDialogEditor.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/StringEditorFactory.h"

#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/ParameterPropertyManager.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qteditorfactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qttreepropertybrowser.h"

#include <Poco/ActiveResult.h>

#include <QApplication>
#include <QClipboard>
#include <QGridLayout>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSignalMapper>
#include <QTreeWidget>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>
#include <iostream>

namespace MantidQt {
using API::MantidDesktopServices;

namespace MantidWidgets {

namespace {
Mantid::Kernel::Logger g_log("FitPropertyBrowser");

using namespace Mantid::API;

Workspace_sptr getADSWorkspace(std::string const &workspaceName) {
  return AnalysisDataService::Instance().retrieve(workspaceName);
}

MatrixWorkspace_sptr convertToMatrixWorkspace(Workspace_sptr workspace) {
  return boost::dynamic_pointer_cast<MatrixWorkspace>(workspace);
}

int getNumberOfSpectra(MatrixWorkspace_sptr workspace) {
  return static_cast<int>(workspace->getNumberHistograms());
}

} // namespace

/**
 * Constructor
 * @param parent :: The parent widget - must be an ApplicationWindow
 * @param mantidui :: The UI form for MantidPlot
 */
FitPropertyBrowser::FitPropertyBrowser(QWidget *parent, QObject *mantidui)
    : QDockWidget("Fit Function", parent), m_workspaceIndex(nullptr),
      m_startX(nullptr), m_endX(nullptr), m_output(nullptr),
      m_minimizer(nullptr), m_ignoreInvalidData(nullptr),
      m_costFunction(nullptr), m_maxIterations(nullptr), m_peakRadius(nullptr),
      m_logValue(nullptr), m_plotDiff(nullptr), m_plotCompositeMembers(nullptr),
      m_convolveMembers(nullptr), m_rawData(nullptr), m_xColumn(nullptr),
      m_yColumn(nullptr), m_errColumn(nullptr), m_showParamErrors(nullptr),
      m_evaluationType(nullptr), m_compositeFunction(), m_browser(nullptr),
      m_fitActionUndoFit(nullptr), m_fitActionSeqFit(nullptr),
      m_fitActionFit(nullptr), m_fitActionEvaluate(nullptr),
      m_functionsGroup(nullptr), m_settingsGroup(nullptr),
      m_customSettingsGroup(nullptr), m_changeSlotsEnabled(false),
      m_guessOutputName(true),
      m_updateObserver(*this, &FitPropertyBrowser::handleFactoryUpdate),
      m_fitMapper(nullptr), m_fitMenu(nullptr),
      m_displayActionPlotGuess(nullptr), m_displayActionQuality(nullptr),
      m_displayActionClearAll(nullptr), m_setupActionCustomSetup(nullptr),
      m_setupActionRemove(nullptr), m_tip(nullptr), m_fitSelector(nullptr),
      m_fitTree(nullptr), m_currentHandler(nullptr),
      m_defaultFunction("Gaussian"), m_defaultPeak("Gaussian"),
      m_defaultBackground("LinearBackground"), m_peakToolOn(false),
      m_auto_back(false),
      m_autoBgName(QString::fromStdString(
          Mantid::Kernel::ConfigService::Instance().getString(
              "curvefitting.autoBackground"))),
      m_autoBackground(nullptr), m_decimals(-1), m_mantidui(mantidui),
      m_shouldBeNormalised(false), m_oldWorkspaceIndex(-1) {
  Mantid::API::FrameworkManager::Instance().loadPlugins();

  // Try to create a Gaussian. Failing will mean that CurveFitting dll is not
  // loaded
  boost::shared_ptr<Mantid::API::IFunction> f =
      boost::shared_ptr<Mantid::API::IFunction>(
          Mantid::API::FunctionFactory::Instance().createFunction("Gaussian"));
  if (m_autoBgName.toLower() == "none") {
    m_autoBgName = "";
  } else {
    setAutoBackgroundName(m_autoBgName);
  }

  std::string def = Mantid::Kernel::ConfigService::Instance().getString(
      "curvefitting.defaultPeak");
  if (!def.empty()) {
    m_defaultPeak = def;
  }

  def = Mantid::Kernel::ConfigService::Instance().getString(
      "curvefitting.autoBackground");
  if (!def.empty()) {
    m_defaultBackground = def;
  }
  m_defaultFunction = m_defaultPeak;

  setObjectName(
      "FitFunction"); // this is needed for QMainWindow::restoreState()
  setMinimumHeight(150);
  setMinimumWidth(200);

  QWidget *w = new QWidget(this);

  /* Create property managers: they create, own properties, get and set values
   */
  m_groupManager = new QtGroupPropertyManager(w);
  m_doubleManager = new QtDoublePropertyManager(w);
  m_stringManager = new QtStringPropertyManager(w);
  m_enumManager = new QtEnumPropertyManager(w);
  m_intManager = new QtIntPropertyManager(w);
  m_boolManager = new QtBoolPropertyManager(w);
  m_filenameManager = new QtStringPropertyManager(w);
  m_formulaManager = new QtStringPropertyManager(w);
  m_columnManager = new QtEnumPropertyManager(w);
  m_workspace = m_enumManager->addProperty("Workspace");
  m_vectorManager = new QtGroupPropertyManager(w);
  m_vectorSizeManager = new QtIntPropertyManager(w);
  m_vectorDoubleManager = new QtDoublePropertyManager(w);
  m_parameterManager = new ParameterPropertyManager(w);
}

/**
 * Initialise the fit property browser
 */
void FitPropertyBrowser::init() {
  QWidget *w = new QWidget(this);

  QSettings settings;
  settings.beginGroup("Mantid/FitBrowser");

  /* Create function group */
  QtProperty *functionsGroup = m_groupManager->addProperty("Functions");

  if (m_mantidui) {
    connect(this, SIGNAL(xRangeChanged(double, double)), m_mantidui,
            SLOT(x_range_from_picker(double, double)));
  }
  /* Create input - output properties */
  QtProperty *settingsGroup = m_groupManager->addProperty("Settings");
  m_startX = addDoubleProperty("StartX");
  m_endX = addDoubleProperty("EndX");

  m_workspaceIndex = m_intManager->addProperty("Workspace Index");
  m_output = m_stringManager->addProperty("Output");
  m_minimizer = m_enumManager->addProperty("Minimizer");
  m_minimizers << "Levenberg-Marquardt"
               << "Levenberg-MarquardtMD"
               << "Trust Region"
               << "Simplex"
               << "FABADA"
               << "Conjugate gradient (Fletcher-Reeves imp.)"
               << "Conjugate gradient (Polak-Ribiere imp.)"
               << "BFGS"
               << "Damped GaussNewton";

  m_ignoreInvalidData = m_boolManager->addProperty("Ignore invalid data");
  setIgnoreInvalidData(settings.value("Ignore invalid data", false).toBool());

  m_enumManager->setEnumNames(m_minimizer, m_minimizers);
  m_costFunction = m_enumManager->addProperty("Cost function");
  m_costFunctions << "Least squares"
                  << "Rwp"
                  << "Unweighted least squares";
  m_enumManager->setEnumNames(m_costFunction, m_costFunctions);
  m_maxIterations = m_intManager->addProperty("Max Iterations");
  m_intManager->setValue(m_maxIterations,
                         settings.value("Max Iterations", 500).toInt());

  m_peakRadius = m_intManager->addProperty("Peak Radius");
  m_intManager->setValue(m_peakRadius,
                         settings.value("Peak Radius", 0).toInt());

  m_plotDiff = m_boolManager->addProperty("Plot Difference");
  bool plotDiff = settings.value("Plot Difference", QVariant(true)).toBool();
  m_boolManager->setValue(m_plotDiff, plotDiff);

  m_plotCompositeMembers = m_boolManager->addProperty("Plot Composite Members");
  bool plotCompositeItems =
      settings.value(m_plotCompositeMembers->propertyName(), QVariant(false))
          .toBool();
  m_boolManager->setValue(m_plotCompositeMembers, plotCompositeItems);

  m_convolveMembers = m_boolManager->addProperty("Convolve Composite Members");
  bool convolveCompositeItems =
      settings.value(m_plotCompositeMembers->propertyName(), QVariant(false))
          .toBool();
  m_boolManager->setValue(m_convolveMembers, convolveCompositeItems);

  m_showParamErrors = m_boolManager->addProperty("Show Parameter Errors");
  bool showParamErrors =
      settings.value(m_showParamErrors->propertyName(), false).toBool();
  m_boolManager->setValue(m_showParamErrors, showParamErrors);
  m_parameterManager->setErrorsEnabled(showParamErrors);

  m_evaluationType = m_enumManager->addProperty("Evaluate Function As");
  m_evaluationType->setToolTip(
      "Consider using Histogram fit which may produce more accurate results.");
  m_evaluationTypes << "CentrePoint"
                    << "Histogram";
  m_enumManager->setEnumNames(m_evaluationType, m_evaluationTypes);
  int evaluationType =
      settings.value(m_evaluationType->propertyName(), 0).toInt();
  m_enumManager->setValue(m_evaluationType, evaluationType);

  m_xColumn = m_columnManager->addProperty("XColumn");
  m_yColumn = m_columnManager->addProperty("YColumn");
  m_errColumn = m_columnManager->addProperty("ErrColumn");

  settingsGroup->addSubProperty(m_workspace);
  settingsGroup->addSubProperty(m_startX);
  settingsGroup->addSubProperty(m_endX);

  // Only include the cost function when in the dock widget inside mantid plot,
  // not on muon analysis widget
  // Include minimiser and plot difference under a different settings section.
  settingsGroup->addSubProperty(m_output);
  settingsGroup->addSubProperty(m_minimizer);
  settingsGroup->addSubProperty(m_ignoreInvalidData);
  settingsGroup->addSubProperty(m_costFunction);
  settingsGroup->addSubProperty(m_maxIterations);
  settingsGroup->addSubProperty(m_peakRadius);
  settingsGroup->addSubProperty(m_plotDiff);
  settingsGroup->addSubProperty(m_plotCompositeMembers);
  settingsGroup->addSubProperty(m_convolveMembers);
  settingsGroup->addSubProperty(m_showParamErrors);
  settingsGroup->addSubProperty(m_evaluationType);

  /* Create editors and assign them to the managers */
  createEditors(w);

  updateDecimals();

  m_functionsGroup = m_browser->addProperty(functionsGroup);
  m_settingsGroup = m_browser->addProperty(settingsGroup);

  initLayout(w);
}

/**
 * @brief Initialise the layout.
 * This initialization includes:
 *   1. SIGNALs/SLOTs when properties change.
 *   2. Action menus and associated SIGNALs/SLOTs.
 *   3. Initialize the CompositeFunction, the root from which to build the
 * Model. 4. Update the list of available functions
 * @param w widget parenting the action menus and the property tree browser
 */
void FitPropertyBrowser::initLayout(QWidget *w) { initBasicLayout(w); }

/**
 * @brief Initialise the layout for the fit button.
 * This initialization includes:
 *   1. SIGNALs/SLOTs when properties change.
 *   2. Actions and associated SIGNALs/SLOTs.
 * @param w widget parenting the action menus and the property tree browser
 * @return push botton for the fit menu
 */
QPushButton *FitPropertyBrowser::createFitMenuButton(QWidget *w) {
  QPushButton *btnFit = new QPushButton("Fit");
  m_tip = new QLabel("", w);

  m_fitMapper = new QSignalMapper(this);
  m_fitMenu = new QMenu(this);
  populateFitMenuButton(m_fitMapper, m_fitMenu);
  connect(m_fitMapper, SIGNAL(mapped(const QString &)), this,
          SLOT(executeFitMenu(const QString &)));
  btnFit->setMenu(m_fitMenu);
  return btnFit;
}

/**
 * @brief Populate the fit button.
 * This initialization includes:
 *   1. SIGNALs/SLOTs when properties change.
 *   2. Actions and associated SIGNALs/SLOTs.
 * @param fitMapper the QMap to the fit mapper
 * @param fitMenu the QMenu for the fit button
 */
void FitPropertyBrowser::populateFitMenuButton(QSignalMapper *fitMapper,
                                               QMenu *fitMenu) {
  // assert(fitmapper);

  m_fitActionFit = new QAction("Fit", this);
  m_fitActionSeqFit = new QAction("Sequential Fit", this);
  m_fitActionUndoFit = new QAction("Undo Fit", this);
  m_fitActionEvaluate = new QAction("Evaluate function", this);

  fitMapper->setMapping(m_fitActionFit, "Fit");
  fitMapper->setMapping(m_fitActionSeqFit, "SeqFit");
  fitMapper->setMapping(m_fitActionUndoFit, "UndoFit");
  fitMapper->setMapping(m_fitActionEvaluate, "Evaluate");

  connect(m_fitActionFit, SIGNAL(triggered()), fitMapper, SLOT(map()));
  connect(m_fitActionSeqFit, SIGNAL(triggered()), fitMapper, SLOT(map()));
  connect(m_fitActionUndoFit, SIGNAL(triggered()), fitMapper, SLOT(map()));
  connect(m_fitActionEvaluate, SIGNAL(triggered()), fitMapper, SLOT(map()));

  fitMenu->addAction(m_fitActionFit);
  fitMenu->addAction(m_fitActionSeqFit);
  fitMenu->addAction(m_fitActionEvaluate);
  fitMenu->addSeparator();
  fitMenu->addAction(m_fitActionUndoFit);
  fitMenu->addSeparator();
}
/**
 * @brief Initialise the layout, except for the fit button in the menu bar.
 * This initialization includes:
 *   1. SIGNALs/SLOTs when properties change.
 *   2. Action menus and associated SIGNALs/SLOTs.
 *   3. Initialize the CompositeFunction, the root from which to build the
 * Model. 4. Update the list of available functions
 * @param w widget parenting the action menus and the property tree browser
 */
void FitPropertyBrowser::initBasicLayout(QWidget *w) {
  QPushButton *btnFit = createFitMenuButton(w);
  btnFit->setObjectName("button_Fit");
  // to be able to change windows title from tread
  connect(this, SIGNAL(changeWindowTitle(const QString &)), this,
          SLOT(setWindowTitle(const QString &)));

  /* Create the top level group */

  m_groupManager->addProperty("Fit");

  connect(m_enumManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(enumChanged(QtProperty *)));
  connect(m_boolManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(boolChanged(QtProperty *)));
  connect(m_intManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(intChanged(QtProperty *)));
  connect(m_doubleManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(doubleChanged(QtProperty *)));
  connect(m_stringManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(stringChanged(QtProperty *)));
  connect(m_filenameManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(stringChanged(QtProperty *)));
  connect(m_formulaManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(stringChanged(QtProperty *)));
  connect(m_columnManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(columnChanged(QtProperty *)));
  connect(m_vectorDoubleManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(vectorDoubleChanged(QtProperty *)));
  connect(m_parameterManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(parameterChanged(QtProperty *)));
  connect(m_vectorSizeManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(vectorSizeChanged(QtProperty *)));

  QVBoxLayout *layout = new QVBoxLayout(w);
  layout->setObjectName("vlayout");
  QGridLayout *buttonsLayout = new QGridLayout();

  QPushButton *btnDisplay = new QPushButton("Display");
  btnDisplay->setObjectName("button_Display");
  QMenu *displayMenu = new QMenu(this);
  displayMenu->setObjectName("menu_Display");
  m_displayActionPlotGuess = new QAction("Plot Guess", this);
  m_displayActionPlotGuess->setEnabled(false);
  m_displayActionQuality = new QAction("Quality", this);
  m_displayActionQuality->setObjectName("action_Quality");
  m_displayActionQuality->setCheckable(true);
  m_displayActionQuality->setChecked(true);
  m_displayActionClearAll = new QAction("Clear fit curves", this);
  QSignalMapper *displayMapper = new QSignalMapper(this);
  displayMapper->setObjectName("mapper_Display");

  displayMapper->setMapping(m_displayActionPlotGuess, "PlotGuess");
  displayMapper->setMapping(m_displayActionQuality, "Quality");
  displayMapper->setMapping(m_displayActionClearAll, "ClearAll");
  connect(m_displayActionPlotGuess, SIGNAL(triggered()), displayMapper,
          SLOT(map()));
  connect(m_displayActionQuality, SIGNAL(triggered()), displayMapper,
          SLOT(map()));
  connect(m_displayActionClearAll, SIGNAL(triggered()), displayMapper,
          SLOT(map()));
  connect(displayMapper, SIGNAL(mapped(const QString &)), this,
          SLOT(executeDisplayMenu(const QString &)));
  displayMenu->addAction(m_displayActionPlotGuess);
  displayMenu->addAction(m_displayActionClearAll);
  displayMenu->addAction(m_displayActionQuality);
  btnDisplay->setMenu(displayMenu);

  QPushButton *btnSetup = new QPushButton("Setup");
  btnSetup->setObjectName("button_Setup");
  QMenu *setupMenu = new QMenu(this);
  setupMenu->setObjectName("menu_Setup");

  m_setupActionCustomSetup = new QAction("Custom Setup", this);
  QAction *setupActionManageSetup = new QAction("Manage Setup", this);
  setupActionManageSetup->setObjectName("action_ManageSetup");
  QAction *setupActionFindPeaks = new QAction("Find Peaks", this);
  setupActionFindPeaks->setObjectName("action_FindPeaks");
  QAction *setupActionClearFit = new QAction("Clear Model", this);
  setupActionClearFit->setObjectName("action_ClearModel");

  QMenu *setupSubMenuCustom = new QMenu(this);
  m_setupActionCustomSetup->setMenu(setupSubMenuCustom);
  // empty menu for now, so set it disabled to avoid confusing users
  m_setupActionCustomSetup->setEnabled(false);

  QMenu *setupSubMenuManage = new QMenu(this);
  QAction *setupActionSave = new QAction("Save Setup", this);
  m_setupActionRemove = new QAction("Remove Setup", this);
  QAction *setupActionCopyToClipboard = new QAction("Copy To Clipboard", this);
  setupActionCopyToClipboard->setObjectName("action_CopyToClipboard");
  QAction *setupActionLoadFromString = new QAction("Load From String", this);
  setupActionLoadFromString->setObjectName("action_LoadFromString");
  QSignalMapper *setupManageMapper = new QSignalMapper(this);
  setupManageMapper->setMapping(setupActionSave, "SaveSetup");
  setupManageMapper->setMapping(setupActionCopyToClipboard, "CopyToClipboard");
  setupManageMapper->setMapping(setupActionLoadFromString, "LoadFromString");
  connect(setupActionSave, SIGNAL(triggered()), setupManageMapper, SLOT(map()));
  connect(setupActionCopyToClipboard, SIGNAL(triggered()), setupManageMapper,
          SLOT(map()));
  connect(setupActionLoadFromString, SIGNAL(triggered()), setupManageMapper,
          SLOT(map()));
  connect(setupManageMapper, SIGNAL(mapped(const QString &)), this,
          SLOT(executeSetupManageMenu(const QString &)));
  setupSubMenuManage->addAction(setupActionSave);
  setupSubMenuManage->addAction(m_setupActionRemove);
  setupSubMenuManage->addAction(setupActionCopyToClipboard);
  setupSubMenuManage->addAction(setupActionLoadFromString);
  setupActionManageSetup->setMenu(setupSubMenuManage);

  QMenu *setupSubMenuRemove = new QMenu(this);
  m_setupActionRemove->setMenu(setupSubMenuRemove);
  // empty menu for now, so set it disabled to avoid confusing users
  m_setupActionRemove->setEnabled(false);

  QSignalMapper *setupMapper = new QSignalMapper(this);
  setupMapper->setMapping(setupActionClearFit, "ClearFit");
  setupMapper->setMapping(setupActionFindPeaks, "FindPeaks");
  connect(setupActionClearFit, SIGNAL(triggered()), setupMapper, SLOT(map()));
  connect(setupActionFindPeaks, SIGNAL(triggered()), setupMapper, SLOT(map()));
  connect(setupMapper, SIGNAL(mapped(const QString &)), this,
          SLOT(executeSetupMenu(const QString &)));

  setupMenu->addAction(m_setupActionCustomSetup);
  setupMenu->addAction(setupActionManageSetup);
  setupMenu->addSeparator();
  setupMenu->addAction(setupActionFindPeaks);
  setupMenu->addSeparator();
  setupMenu->addAction(setupActionClearFit);
  btnSetup->setMenu(setupMenu);

  updateSetupMenus();

  buttonsLayout->addWidget(btnFit, 0, 0);
  buttonsLayout->addWidget(btnDisplay, 0, 1);
  buttonsLayout->addWidget(btnSetup, 0, 2);

  m_status = new QLabel("Status:", w);
  m_status->hide();
  connect(this, SIGNAL(fitResultsChanged(const QString &)), this,
          SLOT(showFitResultStatus(const QString &)), Qt::QueuedConnection);
  layout->addWidget(m_status);
  layout->addLayout(buttonsLayout);
  layout->addWidget(m_tip);
  layout->addWidget(m_browser);
  m_browser->setObjectName("tree_browser");

  setWidget(w);

  m_browser->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_browser, SIGNAL(customContextMenuRequested(const QPoint &)), this,
          SLOT(popupMenu(const QPoint &)));
  connect(m_browser, SIGNAL(currentItemChanged(QtBrowserItem *)), this,
          SLOT(currentItemChanged(QtBrowserItem *)));
  connect(this, SIGNAL(multifitFinished()), this,
          SLOT(processMultiBGResults()));

  createCompositeFunction();

  // Update tooltips when function structure is (or might've been) changed in
  // any way
  connect(this, SIGNAL(functionChanged()), SLOT(updateStructureTooltips()));
  connect(this, SIGNAL(functionChanged()), SLOT(clearFitResultStatus()));

  // Update available functions in this fit property browser, when the function
  // factory changes.
  using Mantid::API::FunctionFactory;
  FunctionFactory::Instance().notificationCenter.addObserver(m_updateObserver);
  connect(this, SIGNAL(functionFactoryUpdateReceived()), this,
          SLOT(populateFunctionNames()));
  FunctionFactory::Instance().enableNotifications();

  // Initial call, as function is not changed when it's created for the first
  // time
  updateStructureTooltips();

  m_changeSlotsEnabled = true;

  populateFunctionNames();
}

/**
 * @brief Create editors and assign them to the managers.
 * Associates a particular widget factory to each property manager. Thus, the
 * factory will automatically create widgets befitting to edit the properties
 * that we define.
 * @param w :: widget showing the properties tree and the actions buttons
 */
void FitPropertyBrowser::createEditors(QWidget *w) {
  QtCheckBoxFactory *checkBoxFactory = new QtCheckBoxFactory(w);
  QtEnumEditorFactory *comboBoxFactory = new QtEnumEditorFactory(w);
  QtSpinBoxFactory *spinBoxFactory = new QtSpinBoxFactory(w);
  DoubleEditorFactory *doubleEditorFactory = new DoubleEditorFactory(w);
  StringEditorFactory *stringEditFactory = new StringEditorFactory(w);
  FilenameDialogEditorFactory *filenameDialogEditorFactory =
      new FilenameDialogEditorFactory(w);
  FormulaDialogEditorFactory *formulaDialogEditFactory =
      new FormulaDialogEditorFactory(w);

  m_browser = new QtTreePropertyBrowser();
  m_browser->setFactoryForManager(m_enumManager, comboBoxFactory);
  m_browser->setFactoryForManager(m_boolManager, checkBoxFactory);
  m_browser->setFactoryForManager(m_intManager, spinBoxFactory);
  m_browser->setFactoryForManager(m_doubleManager, doubleEditorFactory);
  m_browser->setFactoryForManager(m_stringManager, stringEditFactory);
  m_browser->setFactoryForManager(m_filenameManager,
                                  filenameDialogEditorFactory);
  m_browser->setFactoryForManager(m_formulaManager, formulaDialogEditFactory);
  m_browser->setFactoryForManager(m_columnManager, comboBoxFactory);
  m_browser->setFactoryForManager(m_vectorSizeManager, spinBoxFactory);
  m_browser->setFactoryForManager(m_vectorDoubleManager, doubleEditorFactory);
  m_browser->setFactoryForManager(m_parameterManager,
                                  new ParameterEditorFactory(w));
}

/// Update setup menus according to how these are set in
/// settings
void FitPropertyBrowser::updateSetupMenus() {
  QMenu *menuLoad = m_setupActionCustomSetup->menu();
  menuLoad->clear();
  QMenu *menuRemove = m_setupActionRemove->menu();
  menuRemove->clear();

  QSettings settings;
  settings.beginGroup("Mantid/FitBrowser/SavedFunctions");
  QStringList names = settings.childKeys();

  QSignalMapper *mapperLoad = new QSignalMapper(this);
  QSignalMapper *mapperRemove = new QSignalMapper(this);
  // enable actions that open the menus only if there will be >=1 entries in
  // there
  m_setupActionCustomSetup->setEnabled(names.size() >= 1);
  m_setupActionRemove->setEnabled(names.size() >= 1);
  for (int i = 0; i < names.size(); i++) {
    QAction *itemLoad = new QAction(names.at(i), this);
    QAction *itemRemove = new QAction(names.at(i), this);
    mapperLoad->setMapping(itemLoad, names.at(i));
    mapperRemove->setMapping(itemRemove, names.at(i));
    connect(itemLoad, SIGNAL(triggered()), mapperLoad, SLOT(map()));
    connect(itemRemove, SIGNAL(triggered()), mapperRemove, SLOT(map()));
    menuLoad->addAction(itemLoad);
    menuRemove->addAction(itemRemove);
  }
  connect(mapperLoad, SIGNAL(mapped(const QString &)), this,
          SLOT(executeCustomSetupLoad(const QString &)));
  connect(mapperRemove, SIGNAL(mapped(const QString &)), this,
          SLOT(executeCustomSetupRemove(const QString &)));
}

void FitPropertyBrowser::executeCustomSetupLoad(const QString &name) {
  QSettings settings;
  settings.beginGroup("Mantid/FitBrowser/SavedFunctions");
  QStringList names = settings.childKeys();

  QString str = settings.value(name).toString();
  loadFunction(str);
}

void FitPropertyBrowser::executeCustomSetupRemove(const QString &name) {
  QSettings settings;
  settings.beginGroup("Mantid/FitBrowser/SavedFunctions");
  QStringList names = settings.childKeys();

  settings.remove(name);
  updateSetupMenus();
}

/**
 * Recursively updates structure tooltips for all the functions
 */
void FitPropertyBrowser::updateStructureTooltips() {
  // Call tooltip update func on the root handler - it goes down recursively
  getHandler()->updateStructureTooltip();
}

void FitPropertyBrowser::executeFitMenu(const QString &item) {
  if (item == "Fit") {
    fit();
  } else if (item == "SeqFit") {
    sequentialFit();
  } else if (item == "UndoFit") {
    undoFit();
  } else if (item == "Evaluate") {
    doFit(0);
  }
}

void FitPropertyBrowser::executeDisplayMenu(const QString &item) {
  if (item == "PlotGuess") {
    plotOrRemoveGuessAll();
  } else if (item == "ClearAll") {
    clearAllPlots();
  }
}

void FitPropertyBrowser::executeSetupMenu(const QString &item) {
  if (item == "ClearFit")
    clear();
  if (item == "FindPeaks")
    findPeaks();
}

void FitPropertyBrowser::executeSetupManageMenu(const QString &item) {
  if (item == "SaveSetup")
    saveFunction();
  if (item == "CopyToClipboard")
    copy();
  if (item == "LoadFromString")
    loadFunctionFromString();
}

/// Destructor
FitPropertyBrowser::~FitPropertyBrowser() { m_compositeFunction.reset(); }

/// Get handler to the root composite function
PropertyHandler *FitPropertyBrowser::getHandler() const {
  return static_cast<PropertyHandler *>(m_compositeFunction->getHandler());
}

PropertyHandler *FitPropertyBrowser::addFunction(const std::string &fnName) {
  PropertyHandler *h = getHandler()->addFunction(fnName);
  emit functionChanged();
  return h;
}

void FitPropertyBrowser::removeFunction(PropertyHandler *handler) {
  if (handler) {
    emit removePlotSignal(getHandler());
    handler->removeFunction();
    compositeFunction()->checkFunction();
    emit functionRemoved();
    emit functionChanged();
  }
}

/** Slot. Called to add a new function
 */
void FitPropertyBrowser::addFunction() {
  QtBrowserItem *ci = m_browser->currentItem();
  // Find the function which has ci as its top browser item
  auto cf = getHandler()->findCompositeFunction(ci);
  if (!cf)
    return;

  // Declare new widget for picking fit functions
  m_fitSelector = new QDialog();
  m_fitSelector->setModal(true);
  // QTreeWidget *m_fitTree = new QTreeWidget();
  m_fitTree = new QTreeWidget;

  // Add functions to each of the categories. If it appears in more than one
  // category then add to both
  // Store in a map. Key = category. Value = vector of fit functions belonging
  // to that category.
  std::map<std::string, std::vector<std::string>> categories;
  for (int i = 0; i < m_registeredFunctions.size(); ++i) {
    boost::shared_ptr<Mantid::API::IFunction> f =
        Mantid::API::FunctionFactory::Instance().createFunction(
            m_registeredFunctions[i].toStdString());
    std::vector<std::string> tempCategories = f->categories();
    for (size_t j = 0; j < tempCategories.size(); ++j) {
      categories[tempCategories[boost::lexical_cast<int>(j)]].push_back(
          m_registeredFunctions[i].toStdString());
    }
  }

  // Construct the QTreeWidget based on the map information of categories and
  // their respective fit functions.
  std::map<std::string, std::vector<std::string>>::const_iterator sItr =
      categories.end();
  for (std::map<std::string, std::vector<std::string>>::const_iterator itr =
           categories.begin();
       itr != sItr; ++itr) {
    QTreeWidgetItem *category = new QTreeWidgetItem(m_fitTree);
    category->setText(0, QString::fromStdString(itr->first));

    std::vector<std::string>::const_iterator fitItrEnd = itr->second.end();
    for (std::vector<std::string>::const_iterator fitItrBegin =
             itr->second.begin();
         fitItrBegin != fitItrEnd; ++fitItrBegin) {
      QTreeWidgetItem *fit = new QTreeWidgetItem(category);
      fit->setText(0, QString::fromStdString(fitItrBegin[0]));
    }
  }

  // Set the layout of the widget.
  m_fitTree->setToolTip("Select a function type and press OK.");
  m_fitTree->setHeaderLabel("Fit - Select function type");
  QDialogButtonBox *buttonBox =
      new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(acceptFit()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(closeFit()));
  connect(m_fitTree, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this,
          SLOT(acceptFit()));
  QVBoxLayout *layout = new QVBoxLayout();
  layout->addWidget(m_fitTree);
  layout->addWidget(buttonBox);
  m_fitSelector->setLayout(layout);
  m_fitSelector->show();
}

void FitPropertyBrowser::acceptFit() {
  QtBrowserItem *ci = m_browser->currentItem();
  boost::shared_ptr<const Mantid::API::CompositeFunction> cf =
      getHandler()->findCompositeFunction(ci);
  if (!cf)
    return;

  QList<QTreeWidgetItem *> items(m_fitTree->selectedItems());
  if (items.size() != 1)
    return;

  if (items[0]->parent() == nullptr)
    return;

  PropertyHandler *h = getHandler()->findHandler(cf);
  h->addFunction(items[0]->text(0).toStdString());
  emit functionChanged();

  closeFit();
}

void FitPropertyBrowser::closeFit() { m_fitSelector->close(); }

/**
 * Create CompositeFunction from function pointer
 * @param func :: [input] Pointer to function
 */
void FitPropertyBrowser::createCompositeFunction(
    const Mantid::API::IFunction_sptr func) {
  if (m_compositeFunction) {
    emit functionRemoved();
    m_autoBackground = nullptr;
  }
  if (!func) {
    m_compositeFunction.reset(new Mantid::API::CompositeFunction);
  } else {
    auto cf = boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(func);
    if (!cf || (cf->name() != "CompositeFunction" && cf->name() != "MultiBG" &&
                cf->name() != "MultiDomainFunction")) {
      m_compositeFunction.reset(new Mantid::API::CompositeFunction);
      m_compositeFunction->addFunction(func);
    } else {
      m_compositeFunction = cf;
    }
  }
  setWorkspace(m_compositeFunction);

  auto h = std::make_unique<PropertyHandler>(
      m_compositeFunction, Mantid::API::CompositeFunction_sptr(), this);
  m_compositeFunction->setHandler(std::move(h));
  setCurrentFunction(
      static_cast<PropertyHandler *>(m_compositeFunction->getHandler()));

  if (m_auto_back) {
    addAutoBackground();
  }

  disableUndo();
  setFitEnabled(m_compositeFunction->nFunctions() > 0);
  emit functionChanged();
}

/**
 * Create CompositeFunction from string
 * @param str :: [input] Function string
 */
void FitPropertyBrowser::createCompositeFunction(const QString &str) {
  if (str.isEmpty()) {
    createCompositeFunction(Mantid::API::IFunction_sptr());
  } else {
    auto f = tryCreateFitFunction(str);
    if (f) {
      createCompositeFunction(f);
    } else {
      createCompositeFunction(Mantid::API::IFunction_sptr());
    }
  }
}

Mantid::API::IFunction_sptr
FitPropertyBrowser::tryCreateFitFunction(const QString &str) {
  try {
    return Mantid::API::FunctionFactory::Instance().createInitialized(
        str.toStdString());
  } catch (const std::exception &ex) {
    QMessageBox::critical(this, "Mantid - Error",
                          "Unexpected exception caught:\n\n" +
                              QString(ex.what()));
    return nullptr;
  }
}

void FitPropertyBrowser::popupMenu(const QPoint &) {
  QtBrowserItem *ci = m_browser->currentItem();
  if (!ci)
    return;
  QMenu *menu = new QMenu(this);
  QAction *action;

  bool isFunctionsGroup = ci == m_functionsGroup;
  bool isSettingsGroup = ci == m_settingsGroup;
  bool isASetting = ci->parent() == m_settingsGroup;
  bool isFunction = getHandler()->findFunction(ci) != nullptr;
  bool isCompositeFunction =
      isFunction && getHandler()->findCompositeFunction(ci);

  PropertyHandler *h = getHandler()->findHandler(ci->property());

  if (isFunctionsGroup) {
    action = new QAction("Add function", this);
    connect(action, SIGNAL(triggered()), this, SLOT(addFunction()));
    menu->addAction(action);

    if (m_compositeFunction->name() == "MultiBG" &&
        m_compositeFunction->nFunctions() == 1 &&
        Mantid::API::AnalysisDataService::Instance().doesExist(
            workspaceName())) {
      action = new QAction("Setup multifit", this);
      connect(action, SIGNAL(triggered()), this, SLOT(setupMultifit()));
      menu->addAction(action);
    }

    if (m_peakToolOn) {
      if (h && h->hasPlot()) {
        action = new QAction("Remove plot", this);
        connect(action, SIGNAL(triggered()), this, SLOT(removeGuessAll()));
        menu->addAction(action);
      } else {
        action = new QAction("Plot", this);
        connect(action, SIGNAL(triggered()), this, SLOT(plotGuessAll()));
        menu->addAction(action);
      }
    }

    menu->addSeparator();

    action = new QAction("Save", this);
    connect(action, SIGNAL(triggered()), this, SLOT(saveFunction()));
    menu->addAction(action);

    action = new QAction("Load", this);
    connect(action, SIGNAL(triggered()), this, SLOT(loadFunction()));
    menu->addAction(action);

    action = new QAction("Copy To Clipboard", this);
    connect(action, SIGNAL(triggered()), this, SLOT(copy()));
    menu->addAction(action);

    menu->addSeparator();

    action = new QAction("Help", this);
    connect(action, SIGNAL(triggered()), this, SLOT(browserHelp()));
    menu->addAction(action);
  } else if (isFunctionsGroup || isSettingsGroup || isASetting) {
    if (isFitEnabled()) {
      action = new QAction("Fit", this);
      connect(action, SIGNAL(triggered()), this, SLOT(fit()));
      menu->addAction(action);
    }

    if (isUndoEnabled()) {
      action = new QAction("Undo Fit", this);
      connect(action, SIGNAL(triggered()), this, SLOT(undoFit()));
      menu->addAction(action);
    }

    action = new QAction("Clear all", this);
    connect(action, SIGNAL(triggered()), this, SLOT(clear()));
    menu->addAction(action);

    action = new QAction("Help", this);
    connect(action, SIGNAL(triggered()), this, SLOT(browserHelp()));
    menu->addAction(action);

  } else if (isFunction) {
    if (isCompositeFunction) {
      action = new QAction("Add function", this);
      connect(action, SIGNAL(triggered()), this, SLOT(addFunction()));
      menu->addAction(action);
    }

    action = new QAction("Remove", this);
    connect(action, SIGNAL(triggered()), this, SLOT(deleteFunction()));
    menu->addAction(action);

    if (m_peakToolOn) {
      if (h && h->hasPlot()) {
        action = new QAction("Remove plot", this);
        connect(action, SIGNAL(triggered()), this, SLOT(removeGuessCurrent()));
        menu->addAction(action);
      } else {
        action = new QAction("Plot", this);
        connect(action, SIGNAL(triggered()), this, SLOT(plotGuessCurrent()));
        menu->addAction(action);
      }
    }

    action = new QAction("Help", this);
    connect(action, SIGNAL(triggered()), this, SLOT(functionHelp()));
    menu->addAction(action);

    menu->addSeparator();
  } else if (h) {
    bool isParameter = h->isParameter(ci->property());
    bool isTie = !isParameter && ci->property()->propertyName() == "Tie";
    bool isLowerBound =
        !isParameter && ci->property()->propertyName() == "Lower Bound";
    bool isUpperBound =
        !isParameter && ci->property()->propertyName() == "Upper Bound";
    bool isType = isParameter && ci->property()->propertyName() == "Type";
    if (isType) {
      isParameter = false;
    }
    if (isTie) {
      action = new QAction("Remove", this);
      connect(action, SIGNAL(triggered()), this, SLOT(deleteTie()));
      menu->addAction(action);
    } else if (isLowerBound || isUpperBound) {
      action = new QAction("Remove", this);
      connect(action, SIGNAL(triggered()), this, SLOT(removeBounds()));
      menu->addAction(action);
    } else if (count() > 0 && isParameter) {
      bool hasTies;
      bool hasBounds;
      hasConstraints(ci->property(), hasTies, hasBounds);

      if (!hasTies && !hasBounds) {
        action = new QAction("Fix", this);
        connect(action, SIGNAL(triggered()), this, SLOT(addFixTie()));
        menu->addAction(action);
      }

      if (!hasTies) {
        QMenu *constraintMenu = menu->addMenu("Constraint");

        QMenu *detailMenu = constraintMenu->addMenu("Lower Bound");

        action = new QAction("10%", this);
        connect(action, SIGNAL(triggered()), this, SLOT(addLowerBound10()));
        detailMenu->addAction(action);

        action = new QAction("50%", this);
        connect(action, SIGNAL(triggered()), this, SLOT(addLowerBound50()));
        detailMenu->addAction(action);

        action = new QAction("Custom", this);
        connect(action, SIGNAL(triggered()), this, SLOT(addLowerBound()));
        detailMenu->addAction(action);
        detailMenu = constraintMenu->addMenu("Upper Bound");

        action = new QAction("10%", this);
        connect(action, SIGNAL(triggered()), this, SLOT(addUpperBound10()));
        detailMenu->addAction(action);

        action = new QAction("50%", this);
        connect(action, SIGNAL(triggered()), this, SLOT(addUpperBound50()));
        detailMenu->addAction(action);

        action = new QAction("Custom", this);
        connect(action, SIGNAL(triggered()), this, SLOT(addUpperBound()));
        detailMenu->addAction(action);
        detailMenu = constraintMenu->addMenu("Both Bounds");

        action = new QAction("10%", this);
        connect(action, SIGNAL(triggered()), this, SLOT(addBothBounds10()));
        detailMenu->addAction(action);

        action = new QAction("50%", this);
        connect(action, SIGNAL(triggered()), this, SLOT(addBothBounds50()));
        detailMenu->addAction(action);

        action = new QAction("Custom", this);
        connect(action, SIGNAL(triggered()), this, SLOT(addBothBounds()));
        detailMenu->addAction(action);
      }

      if (hasBounds) {
        action = new QAction("Remove constraints", this);
        connect(action, SIGNAL(triggered()), this, SLOT(removeBounds()));
        menu->addAction(action);
      }

      if (!hasTies && !hasBounds) {
        if (count() == 1) {
          action = new QAction("Tie", this);
          connect(action, SIGNAL(triggered()), this, SLOT(addTie()));
          menu->addAction(action);
        } else {
          QMenu *detail = menu->addMenu("Tie");

          action = new QAction("To function", this);
          connect(action, SIGNAL(triggered()), this, SLOT(addTieToFunction()));
          detail->addAction(action);

          action = new QAction("Custom Tie", this);
          connect(action, SIGNAL(triggered()), this, SLOT(addTie()));
          detail->addAction(action);
        }
      } else if (hasTies) {
        action = new QAction("Remove tie", this);
        connect(action, SIGNAL(triggered()), this, SLOT(deleteTie()));
        menu->addAction(action);
      }
    }
  }

  menu->popup(QCursor::pos());
}

/** Slot. Called to remove a function
 */
void FitPropertyBrowser::deleteFunction() {
  QtBrowserItem *ci = m_browser->currentItem();
  PropertyHandler *h = getHandler()->findHandler(ci->property());
  removeFunction(h);
}

//***********************************************************************************//

// Get the default function name
std::string FitPropertyBrowser::defaultFunctionType() const {
  return m_defaultFunction;
}

// Get the default function name
void FitPropertyBrowser::setDefaultFunctionType(const std::string &fnType) {
  m_defaultFunction = fnType;
}

/// Get the default peak type
std::string FitPropertyBrowser::defaultPeakType() const {
  return m_defaultPeak;
}
/// Set the default peak type
void FitPropertyBrowser::setDefaultPeakType(const std::string &fnType) {
  m_defaultPeak = fnType;
  setDefaultFunctionType(fnType);
  Mantid::Kernel::ConfigService::Instance().setString(
      "curvefitting.defaultPeak", fnType);
}
/// Get the default background type
std::string FitPropertyBrowser::defaultBackgroundType() const {
  return m_defaultBackground;
}
/// Set the default background type
void FitPropertyBrowser::setDefaultBackgroundType(const std::string &fnType) {
  m_defaultBackground = fnType;
  setDefaultFunctionType(fnType);
}

boost::shared_ptr<Mantid::API::Workspace>
FitPropertyBrowser::getWorkspace() const {
  std::string wsName = workspaceName();
  if (wsName.empty())
    return boost::shared_ptr<Mantid::API::Workspace>();
  try {
    if (m_settingsGroup->property()->subProperties().contains(m_xColumn)) {
      return createMatrixFromTableWorkspace();
    }

    return Mantid::API::AnalysisDataService::Instance().retrieve(wsName);
  } catch (...) {
    return boost::shared_ptr<Mantid::API::Workspace>();
  }
}

/// Get the input workspace name
std::string FitPropertyBrowser::workspaceName() const {
  int i = m_enumManager->value(m_workspace);
  std::string res = "";
  if (i >= 0) {
    res = m_workspaceNames[i].toStdString();
  }
  return res;
}

/// Set the input workspace name
void FitPropertyBrowser::setWorkspaceName(const QString &wsName) {
  int i = m_workspaceNames.indexOf(wsName);
  if (i < 0) {
    // workspace may not be found because add notification hasn't been
    // processed yet
    populateWorkspaceNames();
    i = m_workspaceNames.indexOf(wsName);
  }
  if (i >= 0) {
    m_enumManager->setValue(m_workspace, i);
    Mantid::API::MatrixWorkspace_sptr mws;
    try {
      mws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
          Mantid::API::AnalysisDataService::Instance().retrieve(
              wsName.toStdString()));
    } catch (Mantid::Kernel::Exception::NotFoundError &) {
    }
    if (mws) {
      size_t wi = static_cast<size_t>(workspaceIndex());
      if (wi < mws->getNumberHistograms() && !mws->x(wi).empty()) {
        setStartX(mws->x(wi).front());
        setEndX(mws->x(wi).back());
      }
    }
  }
  setWorkspaceIndex(0);
}

/// Get workspace index
int FitPropertyBrowser::workspaceIndex() const {
  return m_intManager->value(m_workspaceIndex);
}

/// Set workspace index
void FitPropertyBrowser::setWorkspaceIndex(int i) {
  try {
    auto const index = getAllowedIndex(i);
    m_intManager->setValue(m_workspaceIndex, index);
  } catch (Mantid::Kernel::Exception::NotFoundError &) {
    // ignore this error
  }
}

/// Get the output name
std::string FitPropertyBrowser::outputName() const {
  return m_stringManager->value(m_output).toStdString();
}

/// Set the output name
void FitPropertyBrowser::setOutputName(const std::string &name) {
  m_stringManager->setValue(m_output, QString::fromStdString(name));
}

/// Get the minimizer
std::string FitPropertyBrowser::minimizer(bool withProperties) const {
  int i = m_enumManager->value(m_minimizer);
  QString minimStr = m_minimizers[i];
  // append minimizer properties as name=value pairs
  if (withProperties) {
    foreach (QtProperty *prop, m_minimizerProperties) {
      if (prop->propertyManager() == m_stringManager) {
        QString value = m_stringManager->value(prop);
        if (!value.isEmpty()) {
          minimStr += "," + prop->propertyName() + "=" + value;
        }
      } else {
        minimStr += "," + prop->propertyName() + "=";
        if (prop->propertyManager() == m_intManager) {
          minimStr += QString::number(m_intManager->value(prop));
        } else if (prop->propertyManager() == m_doubleManager) {
          minimStr += QString::number(m_doubleManager->value(prop));
        } else if (prop->propertyManager() == m_boolManager) {
          minimStr += QString::number(m_boolManager->value(prop));
        } else {
          throw std::runtime_error("The fit browser doesn't support the type "
                                   "of minimizer's property " +
                                   prop->propertyName().toStdString());
        }
      }
    }
  }
  return minimStr.toStdString();
}

/// Get the ignore invalid data option
bool FitPropertyBrowser::ignoreInvalidData() const {
  return m_boolManager->value(m_ignoreInvalidData);
}

/// Set the ignore invalid data option
void FitPropertyBrowser::setIgnoreInvalidData(bool on) {
  m_boolManager->setValue(m_ignoreInvalidData, on);
}

/// Get the cost function
std::string FitPropertyBrowser::costFunction() const {
  int i = m_enumManager->value(m_costFunction);
  return m_costFunctions[i].toStdString();
}

/**
 * Get the "ConvolveMembers" option
 */
bool FitPropertyBrowser::convolveMembers() const {
  return m_boolManager->value(m_convolveMembers);
}

/// Get "HistogramFit" option
bool FitPropertyBrowser::isHistogramFit() const {
  if (!m_evaluationType->isEnabled()) {
    return false;
  }
  int i = m_enumManager->value(m_evaluationType);
  return m_evaluationTypes[i].toStdString() == "Histogram";
}

/// Get the max number of iterations
int FitPropertyBrowser::maxIterations() const {
  return m_intManager->value(m_maxIterations);
}

/// Get the peak radius for peak functions
int FitPropertyBrowser::getPeakRadius() const {
  return m_intManager->value(m_peakRadius);
}

/// Get the registered function names
void FitPropertyBrowser::populateFunctionNames() {
  const std::vector<std::string> names =
      Mantid::API::FunctionFactory::Instance().getFunctionNamesGUI();
  m_registeredFunctions.clear();
  m_registeredPeaks.clear();
  m_registeredBackgrounds.clear();
  m_registeredOther.clear();

  const auto &functionFactory = Mantid::API::FunctionFactory::Instance();
  for (const auto &fnName : names) {
    if (fnName == "MultiBG")
      continue;
    Mantid::API::IFunction_sptr function;
    try {
      function = functionFactory.createFunction(fnName);
    } catch (std::exception &exc) {
      g_log.warning() << "Unable to create " << fnName << ": " << exc.what()
                      << "\n";
      continue;
    }
    QString qfnName = QString::fromStdString(fnName);
    m_registeredFunctions << qfnName;
    if (dynamic_cast<Mantid::API::IPeakFunction *>(function.get())) {
      m_registeredPeaks << qfnName;
    } else if (dynamic_cast<Mantid::API::IBackgroundFunction *>(
                   function.get())) {
      m_registeredBackgrounds << qfnName;
    } else {
      m_registeredOther << qfnName;
    }
  }
}

/** Called when the function name property changed
 * @param prop :: A pointer to the function name property m_functionName
 */
void FitPropertyBrowser::enumChanged(QtProperty *prop) {
  if (!m_changeSlotsEnabled)
    return;

  bool storeSettings = false;
  if (prop == m_workspace) {
    workspaceChange(QString::fromStdString(workspaceName()));
    setWorkspaceProperties();
    m_storedWorkspaceName = workspaceName();
  } else if (prop->propertyName() == "Type") {
    disableUndo();
    PropertyHandler *h = getHandler()->findHandler(prop);
    if (!h)
      return;
    // if (!h->parentHandler()) return;
    auto f = h->changeType(prop);
    if (!h->parentHandler()) {
      m_compositeFunction =
          boost::dynamic_pointer_cast<Mantid::API::CompositeFunction>(f);
    }
    if (f)
      setCurrentFunction(f);
    emit functionChanged();

  } else if (prop == m_minimizer) {
    minimizerChanged();
  } else if (prop == m_evaluationType) {
    storeSettings = true;
  }

  if (storeSettings) {
    QSettings settings;
    settings.beginGroup("Mantid/FitBrowser");
    auto val = m_enumManager->value(prop);
    settings.setValue(prop->propertyName(), val);
  }
}

/** Called when a bool property changed
 * @param prop :: A pointer to the property
 */
void FitPropertyBrowser::boolChanged(QtProperty *prop) {
  if (!m_changeSlotsEnabled)
    return;

  if (prop == m_plotDiff || prop == m_plotCompositeMembers ||
      prop == m_ignoreInvalidData || prop == m_showParamErrors) {
    bool val = m_boolManager->value(prop);

    QSettings settings;
    settings.beginGroup("Mantid/FitBrowser");
    settings.setValue(prop->propertyName(), val);

    if (prop == m_showParamErrors) {
      m_parameterManager->setErrorsEnabled(val);
      emit errorsEnabled(val);
    }
  } else { // it could be an attribute
    PropertyHandler *h = getHandler()->findHandler(prop);
    if (!h)
      return;
    h->setAttribute(prop);
  }
}

/** Called when an int property changed
 * @param prop :: A pointer to the property
 */
void FitPropertyBrowser::intChanged(QtProperty *prop) {
  if (!m_changeSlotsEnabled)
    return;

  if (prop == m_workspaceIndex) {
    auto const currentIndex = workspaceIndex();
    auto const allowedIndex = getAllowedIndex(currentIndex);
    if (allowedIndex != currentIndex) {
      setWorkspaceIndex(allowedIndex);
      emit workspaceIndexChanged(allowedIndex);
    }
    m_oldWorkspaceIndex = allowedIndex;
  } else if (prop->propertyName() == "Workspace Index") {
    PropertyHandler *h = getHandler()->findHandler(prop);
    if (!h)
      return;
    h->setFunctionWorkspace();
  } else if (prop->propertyName() == "WorkspaceIndex") {
    PropertyHandler *h = getHandler()->findHandler(prop);
    auto const index = prop->valueText().toInt();
    if (h && index != workspaceIndex()) {
      h->setAttribute(prop);
      setWorkspaceIndex(index);
      emit workspaceIndexChanged(index);
      emit updatePlotSpectrum(index);
    }
  } else if (prop == m_maxIterations || prop == m_peakRadius) {
    QSettings settings;
    settings.beginGroup("Mantid/FitBrowser");
    int val = m_intManager->value(prop);
    settings.setValue(prop->propertyName(), val);
    if (prop == m_peakRadius) {
      sendParameterChanged(m_compositeFunction.get());
    }
  } else { // it could be an attribute
    PropertyHandler *h = getHandler()->findHandler(prop);
    if (!h)
      return;
    h->setAttribute(prop);
  }
}

/** Called when a double property changed
 * @param prop :: A pointer to the property
 */
void FitPropertyBrowser::doubleChanged(QtProperty *prop) {
  if (!m_changeSlotsEnabled)
    return;

  double value = m_doubleManager->value(prop);
  if (prop == m_startX) {
    // call setWorkspace to change maxX in functions
    setWorkspace(m_compositeFunction);
    m_doubleManager->setMinimum(m_endX, value);
    getHandler()->setAttribute("StartX", value);
    emit startXChanged(startX());
    emit xRangeChanged(startX(), endX());
    return;
  } else if (prop == m_endX) {
    // call setWorkspace to change minX in functions
    setWorkspace(m_compositeFunction);
    m_doubleManager->setMaximum(m_startX, value);
    getHandler()->setAttribute("EndX", value);
    emit endXChanged(endX());
    emit xRangeChanged(startX(), endX());
    return;
  } else { // check if it is a constraint
    PropertyHandler *h = getHandler()->findHandler(prop);
    if (!h)
      return;

    QtProperty *parProp = h->getParameterProperty(prop);
    if (parProp) {
      if (prop->propertyName() == "LowerBound") {
        double loBound = m_doubleManager->value(prop);
        h->addConstraint(parProp, true, false, loBound, 0);
      } else if (prop->propertyName() == "UpperBound") {
        double upBound = m_doubleManager->value(prop);
        h->addConstraint(parProp, false, true, 0, upBound);
      }
    } else { // it could be an attribute
      h->setAttribute(prop);
    }
  }
}

/**
 * Called when one of the parameter values gets changed. This could be caused
 * either by user setting
 * the value or programmatically.
 * @param prop :: Parameter property which value got changed
 */
void FitPropertyBrowser::parameterChanged(QtProperty *prop) {
  if (!m_changeSlotsEnabled)
    return;

  getHandler()->setParameter(prop);
}

/** Called when a string property changed
 * @param prop :: A pointer to the property
 */
void FitPropertyBrowser::stringChanged(QtProperty *prop) {
  if (!m_changeSlotsEnabled)
    return;

  if (prop == m_output) {
    std::string oName = outputName();
    if (oName.find_first_not_of(' ') == std::string::npos) {
      setOutputName("");
    } else if (workspaceName() == oName || oName.empty()) {
      m_guessOutputName = true;
    } else {
      m_guessOutputName = false;
    }
  } else if (prop->propertyName() == "Tie") {
    PropertyHandler *h = getHandler()->findHandler(prop);
    if (!h)
      return;

    QtProperty *parProp = h->getParameterProperty(prop);
    if (!parProp)
      return;

    QString parName = h->functionPrefix() + "." + parProp->propertyName();

    QString str = m_stringManager->value(prop);
    Mantid::API::ParameterTie *tie = new Mantid::API::ParameterTie(
        compositeFunction().get(), parName.toStdString());
    try {
      tie->set(str.toStdString());
      h->addTie(parName + "=" + str);
    } catch (...) {
      g_log.warning() << "Failed to update tie on "
                      << parName.toLatin1().constData() << "\n";
    }
    delete tie;
  } else if (getHandler()->setAttribute(prop)) { // setting an attribute may
                                                 // change function parameters
    emit functionChanged();
    return;
  }
}

/** Called when a filename property changed
 * @param prop :: A pointer to the property
 */
void FitPropertyBrowser::filenameChanged(QtProperty *prop) {
  if (!m_changeSlotsEnabled)
    return;

  if (getHandler()->setAttribute(prop)) {
    return;
  }
}
// Centre of the current peak
double FitPropertyBrowser::centre() const {
  if (m_currentHandler && m_currentHandler->pfun()) {
    return m_currentHandler->pfun()->centre();
  }
  return 0;
}

/** Set centre of the current peak
 * @param value :: The new centre value
 */
void FitPropertyBrowser::setCentre(double value) {
  if (m_currentHandler) {
    m_currentHandler->setCentre(value);
    m_currentHandler->updateParameters();
    emit parameterChanged(m_currentHandler->function().get());
  }
}

// Height of the current peak
double FitPropertyBrowser::height() const {
  if (m_currentHandler && m_currentHandler->pfun()) {
    return m_currentHandler->pfun()->height();
  }
  return 0.;
}

/** Set height of the current peak
 * @param value :: The new height value
 */
void FitPropertyBrowser::setHeight(double value) {
  if (m_currentHandler) {
    m_currentHandler->setHeight(value);
    m_currentHandler->updateParameters();
    emit parameterChanged(m_currentHandler->function().get());
  }
}

// Width of the current peak
double FitPropertyBrowser::fwhm() const {
  if (m_currentHandler && m_currentHandler->pfun()) {
    return m_currentHandler->pfun()->fwhm();
  }
  return 0;
}

/** Set width of the current peak
 * @param value :: The new width value
 */
void FitPropertyBrowser::setFwhm(double value) {
  if (m_currentHandler) {
    m_currentHandler->setFwhm(value);
    m_currentHandler->updateParameters();
    emit parameterChanged(m_currentHandler->function().get());
  }
}

/// Get number of functions in CompositeFunction
int FitPropertyBrowser::count() const {
  return static_cast<int>(m_compositeFunction->nFunctions());
}

/// Get the current function
PropertyHandler *FitPropertyBrowser::currentHandler() const {
  return m_currentHandler;
}

/** Set new current function
 * @param h :: New current function
 */
void FitPropertyBrowser::setCurrentFunction(PropertyHandler *h) const {
  m_currentHandler = h;
  if (m_currentHandler) {
    m_browser->setCurrentItem(m_currentHandler->item());
    emit currentChanged();
  }
}

/** Set new current function
 * @param f :: New current function
 */
void FitPropertyBrowser::setCurrentFunction(
    Mantid::API::IFunction_const_sptr f) const {
  setCurrentFunction(getHandler()->findHandler(f));
}

/**
 * Creates an instance of Fit algorithm, sets its properties and launches it.
 */
void FitPropertyBrowser::doFit(int maxIterations) {
  const std::string wsName = workspaceName();

  if (wsName.empty()) {
    QMessageBox::critical(this, "Mantid - Error", "Workspace name is not set");
    return;
  }

  const auto ws = getWorkspace();
  if (!ws) {
    return;
  }

  try {
    m_initialParameters.resize(compositeFunction()->nParams());
    for (size_t i = 0; i < compositeFunction()->nParams(); i++) {
      m_initialParameters[i] = compositeFunction()->getParameter(i);
    }
    m_fitActionUndoFit->setEnabled(true);

    const std::string funStr = getFittingFunction()->asString();

    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create("Fit");
    alg->initialize();
    if (isHistogramFit()) {
      alg->setProperty("EvaluationType", "Histogram");
    }
    alg->setPropertyValue("Function", funStr);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("WorkspaceIndex", workspaceIndex());
    alg->setProperty("StartX", startX());
    alg->setProperty("EndX", endX());
    alg->setPropertyValue("Output", outputName());
    alg->setPropertyValue("Minimizer", minimizer(true));
    alg->setProperty("IgnoreInvalidData", ignoreInvalidData());
    alg->setPropertyValue("CostFunction", costFunction());
    alg->setProperty("MaxIterations", maxIterations);
    alg->setProperty("PeakRadius", getPeakRadius());
    if (!isHistogramFit()) {
      alg->setProperty("Normalise", m_shouldBeNormalised);
      // Always output each composite function but not necessarily plot it
      alg->setProperty("OutputCompositeMembers", true);
      if (alg->existsProperty("ConvolveMembers")) {
        alg->setProperty("ConvolveMembers", convolveMembers());
      }
    }
    observeFinish(alg);
    alg->executeAsync();

  } catch (const std::exception &e) {
    QString msg = "Fit algorithm failed.\n\n" + QString(e.what()) + "\n";
    QMessageBox::critical(this, "Mantid - Error", msg);
  }
}

/**
 * Return the function that will be passed to Fit.
 */
Mantid::API::IFunction_sptr FitPropertyBrowser::getFittingFunction() const {
  Mantid::API::IFunction_sptr function;
  if (m_compositeFunction->nFunctions() > 1) {
    function = m_compositeFunction;
  } else {
    function = getFunctionAtIndex(0);
  }
  return function;
}

/**
 * Return the function within a composite function at the position specified
 */
Mantid::API::IFunction_sptr
FitPropertyBrowser::getFunctionAtIndex(std::size_t const &index) const {
  return m_compositeFunction->getFunction(index);
}

void FitPropertyBrowser::finishHandle(const Mantid::API::IAlgorithm *alg) {
  // Emit a signal to show that the fitting has completed. (workspaceName that
  // the fit has been done against is sent as a parameter)
  QString name(QString::fromStdString(alg->getProperty("InputWorkspace")));
  if (name.contains('_')) // Must be fitting to raw data, need to group under
    // name without "_Raw".
    emit fittingDone(name.left(name.indexOf('_')));
  else // else fitting to current workspace, group under same name.
    emit fittingDone(name);

  getFitResults();
  if (!isWorkspaceAGroup() && alg->existsProperty("OutputWorkspace")) {
    std::string out = alg->getProperty("OutputWorkspace");
    emit algorithmFinished(QString::fromStdString(out));
  }
  // Update Status string
  auto status = QString::fromStdString(alg->getPropertyValue("OutputStatus"));
  emit fitResultsChanged(status);
  // update Quality string
  if (m_displayActionQuality->isChecked()) {
    double quality = alg->getProperty("OutputChi2overDoF");
    std::string costFunction = alg->getProperty("CostFunction");
    boost::shared_ptr<Mantid::API::ICostFunction> costfun =
        Mantid::API::CostFunctionFactory::Instance().create(costFunction);
    if (status != "success") {
      status = "failed";
    }
    emit changeWindowTitle(QString("Fit Function (") +
                           costfun->shortName().c_str() + " = " +
                           QString::number(quality) + ", " + status + ")");
  } else
    emit changeWindowTitle("Fit Function");
  if (m_compositeFunction->name() == "MultiBG") {
    emit multifitFinished();
  }
}

/// Display the status string returned from Fit
/// @param status :: A status string as returned by OutputStatus Fit property.
void FitPropertyBrowser::showFitResultStatus(const QString &status) {
  auto text(status);
  text.replace("\n", "<br>");
  QString color("green");
  if (status != "success") {
    color = "red";
  }

  m_status->setText(
      QString("Status: <span style='color:%2'>%1</span>").arg(text, color));
  m_status->show();
}

/// Clear the Fit status display
void FitPropertyBrowser::clearFitResultStatus() {
  m_status->setText("Status:");
  m_status->hide();
}

/// Get and store available workspace names
void FitPropertyBrowser::populateWorkspaceNames() {
  m_workspaceNames.clear();
  bool allAreAllowed = m_allowedSpectra.isEmpty();

  QStringList tmp;
  auto sv = Mantid::API::AnalysisDataService::Instance().getObjectNames();
  for (auto &it : sv) {
    auto const &name = QString::fromStdString(it);
    if (allAreAllowed || m_allowedSpectra.contains(name)) {
      tmp << name;
    }
  }

  for (int i = 0; i < tmp.size(); i++) {
    Mantid::API::Workspace_sptr ws =
        Mantid::API::AnalysisDataService::Instance().retrieve(
            tmp[i].toStdString());
    if (isWorkspaceValid(ws)) {
      m_workspaceNames.append(tmp[i]);
    }
  }
  m_enumManager->setEnumNames(m_workspace, m_workspaceNames);
  setWorkspaceIndex(0);
}

/**
 * Connect to the AnalysisDataService when shown
 */
void FitPropertyBrowser::showEvent(QShowEvent *e) {
  (void)e;
  // Observe what workspaces are added and deleted unless it's a custom
  // fitting, all workspaces for custom fitting (eg muon analysis) should be
  // manually added.
  setADSObserveEnabled(true);
  populateWorkspaceNames();
}

/**
 * Disconnect from the AnalysisDataService when hiden
 */
void FitPropertyBrowser::hideEvent(QHideEvent *e) {
  (void)e;
  setADSObserveEnabled(false);
}

void FitPropertyBrowser::setADSObserveEnabled(bool enabled) {
  observeAdd(enabled);
  observePostDelete(enabled);
}

/// workspace was added
void FitPropertyBrowser::addHandle(
    const std::string &wsName,
    const boost::shared_ptr<Mantid::API::Workspace> ws) {
  auto const qName = QString::fromStdString(wsName);
  if (!isWorkspaceValid(ws) ||
      (!m_allowedSpectra.isEmpty() && !m_allowedSpectra.contains(qName)))
    return;
  QStringList oldWorkspaces = m_workspaceNames;
  QString oldName = QString::fromStdString(workspaceName());
  int i = m_workspaceNames.indexOf(qName);

  bool initialSignalsBlocked = m_enumManager->signalsBlocked();

  // if new workspace append this workspace name
  if (i < 0) {
    if (!m_workspaceNames.isEmpty()) {
      m_enumManager->blockSignals(true);
    }

    m_workspaceNames.append(qName);
    m_workspaceNames.sort();
    m_enumManager->setEnumNames(m_workspace, m_workspaceNames);
  }
  // get hold of index of oldName
  i = m_workspaceNames.indexOf(oldName);
  if (i >= 0) {
    m_enumManager->setValue(m_workspace, i);
  }

  m_enumManager->blockSignals(initialSignalsBlocked);
}

/// workspace was removed
void FitPropertyBrowser::postDeleteHandle(const std::string &wsName) {
  QStringList oldWorkspaces = m_workspaceNames;
  QString oldName = QString::fromStdString(workspaceName());
  int i = m_workspaceNames.indexOf(QString(wsName.c_str()));
  if (i >= 0) {
    m_workspaceNames.removeAt(i);
  }

  bool initialSignalsBlocked = m_enumManager->signalsBlocked();

  if (QString::fromStdString(wsName) != oldName) {
    m_enumManager->blockSignals(true);
  }

  m_enumManager->setEnumNames(m_workspace, m_workspaceNames);

  i = m_workspaceNames.indexOf(oldName);
  if (i >= 0) {
    m_enumManager->setValue(m_workspace, i);
  }

  m_enumManager->blockSignals(initialSignalsBlocked);
}

/** Check if the workspace can be used in the fit. The accepted types are
 * MatrixWorkspaces same size
 * @param ws :: The workspace
 */
bool FitPropertyBrowser::isWorkspaceValid(
    Mantid::API::Workspace_sptr ws) const {
  return (dynamic_cast<Mantid::API::MatrixWorkspace *>(ws.get()) != nullptr ||
          dynamic_cast<Mantid::API::ITableWorkspace *>(ws.get()) != nullptr);
}

bool FitPropertyBrowser::isWorkspaceAGroup() const {
  // MG: Disabled as there is an issue with replacing workspace groups and the
  // browser
  return false;
}

/// Is the current function a peak?
bool FitPropertyBrowser::isPeak() const {
  if (count() == 0) {
    return false;
  }
  return m_currentHandler && m_currentHandler->pfun();
}

/// Get the start X
double FitPropertyBrowser::startX() const {
  return m_doubleManager->value(m_startX);
}

/// Set the start X
void FitPropertyBrowser::setStartX(double value) {
  m_doubleManager->setValue(m_startX, value);
}

/// Get the end X
double FitPropertyBrowser::endX() const {
  return m_doubleManager->value(m_endX);
}

/// Set the end X
void FitPropertyBrowser::setEndX(double value) {
  m_doubleManager->setValue(m_endX, value);
}

void FitPropertyBrowser::setXRange(double start, double end) {
  m_doubleManager->setValue(m_startX, start);
  m_doubleManager->setValue(m_endX, end);
}

///
QtBrowserItem *FitPropertyBrowser::findItem(QtBrowserItem *parent,
                                            QtProperty *prop) const {
  QList<QtBrowserItem *> children = parent->children();
  QtBrowserItem *res = nullptr;
  for (auto &child : children) {
    if (child->property() == prop) {
      return child;
    }
    QList<QtBrowserItem *> grand_children = child->children();
    if (grand_children.size() > 0)
      res = findItem(child, prop);
    if (res)
      return res;
  }
  return nullptr;
}

/**
 * Slot. Responds to changing the current item
 */
void FitPropertyBrowser::currentItemChanged(QtBrowserItem *current) {
  if (current) {
    m_currentHandler = getHandler()->findHandler(current->property());
  } else {
    m_currentHandler = nullptr;
  }
  emit currentChanged();
}

/**
 * Slot. Responds to changing a vector attribute member
 * @param prop :: A property managed by m_vectorDoubleManager.
 */
void FitPropertyBrowser::vectorDoubleChanged(QtProperty *prop) {
  PropertyHandler *h = getHandler()->findHandler(prop);
  if (!h)
    return;
  h->setVectorAttribute(prop);
}

/**
 * Slot. Responds to changing a vector attribute size
 * @param prop :: A property managed by m_vectorSizeManager.
 */
void FitPropertyBrowser::vectorSizeChanged(QtProperty *prop) {
  PropertyHandler *h = getHandler()->findHandler(prop);
  if (!h)
    return;
  h->setVectorAttribute(prop);
}

/**
 * Update the function parameter properties
 */
void FitPropertyBrowser::updateParameters() {
  getHandler()->updateParameters();
}

/**
 * Update the function attributes which have changed
 */
void FitPropertyBrowser::updateAttributes() {
  getHandler()->updateAttributes();
}

/**
 * Slot. Removes all functions.
 */
void FitPropertyBrowser::clear() {
  getHandler()->removeAllPlots();
  clearBrowser();
  createCompositeFunction();
  emit functionCleared();
}

void FitPropertyBrowser::clearBrowser() {
  QList<QtProperty *> props = m_functionsGroup->property()->subProperties();
  foreach (QtProperty *prop, props) {
    m_functionsGroup->property()->removeSubProperty(prop);
  }
}

/// Set the parameters to the fit outcome
void FitPropertyBrowser::getFitResults() {
  std::string wsName = outputName() + "_Parameters";
  if (Mantid::API::AnalysisDataService::Instance().doesExist(wsName)) {
    Mantid::API::ITableWorkspace_sptr ws =
        boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(wsName));

    Mantid::API::TableRow row = ws->getFirstRow();
    do {
      try {
        std::string name;
        double value, error;
        row >> name >> value >> error;

        // In case of a single function Fit doesn't create a CompositeFunction
        if (count() == 1) {
          name.insert(0, "f0.");
        }

        size_t paramIndex = compositeFunction()->parameterIndex(name);

        compositeFunction()->setParameter(paramIndex, value);
        compositeFunction()->setError(paramIndex, error);
      } catch (...) {
        // do nothing
      }
    } while (row.next());
    updateParameters();
    getHandler()->updateErrors();
  }
}

/**
 * Slot. Undoes the fit: restores the parameters to their initial values.
 */
void FitPropertyBrowser::undoFit() {
  if (m_initialParameters.size() == compositeFunction()->nParams()) {
    for (size_t i = 0; i < compositeFunction()->nParams(); i++) {
      compositeFunction()->setParameter(i, m_initialParameters[i]);
    }
    clearFitResultStatus();
    updateParameters();
    getHandler()->clearErrors();
    emit fitUndone();
  }
  disableUndo();
}

/// disable undo when the function changes
void FitPropertyBrowser::disableUndo() {
  m_initialParameters.clear();
  m_fitActionUndoFit->setEnabled(false);
}

/// Tells if undo can be done
bool FitPropertyBrowser::isUndoEnabled() const {
  return m_initialParameters.size() &&
         compositeFunction()->nParams() == m_initialParameters.size();
}

/// Enable/disable the Fit buttons;
void FitPropertyBrowser::setFitEnabled(bool enable) {
  m_fitActionFit->setEnabled(enable);
  m_fitActionSeqFit->setEnabled(enable);
}

/// Returns true if the function is ready for a fit
bool FitPropertyBrowser::isFitEnabled() const {
  return m_fitActionFit->isEnabled();
}

/**
 * Slot. Adds a tie. Full expression to be entered \<name\>=\<formula\>
 */
void FitPropertyBrowser::addTie() {
  QtBrowserItem *ci = m_browser->currentItem();
  QtProperty *paramProp = ci->property();
  PropertyHandler *h = getHandler()->findHandler(paramProp);
  if (!h)
    return;
  if (!h->isParameter(paramProp))
    return;

  auto f = h->function();
  if (!f)
    return;

  bool ok = false;
  QString tieStr =
      QInputDialog::getText(this, "MantidPlot - Fit", "Enter tie expression",
                            QLineEdit::Normal, "", &ok);
  if (ok) {
    tieStr = tieStr.trimmed();
    if (!tieStr.contains('=')) {
      tieStr =
          h->functionPrefix() + "." + paramProp->propertyName() + "=" + tieStr;
    }
    h->addTie(tieStr);
  } // if (ok)
}

/**
 * Slot. Ties a parameter to a parameter with the same name of a different
 * function
 */
void FitPropertyBrowser::addTieToFunction() {
  QtBrowserItem *ci = m_browser->currentItem();
  QtProperty *paramProp = ci->property();
  PropertyHandler *h = getHandler()->findHandler(paramProp);
  if (!h)
    return;
  if (!h->isParameter(paramProp))
    return;
  std::string parName = paramProp->propertyName().toStdString();
  QStringList fnNames;

  int iPar = -1;
  for (size_t i = 0; i < m_compositeFunction->nParams(); i++) {
    Mantid::API::ParameterReference ref(m_compositeFunction.get(), i);
    Mantid::API::IFunction *fun = ref.getLocalFunction();

    // Pick out parameters with the same name as the one we're tying from
    if (fun->parameterName(static_cast<int>(ref.getLocalIndex())) == parName) {
      if (iPar == -1 &&
          fun ==
              h->function()
                  .get()) // If this is the 'tied from' parameter, remember it
      {
        iPar = (int)i;
      } else // Otherwise add it to the list of potential 'tyees'
      {
        fnNames << QString::fromStdString(
            m_compositeFunction->parameterName(i));
      }
    }
  }
  if (fnNames.empty() || iPar < 0) {
    QMessageBox::information(this, "Mantid - information",
                             "Cannot tie this parameter to any function");
    return;
  }

  bool ok;
  QString tieName = QInputDialog::getItem(
      this, "MantidPlot - Fit", "Select function", fnNames, 0, false, &ok);

  if (!ok)
    return;

  QString tieExpr =
      QString::fromStdString(m_compositeFunction->parameterName(iPar)) + "=" +
      tieName;

  h->addTie(tieExpr);
}

/**
 * Slot. Adds a tie. The current item must be a function parameter
 */
void FitPropertyBrowser::addFixTie() {
  QtBrowserItem *ci = m_browser->currentItem();
  QtProperty *paramProp = ci->property();
  PropertyHandler *h = getHandler()->findHandler(paramProp);
  if (!h)
    return;
  if (!h->isParameter(paramProp))
    return;
  h->fix(paramProp->propertyName());
}

/**
 * Slot. Deletes a tie.
 */
void FitPropertyBrowser::deleteTie() {
  QtBrowserItem *ci = m_browser->currentItem();
  QtProperty *paramProp = ci->property();
  PropertyHandler *h = getHandler()->findHandler(paramProp);
  if (!h)
    return;
  // get name of parent property (i.e. function)
  if (paramProp->propertyName() != "Tie") {
    auto parameterMap = h->getTies();
    auto match = parameterMap.find(paramProp->propertyName());
    if (match != parameterMap.end()) {
      paramProp = match.value();
    }
  }
  if (paramProp->propertyName() == "Tie") {
    auto ties = h->getTies();
    QString qParName = ties.key(paramProp, "");
    std::string parName = qParName.toStdString();
    QStringList functionNames;
    // ithParameter = -1 => not found
    int ithParameter = -1;
    for (size_t i = 0; i < m_compositeFunction->nParams(); i++) {
      Mantid::API::ParameterReference parameterRef(m_compositeFunction.get(),
                                                   i);
      Mantid::API::IFunction *function = parameterRef.getLocalFunction();
      // Pick out parameters with the same name as the one we're tying from
      if (function->parameterName(
              static_cast<int>(parameterRef.getLocalIndex())) == parName) {
        if (ithParameter == -1 &&
            function == h->function().get()) // If this is the 'tied from'
                                             // parameter, remember it
        {
          ithParameter = static_cast<int>(i);
        } else // Otherwise add it to the list of potential 'tyees'
        {
          functionNames << QString::fromStdString(
              m_compositeFunction->parameterName(i));
        }
      }
    }
    if (functionNames.empty() && ithParameter < 0) {
      QMessageBox::information(this, "Mantid - information",
                               "Cannot find a parameter with this tie");
    } else {
      QString tieExpr = QString::fromStdString(
          m_compositeFunction->parameterName(ithParameter));
      h->removeTie(paramProp, tieExpr.toStdString());
    }
  } else {
    h->removeTie(ci->property()->propertyName());
  }
}

/** Does a parameter have a tie
 * @param parProp :: The property for a function parameter
 * @param hasTie :: Parameter has a tie
 * @param hasBounds :: Parameter has bounds
 */
void FitPropertyBrowser::hasConstraints(QtProperty *parProp, bool &hasTie,
                                        bool &hasBounds) const {
  hasTie = false;
  hasBounds = false;
  QList<QtProperty *> subs = parProp->subProperties();
  for (auto &sub : subs) {
    if (sub->propertyName() == "Tie") {
      hasTie = true;
    }
    if (sub->propertyName() == "LowerBound") {
      hasBounds = true;
    }
    if (sub->propertyName() == "UpperBound") {
      hasBounds = true;
    }
  }
}

/** Returns the tie property for a parameter property, or nullptr
 * @param parProp :: parameter property
 */
QtProperty *FitPropertyBrowser::getTieProperty(QtProperty *parProp) const {
  QList<QtProperty *> subs = parProp->subProperties();
  for (auto &sub : subs) {
    if (sub->propertyName() == "Tie") {
      return sub;
    }
  }
  return nullptr;
}

/**
 * Called when the function factory has been updated
 * @param notice A Poco notification object
 */
void FitPropertyBrowser::handleFactoryUpdate(
    Mantid::API::FunctionFactoryUpdateNotification_ptr notice) {
  Q_UNUSED(notice);
  // Don't call populate directly as the updates can come from a different
  // thread
  emit functionFactoryUpdateReceived();
}

/** Display a tip
 * @param txt :: The text to display
 */
void FitPropertyBrowser::setTip(const QString &txt) { m_tip->setText(txt); }

/**
 * Slot. Adds lower bound to the selected parameter property
 * and sets it f % below parameter's current value
 */
void FitPropertyBrowser::addConstraint(int f, bool lo, bool up) {
  QtBrowserItem *ci = m_browser->currentItem();
  QtProperty *parProp = ci->property();
  PropertyHandler *h = getHandler()->findHandler(parProp);
  if (!h)
    return;

  double x = parProp->valueText().toDouble();
  double loBound = x * (1 - 0.01 * f);
  double upBound = x * (1 + 0.01 * f);

  h->addConstraint(ci->property(), lo, up, loBound, upBound);
}

/**
 * Slot. Adds lower bound to the selected parameter property
 */
void FitPropertyBrowser::addLowerBound() { addConstraint(0, true, false); }

/**
 * Slot. Adds lower bound to the selected parameter property
 */
void FitPropertyBrowser::addLowerBound10() { addConstraint(10, true, false); }

/**
 * Slot. Adds lower bound to the selected parameter property
 */
void FitPropertyBrowser::addLowerBound50() { addConstraint(50, true, false); }

/**
 * Slot.Adds upper bound to the selected parameter property
 */
void FitPropertyBrowser::addUpperBound10() { addConstraint(10, false, true); }

/**
 * Slot.Adds upper bound to the selected parameter property
 */
void FitPropertyBrowser::addUpperBound50() { addConstraint(50, false, true); }

/**
 * Slot.Adds upper bound to the selected parameter property
 */
void FitPropertyBrowser::addUpperBound() { addConstraint(0, false, true); }

/**
 * Slot.Sets the lower and upper bounds of the selected parameter to 10% of
 * its value
 */
void FitPropertyBrowser::addBothBounds10() { addConstraint(10, true, true); }

/**
 * Slot.Sets the lower and upper bounds of the selected parameter to 50% of
 * its value
 */
void FitPropertyBrowser::addBothBounds50() { addConstraint(50, true, true); }

/**
 * Slot.Adds lower and upper bounds to the selected parameter property
 */
void FitPropertyBrowser::addBothBounds() { addConstraint(0, true, true); }

/**
 * Slot.Removes lower and upper bounds from the selected parameter property
 */
void FitPropertyBrowser::removeBounds() {
  QtBrowserItem *ci = m_browser->currentItem();
  QtProperty *parProp = ci->property();
  PropertyHandler *h = getHandler()->findHandler(parProp);
  if (!h)
    return;

  h->removeConstraint(parProp);
}

/**
 * Slot. Sends a signal to plot the guess for the current (selected) function
 */
void FitPropertyBrowser::plotGuessCurrent() { emit plotCurrentGuess(); }

/**
 * Slot. Sends a signal to plot the guess for the whole function
 */
void FitPropertyBrowser::plotGuessAll() { emit plotGuess(); }

/**
 * Slot. Sends a signal to remove the guess for the current (selected)
 * function
 */
void FitPropertyBrowser::removeGuessCurrent() { emit removeCurrentGuess(); }

/**
 * Slot. Sends a signal to remove the guess for the whole function
 */
void FitPropertyBrowser::removeGuessAll() { emit removeGuess(); }

void FitPropertyBrowser::plotOrRemoveGuessAll() {
  if (getHandler()->hasPlot()) {
    removeGuessAll();
  } else {
    plotGuessAll();
  }
}

void FitPropertyBrowser::clearAllPlots() { emit removeFitCurves(); }

/** Create a double property and set some settings
 * @param name :: The name of the new property
 * @param manager :: The current property manager
 * @return Pointer to the created property
 */
QtProperty *
FitPropertyBrowser::addDoubleProperty(const QString &name,
                                      QtDoublePropertyManager *manager) const {
  if (manager == nullptr)
    manager = m_doubleManager;
  QtProperty *prop = manager->addProperty(name);
  manager->setDecimals(prop, m_decimals);
  manager->setRange(prop, -DBL_MAX, DBL_MAX);
  return prop;
}

/** Create a string property and selects a property manager for it
 * based on the property name
 * @param name :: The name of the new property
 * @return Pointer to the created property
 */
QtProperty *FitPropertyBrowser::addStringProperty(const QString &name) const {
  QtProperty *prop;
  QString propName = name.toLower();
  if (propName == "filename") {
    prop = m_filenameManager->addProperty(name);
  } else if (propName == "formula") {
    //!!! dont forget to change the manager !!!
    prop = m_formulaManager->addProperty(name);
  } else {
    prop = m_stringManager->addProperty(name);
  }
  return prop;
}

/**
 * Set a value to a string property.
 * @param prop :: A pointer to the property
 * @param value :: New value for the property
 */
void FitPropertyBrowser::setStringPropertyValue(QtProperty *prop,
                                                const QString &value) const {
  QtStringPropertyManager *manager =
      dynamic_cast<QtStringPropertyManager *>(prop->propertyManager());
  if (manager) {
    manager->setValue(prop, value);
  }
}

QString FitPropertyBrowser::getStringPropertyValue(QtProperty *prop) const {
  QtStringPropertyManager *manager =
      dynamic_cast<QtStringPropertyManager *>(prop->propertyManager());
  if (manager)
    return manager->value(prop);
  else
    return QString("");
}

Mantid::API::IFunction_const_sptr FitPropertyBrowser::theFunction() const {
  return m_compositeFunction;
}

void FitPropertyBrowser::checkFunction() {}

/**
 * If the current workspace index is set to a disallowed value this function
 * returns the nearest allowed index. Otherwise the current index is returned.
 */
int FitPropertyBrowser::getAllowedIndex(int currentIndex) const {
  auto const workspace =
      convertToMatrixWorkspace(getADSWorkspace(workspaceName()));

  if (!workspace) {
    return 0;
  }

  if (currentIndex == m_oldWorkspaceIndex) {
    return currentIndex < 0 ? 0 : currentIndex;
  }

  auto const allowedIndices =
      m_allowedSpectra.empty()
          ? QList<int>()
          : m_allowedSpectra[QString::fromStdString(workspaceName())];
  auto const firstIndex = m_allowedSpectra.empty() ? 0 : allowedIndices.front();
  auto const lastIndex = m_allowedSpectra.empty()
                             ? getNumberOfSpectra(workspace) - 1
                             : allowedIndices.back();

  auto allowedIndex = currentIndex;
  if (currentIndex < firstIndex) {
    allowedIndex = firstIndex;
  } else if (currentIndex > lastIndex) {
    allowedIndex = lastIndex;
  } else if (!m_allowedSpectra.empty() &&
             !allowedIndices.contains(currentIndex)) {
    allowedIndex = m_oldWorkspaceIndex;
    auto i = allowedIndices.indexOf(m_oldWorkspaceIndex);
    if (i >= 0) {
      i = currentIndex > m_oldWorkspaceIndex ? i + 1 : i - 1;
      if (i >= 0 && i < allowedIndices.size()) {
        allowedIndex = allowedIndices[i];
      }
    }
  }
  return allowedIndex;
}

void FitPropertyBrowser::saveFunction() {
  bool ok(false);
  QString fnName = QInputDialog::getText(
      this, tr("Mantid - Input"), tr("Please select a name for the function"),
      QLineEdit::Normal, "", &ok);
  if (ok && !fnName.isEmpty()) {
    saveFunction(fnName);
  }
}

void FitPropertyBrowser::saveFunction(const QString &fnName) {
  QSettings settings;
  settings.beginGroup("Mantid/FitBrowser/SavedFunctions");
  QStringList names = settings.childKeys();
  if (names.contains(fnName) &&
      QMessageBox::question(this, "Mantid - Question",
                            "Function with this name already exists.\n"
                            "Would you like to replace it?",
                            QMessageBox::Yes) != QMessageBox::Yes) {
    return;
  }
  settings.setValue(fnName, QString::fromStdString(theFunction()->asString()));
  updateSetupMenus();
}

void FitPropertyBrowser::loadFunction() {
  QSettings settings;
  settings.beginGroup("Mantid/FitBrowser/SavedFunctions");
  QStringList names = settings.childKeys();
  if (names.isEmpty()) {
    QMessageBox::information(this, "Mantid - Information",
                             "There are no saved functions");
    return;
  }
  QString name = QInputDialog::getItem(this, "Mantid - Input",
                                       "Please select a function to load",
                                       names, 0, false);
  if (!name.isEmpty()) {
    QString str = settings.value(name).toString();

    loadFunction(str);
  }
}

void FitPropertyBrowser::loadFunctionFromString() {
  QString str = QInputDialog::getText(this, "Mantid - Input",
                                      "Specify fit function string");

  if (!str.isEmpty()) {
    loadFunction(str);
  }
}

void FitPropertyBrowser::loadFunction(const QString &funcString) {
  // when loading a function from a string initially
  // do not try to do auto background even if set
  bool isAutoBGset = false;
  if (m_auto_back) {
    isAutoBGset = true;
    m_auto_back = false;
  }

  getHandler()->removeAllPlots();
  clearBrowser();
  createCompositeFunction(funcString);
  emit functionLoaded(funcString);

  if (isAutoBGset)
    m_auto_back = true;
}

void FitPropertyBrowser::copy() {
  QClipboard *clipboard = QApplication::clipboard();
  clipboard->setText(QString::fromStdString(theFunction()->asString()));
}

void FitPropertyBrowser::paste() {
  QClipboard *clipboard = QApplication::clipboard();
  QString str = clipboard->text();
  createCompositeFunction(str);
}

void FitPropertyBrowser::reset() {
  QString str = QString::fromStdString(theFunction()->asString());
  // getHandler()->removeAllPlots();// this crashes mantidplot
  clearBrowser();
  createCompositeFunction(str);
}

void FitPropertyBrowser::setWorkspace(Mantid::API::IFunction_sptr f) const {
  std::string wsName = workspaceName();
  if (!wsName.empty()) {
    try {
      auto ws = Mantid::API::AnalysisDataService::Instance().retrieve(wsName);
      auto mws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws);
      if (mws) {
        f->setMatrixWorkspace(mws, workspaceIndex(), startX(), endX());
      } else {
        f->setWorkspace(ws);
      }
    } catch (...) {
    }
  }
}

void FitPropertyBrowser::addAutoBackground() {
  if (m_autoBgName.isEmpty())
    return;
  bool hasPlot = false;
  PropertyHandler *ch = currentHandler();
  if (m_autoBackground) { // remove old background
    if (ch == m_autoBackground) {
      ch = nullptr;
    }
    hasPlot = m_autoBackground->hasPlot();
    m_autoBackground->removeFunction();
    m_autoBackground = nullptr;
  }
  // Create the function
  PropertyHandler *h = getHandler()->addFunction(m_autoBgName.toStdString());
  if (!h)
    return;
  if (!m_autoBgAttributes.isEmpty()) { // set attributes
    QStringList attList = m_autoBgAttributes.split(' ');
    foreach (QString att, attList) {
      QStringList name_value = att.split('=');
      if (name_value.size() == 2) {
        QString name = name_value[0].trimmed();
        QString value = name_value[1].trimmed();
        if (h->function()->hasAttribute(name.toStdString())) {
          h->setAttribute(name, value);
        }
      }
    }
  }
  h->fit();
  m_autoBackground = h;
  getHandler()->calcBaseAll();
  if (hasPlot) {
    setCurrentFunction(h);
    emit plotCurrentGuess();
    if (ch) {
      setCurrentFunction(ch);
    }
  }
}

void FitPropertyBrowser::refitAutoBackground() {
  if (m_autoBackground) {
    m_autoBackground->fit();
  }
}

/**
 * Remember a background function name to be used for creating auto-background
 * @param aName :: A name of the auto-background. The may be followed by
 * function
 * attributes as name=value pairs separated by spaces.
 */
void FitPropertyBrowser::setAutoBackgroundName(const QString &aName) {
  try {
    QStringList nameList = aName.split(' ');
    if (nameList.isEmpty())
      return;
    QString name = nameList[0];
    boost::shared_ptr<Mantid::API::IFunction> f =
        boost::shared_ptr<Mantid::API::IFunction>(
            Mantid::API::FunctionFactory::Instance().createFunction(
                name.toStdString()));
    m_auto_back = true;
    m_autoBgName = name;
    if (nameList.size() > 1) {
      nameList.removeFirst();
      m_autoBgAttributes = nameList.join(" ");
    }
    Mantid::Kernel::ConfigService::Instance().setString(
        "curvefitting.autoBackground", aName.toStdString());
  } catch (...) {
    m_auto_back = false;
  }
}

/// Set LogValue for PlotPeakByLogValue
void FitPropertyBrowser::setLogValue(const QString &lv) {
  if (isWorkspaceAGroup()) {
    // validateGroupMember();
    if (!m_logValue) {
      m_logValue = m_enumManager->addProperty("LogValue");
      m_settingsGroup->property()->addSubProperty(m_logValue);
    }
    m_logs.clear();
    m_logs << "";
    /* if (!m_groupMember.empty())
     {
       Mantid::API::MatrixWorkspace_sptr ws =
         boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
         Mantid::API::AnalysisDataService::Instance().retrieve(m_groupMember)
         );
       if (ws)
       {
         const std::vector<Mantid::Kernel::Property*> logs =
     ws->run().getLogData();
         for(int i=0;i<static_cast<int>(logs.size()); ++i)
         {
           m_logs << QString::fromStdString(logs[i]->name());
         }
       }
     }*/
    m_enumManager->setEnumNames(m_logValue, m_logs);
    int i = m_logs.indexOf(lv);
    if (i < 0)
      i = 0;
    m_enumManager->setValue(m_logValue, i);
  }
}

std::string FitPropertyBrowser::getLogValue() const {
  if (isWorkspaceAGroup() && m_logValue) {
    int i = m_enumManager->value(m_logValue);
    if (i < m_logs.size())
      return m_logs[i].toStdString();
  }
  return "";
}

/// Remove LogValue from the browser
void FitPropertyBrowser::removeLogValue() {
  if (isWorkspaceAGroup())
    return;
  m_settingsGroup->property()->removeSubProperty(m_logValue);
  m_logValue = nullptr;
}

void FitPropertyBrowser::sequentialFit() {
  if (workspaceName() == outputName()) {
    setOutputName(outputName() + "_res");
  }
  SequentialFitDialog *dlg = new SequentialFitDialog(this, m_mantidui);
  std::string wsName = workspaceName();
  if (!wsName.empty() &&
      dlg->addWorkspaces(QStringList(QString::fromStdString(wsName)))) {
    dlg->show();
  }
}

void FitPropertyBrowser::findPeaks() {
  std::string wsName = workspaceName();
  if (wsName.empty()) {
    QMessageBox::critical(this, "Mantid - Error", "Workspace name is not set");
    return;
  }

  std::string peakListName = wsName + "_PeakList_tmp";

  int FWHM, Tolerance;
  QString setting = QString::fromStdString(
      Mantid::Kernel::ConfigService::Instance().getString(
          "curvefitting.findPeaksFWHM"));
  FWHM = setting.isEmpty() ? 7 : setting.toInt();

  setting = QString::fromStdString(
      Mantid::Kernel::ConfigService::Instance().getString(
          "curvefitting.findPeaksTolerance"));
  Tolerance = setting.isEmpty() ? 4 : setting.toInt();

  Mantid::API::IAlgorithm_sptr alg =
      Mantid::API::AlgorithmManager::Instance().create("FindPeaks");
  alg->initialize();
  alg->setPropertyValue("InputWorkspace", wsName);
  alg->setProperty("WorkspaceIndex", workspaceIndex());
  alg->setPropertyValue("PeaksList", peakListName);
  alg->setProperty("FWHM", FWHM);
  alg->setProperty("Tolerance", Tolerance);

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  Mantid::API::MatrixWorkspace_sptr inputWS =
      boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
          Mantid::API::AnalysisDataService::Instance().retrieve(
              workspaceName()));

  try {
    alg->execute();
    Mantid::API::ITableWorkspace_sptr ws =
        boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(
                peakListName));

    clear();
    Mantid::API::ColumnVector<double> centre = ws->getVector("centre");
    Mantid::API::ColumnVector<double> width = ws->getVector("width");
    Mantid::API::ColumnVector<double> height = ws->getVector("height");
    for (size_t i = 0; i < centre.size(); ++i) {
      if (centre[i] < startX() || centre[i] > endX())
        continue;
      auto f = boost::dynamic_pointer_cast<Mantid::API::IPeakFunction>(
          Mantid::API::FunctionFactory::Instance().createFunction(
              defaultPeakType()));
      if (!f)
        break;
      f->setMatrixWorkspace(inputWS, workspaceIndex(), startX(), endX());
      f->setCentre(centre[i]);
      f->setFwhm(width[i]);
      f->setHeight(height[i]);
      addFunction(f->asString());
    }
  } catch (...) {
    QApplication::restoreOverrideCursor();
    throw;
  }

  QApplication::restoreOverrideCursor();
}

void FitPropertyBrowser::setPeakToolOn(bool on) {
  m_peakToolOn = on;
  m_displayActionPlotGuess->setEnabled(on);
}

/**
 * @brief impose a number of decimal places on all defined Double properties
 */
void FitPropertyBrowser::updateDecimals() {
  if (m_decimals < 0) {
    QSettings settings;
    settings.beginGroup("Mantid/FitBrowser");
    m_decimals = settings.value("decimals", 6).toInt();
  }
  QSet<QtProperty *> props = m_doubleManager->properties();
  foreach (QtProperty *prop, props) {
    m_doubleManager->setDecimals(prop, m_decimals);
  }
}

void FitPropertyBrowser::setDecimals(int d) {
  m_decimals = d;
  QSettings settings;
  settings.beginGroup("Mantid/FitBrowser");
  settings.setValue("decimals", d);
  updateDecimals();
}

bool FitPropertyBrowser::plotDiff() const {
  return m_boolManager->value(m_plotDiff);
}

/// @returns true if each function with a composite should also be plotted
bool FitPropertyBrowser::plotCompositeMembers() const {
  return m_boolManager->value(m_plotCompositeMembers);
}

bool FitPropertyBrowser::rawData() const {
  return m_boolManager->value(m_rawData);
}

void FitPropertyBrowser::setTextPlotGuess(const QString text) {
  m_displayActionPlotGuess->setText(text);
}

/**
 * Sets a new workspace
 */
void FitPropertyBrowser::workspaceChange(const QString &wsName) {
  if (m_guessOutputName) {
    if (isWorkspaceAGroup()) {
      m_stringManager->setValue(
          m_output, QString::fromStdString(workspaceName() + "_params"));
    } else {
      m_stringManager->setValue(m_output,
                                QString::fromStdString(workspaceName()));
    }
  }
  if (isWorkspaceAGroup()) {
    setLogValue();
  } else {
    removeLogValue();
  }

  emit workspaceNameChanged(wsName);
  emit wsChangePPAssign(wsName);
}

/**
 * Returns the list of workspace names the fit property browser is working on
 */
QStringList FitPropertyBrowser::getWorkspaceNames() { return m_workspaceNames; }

/**
 * Call MultifitSetupDialog to populate MultiBG function.
 */
void FitPropertyBrowser::setupMultifit() {
  MultifitSetupDialog *dlg = new MultifitSetupDialog(this);
  dlg->exec();
  QStringList ties = dlg->getParameterTies();

  if (!ties.isEmpty()) {
    QString wsName = QString::fromStdString(workspaceName());
    Mantid::API::MatrixWorkspace_sptr mws =
        boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(
                workspaceName()));
    if (mws) {
      auto fun = getFunctionAtIndex(0);
      QString fun1Ini = QString::fromStdString(fun->asString());
      QString funIni = "composite=MultiBG;" + fun1Ini + ",Workspace=" + wsName +
                       ",WSParam=(WorkspaceIndex=0);";
      QString tieStr;
      for (size_t i = 1; i < mws->getNumberHistograms(); ++i) {
        QString comma = i > 1 ? "," : "";
        QString fi = comma + "f" + QString::number(i) + ".";
        for (int j = 0; j < static_cast<int>(fun->nParams()); ++j) {
          if (!ties[j].isEmpty()) {
            tieStr += fi + QString::fromStdString(fun->parameterName(j)) + "=" +
                      ties[j];
          }
        }
        QString wsParam = ",WSParam=(WorkspaceIndex=" + QString::number(i);
        wsParam += ",StartX=" + QString::number(startX()) +
                   ",EndX=" + QString::number(endX()) + ")";
        funIni += fun1Ini + ",Workspace=" + wsName + wsParam + ";";
      }
      if (!tieStr.isEmpty()) {
        funIni += "ties=(" + tieStr + ")";
      }
      loadFunction(funIni);
    }
  }
}

/// Process and create some output if it is a MultiBG fit
void FitPropertyBrowser::processMultiBGResults() {
  if (compositeFunction()->name() != "MultiBG")
    return;

  // if all member functions are of the same type and composition
  // create a TableWorkspace with parameter series

  // check if member functions are the same
  QStringList parNames;
  auto fun0 = getFunctionAtIndex(0);
  if (!fun0) {
    throw std::runtime_error(
        "IFunction expected but func function of another type");
  }
  for (size_t i = 0; i < fun0->nParams(); ++i) {
    parNames << QString::fromStdString(fun0->parameterName(i));
  }

  for (size_t i = 1; i < compositeFunction()->nFunctions(); ++i) {
    auto fun = getFunctionAtIndex(i);
    if (!fun) {
      throw std::runtime_error(
          "IFunction expected but func function of another type");
    }
    for (size_t j = 0; j < fun->nParams(); ++j) {
      if (parNames.indexOf(QString::fromStdString(fun->parameterName(j))) < 0) {
        // Functions are different, stop
        return;
      }
    }
  }

  QApplication::setOverrideCursor(Qt::WaitCursor);
  // create a TableWorkspace: first column - function index
  // other colomns - the parameters
  Mantid::API::ITableWorkspace_sptr table =
      Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
  table->addColumn("int", "Index");
  foreach (QString par, parNames) {
    table->addColumn("double", par.toStdString());
  }
  // Create WorkspaceGroup with the fit results
  std::vector<std::string> worspaceNames(compositeFunction()->nFunctions());
  for (size_t i = 0; i < compositeFunction()->nFunctions(); ++i) {
    // Mantid::API::TableRow row = table->appendRow();
    // row << int(i);
    // Mantid::API::IFunctionMW* fun =
    // dynamic_cast<Mantid::API::IFunctionMW*>(compositeFunction()->getFunction(i));
    // for(size_t j = 0; j < fun->nParams(); ++j)
    //{
    //  row << fun->getParameter(j);
    //}
    // size_t wi = fun->getWorkspaceIndex();
    // Mantid::API::MatrixWorkspace_sptr mws =
    // fun->createCalculatedWorkspace(fun->getMatrixWorkspace(),wi);
    // worspaceNames[i] =
    // workspaceName()+"_"+QString::number(wi).toStdString()+"_Workspace";
    // Mantid::API::AnalysisDataService::Instance().addOrReplace(worspaceNames[i],mws);
  }

  // Save the table
  Mantid::API::AnalysisDataService::Instance().addOrReplace(
      workspaceName() + "_Param_series", table);
  try {
    Mantid::API::IAlgorithm_sptr group =
        Mantid::API::AlgorithmManager::Instance().create("GroupWorkspaces");
    group->setProperty("InputWorkspaces", worspaceNames);
    group->setPropertyValue("OutputWorkspace", workspaceName() + "_Workspace");
    group->execute();
  } catch (...) {
  }
  QApplication::restoreOverrideCursor();
}

/**
 * Display properties relevant to the selected workspace:
 * WorkspaceIndex, StartX, and EndX for a MatrixWorkspace;
 * XColumn, YColumn, and ErrColumn for a TableWorkspace.
 */
void FitPropertyBrowser::setWorkspaceProperties() {
  // remove old properties
  if (m_browser->isItemVisible(m_settingsGroup)) {
    if (m_settingsGroup->property()->subProperties().contains(
            m_workspaceIndex)) {
      m_settingsGroup->property()->removeSubProperty(m_workspaceIndex);
    } else if (m_settingsGroup->property()->subProperties().contains(
                   m_xColumn)) {
      m_settingsGroup->property()->removeSubProperty(m_xColumn);
      m_settingsGroup->property()->removeSubProperty(m_yColumn);
      m_settingsGroup->property()->removeSubProperty(m_errColumn);
    }
  }

  Mantid::API::Workspace_sptr ws;
  try {
    ws = Mantid::API::AnalysisDataService::Instance().retrieve(workspaceName());
  } catch (...) {
    return;
  }
  if (!ws)
    return;

  // If this is a MuonFitPropertyBrowser, "evaluation type" goes in the Custom
  // Settings group.
  // If not, there is no Custom Settings group and it goes in the regular
  // Settings group.
  auto *settings =
      m_customSettingsGroup ? m_customSettingsGroup : m_settingsGroup;

  settings->property()->removeSubProperty(m_evaluationType);
  m_evaluationType->setEnabled(false);
  // if it is a MatrixWorkspace insert WorkspaceIndex
  auto mws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws);
  if (mws) {
    addWorkspaceIndexToBrowser();
    auto isHistogram = mws->isHistogramData();
    m_evaluationType->setEnabled(isHistogram);
    if (isHistogram) {
      settings->property()->addSubProperty(m_evaluationType);
    }
    return;
  }

  // if it is a TableWorkspace insert the column properties
  auto tws = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(ws);
  if (tws) {
    setWorkspaceIndex(0);
    // insert properties
    m_settingsGroup->property()->insertSubProperty(m_xColumn, m_workspace);
    m_settingsGroup->property()->insertSubProperty(m_yColumn, m_xColumn);
    m_settingsGroup->property()->insertSubProperty(m_errColumn, m_yColumn);
    if (workspaceName() == m_storedWorkspaceName)
      return;
    QString xName;
    QString yName;
    QString errName;
    auto names = tws->getColumnNames();
    QStringList columns;
    for (const auto &name : names) {
      columns << QString::fromStdString(name);
      auto col = tws->getColumn(name);
      if (xName.isEmpty() && col->getPlotType() == 1 /*X*/) {
        xName = QString::fromStdString(name);
      }
      if (yName.isEmpty() && col->getPlotType() == 2 /*Y*/) {
        yName = QString::fromStdString(name);
      }
      if (errName.isEmpty() && col->getPlotType() == 5 /*yErr*/) {
        errName = QString::fromStdString(name);
      }
    }
    m_columnManager->setEnumNames(m_xColumn, columns);
    m_columnManager->setEnumNames(m_yColumn, columns);
    // set the column values
    if (!xName.isEmpty()) {
      m_columnManager->setValue(m_xColumn, columns.indexOf(xName));
    } else {
      foreach (QString name, columns) {
        if (name != yName) {
          m_columnManager->setValue(m_xColumn, columns.indexOf(name));
          break;
        }
      }
    }
    if (!yName.isEmpty()) {
      m_columnManager->setValue(m_yColumn, columns.indexOf(yName));
    } else {
      foreach (QString name, columns) {
        if (name != xName) {
          m_columnManager->setValue(m_yColumn, columns.indexOf(name));
          break;
        }
      }
    }
    columns.prepend("");
    m_columnManager->setEnumNames(m_errColumn, columns);
    if (!errName.isEmpty()) {
      m_columnManager->setValue(m_errColumn, columns.indexOf(errName));
    }
    return;
  }
}

void FitPropertyBrowser::addWorkspaceIndexToBrowser() {
  if (m_browser->isItemVisible(m_settingsGroup)) {
    if (!m_settingsGroup->property()->subProperties().contains(
            m_workspaceIndex)) {
      m_settingsGroup->property()->insertSubProperty(m_workspaceIndex,
                                                     m_workspace);
    }
  }
}

/**=================================================================================================
 * Create a MatrixWorkspace from a TableWorkspace. Name of the TableWorkspace
 * is in m_workspace property, column names to use are in m_xColumn,
 * m_yColumn, and m_errColumn.
 */
Mantid::API::Workspace_sptr
FitPropertyBrowser::createMatrixFromTableWorkspace() const {
  std::string wsName = workspaceName();
  try {
    auto ws = Mantid::API::AnalysisDataService::Instance().retrieve(wsName);
    auto tws = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(ws);
    if (!tws)
      return boost::shared_ptr<Mantid::API::Workspace>();
    const size_t rowCount = tws->rowCount();
    if (rowCount == 0) {
      QMessageBox::critical(nullptr, "Mantid - Error",
                            "TableWorkspace is empty.");
      return boost::shared_ptr<Mantid::API::Workspace>();
    }

    auto columns = tws->getColumnNames();

    // get the x column
    int ix = m_columnManager->value(m_xColumn);
    if (ix >= static_cast<int>(columns.size())) {
      QMessageBox::critical(nullptr, "Mantid - Error",
                            "X column was not found.");
      return boost::shared_ptr<Mantid::API::Workspace>();
    }
    auto xcol = tws->getColumn(columns[ix]);

    // get the y column
    int iy = m_columnManager->value(m_yColumn);
    if (iy >= static_cast<int>(columns.size())) {
      QMessageBox::critical(nullptr, "Mantid - Error",
                            "Y column was not found.");
      return boost::shared_ptr<Mantid::API::Workspace>();
    }
    auto ycol = tws->getColumn(columns[iy]);

    // get the err column
    int ie =
        m_columnManager->value(m_errColumn) - 1; // first entry is empty string
    if (ie >= 0 && ie >= static_cast<int>(columns.size())) {
      QMessageBox::critical(nullptr, "Mantid - Error",
                            "Error column was not found.");
      return boost::shared_ptr<Mantid::API::Workspace>();
    }
    auto ecol =
        ie < 0 ? Mantid::API::Column_sptr() : tws->getColumn(columns[ie]);

    // create the MatrixWorkspace
    Mantid::API::MatrixWorkspace_sptr mws =
        Mantid::API::WorkspaceFactory::Instance().create("Workspace2D", 1,
                                                         rowCount, rowCount);
    auto &X = mws->mutableX(0);
    auto &Y = mws->mutableY(0);
    auto &E = mws->mutableE(0);

    for (size_t row = 0; row < rowCount; ++row) {
      X[row] = xcol->toDouble(row);
      Y[row] = ycol->toDouble(row);
      E[row] = ecol ? ecol->toDouble(row) : 1.0;
    }

    return mws;
  } catch (std::exception &e) {
    QMessageBox::critical(nullptr, "Mantid - Error", e.what());
    return boost::shared_ptr<Mantid::API::Workspace>();
  }
}

/**
 * Do the fit.
 */
void FitPropertyBrowser::fit() {
  int maxIterations = m_intManager->value(m_maxIterations);
  doFit(maxIterations);
}

/**=================================================================================================
 * Slot connected to the change signals of properties m_xColumn, m_yColumn,
 * and m_errColumn.
 * @param prop :: Property that changed.
 */
void FitPropertyBrowser::columnChanged(QtProperty *prop) {
  if (prop == m_xColumn) { // update startX and endX
    try {
      auto ws = Mantid::API::AnalysisDataService::Instance().retrieve(
          workspaceName());
      auto tws = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(ws);
      if (!tws)
        return;
      int i = m_columnManager->value(m_xColumn);
      if (i < 0 || i >= static_cast<int>(tws->rowCount()) ||
          tws->rowCount() == 0)
        return;
      auto col = tws->getColumn(static_cast<size_t>(i));
      const double startX = col->toDouble(0);
      const double endX = col->toDouble(tws->rowCount() - 1);
      m_doubleManager->setValue(m_startX, startX);
      m_doubleManager->setValue(m_endX, endX);
    } catch (...) {
      // do nothing
    }
  }
}

/**=================================================================================================
 * Called when the minimizer changes. Creates minimizes's properties.
 */
void FitPropertyBrowser::minimizerChanged() {
  // delete old minimizer properties
  foreach (QtProperty *prop, m_minimizerProperties) {
    m_settingsGroup->property()->removeSubProperty(prop);
  }

  // add new minimizer properties
  auto minzer = Mantid::API::FuncMinimizerFactory::Instance().createMinimizer(
      this->minimizer());
  auto &properties = minzer->getProperties();
  for (auto property : properties) {
    QString propName = QString::fromStdString((*property).name());
    QtProperty *prop = nullptr;
    if (auto prp =
            dynamic_cast<Mantid::Kernel::PropertyWithValue<bool> *>(property)) {
      prop = m_boolManager->addProperty(propName);
      bool val = *prp;
      m_boolManager->setValue(prop, val);
    } else if (auto prp =
                   dynamic_cast<Mantid::Kernel::PropertyWithValue<double> *>(
                       property)) {
      prop = this->addDoubleProperty(propName);
      double val = *prp;
      m_doubleManager->setValue(prop, val);
    } else if (auto prp =
                   dynamic_cast<Mantid::Kernel::PropertyWithValue<int> *>(
                       property)) {
      prop = m_intManager->addProperty(propName);
      int val = *prp;
      m_intManager->setValue(prop, val);
    } else if (auto prp =
                   dynamic_cast<Mantid::Kernel::PropertyWithValue<size_t> *>(
                       property)) {
      prop = m_intManager->addProperty(propName);
      size_t val = *prp;
      m_intManager->setValue(prop, static_cast<int>(val));
    } else if (auto prp = dynamic_cast<
                   Mantid::Kernel::PropertyWithValue<std::string> *>(
                   property)) {
      prop = m_stringManager->addProperty(propName);
      QString val = QString::fromStdString(prp->value());
      m_stringManager->setValue(prop, val);
    } else if (dynamic_cast<Mantid::API::IWorkspaceProperty *>(property)) {
      prop = m_stringManager->addProperty(propName);
      m_stringManager->setValue(prop,
                                QString::fromStdString((*property).value()));
    } else {
      QMessageBox::warning(this, "MantidPlot - Error",
                           "Type of minimizer's property " + propName +
                               " is not yet supported by the browser.");
      continue;
    }

    if (!prop)
      continue;
    // set the tooltip from property doc string
    QString toolTip = QString::fromStdString((*property).documentation());
    if (!toolTip.isEmpty()) {
      prop->setToolTip(toolTip);
    }
    m_settingsGroup->property()->addSubProperty(prop);
    m_minimizerProperties.append(prop);
  }
}

/**=================================================================================================
 * Get function parameter values
 */
QList<double> FitPropertyBrowser::getParameterValues() const {
  auto fun = theFunction();
  size_t np = fun->nParams();
  QList<double> out;
  for (size_t i = 0; i < np; ++i) {
    const double parValue = fun->getParameter(i);
    out.append(parValue);
  }
  return out;
}

/**=================================================================================================
 * Get parameter names
 */
QStringList FitPropertyBrowser::getParameterNames() const {
  auto fun = theFunction();
  size_t np = fun->nParams();
  QStringList out;
  for (size_t i = 0; i < np; ++i) {
    std::string parName = fun->parameterName(i);
    out.append(QString::fromStdString(parName));
  }
  return out;
}

/**=================================================================================================
 * Show online function help
 */
void FitPropertyBrowser::functionHelp() {
  PropertyHandler *handler = currentHandler();
  if (handler) {
    MantidQt::API::HelpWindow::showFitFunction(this->nativeParentWidget(),
                                               handler->ifun()->name());
  }
}

/**=================================================================================================
 * Show online browser help
 */
void FitPropertyBrowser::browserHelp() {
  MantidDesktopServices::openUrl(
      QUrl("http://www.mantidproject.org/"
           "MantidPlot:_Simple_Peak_Fitting_with_the_Fit_"
           "Wizard#Fit_Properties_Browser"));
}

/**=================================================================================================
 * Allow/disallow sequential fits, depending on whether other conditions are
 * met
 * @param allow :: [input] Allow or disallow
 */
void FitPropertyBrowser::allowSequentialFits(bool allow) {
  if (m_fitActionSeqFit) {
    if (allow) {
      m_fitActionSeqFit->setEnabled(m_compositeFunction->nFunctions() > 0);
    } else {
      m_fitActionSeqFit->setEnabled(false);
    }
  }
}

void FitPropertyBrowser::modifyFitMenu(QAction *fitAction, bool enabled) {
  if (enabled) {
    m_fitMenu->addAction(fitAction);
  } else {
    m_fitMenu->removeAction(fitAction);
  }
}

int MantidQt::MantidWidgets::FitPropertyBrowser::sizeOfFunctionsGroup() const {
  return m_functionsGroup->children().size();
}

void FitPropertyBrowser::addAllowedSpectra(const QString &wsName,
                                           const QList<int> &wsSpectra) {
  auto const name = wsName.toStdString();
  auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(name);
  if (ws) {
    QList<int> indices;
    for (auto const i : wsSpectra) {
      indices.push_back(static_cast<int>(ws->getIndexFromSpectrumNumber(i)));
    }
    m_allowedSpectra.insert(wsName, indices);
  } else {
    throw std::runtime_error("Workspace " + name + " is not a MatrixWorkspace");
  }
}

/**
 * A specialisation of addFunction to be used in the workbench.
 * It returns an id of the added function as a string of "f-dot"
 * indices. This provides function identification that doesn't use
 * PropertyHandlers and can be used on the python side.
 * @param fnName :: Name of a function to add.
 * @return :: An ID string, eg: f0.f2.f1 (There is no dot at the end).
 */
QString FitPropertyBrowser::addFunction(const QString &fnName) {
  return addFunction(fnName.toStdString())->functionPrefix();
}

PropertyHandler *FitPropertyBrowser::getPeakHandler(const QString &prefix) {
  if (prefix.isEmpty())
    throw std::runtime_error("Peak function prefix cannot be empty");
  auto const indexList = prefix.split(".");
  auto const n = indexList.size() - 1;
  auto handler = getHandler();
  for (int i = 0; i < n; ++i) {
    auto const index = indexList[i].mid(1).toInt();
    handler = handler->getHandler(index);
  }
  return handler->getHandler(indexList[n].mid(1).toInt());
}

void FitPropertyBrowser::setPeakCentreOf(const QString &prefix, double value) {
  auto handler = getPeakHandler(prefix);
  handler->setCentre(value);
  handler->updateParameters();
}

double FitPropertyBrowser::getPeakCentreOf(const QString &prefix) {
  auto handler = getPeakHandler(prefix);
  return handler->centre();
}

void FitPropertyBrowser::setPeakHeightOf(const QString &prefix, double value) {
  auto handler = getPeakHandler(prefix);
  handler->setHeight(value);
  handler->updateParameters();
}

double FitPropertyBrowser::getPeakHeightOf(const QString &prefix) {
  auto handler = getPeakHandler(prefix);
  return handler->height();
}

void FitPropertyBrowser::setPeakFwhmOf(const QString &prefix, double value) {
  auto handler = getPeakHandler(prefix);
  handler->setFwhm(value);
  handler->updateParameters();
}

double FitPropertyBrowser::getPeakFwhmOf(const QString &prefix) {
  auto handler = getPeakHandler(prefix);
  return handler->fwhm();
}

QStringList FitPropertyBrowser::getPeakPrefixes() const {
  QStringList peaks;
  auto parentHandler = getHandler();
  auto nFunctions = parentHandler->cfun()->nFunctions();
  for (size_t i = 0; i < nFunctions; ++i) {
    auto handler = parentHandler->getHandler(i);
    if (handler->pfun()) {
      peaks << handler->functionPrefix();
    }
  }
  return peaks;
}

} // namespace MantidWidgets
} // namespace MantidQt
