// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/MuonFitPropertyBrowser.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidQtWidgets/Common/PropertyHandler.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/StringEditorFactory.h"

#include "MantidAPI/MultiDomainFunction.h"
#include "MantidQtWidgets/Common/MuonFitDataSelector.h"

// Suppress a warning coming out of code that isn't ours
#if defined(__INTEL_COMPILER)
#pragma warning disable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#include "MantidQtWidgets/Common/QtPropertyBrowser/DoubleEditorFactory.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qteditorfactory.h"
#if defined(__INTEL_COMPILER)
#pragma warning enable 1125
#elif defined(__GNUC__)
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
#endif

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidAPI/IPeakFunction.h"

#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertymanager.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qttreepropertybrowser.h"

#include <Poco/ActiveResult.h>

#include <QAction>
#include <QFormLayout>
#include <QMessageBox>
#include <QSettings>

#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QSplitter>

#include <QCheckBox>
#include <QMenu>
#include <QMessageBox>
#include <QSignalMapper>
#include <QTableWidgetItem>

namespace {
Mantid::Kernel::Logger g_log("MuonFitPropertyBrowser");
const QString CUSTOM_LABEL{"Custom"};
const QString ALL_GROUPS_LABEL{"All Groups"};
const QString ALL_PAIRS_LABEL{"All Pairs"};
const QString ALL_PERIODS_LABEL{"All Periods"};
const std::string UNNORM = "_unNorm";
} // namespace

