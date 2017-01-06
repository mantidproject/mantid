#include "MantidQtMantidWidgets/MuonFitPropertyBrowser.h"
#include "MantidQtMantidWidgets/PropertyHandler.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtMantidWidgets/StringEditorFactory.h"

// Suppress a warning coming out of code that isn't ours
#if defined(__INTEL_COMPILER)
#pragma warning disable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#include "DoubleEditorFactory.h"
#include "qteditorfactory.h"
#if defined(__INTEL_COMPILER)
#pragma warning enable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
#endif

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidAPI/IPeakFunction.h"

#include "qttreepropertybrowser.h"
#include "qtpropertymanager.h"

#include <Poco/ActiveResult.h>

#include <QSettings>
#include <QMessageBox>
#include <QAction>
#include <QLayout>
#include <QSplitter>

namespace {
Mantid::Kernel::Logger g_log("MuonFitPropertyBrowser");
}

namespace MantidQt {
namespace MantidWidgets {

using namespace Mantid::API;

const std::string MuonFitPropertyBrowser::SIMULTANEOUS_PREFIX{"MuonSimulFit_"};

/**
 * Constructor
 * @param parent :: The parent widget - must be an ApplicationWindow
 * @param mantidui :: The UI form for MantidPlot
*/
MuonFitPropertyBrowser::MuonFitPropertyBrowser(QWidget *parent,
                                               QObject *mantidui)
    : FitPropertyBrowser(parent, mantidui), m_widgetSplitter(nullptr),
      m_mainSplitter(nullptr) {}

/**
* Initialise the muon fit property browser.
*/
void MuonFitPropertyBrowser::init() {
  QWidget *w = new QWidget(this);

  QSettings settings;
  settings.beginGroup("Mantid/FitBrowser");

  /* Create function group */
  QtProperty *functionsGroup = m_groupManager->addProperty("Functions");
  QtProperty *settingsGroup(NULL);

  // Seperates the data and the settings into two seperate categories
  settingsGroup = m_groupManager->addProperty("Data");

  // Have slightly different names as requested by the muon scientists.
  m_startX =
      addDoubleProperty(QString("Start (%1s)").arg(QChar(0x03BC))); //(mu);
  m_endX = addDoubleProperty(QString("End (%1s)").arg(QChar(0x03BC)));

  // m_workspace = m_enumManager->addProperty("Workspace");
  m_workspaceIndex = m_intManager->addProperty("Workspace Index");
  m_output = m_stringManager->addProperty("Output");
  m_minimizer = m_enumManager->addProperty("Minimizer");
  m_minimizers << "Levenberg-Marquardt"
               << "Simplex"
               << "Conjugate gradient (Fletcher-Reeves imp.)"
               << "Conjugate gradient (Polak-Ribiere imp.)"
               << "BFGS";
  m_enumManager->setEnumNames(m_minimizer, m_minimizers);
  m_costFunction = m_enumManager->addProperty("Cost function");
  m_costFunctions << "Least squares"
                  << "Ignore positive peaks";
  m_enumManager->setEnumNames(m_costFunction, m_costFunctions);

  m_plotDiff = m_boolManager->addProperty("Plot Difference");
  bool plotDiff = settings.value("Plot Difference", QVariant(true)).toBool();
  m_boolManager->setValue(m_plotDiff, plotDiff);

  m_evaluationType = m_enumManager->addProperty("Evaluate Function As");
  m_evaluationType->setToolTip(
      "Consider using Histogram fit which may produce more accurate results.");
  m_evaluationTypes << "CentrePoint"
                    << "Histogram";
  m_enumManager->setEnumNames(m_evaluationType, m_evaluationTypes);
  int evaluationType =
      settings.value(m_evaluationType->propertyName(), 0).toInt();
  m_enumManager->setValue(m_evaluationType, evaluationType);

  settingsGroup->addSubProperty(m_workspace);
  settingsGroup->addSubProperty(m_workspaceIndex);
  settingsGroup->addSubProperty(m_startX);
  settingsGroup->addSubProperty(m_endX);

  /* Create editors and assign them to the managers */
  createEditors(w);

  updateDecimals();

  m_functionsGroup = m_browser->addProperty(functionsGroup);
  m_settingsGroup = m_browser->addProperty(settingsGroup);

  // Don't show "Function" or "Data" sections as they have separate widgets
  m_browser->setItemVisible(m_functionsGroup, false);
  m_browser->setItemVisible(m_settingsGroup, false);

  // Custom settings that are specific and asked for by the muon scientists.
  QtProperty *customSettingsGroup = m_groupManager->addProperty("Settings");

  m_rawData = m_boolManager->addProperty("Fit To Raw Data");
  bool data = settings.value("Fit To Raw Data", QVariant(false)).toBool();
  m_boolManager->setValue(m_rawData, data);

  m_showParamErrors = m_boolManager->addProperty("Show Parameter Errors");
  // XXX: showParamErrors is true by default for Muons
  bool showParamErrors =
      settings.value(m_showParamErrors->propertyName(), true).toBool();
  m_boolManager->setValue(m_showParamErrors, showParamErrors);
  m_parameterManager->setErrorsEnabled(showParamErrors);

  customSettingsGroup->addSubProperty(m_minimizer);
  customSettingsGroup->addSubProperty(m_plotDiff);
  customSettingsGroup->addSubProperty(m_rawData);
  customSettingsGroup->addSubProperty(m_showParamErrors);
  customSettingsGroup->addSubProperty(m_evaluationType);

  m_customSettingsGroup = m_browser->addProperty(customSettingsGroup);

  // Initialise the layout.
  initLayout(w);

  // Create an empty splitter that can hold extra widgets
  m_widgetSplitter = new QSplitter(Qt::Vertical, w);
  m_widgetSplitter->setSizePolicy(QSizePolicy::Policy::Expanding,
                                  QSizePolicy::Policy::Expanding);

  // This splitter separates the "extra widgets" region from the browser
  m_mainSplitter = new QSplitter(Qt::Vertical, w);
  m_mainSplitter->insertWidget(0, m_widgetSplitter);
  m_mainSplitter->insertWidget(1, m_browser);
  m_mainSplitter->setStretchFactor(0, 1);
  m_mainSplitter->setStretchFactor(1, 0);

  // Insert after the buttons
  auto parentLayout = qobject_cast<QVBoxLayout *>(w->layout());
  if (parentLayout) {
    const int index = parentLayout->count() - 1;
    constexpr int stretchFactor = 10; // so these widgets get any extra space
    parentLayout->insertWidget(index, m_mainSplitter, stretchFactor);
    parentLayout->setSpacing(0);
    parentLayout->setMargin(0);
    parentLayout->setContentsMargins(0, 0, 0, 0);
  }
}

/**
* Set the input workspace name
*/
void MuonFitPropertyBrowser::setWorkspaceName(const QString &wsName) {
  int i = m_workspaceNames.indexOf(wsName);
  if (i < 0) {
    // workspace may not be found because add notification hasn't been processed
    // yet
    populateWorkspaceNames();
    i = m_workspaceNames.indexOf(wsName);
  }
  if (i >= 0)
    m_enumManager->setValue(m_workspace, i);
}

/** Called when a double property changed
 * @param prop :: A pointer to the property
 */
void MuonFitPropertyBrowser::doubleChanged(QtProperty *prop) {
  if (!m_changeSlotsEnabled)
    return;

  double value = m_doubleManager->value(prop);
  if (prop == m_startX) {
    // call setWorkspace to change maxX in functions
    setWorkspace(m_compositeFunction);
    getHandler()->setAttribute(QString("Start (%1s)").arg(QChar(0x03BC)),
                               value); // (mu)
    emit startXChanged(startX());
    emit xRangeChanged(startX(), endX());
    return;
  } else if (prop == m_endX) {
    // call setWorkspace to change minX in functions
    setWorkspace(m_compositeFunction);
    getHandler()->setAttribute(QString("End (%1s)").arg(QChar(0x03BC)), value);
    emit endXChanged(endX());
    emit xRangeChanged(startX(), endX());
    return;
  } else { // check if it is a constraint
    MantidQt::MantidWidgets::PropertyHandler *h =
        getHandler()->findHandler(prop);
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

/** Called when a bool property changed
 * @param prop :: A pointer to the property
 */
void MuonFitPropertyBrowser::boolChanged(QtProperty *prop) {
  if (prop == m_rawData) {
    const bool val = m_boolManager->value(prop);
    emit fitRawDataClicked(val);
  } else {
    // defer to parent class
    FitPropertyBrowser::boolChanged(prop);
  }
}

/**
*Get the registered function names
*/
void MuonFitPropertyBrowser::populateFunctionNames() {
  const std::vector<std::string> names = FunctionFactory::Instance().getKeys();
  m_registeredFunctions.clear();
  m_registeredPeaks.clear();
  m_registeredBackgrounds.clear();
  for (size_t i = 0; i < names.size(); i++) {
    std::string fnName = names[i];
    QString qfnName = QString::fromStdString(fnName);
    if (qfnName == "MultiBG")
      continue;

    auto f = FunctionFactory::Instance().createFunction(fnName);
    const std::vector<std::string> categories = f->categories();
    bool muon = false;
    for (size_t j = 0; j < categories.size(); ++j) {
      if ((categories[j] == "Muon") || (categories[j] == "General") ||
          (categories[j] == "Background"))
        muon = true;
    }
    if (muon) {
      m_registeredFunctions << qfnName;
    }
    IPeakFunction *pf = dynamic_cast<IPeakFunction *>(f.get());
    // CompositeFunction* cf = dynamic_cast<CompositeFunction*>(f.get());
    if (pf) {
      m_registeredPeaks << qfnName;
    } else if (dynamic_cast<IBackgroundFunction *>(f.get())) {
      m_registeredBackgrounds << qfnName;
    } else {
      m_registeredOther << qfnName;
    }
  }
}

/**
 * Updates function prior to running a fit
 */
void MuonFitPropertyBrowser::fit() {
  emit functionUpdateAndFitRequested(false);
}

/**
 * Creates an instance of Fit algorithm, sets its properties and launches it.
 */
void MuonFitPropertyBrowser::runFit() {
  std::string wsName = workspaceName();

  if (wsName.empty()) {
    QMessageBox::critical(this, "Mantid - Error", "Workspace name is not set");
    return;
  }
  try {
    m_initialParameters.resize(compositeFunction()->nParams());
    for (size_t i = 0; i < compositeFunction()->nParams(); i++) {
      m_initialParameters[i] = compositeFunction()->getParameter(i);
    }
    m_fitActionUndoFit->setEnabled(true);

    // Delete any existing results for this workspace, UNLESS we are doing a
    // simultaneous fit
    if (m_workspacesToFit.size() < 2) {
      if (AnalysisDataService::Instance().doesExist(
              wsName + "_NormalisedCovarianceMatrix")) {
        FrameworkManager::Instance().deleteWorkspace(
            wsName + "_NormalisedCovarianceMatrix");
      }
      if (AnalysisDataService::Instance().doesExist(wsName + "_Parameters")) {
        FrameworkManager::Instance().deleteWorkspace(wsName + "_Parameters");
      }
      if (AnalysisDataService::Instance().doesExist(wsName + "_Workspace")) {
        FrameworkManager::Instance().deleteWorkspace(wsName + "_Workspace");
      }
    }

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("Fit");
    alg->initialize();
    if (m_compositeFunction->name() == "MultiBG") {
      alg->setPropertyValue("Function", "");
    } else if (m_compositeFunction->nFunctions() > 1) {
      alg->setProperty("Function", boost::dynamic_pointer_cast<IFunction>(
                                       m_compositeFunction));
    } else {
      alg->setProperty("Function", boost::dynamic_pointer_cast<IFunction>(
                                       m_compositeFunction->getFunction(0)));
    }
    if (rawData())
      alg->setPropertyValue("InputWorkspace", wsName + "_Raw");
    else
      alg->setPropertyValue("InputWorkspace", wsName);
    alg->setProperty("WorkspaceIndex", workspaceIndex());
    alg->setProperty("StartX", startX());
    alg->setProperty("EndX", endX());
    alg->setPropertyValue("Output", outputName());
    alg->setPropertyValue("Minimizer", minimizer());
    alg->setPropertyValue("CostFunction", costFunction());

    // If we are doing a simultaneous fit, set this up here
    const int nWorkspaces = static_cast<int>(m_workspacesToFit.size());
    if (nWorkspaces > 1) {
      alg->setPropertyValue("InputWorkspace", m_workspacesToFit[0]);
      // Remove existing results with the same name
      if (AnalysisDataService::Instance().doesExist(outputName())) {
        AnalysisDataService::Instance().deepRemoveGroup(outputName());
      }
      for (int i = 1; i < nWorkspaces; i++) {
        std::string suffix = boost::lexical_cast<std::string>(i);
        alg->setPropertyValue("InputWorkspace_" + suffix, m_workspacesToFit[i]);
        alg->setProperty("WorkspaceIndex_" + suffix, workspaceIndex());
        alg->setProperty("StartX_" + suffix, startX());
        alg->setProperty("EndX_" + suffix, endX());
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
 * Show sequential fit dialog.
 */
void MuonFitPropertyBrowser::runSequentialFit() {
  emit sequentialFitRequested();
}

/**
 * Update function prior to running a sequential fit
 */
void MuonFitPropertyBrowser::sequentialFit() {
  emit functionUpdateAndFitRequested(true);
}

/**
 * Connect to the AnalysisDataService when shown
 */
void MuonFitPropertyBrowser::showEvent(QShowEvent *e) {
  (void)e;
  observePostDelete();
  populateWorkspaceNames();
}

/** Check if the workspace can be used in the fit. The accepted types are
  * MatrixWorkspaces same size and that it isn't the generated raw file.
  * @param ws :: The workspace
  */
bool MuonFitPropertyBrowser::isWorkspaceValid(Workspace_sptr ws) const {
  QString workspaceName(QString::fromStdString(ws->name()));

  if ((workspaceName.contains("_Raw")) ||
      (workspaceName.contains("MuonAnalysis")))
    return false;

  // Exclude fitting results
  if (workspaceName.endsWith("_Workspace"))
    return false;

  return dynamic_cast<MatrixWorkspace *>(ws.get()) != 0;
}

void MuonFitPropertyBrowser::finishHandle(const IAlgorithm *alg) {
  // Copy experiment info to output workspace
  if (AnalysisDataService::Instance().doesExist(outputName() + "_Workspace")) {
    // Input workspace should be a MatrixWorkspace according to isWorkspaceValid
    auto inWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        static_cast<std::string>(alg->getProperty("InputWorkspace")));
    auto outWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        outputName() + "_Workspace");
    if (inWs && outWs) {
      outWs->copyExperimentInfoFrom(inWs.get());
    }
  } else if (AnalysisDataService::Instance().doesExist(outputName() +
                                                       "_Workspaces")) {
    // Output workspace was a group
    auto outGroup = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
        outputName() + "_Workspaces");
    if (outGroup->size() == m_workspacesToFit.size()) {
      for (size_t i = 0; i < outGroup->size(); i++) {
        auto outWs =
            boost::dynamic_pointer_cast<MatrixWorkspace>(outGroup->getItem(i));
        auto inWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            m_workspacesToFit[i]);
        if (inWs && outWs) {
          outWs->copyExperimentInfoFrom(inWs.get());
        }
      }
    }
  }

  // If fit was simultaneous, insert extra information into params table
  // and group the output workspaces
  const int nWorkspaces = static_cast<int>(m_workspacesToFit.size());
  if (nWorkspaces > 1) {
    finishAfterSimultaneousFit(alg, nWorkspaces);
  }

  FitPropertyBrowser::finishHandle(alg);
}

/**
 * After a simultaneous fit, insert extra information into parameters table
 * (i.e. what runs, groups, periods "f0", "f1" etc were)
 * and group the output workspaces
 * @param fitAlg :: [input] Pointer to fit algorithm that just finished
 * @param nWorkspaces :: [input] Number of workspaces that were fitted
 */
void MuonFitPropertyBrowser::finishAfterSimultaneousFit(
    const Mantid::API::IAlgorithm *fitAlg, const int nWorkspaces) const {
  AnalysisDataServiceImpl &ads = AnalysisDataService::Instance();
  try {
    const std::string paramTableName = fitAlg->getProperty("OutputParameters");
    const auto paramTable = ads.retrieveWS<ITableWorkspace>(paramTableName);
    if (paramTable) {
      Mantid::API::TableRow f0Row = paramTable->appendRow();
      f0Row << "f0=" + fitAlg->getPropertyValue("InputWorkspace") << 0.0 << 0.0;
      for (int i = 1; i < nWorkspaces; i++) {
        const std::string suffix = boost::lexical_cast<std::string>(i);
        const auto wsName =
            fitAlg->getPropertyValue("InputWorkspace_" + suffix);
        Mantid::API::TableRow row = paramTable->appendRow();
        row << "f" + suffix + "=" + wsName << 0.0 << 0.0;
      }
    }
  } catch (const Mantid::Kernel::Exception::NotFoundError &) {
    // Not a fatal error, but shouldn't happen
    g_log.warning(
        "Could not find output parameters table for simultaneous fit");
  }

  // Group output together
  std::string groupName = fitAlg->getPropertyValue("Output");
  std::string baseName = groupName;
  if (ads.doesExist(groupName)) {
    ads.deepRemoveGroup(groupName);
  }

  // Create a group for label
  try {
    ads.add(groupName, boost::make_shared<WorkspaceGroup>());
    ads.addToGroup(groupName, baseName + "_NormalisedCovarianceMatrix");
    ads.addToGroup(groupName, baseName + "_Parameters");
    ads.addToGroup(groupName, baseName + "_Workspaces");
  } catch (const Mantid::Kernel::Exception::NotFoundError &err) {
    g_log.warning(err.what());
  }
}

/**
 * Adds an extra widget in between the fit buttons and the browser
 * @param widget :: [input] Pointer to widget to add
 */
void MuonFitPropertyBrowser::addExtraWidget(QWidget *widget) {
  widget->setSizePolicy(QSizePolicy::Policy::Expanding,
                        QSizePolicy::Policy::Expanding);
  if (m_widgetSplitter) {
    m_widgetSplitter->addWidget(widget);
  }
}

/**
 * Called externally to set the function
 * @param func :: [input] Fit function to use
 */
void MuonFitPropertyBrowser::setFunction(const IFunction_sptr func) {
  createCompositeFunction(func);
}

/**
 * Set the list of workspaces to fit to the given list
 * @param wsNames :: [input] List of workspace names to fit
 */
void MuonFitPropertyBrowser::setWorkspaceNames(const QStringList &wsNames) {
  // Extend base class behaviour
  IWorkspaceFitControl::setWorkspaceNames(wsNames);

  m_workspacesToFit.clear();
  std::transform(wsNames.begin(), wsNames.end(),
                 std::back_inserter(m_workspacesToFit),
                 [](const QString &qs) { return qs.toStdString(); });
  // Update listeners
  emit workspacesToFitChanged(static_cast<int>(m_workspacesToFit.size()));
}

/**
 * Override in the case of simultaneous fits to use a special prefix.
 * Otherwise, use the parent class method.
 * @returns :: output name for Fit algorithm
 */
std::string MuonFitPropertyBrowser::outputName() const {
  const int nWorkspaces = static_cast<int>(m_workspacesToFit.size());
  if (nWorkspaces > 1) {
    // simultaneous fit
    return SIMULTANEOUS_PREFIX + m_simultaneousLabel;
  } else {
    // use parent class behaviour
    return FitPropertyBrowser::outputName();
  }
}

/**
 * Set multiple fitting mode on or off.
 * If turned off, all parts of the fit property browser are shown and all extra
 * widgets (like the function browser or data selector) are hidden, so it looks
 * just like it used to before the changes in Mantid 3.8.
 * If turned on, the "Function" and "Data" sections of the fit property browser
 * are hidden and the extra widgets are shown.
 * @param enabled :: [input] Whether to turn this mode on or off
 */
void MuonFitPropertyBrowser::setMultiFittingMode(bool enabled) {
  // First, clear whatever model is currently set
  this->clear();

  // Show or hide "Function" and "Data" sections
  m_browser->setItemVisible(m_functionsGroup, !enabled);
  m_browser->setItemVisible(m_settingsGroup, !enabled);

  // Show or hide additional widgets
  for (int i = 0; i < m_widgetSplitter->count(); ++i) {
    if (auto *widget = m_widgetSplitter->widget(i)) {
      widget->setVisible(enabled);
    }
  }
}

/**
 * Returns whether or not a guess is plotted
 * @returns :: True if a plot guess is plotted, false if not.
 */
bool MuonFitPropertyBrowser::hasGuess() const {
  auto *handler = getHandler();
  if (handler) {
    const bool hasPlot = handler->hasPlot(); // don't allow caller to modify
    return hasPlot;
  } else {
    return false;
  }
}

} // MantidQt
} // API
