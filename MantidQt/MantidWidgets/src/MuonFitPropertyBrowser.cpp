#include "MantidQtMantidWidgets/MuonFitPropertyBrowser.h"
#include "MantidQtMantidWidgets/PropertyHandler.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidQtMantidWidgets/StringEditorFactory.h"

#include "MantidQtMantidWidgets/MuonFitDataSelector.h"

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
#include <QFormLayout>

#include <QLayout>
#include <QSplitter>
#include <QLabel>
#include <QPushButton>

#include <QMenu>
#include <QSignalMapper>

#include <QCheckBox>

namespace {
Mantid::Kernel::Logger g_log("MuonFitPropertyBrowser");
const QString CUSTOM_LABEL{"Custom"};
const QString ALL_GROUPS_LABEL{"All Groups"};
const QString ALL_PAIRS_LABEL{"All Pairs"};
const QString ALL_PERIODS_LABEL{"All Periods"};
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

  QSettings multiFitSettings;
  multiFitSettings.beginGroup("");

  /* Create function group */
  QtProperty *multiFitSettingsGroup(NULL);

  // Seperates the data and the settings into two seperate categories
  multiFitSettingsGroup = m_groupManager->addProperty("Data");

  // Have slightly different names as requested by the muon scientists.
  m_startX =
      addDoubleProperty(QString("Start (%1s)").arg(QChar(0x03BC))); //(mu);
  m_endX = addDoubleProperty(QString("End (%1s)").arg(QChar(0x03BC)));

  m_normalization = m_enumManager->addProperty("Normalization");
  setNormalization();

  m_keepNorm = m_boolManager->addProperty("Fix Normalization");
  bool keepNorm = settings.value("Fix Normalization", QVariant(false)).toBool();
  m_boolManager->setValue(m_keepNorm, keepNorm);

  m_workspace = m_enumManager->addProperty("Workspace");
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
  settingsGroup->addSubProperty(m_normalization);
  settingsGroup->addSubProperty(m_keepNorm);

  // Disable "Browse" button - use case is that first run will always be the one
  // selected on front tab. User will type in the runs they want rather than
  // using the Browse button. (If they want to "Browse" they can use front tab).

  multiFitSettingsGroup->addSubProperty(m_startX);
  multiFitSettingsGroup->addSubProperty(m_endX);
  m_groupsToFit = m_enumManager->addProperty("Groups/Pairs to fit");
  m_groupsToFitOptions << ALL_GROUPS_LABEL << ALL_PAIRS_LABEL << CUSTOM_LABEL;
  m_showGroupValue << "groups";
  m_showGroup = m_enumManager->addProperty("Selected Groups");
  m_enumManager->setEnumNames(m_groupsToFit, m_groupsToFitOptions);
  multiFitSettingsGroup->addSubProperty(m_groupsToFit);
  multiFitSettingsGroup->addSubProperty(m_showGroup);

  m_enumManager->setEnumNames(m_showGroup, m_showGroupValue);
  QString tmp = "fwd";
  addGroupCheckbox(tmp);
  tmp = "bwd";
  addGroupCheckbox(tmp);
  m_periodsToFit = m_enumManager->addProperty("Periods to fit");
  m_periodsToFitOptions << ALL_PERIODS_LABEL << "1"
                        << "2" << CUSTOM_LABEL;
  m_showPeriodValue << "1";
  m_showPeriods = m_enumManager->addProperty("Selected Periods");
  m_enumManager->setEnumNames(m_periodsToFit, m_periodsToFitOptions);
  multiFitSettingsGroup->addSubProperty(m_periodsToFit);
  multiFitSettingsGroup->addSubProperty(m_showPeriods);
  m_enumManager->setEnumNames(m_showPeriods, m_showPeriodValue);

  /* Create editors and assign them to the managers */
  createEditors(w);

  updateDecimals();

  m_functionsGroup = m_browser->addProperty(functionsGroup);
  m_settingsGroup = m_browser->addProperty(settingsGroup);
  m_multiFitSettingsGroup = m_browser->addProperty(multiFitSettingsGroup);
  connect(m_browser, SIGNAL(currentItemChanged(QtBrowserItem *)), this,
          SLOT(currentItemChanged(QtBrowserItem *)));