namespace MantidQt {
namespace MantidWidgets {

using namespace Mantid::API;

const std::string MuonFitPropertyBrowser::SIMULTANEOUS_PREFIX = {
    "MuonSimulFit_"};

/**
 * Constructor
 * @param parent :: The parent widget - must be an ApplicationWindow
 * @param mantidui :: The UI form for MantidPlot
 */
MuonFitPropertyBrowser::MuonFitPropertyBrowser(QWidget *parent,
                                               QObject *mantidui)
    : FitPropertyBrowser(parent, mantidui), m_widgetSplitter(nullptr),
      m_mainSplitter(nullptr), m_isMultiFittingMode(false) {}

/**
 * Initialise the muon fit property browser.
 */
void MuonFitPropertyBrowser::init() {
  QWidget *w = new QWidget(this);

  QSettings settings;
  settings.beginGroup("Mantid/FitBrowser");

  /* Create function group */
  QtProperty *functionsGroup = m_groupManager->addProperty("Functions");
  QtProperty *settingsGroup(nullptr);

  // Seperates the data and the settings into two seperate categories
  settingsGroup = m_groupManager->addProperty("Data");

  QSettings multiFitSettings;
  multiFitSettings.beginGroup("");

  /* Create function group */
  QtProperty *multiFitSettingsGroup(nullptr);

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
  m_enumManager->setValue(m_groupsToFit, 2);
  clearChosenGroups();
  QString tmp = "fwd";
  addGroupCheckbox(tmp);
  tmp = "bwd";
  addGroupCheckbox(tmp);
  m_periodsToFit = m_enumManager->addProperty("Periods to fit");
  m_periodsToFitOptions << ALL_PERIODS_LABEL << CUSTOM_LABEL << "1"
                        << "2";
  m_showPeriodValue << "1";
  m_showPeriods = m_enumManager->addProperty("Selected Periods");
  m_enumManager->setEnumNames(m_periodsToFit, m_periodsToFitOptions);
  multiFitSettingsGroup->addSubProperty(m_periodsToFit);
  multiFitSettingsGroup->addSubProperty(m_showPeriods);
  m_enumManager->setEnumNames(m_showPeriods, m_showPeriodValue);

  multiFitSettingsGroup->addSubProperty(m_normalization);

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

  m_TFAsymmMode = m_boolManager->addProperty("TF Asymmetry Mode");
  bool TFAsymmMode =
      settings.value("TF Asymmetry Mode", QVariant(false)).toBool();
  m_boolManager->setValue(m_TFAsymmMode, TFAsymmMode);

  customSettingsGroup->addSubProperty(m_minimizer);
  customSettingsGroup->addSubProperty(m_TFAsymmMode);
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
  // disable TFAsymm mode by default
  setTFAsymmMode(TFAsymmMode);
  m_autoBackground = getAutoBackgroundString();
}
// Set up the execution of the muon fit menu
void MuonFitPropertyBrowser::executeFitMenu(const QString &item) {
  if (item == "Fit" && m_boolManager->value(m_TFAsymmMode)) {
    doTFAsymmFit();
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

/// Enable/disable the Fit button;
void MuonFitPropertyBrowser::setFitEnabled(bool yes) {
  m_fitActionFit->setEnabled(yes);
  m_fitActionSeqFit->setEnabled(yes);
}

void MuonFitPropertyBrowser::checkFitEnabled() {
  if (count() == 0) {
    setFitEnabled(false);
  } else {
    setFitEnabled(true);
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
  if (m_workspaceNames.empty()) {
    if (this->isVisible()) {
      g_log.error("No Data available. Please load Some data.");
    }
    return;
  }
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
    int j = m_enumManager->value(m_workspace);
    std::string option = m_workspaceNames[j].toStdString();
    // update plot
    emit workspaceNameChanged(QString::fromStdString(option));

    setOutputName(option);
    // only do this if in single fit mode
    if (m_periodBoxes.size() > 1 &&
        !m_browser->isItemVisible(m_multiFitSettingsGroup)) {
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
    if (!m_browser->isItemVisible(m_multiFitSettingsGroup)) {
      size_t end = 0;
      // assumed structure of name
      // isolate the group/pair
      for (int k = 0; k < 2; k++) {
        end = option.find_first_of(";");
        option = option.substr(end + 1, option.size());
      }
      end = option.find_first_of(";");

      boost::erase_all(option, " ");

      auto tmp = option.substr(0, end - 1);
      QString selectedGroup = QString::fromStdString(tmp);
      // turn on only the relevant box
      for (auto iter = m_groupBoxes.constBegin();
           iter != m_groupBoxes.constEnd(); ++iter) {
        m_boolManager->setValue(iter.value(), selectedGroup == iter.key());
      }
    }
    // update plot for TF Asymm mode
    updateTFPlot();

  } else {
    FitPropertyBrowser::enumChanged(prop);
  }
}
/** Sets the display for
 * selected groups
 */
void MuonFitPropertyBrowser::updateGroupDisplay() {
  m_showGroupValue.clear();
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

void MuonFitPropertyBrowser::setNormalization() {
  setNormalization(workspaceName());
}
/**
 * @param name :: the ws name to get normalization for
 * @returns the normalization
 */
void MuonFitPropertyBrowser::setNormalization(const std::string name) {
  m_normalizationValue.clear();
  QString label;
  auto norms = readMultipleNormalization();
  std::string tmp = name;
  if (rawData()) {
    tmp = tmp + "_Raw";
  }
  // stored with ; instead of spaces
  std::replace(tmp.begin(), tmp.end(), ' ', ';');
  auto it = norms.find(tmp);
  if (it == norms.end()) {
    label = QString::fromStdString("N/A");
  } else {
    label = QString::number(it->second);
  }
  m_normalizationValue.append(label);
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
  if (prop == m_TFAsymmMode) {
    const bool val = m_boolManager->value(prop);
    setTFAsymmMode(val);
  }
  if (prop == m_keepNorm) {
    const bool val = m_boolManager->value(prop);
    if (val) { // record data for later
      double norm = 0.0;
      int j = m_enumManager->value(m_workspace);
      std::string name = m_workspaceNames[j].toStdString();

      auto norms = readMultipleNormalization();
      std::string tmp = name;
      if (rawData()) {
        tmp = tmp + "_Raw";
      }
      // stored with ; instead of spaces
      std::replace(tmp.begin(), tmp.end(), ' ', ';');
      auto it = norms.find(tmp);
      if (it != norms.end()) {
        norm = it->second;
      }
      ITableWorkspace_sptr table = WorkspaceFactory::Instance().createTable();
      AnalysisDataService::Instance().addOrReplace("__keepNorm__", table);
      table->addColumn("double", "norm");
      table->addColumn("int", "spectra");
      TableRow row = table->appendRow();
      row << norm << 0;

    } else { // remove data so it is not used later
      AnalysisDataService::Instance().remove("__keepNorm__");
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
  for (auto fnName : names) {
    QString qfnName = QString::fromStdString(fnName);
    if (qfnName == "MultiBG")
      continue;

    auto f = FunctionFactory::Instance().createFunction(fnName);
    const std::vector<std::string> categories = f->categories();
    bool muon = false;
    for (const auto &category : categories) {
      if ((category == "Muon") || (category == "General") ||
          (category == "Background"))
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
std::string MuonFitPropertyBrowser::getUnnormName(std::string wsName) {
  if (wsName.find(UNNORM) == std::string::npos) {
    auto raw = wsName.find("_Raw");

    if (raw == std::string::npos) {
      wsName += TFExtension();
    } else {
      wsName.insert(raw, UNNORM);
    }
  }
  if (rawData() && wsName.find("_Raw") == std::string::npos) {
    wsName += "_Raw";
  }
  return wsName;
}

/**
 * Creates an instance of Fit algorithm, sets its properties and launches it.
 */
void MuonFitPropertyBrowser::doTFAsymmFit() {
  std::string wsName = workspaceName();
  wsName = getUnnormName(wsName);
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

    IAlgorithm_sptr alg =
        AlgorithmManager::Instance().create("CalculateMuonAsymmetry");
    alg->initialize();
    auto fa = m_compositeFunction->asString();
    if (m_compositeFunction->nFunctions() > 1) {

      alg->setProperty("InputFunction",
                       boost::dynamic_pointer_cast<IFunction>(
                           m_functionBrowser->getGlobalFunction()));
    } else {
      alg->setProperty("InputFunction",
                       boost::dynamic_pointer_cast<IFunction>(
                           m_compositeFunction->getFunction(0)));
    }

    auto unnorm = m_workspacesToFit;
    std::string tmp = UNNORM;
    bool raw = rawData();
    std::for_each(unnorm.begin(), unnorm.end(),
                  [tmp, raw](std::string &wsName) {
                    if (wsName.find(UNNORM) == std::string::npos) {
                      auto rawIndex = wsName.find("_Raw");

                      if (rawIndex == std::string::npos) {
                        wsName += UNNORM;
                      } else {
                        wsName.insert(rawIndex, UNNORM);
                      }
                    }
                    if (raw && wsName.find("_Raw") == std::string::npos) {
                      wsName += "_Raw";
                    }
                  });

    alg->setProperty("UnNormalizedWorkspaceList", unnorm);
    alg->setProperty("ReNormalizedWorkspaceList", m_workspacesToFit);
    alg->setProperty("NormalizationTable", "MuonAnalysisTFNormalizations");

    alg->setProperty("StartX", startX());
    alg->setProperty("EndX", endX());
    alg->setPropertyValue("Minimizer", minimizer());

    // If we are doing a simultaneous fit, set this up here
    const int nWorkspaces = static_cast<int>(m_workspacesToFit.size());
    std::string output = outputName();
    if (nWorkspaces == 1) {
      setSingleFitLabel(wsName);
      output = getUnnormName(output);
    }

    alg->setPropertyValue("OutputFitWorkspace", output);

    observeFinish(alg);
    alg->execute();

  } catch (const std::exception &e) {
    QString msg = "CalculateMuonAsymmetry algorithm failed.\n\n" +
                  QString(e.what()) + "\n";
    QMessageBox::critical(this, "Mantid - Error", msg);
  }
}
/** Reads the normalization constants and which WS
 * they belong to
 * @returns :: A map of normalization constants and WS names
 */
std::map<std::string, double> readMultipleNormalization() {
  std::map<std::string, double> norm;
  if (AnalysisDataService::Instance().doesExist(
          "MuonAnalysisTFNormalizations")) {
    Mantid::API::ITableWorkspace_sptr table =
        boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(
                "MuonAnalysisTFNormalizations"));
    auto colNorm = table->getColumn("norm");
    auto colName = table->getColumn("name");
    for (size_t j = 0; j < table->rowCount(); j++) {
      norm[colName->cell<std::string>(j)] = ((*colNorm)[j]); // read norm
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

  return dynamic_cast<MatrixWorkspace *>(ws.get()) != nullptr;
}

void MuonFitPropertyBrowser::setFitWorkspaces(const std::string input) {
  // Copy experiment info to output workspace
  if (AnalysisDataService::Instance().doesExist(outputName() + "_Workspace")) {
    // Input workspace should be a MatrixWorkspace according to isWorkspaceValid
    auto inWs =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(input);
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
}

void MuonFitPropertyBrowser::finishHandle(const IAlgorithm *alg) {
  if (alg->name() == "CalculateMuonAsymmetry") {
    finishHandleTF(alg);
  } else {
    finishHandleNormal(alg);
  }
}
void MuonFitPropertyBrowser::finishHandleTF(const IAlgorithm *alg) {

  setFitWorkspaces(
      static_cast<std::string>(alg->getProperty("UnNormalizedWorkspaceList")));

  auto status = QString::fromStdString(alg->getPropertyValue("OutputStatus"));
  emit fitResultsChanged(status);
  FitPropertyBrowser::fitResultsChanged(status);

  // If fit was simultaneous, insert extra information into params table
  // and group the output workspaces
  const int nWorkspaces = static_cast<int>(m_workspacesToFit.size());
  if (nWorkspaces > 1) {
    std::string baseName = outputName();
    finishAfterTFSimultaneousFit(alg, baseName);
  }

  getFitResults();
  std::vector<std::string> wsList =
      alg->getProperty("UnNormalizedWorkspaceList");
  emit fittingDone(QString::fromStdString(wsList[0]));
  double quality = alg->getProperty("ChiSquared");
  // std::string costFunction =

  emit changeWindowTitle(QString("Fit Function (") + "Chi-sq " + " = " +
                         QString::number(quality) + ", " + status + ")");
  if (nWorkspaces == 1) {
    emit algorithmFinished(QString::fromStdString(wsList[0] + "_workspace"));
  }
}
void MuonFitPropertyBrowser::finishHandleNormal(const IAlgorithm *alg) {
  // Copy experiment info to output workspace
  setFitWorkspaces(
      static_cast<std::string>(alg->getProperty("InputWorkspace")));
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
// need own version of this
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
  const std::string &baseName = groupName;

  // Create a group for label
  try {
    ads.addOrReplace(groupName, boost::make_shared<WorkspaceGroup>());
    ads.addToGroup(groupName, baseName + "_NormalisedCovarianceMatrix");
    ads.addToGroup(groupName, baseName + "_Parameters");
    ads.addToGroup(groupName, baseName + "_Workspaces");
  } catch (const Mantid::Kernel::Exception::NotFoundError &err) {
    g_log.warning(err.what());
  }
}

/**
 * After a TF simultaneous fit, insert extra information into parameters table
 * (i.e. what runs, groups, periods "f0", "f1" etc were)
 * and group the output workspaces
 * @param alg :: [input] Pointer to fit algorithm that just finished
 * @param baseName :: [input] The common name of the workspaces of interest
 */
void MuonFitPropertyBrowser::finishAfterTFSimultaneousFit(
    const Mantid::API::IAlgorithm *alg, const std::string baseName) const {
  AnalysisDataServiceImpl &ads = AnalysisDataService::Instance();
  try {
    std::vector<std::string> wsList =
        alg->getProperty("UnNormalizedWorkspaceList");
    std::string paramTableName = baseName + "_Parameters";
    const auto paramTable = ads.retrieveWS<ITableWorkspace>(paramTableName);
    if (paramTable) {
      for (size_t i = 0; i < wsList.size(); i++) {
        const std::string suffix = boost::lexical_cast<std::string>(i);

        const auto wsName = wsList[i];
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

  std::string groupName = baseName;
  // Create a group for label
  try {
    ads.addOrReplace(groupName, boost::make_shared<WorkspaceGroup>());
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
  m_isMultiFittingMode = enabled;
  // First, clear whatever model is currently set
  this->clear();
  // set default selection (all groups)
  if (enabled) {
    setAllGroups();
    setAllPeriods();
    setAutoBackgroundName("");
    this->clear(); // force update of composite function
  } else {         // clear current selection
    if (m_autoBackground != "") {
      setAutoBackgroundName(m_autoBackground);
      addAutoBackground();
    }
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
  if (enabled) {
    setFitEnabled(false);
  }
}

/**
 * Returns true is the browser is set to multi fitting mode
 * This works using the visibility state of the button group
 * which is controlled in setMultiFittingMode
 */
bool MuonFitPropertyBrowser::isMultiFittingMode() const {
  return m_isMultiFittingMode;
}
void MuonFitPropertyBrowser::ConvertFitFunctionForMuonTFAsymmetry(
    bool enabled) {
  // set new fit func
  IAlgorithm_sptr alg = AlgorithmManager::Instance().create(
      "ConvertFitFunctionForMuonTFAsymmetry");
  // do not preserve the ties
  if (AnalysisDataService::Instance().doesExist(
          "MuonAnalysisTFNormalizations") &&
      m_compositeFunction->nFunctions() > 0) {
    alg->initialize();
    auto das = FitPropertyBrowser::getFittingFunction();
    auto dasd = das->asString();
    IFunction_sptr old = boost::dynamic_pointer_cast<IFunction>(m_compositeFunction);

    QStringList globals;

    if (m_isMultiFittingMode) {
      // manually set the function values
      old = m_functionBrowser->getGlobalFunction();
      globals = m_functionBrowser->getGlobalParameters();
    } else if (!enabled && !m_isMultiFittingMode) {
      old = boost::dynamic_pointer_cast<CompositeFunction>(old);
	  }
    auto non = old->asString();
    alg->setProperty("InputFunction", old);
    alg->setProperty("NormalizationTable", "MuonAnalysisTFNormalizations");
    alg->setProperty("WorkspaceList", m_workspacesToFit);
    std::string mode = (enabled) ? "Construct" : "Extract";
    alg->setProperty("Mode", mode);
    alg->execute();
    if (!alg->isExecuted()) {
      return;
    }
    IFunction_sptr func = alg->getProperty("OutputFunction");

    // multiple fit
    if (m_isMultiFittingMode) {
      // update values in browser
      if (func->getNumberDomains() > 1) {
        auto tmp = boost::dynamic_pointer_cast<MultiDomainFunction>(func);
        old = tmp->getFunction(0);
      } else {
        old = func;
      }
      m_functionBrowser->setFunction(old);
      // preserve global parameters
      QStringList newGlobals;
      const std::string INSERT_FUNCTION{"f0.f1.f1."};
      if (enabled) {
        for (auto global : globals) {
          newGlobals << QString::fromStdString(INSERT_FUNCTION) + global;
        }
      } else {
        for (auto global : globals) {
          newGlobals << global.remove(0, 9);
        }
      }
      m_functionBrowser->updateMultiDatasetParameters(*func);

      m_functionBrowser->setGlobalParameters(newGlobals);
      // if multi data set we need to do the fixes manually
      // the current domain is automatic
      auto originalNames = func->getParameterNames();
      for (auto name : originalNames) {
        auto index = func->parameterIndex(name);
        if (func->isFixed(index) && func->getNumberDomains() > 1) {
          // get domain
          auto separatorIndex = name.find_first_of(".");
          std::string domainStr = name.substr(1, separatorIndex - 1);
          int domain = std::stoi(domainStr);
          // remove domain from name
          auto newName = name.substr(separatorIndex + 1);
          // set fix
          m_functionBrowser->setLocalParameterFixed(
              QString::fromStdString(newName), domain, true);
        }
      }
    } // single fit
    else {
	  m_functionBrowser->clear();
      m_functionBrowser->setFunction(func);
    }

    updateTFPlot();
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
  IFunction_sptr old =
      boost::dynamic_pointer_cast<IFunction>(m_compositeFunction);
  if (old->nParams() > 0) {
    ConvertFitFunctionForMuonTFAsymmetry(enabled);
    // Show or hide the TFAsymmetry fit
    if (enabled) {
      m_settingsGroup->property()->addSubProperty(m_keepNorm);
    } else {
      m_settingsGroup->property()->removeSubProperty(m_keepNorm);
    }
  } else if (enabled) {
    // will update when user clicks elsewhere
    m_boolManager->setValue(m_TFAsymmMode, false);
    QMessageBox::warning(this, "Muon Analysis",
                         "No fitting function provided. TF Asymmetry mode "
                         "requires a fitting function to be added before "
                         "enabling. Please add a fitting function and enable "
                         "TF Asymmetry Mode again.");
  }
}
std::string MuonFitPropertyBrowser::TFExtension() const {

  return (m_boolManager->value(m_TFAsymmMode)) ? UNNORM : "";
}
/**
 * Makes sure we have the TF plot in TFAsymm mode
 */
void MuonFitPropertyBrowser::updateTFPlot() {
  // update plot
  int j = m_enumManager->value(m_workspace);
  std::string option = m_workspaceNames[j].toStdString();
  if (m_boolManager->value(m_TFAsymmMode) &&
      option.find(UNNORM) == std::string::npos) {
    auto raw = option.find("_Raw");

    if (raw == std::string::npos) {
      option += TFExtension();
    } else {
      option.insert(raw, UNNORM);
    }
  }
  // update plot
  emit TFPlot(QString::fromStdString(option));
}

/**
 * Adds an extra widget in between the fit buttons and the browser
 * @param widget :: [input] Pointer to widget to add
 * @param functionBrowser :: [input] pointer to the function browser
 */
void MuonFitPropertyBrowser::addFitBrowserWidget(
    QWidget *widget,
    MantidQt::MantidWidgets::FunctionBrowser *functionBrowser) {
  widget->setSizePolicy(QSizePolicy::Policy::Expanding,
                        QSizePolicy::Policy::Expanding);
  if (m_widgetSplitter) {
    m_widgetSplitter->addWidget(widget);
  }
  m_functionBrowser = functionBrowser;
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
  // If it's the same list, do nothing
  auto selected = getChosenGroups();
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
  for (const auto &group : groups) {
    addGroupCheckbox(group);
  }
  // sets the same selection as before
  for (const auto &group : selected) {

    for (auto iter = m_groupBoxes.constBegin(); iter != m_groupBoxes.constEnd();
         ++iter) {
      if (iter.key().toStdString() == group.toStdString()) {
        m_boolManager->setValue(iter.value(), true);
      }
    }
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
    for (const auto &group : m_groupsList) {
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
    const auto keyString = iter.key().toStdString();
    const bool isItGroup = std::any_of(
        m_groupsList.cbegin(), m_groupsList.cend(),
        [&keyString](const auto &group) { return group == keyString; });
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
  // delete period checkboxes
  clearPeriodCheckboxes();
  if (!m_periodsToFitOptions.empty()) {
    m_periodsToFitOptions.clear();
  }

  if (numPeriods > 1) {
    m_periodsToFitOptions << ALL_PERIODS_LABEL;
    m_periodsToFitOptions << CUSTOM_LABEL;
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
    // for now always reset to all groups when data is changed
    // the commented out code can be used to keep the selection when changing
    // run - but has a bug
    m_multiFitSettingsGroup->property()->insertSubProperty(m_periodsToFit,
                                                           m_showGroup);
    m_multiFitSettingsGroup->property()->addSubProperty(m_showPeriods);
    m_generateBtn->setDisabled(false);

    updatePeriods(0);
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

  for (const auto &group : periods) {
    addPeriodCheckbox(group);
  }
}
/**
 * Clears all pair names and checkboxes
 * (ready to add new ones)
 */
void MuonFitPropertyBrowser::clearPeriodCheckboxes() {
  if (m_periodBoxes.size() > 1) {
    for (auto iter = std::next(m_periodBoxes.constBegin());
         iter != m_periodBoxes.constEnd(); ++iter) {
      delete (*iter);
    }
    m_periodBoxes.clear();
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
 * updates the period displays
 */
void MuonFitPropertyBrowser::updatePeriods() {
  int j = m_enumManager->value(m_periodsToFit);
  // auto selected = getChosenPeriods();
  updatePeriods(j);
}
/**
 * updates the period displays and conserves the selection
 * if selection is niot available default to all periods
 * @param j :: [input] index of selection in combobox
 * selected is an input for changing runs and preserving selection (list of
 * selected periods)
 * currently has a bug
 */
void MuonFitPropertyBrowser::updatePeriods(const int j) {
  // this is for switching but has a bug at the moment
  // const QStringList &selected) {
  if (m_periodsToFitOptions.size() == 0) {
    QMessageBox::warning(this, "Muon Analysis",
                         "Data not found. Please turn on the data archive, "
                         "using the Manage Directories button.");
    return;
  }
  m_enumManager->setEnumNames(m_periodsToFit, m_periodsToFitOptions);
  m_enumManager->setValue(m_periodsToFit, j);
  if (m_periodsToFitOptions[j] == CUSTOM_LABEL) {
    // currently the below does not work reliably (if period arithmatic is
    // presemnt it gives bad results
    /*
    setChosenPeriods(selected);*/
    // lets default to all periods for now
    // explictly set all periods
    setAllPeriods();
  } else if (m_periodsToFitOptions[j] == ALL_PERIODS_LABEL) {
    // explictly set all periods
    setAllPeriods();
  } else { // single number
    setChosenPeriods(m_periodsToFitOptions[j]);
  }
}
/**
 * Adds a new checkbox to the list of periods with given name
 * It updates the display
 * @param name :: [input] Name of period to add
 */
void MuonFitPropertyBrowser::addPeriodCheckboxToMap(const QString &name) {
  if (m_periodBoxes.find(name) != m_periodBoxes.end()) {
    // if the box already exists
    return;
  }
  // has to go here to get the original value
  int j = m_enumManager->value(m_periodsToFit);
  // auto selected = getChosenPeriods();
  addPeriodCheckbox(name);
  updatePeriods(j);
}
/**
 * Check if a period is valid
 * @param name :: [input] Name of period to add
 */
bool MuonFitPropertyBrowser::isPeriodValid(const QString &name) {
  // check period is sensible
  // no frational periods
  if (name.contains(".")) {
    return false;
  }
  // wshould only ever have 1 minus sign
  else if (name.count("-") > 1) {
    return false;
  } else {
    std::vector<std::string> numbers;
    std::string nameString = name.toStdString();
    boost::algorithm::split(numbers, nameString, boost::is_any_of(","));
    // loop over results
    for (auto value : numbers) {
      auto tmp = value.find("-");
      if (tmp != std::string::npos) {
        // find a minus sign
        auto before = value.substr(0, tmp);
        auto after = value.substr(tmp + 1);

      } else {
        try {
          boost::lexical_cast<int>(value);
          if (m_periodBoxes.find(QString::fromStdString(value)) ==
                  m_periodBoxes.end() &&
              numbers.size() > 1) {
            // if the box does not exist and there is more than 1 period in name
            return false;
          }
        } catch (const boost::bad_lexical_cast &) {
          // none int value
          return false;
        }
      }
    }
  }
  return true;
}
/**
 * Add a new checkbox to the list of periods with given name
 * The new checkbox is unchecked by default
 * @param name :: [input] Name of period to add
 */
void MuonFitPropertyBrowser::addPeriodCheckbox(const QString &name) {
  // check period is sensible
  // no frational periods
  if (isPeriodValid(name)) {
    m_periodBoxes.insert(name, m_boolManager->addProperty(name));
    int j = m_enumManager->value(m_periodsToFit);
    // add new period to list will go after inital list
    m_periodsToFitOptions << name;

    auto active = getChosenPeriods();
    m_enumManager->setEnumNames(m_periodsToFit, m_periodsToFitOptions);
    setChosenPeriods(active);
    m_enumManager->setValue(m_periodsToFit, j);
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
  for (const auto &selected : chosenPeriods) {
    for (auto iter = m_periodBoxes.constBegin();
         iter != m_periodBoxes.constEnd(); ++iter) {
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
  int j = m_enumManager->value(m_periodsToFit);
  if (m_periodsToFitOptions[j] == ALL_PERIODS_LABEL) {
    setAllPeriods();
  }
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

  auto index = m_enumManager->value(m_groupsToFit);
  QString name = m_groupsToFitOptions[index];
  if (name == CUSTOM_LABEL) {
    auto vals = getChosenGroups();
    clearChosenGroups();
    for (const auto &group : vals) {

      for (auto iter = m_groupBoxes.constBegin();
           iter != m_groupBoxes.constEnd(); ++iter) {
        if (iter.key().toStdString() == group.toStdString()) {
          m_boolManager->setValue(iter.value(), true);
        }
      }
    }
  } else if (name == ALL_GROUPS_LABEL) {
    m_enumManager->setValue(m_groupsToFit, 0);
    setAllGroups();
    if (getChosenGroups().size() > 0) {
      return;
    }
  } else if (name == ALL_PAIRS_LABEL) { // all pairs is index 1
    m_enumManager->setValue(m_groupsToFit, 1);
    setAllPairs();
  }
  if (getChosenGroups().size() > 0) {
    return;
  } else {

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
}
void MuonFitPropertyBrowser::setGroupNames(
    std::vector<std::string> groupNames) {
  m_groupsList = groupNames;
}
void MuonFitPropertyBrowser::setTFAsymm(bool state) {
  m_boolManager->setValue(m_TFAsymmMode, state);
}

} // namespace MantidWidgets
} // namespace MantidQt
