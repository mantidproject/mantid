//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MuonAnalysisOptionTab.h"
#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MuonAnalysisHelper.h"

#include <QLineEdit>
#include <QSettings>
//-----------------------------------------------------------------------------
using namespace Mantid::Kernel;

namespace MantidQt {
using namespace API;
using namespace MantidWidgets;

namespace CustomInterfaces {
using namespace MuonAnalysisHelper;

namespace Muon {

const QString MuonAnalysisOptionTab::START_TIME_DEFAULT("0.3");
const QString MuonAnalysisOptionTab::FINISH_TIME_DEFAULT("16.0");
const QString MuonAnalysisOptionTab::MIN_Y_DEFAULT("");
const QString MuonAnalysisOptionTab::MAX_Y_DEFAULT("");
const QString MuonAnalysisOptionTab::FIXED_REBIN_DEFAULT("2");
const QString MuonAnalysisOptionTab::VARIABLE_REBIN_DEFAULT("0.032");

namespace {
/// static logger instance
Logger g_log("MuonAnalysis");
} // namespace

MuonAnalysisOptionTab::MuonAnalysisOptionTab(Ui::MuonAnalysis &uiForm,
                                             const QString &settingsGroup)
    : m_uiForm(uiForm), m_autoSaver(settingsGroup) {}

/**
 * Initialise the layout of the tab
 */
void MuonAnalysisOptionTab::initLayout() {
  // Register all the widgets for auto-saving
  m_autoSaver.beginGroup("PlotStyleOptions");
  m_autoSaver.registerWidget(m_uiForm.connectPlotType, "connectPlotStyle", 0);
  m_autoSaver.registerWidget(m_uiForm.timeAxisStartAtInput, "timeAxisStart",
                             START_TIME_DEFAULT);
  m_autoSaver.registerWidget(m_uiForm.timeAxisFinishAtInput, "timeAxisFinish",
                             FINISH_TIME_DEFAULT);
  m_autoSaver.registerWidget(m_uiForm.timeComboBox, "timeComboBoxIndex", 0);
  m_autoSaver.registerWidget(m_uiForm.yAxisMinimumInput, "yAxisStart",
                             MIN_Y_DEFAULT);
  m_autoSaver.registerWidget(m_uiForm.yAxisMaximumInput, "yAxisFinish",
                             MAX_Y_DEFAULT);
  m_autoSaver.registerWidget(m_uiForm.yAxisAutoscale, "axisAutoScaleOnOff",
                             true);
  m_autoSaver.registerWidget(m_uiForm.showErrorBars, "errorBars", 0);
  m_autoSaver.endGroup();

  m_autoSaver.beginGroup("BinningOptions");
  m_autoSaver.registerWidget(m_uiForm.optionStepSizeText, "rebinFixed",
                             FIXED_REBIN_DEFAULT);
  m_autoSaver.registerWidget(m_uiForm.binBoundaries, "rebinVariable",
                             VARIABLE_REBIN_DEFAULT);
  m_autoSaver.registerWidget(m_uiForm.rebinComboBox, "rebinComboBoxIndex", 0);
  m_autoSaver.endGroup();

  m_autoSaver.beginGroup("GeneralOptions");
  m_autoSaver.registerWidget(m_uiForm.plotCreation, "plotCreation", 0);
  m_autoSaver.registerWidget(m_uiForm.newPlotPolicy, "newPlotPolicy", 1);
  m_autoSaver.registerWidget(m_uiForm.hideToolbars, "toolbars", true);
  m_autoSaver.registerWidget(m_uiForm.hideGraphs, "hiddenGraphs", true);
  m_autoSaver.registerWidget(m_uiForm.spinBoxNPlotsToKeep, "fitsToKeep", 0);
  m_autoSaver.registerWidget(m_uiForm.chkEnableMultiFit, "enableMultiFit",
                             false);
  m_autoSaver.endGroup();

  // Set validators for double fields
  setDoubleValidator(m_uiForm.timeAxisStartAtInput);
  setDoubleValidator(m_uiForm.timeAxisFinishAtInput, true);
  setDoubleValidator(m_uiForm.yAxisMinimumInput, true);
  setDoubleValidator(m_uiForm.yAxisMaximumInput, true);
  setDoubleValidator(m_uiForm.optionStepSizeText);

  // Load saved values
  m_autoSaver.loadWidgetValues();

  // Run slots manually, because default values might not have been changed
  onTimeAxisChanged(m_uiForm.timeComboBox->currentIndex());
  onAutoscaleToggled(m_uiForm.yAxisAutoscale->isChecked());
  m_uiForm.rebinEntryState->setCurrentIndex(
      m_uiForm.rebinComboBox->currentIndex());

  // Enable auto-saving
  m_autoSaver.setAutoSaveEnabled(true);

  // Connect various sync stuff
  connect(m_uiForm.timeComboBox, SIGNAL(currentIndexChanged(int)), this,
          SLOT(onTimeAxisChanged(int)));
  connect(m_uiForm.yAxisAutoscale, SIGNAL(toggled(bool)), this,
          SLOT(onAutoscaleToggled(bool)));
  connect(m_uiForm.rebinComboBox, SIGNAL(currentIndexChanged(int)),
          m_uiForm.rebinEntryState, SLOT(setCurrentIndex(int)));

  // Connect help clicked
  connect(m_uiForm.muonAnalysisHelpPlotting, SIGNAL(clicked()), this,
          SLOT(muonAnalysisHelpSettingsClicked()));
  connect(m_uiForm.binBoundariesHelp, SIGNAL(clicked()), this,
          SLOT(rebinHelpClicked()));

  // Connect auto-updates for plot style
  connect(m_uiForm.connectPlotType, SIGNAL(currentIndexChanged(int)), this,
          SIGNAL(plotStyleChanged()));
  connect(m_uiForm.showErrorBars, SIGNAL(clicked()), this,
          SIGNAL(plotStyleChanged()));
  connect(m_uiForm.yAxisAutoscale, SIGNAL(clicked()), this,
          SIGNAL(plotStyleChanged()));
  connect(m_uiForm.yAxisMinimumInput, SIGNAL(returnPressed()), this,
          SIGNAL(plotStyleChanged()));
  connect(m_uiForm.yAxisMaximumInput, SIGNAL(returnPressed()), this,
          SIGNAL(plotStyleChanged()));

  // Connect auto updates of plot data
  connect(m_uiForm.timeComboBox, SIGNAL(currentIndexChanged(int)), this,
          SIGNAL(settingsTabUpdatePlot()));
  connect(m_uiForm.timeAxisStartAtInput, SIGNAL(returnPressed()), this,
          SIGNAL(settingsTabUpdatePlot()));
  connect(m_uiForm.timeAxisFinishAtInput, SIGNAL(returnPressed()), this,
          SIGNAL(settingsTabUpdatePlot()));
  connect(m_uiForm.rebinComboBox, SIGNAL(currentIndexChanged(int)), this,
          SIGNAL(settingsTabUpdatePlot()));
  connect(m_uiForm.optionStepSizeText, SIGNAL(returnPressed()), this,
          SIGNAL(settingsTabUpdatePlot()));
  connect(m_uiForm.binBoundaries, SIGNAL(returnPressed()), this,
          SIGNAL(settingsTabUpdatePlot()));
  connect(m_uiForm.chkEnableMultiFit, SIGNAL(stateChanged(int)), this,
          SIGNAL(multiFitStateChanged(int)));
  connect(m_uiForm.loadAllGroupsCheckBox, SIGNAL(stateChanged(int)), this,
          SIGNAL(loadAllGroupChanged(int)));
  connect(m_uiForm.loadAllPairsCheckBox, SIGNAL(stateChanged(int)), this,
          SIGNAL(loadAllPairsChanged(int)));
}

/**
 * Muon Analysis Settings help.
 */
void MuonAnalysisOptionTab::muonAnalysisHelpSettingsClicked() {
  MantidQt::API::HelpWindow::showCustomInterface(
      nullptr, QString("Muon Analysis"), QString("settings"));
}

/*
 * Muon Analysis Rebin help (located in settings section).
 */
void MuonAnalysisOptionTab::rebinHelpClicked() {
  MantidQt::API::HelpWindow::showCustomInterface(
      nullptr, QString("Muon Analysis"), QString("data-binning"));
}

/**
 * Run when autoscale check-box state is changed
 * @param state :: New state of the check-box
 */
void MuonAnalysisOptionTab::onAutoscaleToggled(bool state) {
  // Max and min input widgets
  auto maxInput = m_uiForm.yAxisMaximumInput;
  auto minInput = m_uiForm.yAxisMinimumInput;

  // Disable if autoscale
  maxInput->setEnabled(!state);
  minInput->setEnabled(!state);

  // Disable auto-save if autoscale
  m_autoSaver.setAutoSaveEnabled(maxInput, !state);
  m_autoSaver.setAutoSaveEnabled(minInput, !state);

  if (state) {
    maxInput->setText("N/A");
    minInput->setText("N/A");
  } else {
    m_autoSaver.loadWidgetValue(maxInput);
    m_autoSaver.loadWidgetValue(minInput);
  }
}

/**
 * Run when time axis combo-box is changed
 * @param index :: New index selected in the combo box
 */
void MuonAnalysisOptionTab::onTimeAxisChanged(int index) {
  // Start input widget
  auto startInput = m_uiForm.timeAxisStartAtInput;

  // Start input enabled only if Custom value selected
  startInput->setEnabled(index == 2);

  // Auto-save enabled only for Custom value
  m_autoSaver.setAutoSaveEnabled(startInput, index == 2);

  // Get new value of the Start input
  switch (index) {
  case (0): // Start at First Good Data
    startInput->setText(m_uiForm.firstGoodBinFront->text());
    break;
  case (1): // Start at Time Zero
    startInput->setText("0.0");
    break;
  case (2): // Custom Value
    m_autoSaver.loadWidgetValue(startInput);
    break;
  }

  if (index == 0) {
    // Synchronize First Good Data box on Home tab with the one on this tab, if
    // Start at First Good
    // Data is enabled.
    connect(m_uiForm.firstGoodBinFront, SIGNAL(textChanged(const QString &)),
            startInput, SLOT(setText(const QString &)));
  } else {
    // Disable synchronization otherwise
    disconnect(m_uiForm.firstGoodBinFront, SIGNAL(textChanged(const QString &)),
               startInput, SLOT(setText(const QString &)));
  }
}

/**
 * Get plot style parameters from widgets. Parameters are as follows:
 *   - ConnectType: 0 for Line, 1 for Scatter, 3 for Line + Symbol
 *   - ShowErrors: True of False
 *   - YAxisAuto: True or False
 *   - YAxisMin/YAxisMax: Double values
 */
QMap<QString, QString> MuonAnalysisOptionTab::parsePlotStyleParams() const {
  QMap<QString, QString> params;

  params["ConnectType"] =
      QString::number(m_uiForm.connectPlotType->currentIndex());

  params["ShowErrors"] = m_uiForm.showErrorBars->isChecked() ? "True" : "False";

  bool isAutoScaleEnabled = m_uiForm.yAxisAutoscale->isChecked();

  params["YAxisAuto"] = isAutoScaleEnabled ? "True" : "False";

  params["YAxisMin"] = params["YAxisMax"] = "";

  if (!isAutoScaleEnabled) {
    // If auto-scale not enabled, retrieve start/end values

    QLineEdit *minY = m_uiForm.yAxisMinimumInput;
    QLineEdit *maxY = m_uiForm.yAxisMaximumInput;

    double minYVal(Mantid::EMPTY_DBL());
    double maxYVal(Mantid::EMPTY_DBL());

    if (!minY->text().isEmpty()) {
      minYVal =
          getValidatedDouble(minY, MIN_Y_DEFAULT, "Y axis minimum", g_log);
    }

    if (!maxY->text().isEmpty()) {
      maxYVal =
          getValidatedDouble(maxY, MAX_Y_DEFAULT, "Y axis maximum", g_log);
    }

    // If both specified, check if min is less than max
    if (minYVal != Mantid::EMPTY_DBL() && maxYVal != Mantid::EMPTY_DBL() &&
        minYVal >= maxYVal) {
      g_log.warning("Y min should be less than Y max. Reset to default.");
      minY->setText(MIN_Y_DEFAULT);
      maxY->setText(MAX_Y_DEFAULT);
    } else {
      if (minYVal != Mantid::EMPTY_DBL())
        params["YAxisMin"] = QString::number(minYVal);

      if (maxYVal != Mantid::EMPTY_DBL())
        params["YAxisMax"] = QString::number(maxYVal);
    }
  }

  return (params);
}

/**
 * Retrieve selected type of the start time
 * @return Type of the start time as selected by user
 */
MuonAnalysisOptionTab::StartTimeType MuonAnalysisOptionTab::getStartTimeType() {
  QString selectedType = m_uiForm.timeComboBox->currentText();

  if (selectedType == "Start at First Good Data") {
    return FirstGoodData;
  } else if (selectedType == "Start at Time Zero") {
    return TimeZero;
  } else if (selectedType == "Custom Value") {
    return Custom;
  } else {
    // Just in case misspelled type or added a new one
    throw std::runtime_error("Unknown start time type selection");
  }
}

/**
 * Retrieve custom start time value. This only makes sense when
 * getStartTimeType() is Custom.
 * @return Value in the custom start time field
 */
double MuonAnalysisOptionTab::getCustomStartTime() {
  QLineEdit *w = m_uiForm.timeAxisStartAtInput;

  return getValidatedDouble(w, START_TIME_DEFAULT, "custom start time", g_log);
}

/**
 * Retrieve custom finish time value. If the value is not specified - returns
 * EMPTY_DBL().
 * @return Value in the custom finish field or EMPTY_DBL()
 */
double MuonAnalysisOptionTab::getCustomFinishTime() {
  QLineEdit *w = m_uiForm.timeAxisFinishAtInput;

  if (w->text().isEmpty()) {
    return Mantid::EMPTY_DBL();
  } else {
    return getValidatedDouble(w, FINISH_TIME_DEFAULT, "custom finish time",
                              g_log);
  }
}

/**
 * Returns rebin type as selected by user
 * @return Rebin type
 */
MuonAnalysisOptionTab::RebinType MuonAnalysisOptionTab::getRebinType() {
  QString selectedType = m_uiForm.rebinComboBox->currentText();

  if (selectedType == "None") {
    return NoRebin;
  } else if (selectedType == "Fixed") {
    return FixedRebin;
  } else if (selectedType == "Variable") {
    return VariableRebin;
  } else {
    throw std::runtime_error("Unknow rebin type selection");
  }
}

/**
 * Returns variable rebing params as set by user. Makes sense only if
 * getRebinType() is VariableRebin
 * @return Rebin params string
 */
std::string MuonAnalysisOptionTab::getRebinParams() {
  QLineEdit *w = m_uiForm.binBoundaries;

  if (w->text().isEmpty()) {
    g_log.warning("Binning parameters are empty. Reset to default value.");
    w->setText(VARIABLE_REBIN_DEFAULT);
    return VARIABLE_REBIN_DEFAULT.toStdString();
  } else {
    return w->text().toStdString();
  }
}

/**
 * Returns rebin step size as set by user. Make sense only if getRebinType() is
 * FixedRebin
 * @return Rebin step size
 */
double MuonAnalysisOptionTab::getRebinStep() {
  return getValidatedDouble(m_uiForm.optionStepSizeText, FIXED_REBIN_DEFAULT,
                            "binning step", g_log);
}

/**
 * @return Currently selected new plot policy
 */
MuonAnalysisOptionTab::NewPlotPolicy MuonAnalysisOptionTab::newPlotPolicy() {
  QMap<QString, NewPlotPolicy> policyMap;
  policyMap["Create new window"] = NewWindow;
  policyMap["Use previous window"] = PreviousWindow;

  QString selectedPolicy = m_uiForm.newPlotPolicy->currentText();
  if (!policyMap.contains(selectedPolicy)) {
    throw std::runtime_error("Unknown new plot policy selection");
  } else {
    return policyMap[selectedPolicy];
  }
}

/**
 * Returns whether or not "enable multiple fitting" is set.
 * @returns whether the checkbox is ticked
 */
Muon::MultiFitState MuonAnalysisOptionTab::getMultiFitState() const {
  if (m_uiForm.chkEnableMultiFit->isChecked()) {
    return Muon::MultiFitState::Enabled;
  } else {
    return Muon::MultiFitState::Disabled;
  }
}
} // namespace Muon
} // namespace CustomInterfaces
} // namespace MantidQt