  m_btnGroup = new QGroupBox(tr("Reselect Data"));
  QHBoxLayout *btnLayout = new QHBoxLayout;
  m_reselectGroupBtn = new QPushButton("Groups/Pairs");
  m_reselectPeriodBtn = new QPushButton("Periods");
  m_generateBtn = new QPushButton("Combine Periods");
  m_groupWindow = new QDialog(this);
  m_periodWindow = new QDialog(this);
  m_comboWindow = new QDialog(this);

  m_reselectGroupBtn->setEnabled(false);
  m_reselectPeriodBtn->setEnabled(false);
  connect(m_reselectGroupBtn, SIGNAL(released()), this,
          SLOT(groupBtnPressed()));
  connect(m_reselectPeriodBtn, SIGNAL(released()), this,
          SLOT(periodBtnPressed()));
  connect(m_generateBtn, SIGNAL(released()), this, SLOT(generateBtnPressed()));

  btnLayout->addWidget(m_reselectGroupBtn);
  btnLayout->addWidget(m_reselectPeriodBtn);
  btnLayout->addWidget(m_generateBtn);

  m_btnGroup->setLayout(btnLayout);

  // Don't show "Function" or "Data" sections as they have separate widgets
  m_browser->setItemVisible(m_functionsGroup, false);
  m_browser->setItemVisible(m_settingsGroup, false);
  m_browser->setItemVisible(m_multiFitSettingsGroup, true);
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
  initBasicLayout(w);

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
    parentLayout->insertWidget(index + 1, m_btnGroup);
  }
  // Update tooltips when function structure is (or might've been) changed in
  // any way
  connect(this, SIGNAL(functionChanged()), SLOT(updateStructureTooltips()));
}
// Set up the execution of the muon fit menu
void MuonFitPropertyBrowser::executeFitMenu(const QString &item) {
  if (item == "TFAsymm") {
    doTFAsymmFit(1000);
  } else {
    FitPropertyBrowser::executeFitMenu(item);
  }
}
// Create group/pair selection pop up
void MuonFitPropertyBrowser::groupBtnPressed() { genGroupWindow(); }
// Create period selection pop up
void MuonFitPropertyBrowser::periodBtnPressed() { genPeriodWindow(); }
// Create combination selection pop up
void MuonFitPropertyBrowser::generateBtnPressed() { genCombinePeriodWindow(); }
/**
pulate the fit button.
* This initialization includes:
*   1. SIGNALs/SLOTs when properties change.
*   2. Actions and associated SIGNALs/SLOTs.
* @param fitMapper the QMap to the fit mapper
* @param fitMenu the QMenu for the fit button
*/
void MuonFitPropertyBrowser::populateFitMenuButton(QSignalMapper *fitMapper,
                                                   QMenu *fitMenu) {

  m_fitActionTFAsymm = new QAction("TF Asymmetry Fit", this);
  fitMapper->setMapping(m_fitActionTFAsymm, "TFAsymm");

  FitPropertyBrowser::populateFitMenuButton(fitMapper, fitMenu);
  connect(m_fitActionTFAsymm, SIGNAL(triggered()), fitMapper, SLOT(map()));
  fitMenu->addSeparator();
  fitMenu->addAction(m_fitActionTFAsymm);
}
/// Enable/disable the Fit button;
void MuonFitPropertyBrowser::setFitEnabled(bool yes) {
  m_fitActionFit->setEnabled(yes);
  m_fitActionSeqFit->setEnabled(yes);
  // only allow TFAsymm fit if not keeping norm
  if (!m_boolManager->value(m_keepNorm) && yes) {
    m_fitActionTFAsymm->setEnabled(yes);
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
/** Called when a dropdown menu is changed
* @param prop :: A pointer to the function name property
*/
void MuonFitPropertyBrowser::enumChanged(QtProperty *prop) {
  if (!m_changeSlotsEnabled)
    return;
  if (prop == m_groupsToFit) {
    int j = m_enumManager->value(m_groupsToFit);
    QString option = m_groupsToFitOptions[j];

    if (option == ALL_GROUPS_LABEL) {
      setAllGroups();
      m_reselectGroupBtn->setEnabled(false);
    } else if (option == ALL_PAIRS_LABEL) {
      setAllPairs();
      m_reselectGroupBtn->setEnabled(false);
    } else if (option == CUSTOM_LABEL) {
      m_reselectGroupBtn->setEnabled(true);
      genGroupWindow();
    }
    updateGroupDisplay();

  } else if (prop == m_periodsToFit) {
    int j = m_enumManager->value(m_periodsToFit);
    QString option = m_periodsToFitOptions[j];
    if (option == CUSTOM_LABEL) {
      m_reselectPeriodBtn->setEnabled(true);
      genPeriodWindow();
    } else if (option == ALL_PERIODS_LABEL) {
      setAllPeriods();
      m_reselectPeriodBtn->setEnabled(false);
    } else {
      for (auto iter = m_periodBoxes.constBegin();
           iter != m_periodBoxes.constEnd(); ++iter) {
        if (option == iter.key()) {
          m_boolManager->setValue(iter.value(), true);
        } else {
          m_boolManager->setValue(iter.value(), false);
        }
        m_reselectPeriodBtn->setEnabled(false);
      }
    }
    updatePeriodDisplay();
  } else if (prop == m_workspace) {
    // make sure the output is updated
    FitPropertyBrowser::enumChanged(prop);
    int j = m_enumManager->value(m_workspace);
    std::string option = m_workspaceNames[j].toStdString();
    setOutputName(option);
    if (m_periodBoxes.size() > 1) {
      size_t end = 0;
      // assumed structure of name
      // isolate the period
      for (int k = 0; k < 4; k++) {
        end = option.find_first_of(";");
        option = option.substr(end + 1, option.size());
      }
      end = option.find_first_of(";");
      QString selectedPeriod = QString::fromStdString(option.substr(0, end));
      // turn on only the relevant box
      for (auto iter = m_periodBoxes.constBegin();
           iter != m_periodBoxes.constEnd(); ++iter) {
        m_boolManager->setValue(iter.value(), selectedPeriod == iter.key());
      }
    }
  } else {
    FitPropertyBrowser::enumChanged(prop);
  }
}
/** Sets the display for
* selected groups
*/
void MuonFitPropertyBrowser::updateGroupDisplay() {
  m_showGroupValue.clear();
  auto tmp = getChosenGroups().join(",").toStdString();
  m_showGroupValue << getChosenGroups().join(",");
  m_enumManager->setEnumNames(m_showGroup, m_showGroupValue);
  m_multiFitSettingsGroup->property()->addSubProperty(m_showGroup);
}
/** Sets the display for
* selected periods
*/
void MuonFitPropertyBrowser::updatePeriodDisplay() {
  m_showPeriodValue.clear();
  auto tmp = getChosenPeriods();
  tmp.replaceInStrings(QRegExp(","), "+");
  m_showPeriodValue << tmp.join(",");
  m_enumManager->setEnumNames(m_showPeriods, m_showPeriodValue);
  if (m_periodsToFitOptions.size() > 1) {
    m_multiFitSettingsGroup->property()->addSubProperty(m_showPeriods);
  }
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
/** @returns the normalization
*/
double MuonFitPropertyBrowser::normalization() const {
  return readNormalization()[0];
}
void MuonFitPropertyBrowser::setNormalization() {
  m_normalizationValue.clear();
  m_normalizationValue.append(QString::number(normalization()));
  m_enumManager->setEnumNames(m_normalization, m_normalizationValue);
}

/** Called when a bool property changed
 * @param prop :: A pointer to the property
 */
void MuonFitPropertyBrowser::boolChanged(QtProperty *prop) {
  if (prop == m_rawData) {
    const bool val = m_boolManager->value(prop);
    emit fitRawDataClicked(val);
  }
  if (prop == m_keepNorm) {
    const bool val = m_boolManager->value(prop);
    if (val) { // record data for later
      double norm = readNormalization()[0];
      ITableWorkspace_sptr table = WorkspaceFactory::Instance().createTable();
      AnalysisDataService::Instance().addOrReplace("__keepNorm__", table);
      table->addColumn("double", "norm");
      table->addColumn("int", "spectra");
      TableRow row = table->appendRow();
      row << norm << 0;
      // remove TFAsymm fit
      m_fitActionTFAsymm->setEnabled(false);

    } else { // remove data so it is not used later
      AnalysisDataService::Instance().remove("__keepNorm__");

      // if fit is enabled so should TFAsymm
      if (m_fitActionSeqFit->isEnabled()) {
        m_fitActionTFAsymm->setEnabled(true);
      }
    }
  } else {
    // search map for group/pair change
    bool done = false;
    for (auto iter = m_groupBoxes.constBegin(); iter != m_groupBoxes.constEnd();
         ++iter) {
      if (iter.value() == prop) {
        done = true;
        updateGroupDisplay();
        emit groupBoxClicked();
      }
    }
    // search map for period change
    if (done == false) {
      for (auto iter = m_periodBoxes.constBegin();
           iter != m_periodBoxes.constEnd(); ++iter) {
        if (iter.value() == prop) {
          done = true;
          updatePeriodDisplay();
          emit periodBoxClicked();
        }
      }
    }
    if (done == false) {
      // defer to parent class
      FitPropertyBrowser::boolChanged(prop);
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
* Creates an instance of Fit algorithm, sets its properties and launches it.
* @param maxIterations is the maximum number of iterations for the fit
*/
void MuonFitPropertyBrowser::doTFAsymmFit(int maxIterations) {
  const std::string wsName = workspaceName();

  if (wsName.empty()) {
    QMessageBox::critical(this, "Mantid - Error", "Workspace name is not set");
    return;
  }

  const auto ws = getWorkspace();
  if (!ws) {
    return;
  }

  if (compositeFunction()->nParams() == 0) {
    throw std::runtime_error("No function has been defiend for fitting");
  }
  m_initialParameters.resize(compositeFunction()->nParams());
  for (size_t i = 0; i < compositeFunction()->nParams(); i++) {
    m_initialParameters[i] = compositeFunction()->getParameter(i);
  }
  m_fitActionUndoFit->setEnabled(true);

  // Calculate the asymmetry

  std::string funStr = getFittingFunction()->asString();

  Mantid::API::IAlgorithm_sptr asymmAlg =
      Mantid::API::AlgorithmManager::Instance().create(
          "CalculateMuonAsymmetry");
  asymmAlg->initialize();
  asymmAlg->setPropertyValue("FittingFunction", funStr);
  asymmAlg->setProperty("InputDataType", "asymmetry");
  asymmAlg->setProperty("InputWorkspace", ws);
  asymmAlg->setProperty("StartX", startX());
  asymmAlg->setProperty("EndX", endX());
  asymmAlg->setPropertyValue("OutputWorkspace", wsName);
  asymmAlg->setPropertyValue("Minimizer", minimizer(true));
  asymmAlg->setProperty("MaxIterations", maxIterations);
  std::vector<double> norm = readNormalization();
  std::vector<int> spectra;
  spectra.push_back(0);

  asymmAlg->setProperty("Spectra", spectra);
  asymmAlg->setProperty("PreviousNormalizationConstant", norm);
  asymmAlg->execute();
  if (!asymmAlg->isExecuted()) {
    throw std::runtime_error("Asymmetry Calculation has failed.");
  }
  // record result
  auto tmp = asymmAlg->getPropertyValue("NormalizationConstant");
  std::vector<double> normEst =
      Mantid::Kernel::VectorHelper::splitStringIntoVector<double>(tmp);
  ITableWorkspace_sptr table = WorkspaceFactory::Instance().createTable();
  AnalysisDataService::Instance().addOrReplace("__norm__", table);
  table->addColumn("double", "norm");
  table->addColumn("int", "spectra");

  for (double norm : normEst) {
    TableRow row = table->appendRow();

    row << norm << 0;
  }
  /////////////////////////////////////////////////
  // calculate the fit explicitly -> above does not get the function exactly
  // right
  try {
    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().create("Fit");
    alg->initialize();
    if (isHistogramFit()) {
      alg->setProperty("EvaluationType", "Histogram");
    }
    alg->setPropertyValue("Function", funStr);
    alg->setProperty("InputWorkspace", ws); // try the raw workspace....
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
      alg->setProperty("Normalise", getShouldBeNormalised());
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
  setNormalization();
}

std::vector<double> readNormalization() {
  std::vector<double> norm;
  if (!AnalysisDataService::Instance().doesExist("__norm__")) {
    norm.push_back(22.423);
  } else {
    Mantid::API::ITableWorkspace_sptr table =
        boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve("__norm__"));
    auto colNorm = table->getColumn("norm");

    for (size_t j = 0; j < table->rowCount(); j++) {
      norm.push_back((*colNorm)[j]); // record and update norm....
    }
  }
  return norm;
}
/**
 * Requests checks and updates prior to running a fit
 */
void MuonFitPropertyBrowser::fit() { emit preFitChecksRequested(false); }

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
    } else {
      setSingleFitLabel(wsName);
    }
    alg->setPropertyValue("Output", outputName());

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
 * Requests checks and updates prior to running a sequential fit
 */
void MuonFitPropertyBrowser::sequentialFit() {
  emit preFitChecksRequested(true);
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
  QString workspaceName(QString::fromStdString(ws->getName()));

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
  m_workspacesToFit.clear();
  std::transform(wsNames.begin(), wsNames.end(),
                 std::back_inserter(m_workspacesToFit),
                 [](const QString &qs) { return qs.toStdString(); });
  // Update listeners
  emit workspacesToFitChanged(static_cast<int>(m_workspacesToFit.size()));
  // Update norm
  setNormalization();
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
  modifyFitMenu(m_fitActionEvaluate, !enabled);
  modifyFitMenu(m_fitActionSeqFit, !enabled);
  // set default selection (all groups)
  if (enabled) {
    setAllGroups();
    setAllPeriods();
  } else { // clear current selection
    clearChosenGroups();
    clearChosenPeriods();
  }
  // Show or hide "Function" and "Data" sections
  m_browser->setItemVisible(m_functionsGroup, !enabled);
  m_browser->setItemVisible(m_settingsGroup, !enabled);
  m_browser->setItemVisible(m_multiFitSettingsGroup, enabled);
  m_btnGroup->setVisible(enabled);
  // Show or hide additional widgets
  for (int i = 0; i < m_widgetSplitter->count(); ++i) {
    if (auto *widget = m_widgetSplitter->widget(i)) {
      widget->setVisible(enabled);
    }
  }
}
/**
* Set TF asymmetry mode on or off.
* If turned off, the fit property browser looks like Mantid 3.8.
* If turned on, the fit menu has an extra button and
* normalization is shown in the data table
* @param enabled :: [input] Whether to turn this mode on or off
*/
void MuonFitPropertyBrowser::setTFAsymmMode(bool enabled) {
  modifyFitMenu(m_fitActionTFAsymm, enabled);

  // Show or hide the TFAsymmetry fit
  if (enabled) {
    m_settingsGroup->property()->addSubProperty(m_normalization);
    m_settingsGroup->property()->addSubProperty(m_keepNorm);
    setNormalization();
  } else {
    m_settingsGroup->property()->removeSubProperty(m_normalization);
    m_settingsGroup->property()->removeSubProperty(m_keepNorm);
  }
}
/**
 * The pre-fit checks have been successfully completed. Continue by emitting a
 * signal to update the function and request the fit.
 * @param sequential :: [input] Whether fit is sequential or not
 */
void MuonFitPropertyBrowser::continueAfterChecks(bool sequential) {
  emit functionUpdateAndFitRequested(sequential);
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
/**
* Sets group names and updates checkboxes on UI
* By default sets all unchecked
* @param groups :: [input] List of group names
*/
void MuonFitPropertyBrowser::setAvailableGroups(const QStringList &groups) {

  m_enumManager->setValue(m_groupsToFit, 0);
  // If it's the same list, do nothing
  if (groups.size() == m_groupBoxes.size()) {
    auto existingGroups = m_groupBoxes.keys();
    auto newGroups = groups;
    qSort(existingGroups);
    qSort(newGroups);
    if (existingGroups == newGroups) {
      return;
    }
  }
  clearGroupCheckboxes();
  QSettings settings;
  for (const auto group : groups) {
    addGroupCheckbox(group);
  }
}
/**
* Selects a single group/pair
* @param group :: [input] Group/pair to select
*/
void MuonFitPropertyBrowser::setChosenGroup(const QString &group) {
  clearChosenGroups();
  for (auto iter = m_groupBoxes.constBegin(); iter != m_groupBoxes.constEnd();
       ++iter) {
    if (iter.key() == group) {
      m_boolManager->setValue(iter.value(), true);
    }
  }
}
/**
* Clears all group names and checkboxes
* (ready to add new ones)
*/
void MuonFitPropertyBrowser::clearGroupCheckboxes() {
  for (const auto &checkbox : m_groupBoxes) {
    delete (checkbox);
  }
  m_groupBoxes.clear();
}
/**
* Add a new checkbox to the list of groups with given name
* The new checkbox is checked according to dropdown menu selection
* @param name :: [input] Name of group to add
*/
void MuonFitPropertyBrowser::addGroupCheckbox(const QString &name) {
  m_groupBoxes.insert(name, m_boolManager->addProperty(name));
  int j = m_enumManager->value(m_groupsToFit);
  auto option = m_groupsToFitOptions[j].toStdString();
  if (option == "All groups") {
    setAllGroups();
  } else if (option == "All Pairs") {
    setAllPairs();
  }
}
/**
* Returns a list of the selected groups (checked boxes)
* @returns :: list of selected groups
*/
QStringList MuonFitPropertyBrowser::getChosenGroups() const {
  QStringList chosen;
  for (auto iter = m_groupBoxes.constBegin(); iter != m_groupBoxes.constEnd();
       ++iter) {
    if (m_boolManager->value(iter.value()) == true) {
      chosen.append(iter.key());
    }
  }
  return chosen;
}
/**
* Clears the list of selected groups (unchecks boxes)
*/
void MuonFitPropertyBrowser::clearChosenGroups() const {
  for (auto iter = m_groupBoxes.constBegin(); iter != m_groupBoxes.constEnd();
       ++iter) {
    m_boolManager->setValue(iter.value(), false);
  }
}

/**
* Selects all groups
*/
void MuonFitPropertyBrowser::setAllGroups() {

  clearChosenGroups();
  for (auto iter = m_groupBoxes.constBegin(); iter != m_groupBoxes.constEnd();
       ++iter) {
    for (auto group : m_groupsList) {
      if (iter.key().toStdString() == group) {
        m_boolManager->setValue(iter.value(), true);
      }
    }
  }
}
/*
* Sets all pairs
*/
void MuonFitPropertyBrowser::setAllPairs() {
  clearChosenGroups();
  for (auto iter = m_groupBoxes.constBegin(); iter != m_groupBoxes.constEnd();
       ++iter) {
    bool isItGroup = false;
    for (auto group : m_groupsList) {
      if (iter.key().toStdString() == group) {
        isItGroup = true;
      }
    }
    if (!isItGroup) {
      m_boolManager->setValue(iter.value(), true);
    }
  }
}

/*
* Create a popup window to select a custom
* selection of groups/pairs
*/
void MuonFitPropertyBrowser::genGroupWindow() {
  // reset group window
  m_groupWindow = new QDialog(this);
  QtGroupPropertyManager *groupManager =
      new QtGroupPropertyManager(m_groupWindow);
  QVBoxLayout *layout = new QVBoxLayout(m_groupWindow);
  QtTreePropertyBrowser *groupBrowser = new QtTreePropertyBrowser();
  QtProperty *groupSettings = groupManager->addProperty("Group/Pair selection");
  for (auto iter = m_groupBoxes.constBegin(); iter != m_groupBoxes.constEnd();
       ++iter) {
    groupSettings->addSubProperty(m_groupBoxes.value(iter.key()));
    m_boolManager->setValue(iter.value(), m_boolManager->value(iter.value()));
  }
  QtCheckBoxFactory *checkBoxFactory = new QtCheckBoxFactory(m_groupWindow);
  groupBrowser->setFactoryForManager(m_boolManager, checkBoxFactory);
  groupBrowser->addProperty(groupSettings);
  layout->addWidget(groupBrowser);
  m_groupWindow->setLayout(layout);
  m_groupWindow->show();
}
/**
* Selects all periods
*/
void MuonFitPropertyBrowser::setAllPeriods() {

  clearChosenPeriods();
  for (auto iter = m_periodBoxes.constBegin(); iter != m_periodBoxes.constEnd();
       ++iter) {
    m_boolManager->setValue(iter.value(), true);
  }
}

/**
* Sets checkboxes for periods
* @param numPeriods :: [input] Number of periods
*/
void MuonFitPropertyBrowser::setNumPeriods(size_t numPeriods) {
  m_periodsToFitOptions.clear();
  if (numPeriods > 1) {
    m_periodsToFitOptions << ALL_PERIODS_LABEL;
  }
  // create more boxes
  for (size_t i = 0; i != numPeriods; i++) {
    QString name = QString::number(i + 1);
    addPeriodCheckbox(name);
  }
  if (m_periodsToFitOptions.size() == 1) {
    m_generateBtn->setDisabled(true);
    m_multiFitSettingsGroup->property()->removeSubProperty(m_periodsToFit);
    m_multiFitSettingsGroup->property()->removeSubProperty(m_showPeriods);
    m_enumManager->setValue(m_periodsToFit, 0);
    clearChosenPeriods();
    m_boolManager->setValue(m_periodBoxes.constBegin().value(), true);
  } else {
    // add custom back into list
    m_multiFitSettingsGroup->property()->insertSubProperty(m_periodsToFit,
                                                           m_showGroup);
    m_multiFitSettingsGroup->property()->addSubProperty(m_showPeriods);
    m_generateBtn->setDisabled(false);

    m_periodsToFitOptions << CUSTOM_LABEL;
    m_enumManager->setEnumNames(m_periodsToFit, m_periodsToFitOptions);
  }
}
/**
* Sets period names and updates checkboxes on UI
* By default sets all unchecked
* @param periods :: [input] List of period names
*/
void MuonFitPropertyBrowser::setAvailablePeriods(const QStringList &periods) {
  // If it's the same list, do nothing
  if (periods.size() == m_periodBoxes.size()) {
    auto existingGroups = m_periodBoxes.keys();
    auto newGroups = periods;
    qSort(existingGroups);
    qSort(newGroups);
    if (existingGroups == newGroups) {
      return;
    }
  }

  clearPeriodCheckboxes();

  for (const auto group : periods) {
    addPeriodCheckbox(group);
  }
}
/**
* Clears all pair names and checkboxes
* (ready to add new ones)
*/
void MuonFitPropertyBrowser::clearPeriodCheckboxes() {
  if (m_periodBoxes.size() > 1) {
    for (auto iter = m_periodBoxes.constBegin();
         iter != m_periodBoxes.constEnd(); ++iter) {
      if (iter != m_periodBoxes.constBegin()) {
        delete (iter);
      }
    }
  }
  m_periodsToFitOptions.clear();
  m_periodsToFitOptions << "1";
  m_enumManager->setEnumNames(m_periodsToFit, m_periodsToFitOptions);
}
/**
* Clears the list of selected groups (unchecks boxes)
*/
void MuonFitPropertyBrowser::clearChosenPeriods() const {
  for (auto iter = m_periodBoxes.constBegin(); iter != m_periodBoxes.constEnd();
       ++iter) {
    m_boolManager->setValue(iter.value(), false);
  }
}
/**
* Add a new checkbox to the list of periods with given name
* The new checkbox is unchecked by default
* @param name :: [input] Name of period to add
*/
void MuonFitPropertyBrowser::addPeriodCheckbox(const QString &name) {
  m_periodBoxes.insert(name, m_boolManager->addProperty(name));
  int j = m_enumManager->value(m_periodsToFit);
  // add new period to list will go after inital list
  m_periodsToFitOptions << name;

  auto active = getChosenPeriods();
  m_enumManager->setEnumNames(m_periodsToFit, m_periodsToFitOptions);
  setChosenPeriods(active);
  m_enumManager->setValue(m_periodsToFit, j);
  if (m_periodsToFitOptions[j] == ALL_PERIODS_LABEL) {
    setAllPeriods();
  }
}
/**
* Returns a list of the selected periods (checked boxes)
* @returns :: list of selected periods
*/
QStringList MuonFitPropertyBrowser::getChosenPeriods() const {
  QStringList chosen;
  // if single period
  if (m_periodsToFitOptions.size() == 1) {
    chosen << "";
  } else {
    for (auto iter = m_periodBoxes.constBegin();
         iter != m_periodBoxes.constEnd(); ++iter) {
      if (m_boolManager->value(iter.value()) == true) {
        chosen.append(iter.key());
      }
    }
  }
  return chosen;
}
/**
* Ticks the selected periods
* @param chosenPeriods :: list of selected periods
*/
void MuonFitPropertyBrowser::setChosenPeriods(
    const QStringList &chosenPeriods) {
  clearChosenPeriods();
  for (auto selected : chosenPeriods) {
    for (auto iter = m_periodBoxes.constBegin();
         iter != m_periodBoxes.constEnd(); ++iter) {
      auto tmp = iter.key();
      if (iter.key() == selected) {
        m_boolManager->setValue(iter.value(), true);
      }
    }
  }
}
/**
* Ticks the selected periods
* @param period :: selected periods
*/
void MuonFitPropertyBrowser::setChosenPeriods(const QString &period) {
  clearChosenPeriods();
  for (auto iter = m_periodBoxes.constBegin(); iter != m_periodBoxes.constEnd();
       ++iter) {
    auto tmp = iter.key();
    if (iter.key() == period) {
      m_boolManager->setValue(iter.value(), true);
    }
  }
}
/*
* Create a pop up window to select a custom
* selection of periods
*/
void MuonFitPropertyBrowser::genPeriodWindow() {
  // reset period window
  m_periodWindow = new QDialog(this);
  QtGroupPropertyManager *groupManager =
      new QtGroupPropertyManager(m_periodWindow);
  QVBoxLayout *layout = new QVBoxLayout(m_periodWindow);
  QtTreePropertyBrowser *groupBrowser = new QtTreePropertyBrowser();
  QtProperty *groupSettings = groupManager->addProperty("Period selection");
  for (auto iter = m_periodBoxes.constBegin(); iter != m_periodBoxes.constEnd();
       ++iter) {
    groupSettings->addSubProperty(m_periodBoxes.value(iter.key()));
    m_boolManager->setValue(iter.value(), m_boolManager->value(iter.value()));
  }
  QtCheckBoxFactory *checkBoxFactory = new QtCheckBoxFactory(m_periodWindow);
  groupBrowser->setFactoryForManager(m_boolManager, checkBoxFactory);
  groupBrowser->addProperty(groupSettings);
  layout->addWidget(groupBrowser);
  m_periodWindow->setLayout(layout);
  m_periodWindow->show();
}
/*
* Create a pop up window to create
* a combination of periods
*/
void MuonFitPropertyBrowser::genCombinePeriodWindow() {
  // reset combine window
  m_comboWindow = new QDialog(this);
  QVBoxLayout *layout = new QVBoxLayout(m_comboWindow);
  QFormLayout *formLayout = new QFormLayout;
  m_positiveCombo = new QLineEdit();
  m_negativeCombo = new QLineEdit();
  formLayout->addRow(new QLabel(tr("Combine:")), m_positiveCombo);
  formLayout->addRow(new QLabel(tr("   -    ")), m_negativeCombo);
  layout->addLayout(formLayout);

  QPushButton *applyBtn = new QPushButton("Apply");

  connect(applyBtn, SIGNAL(released()), this, SLOT(combineBtnPressed()));

  layout->addWidget(applyBtn);
  m_comboWindow->setLayout(layout);
  m_comboWindow->show();
}
/*
* Get the positive and negative parts of the
* combination of periods and produce a new
* tick box. Unticked by default.
*/
void MuonFitPropertyBrowser::combineBtnPressed() {
  QString value = m_positiveCombo->text();
  if (value.isEmpty()) {
    g_log.error("There are no positive periods (top box)");
    return;
  }
  if (!m_negativeCombo->text().isEmpty()) {
    value.append("-").append(m_negativeCombo->text());
  }
  m_positiveCombo->clear();
  m_negativeCombo->clear();
  addPeriodCheckbox(value);
}
/**
* sets the label for a single fit and
* selects the relevant group/pair
* @param name :: string of the ws
*/
void MuonFitPropertyBrowser::setSingleFitLabel(std::string name) {
  clearChosenGroups();
  clearChosenPeriods();
  std::vector<std::string> splitName;
  std::string tmpName = name;
  boost::erase_all(tmpName, " ");
  boost::split(splitName, tmpName, boost::is_any_of(";"));
  // set single group/pair
  QString group = QString::fromUtf8(splitName[2].c_str());
  setChosenGroup(group);
  // set period if available
  if (splitName.size() == 6) {
    QString period = QString::fromUtf8(splitName[4].c_str());
    setChosenPeriods(period);
  }
  setOutputName(name);
  // for single fit in multi fit mode
  if (m_browser->isItemVisible(m_multiFitSettingsGroup)) {
    updateGroupDisplay();
    updatePeriodDisplay();
  }
}
/**
* Sets the multifit mode to all groups
* or all pairs depending on if a  group
* or pair is selected in the home tab
* @param isItGroup :: [input] if it is a group (true)
*/
void MuonFitPropertyBrowser::setAllGroupsOrPairs(const bool isItGroup) {
  if (isItGroup) {
    // all groups is index 0
    m_enumManager->setValue(m_groupsToFit, 0);
    setAllGroups();
  } else {
    // all pairs is index 1
    m_enumManager->setValue(m_groupsToFit, 1);
    setAllPairs();
  }
}
void MuonFitPropertyBrowser::setGroupNames(
    std::vector<std::string> groupNames) {
  m_groupsList = groupNames;
}

} // MantidQt
} // API
