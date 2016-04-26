#include "MantidQtMantidWidgets/MuonFitPropertyBrowser.h"
#include "MantidQtMantidWidgets/PropertyHandler.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
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

namespace MantidQt {
namespace MantidWidgets {

using namespace Mantid::API;

/**
 * Constructor
 * @param parent :: The parent widget - must be an ApplicationWindow
 * @param mantidui :: The UI form for MantidPlot
*/
MuonFitPropertyBrowser::MuonFitPropertyBrowser(QWidget *parent,
                                               QObject *mantidui)
    : FitPropertyBrowser(parent, mantidui) {}

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

  m_customSettingsGroup = m_browser->addProperty(customSettingsGroup);

  // Initialise the layout.
  initLayout(w);

  // Create an empty layout that can hold extra widgets
  // and add it after the buttons but before the browser
  m_additionalLayout = new QVBoxLayout();
  auto parentLayout = qobject_cast<QVBoxLayout *>(w->layout());
  if (parentLayout) {
    const int index = parentLayout->count() - 2;
    parentLayout->insertLayout(index, m_additionalLayout);
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

    std::string funStr;
    if (m_compositeFunction->name() == "MultiBG") {
      funStr = "";
    } else if (m_compositeFunction->nFunctions() > 1) {
      funStr = m_compositeFunction->asString();
    } else {
      funStr = (m_compositeFunction->getFunction(0))->asString();
    }

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

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("Fit");
    alg->initialize();
    alg->setPropertyValue("Function", funStr);
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
    observeFinish(alg);
    alg->executeAsync();
  } catch (std::exception &e) {
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
  // Input workspace should be a MatrixWorkspace according to isWorkspaceValid
  auto inWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      static_cast<std::string>(alg->getProperty("InputWorkspace")));

  auto outWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      outputName() + "_Workspace");

  if (inWs && outWs)
    outWs->copyExperimentInfoFrom(inWs.get());

  FitPropertyBrowser::finishHandle(alg);
}

/**
 * Adds an extra widget in between the fit buttons and the browser
 * @param widget :: [input] Pointer to widget to add
 */
void MuonFitPropertyBrowser::addExtraWidget(QWidget *widget) {
  if (m_additionalLayout) {
    m_additionalLayout->addWidget(widget);
  }
}

/**
 * Called externally to set the function via a string
 * @param funcString :: [input] Fit function as a string
 */
void MuonFitPropertyBrowser::setFunction(const QString &funcString) {
  createCompositeFunction(funcString);
}

/**
 * Called externally to set the value of a parameter
 * @param funcIndex :: [input] Function index
 * @param paramName :: [input] Name of parameter
 * @param value :: [input] Value to set parameter to
 */
void MuonFitPropertyBrowser::setParameterValue(const QString &funcIndex,
                                               const QString &paramName,
                                               double value) {
  throw std::runtime_error("TODO: implement setParameterValue");
}

} // MantidQt
} // API
