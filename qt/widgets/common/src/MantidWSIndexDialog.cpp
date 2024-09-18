// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/MantidWSIndexDialog.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectraDetectorTypes.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <QMessageBox>
#include <QPalette>
#include <QPushButton>
#include <QRegExp>
#include <QtAlgorithms>

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <cstdlib>
#include <exception>
#include <numeric>
#include <utility>

namespace MantidQt::MantidWidgets {
/// The string "Workspace index"
const QString MantidWSIndexWidget::WORKSPACE_NAME = "Workspace name";
const QString MantidWSIndexWidget::WORKSPACE_INDEX = "Workspace index";

/// The string "Custom"
const QString MantidWSIndexWidget::CUSTOM = "Custom";

// String for plot types
const QString MantidWSIndexWidget::SIMPLE_PLOT = "1D Plot";
const QString MantidWSIndexWidget::WATERFALL_PLOT = "Waterfall Plot";
const QString MantidWSIndexWidget::SURFACE_PLOT = "Surface Plot";
const QString MantidWSIndexWidget::CONTOUR_PLOT = "Contour Plot";

//----------------------------------
// MantidWSIndexWidget methods
//----------------------------------
/**
 * Construct a widget of this type
 * @param parent :: The owning dialog
 * @param flags :: Window flags that are passed to the QWidget constructor
 * @param wsNames :: the names of the workspaces to be plotted
 * @param showWaterfallOption :: true if waterfall plot enabled
 * @param showTiledOption :: true if tiled plot enabled
 * @param isAdvanced :: true if advanced plotting has been selected
 */
MantidWSIndexWidget::MantidWSIndexWidget(QWidget *parent, const Qt::WindowFlags &flags, const QList<QString> &wsNames,
                                         const bool showWaterfallOption, const bool showTiledOption,
                                         const bool isAdvanced)
    : QWidget(parent, flags), m_spectra(false), m_waterfall(showWaterfallOption), m_tiled(showTiledOption),
      m_advanced(isAdvanced), m_plotOptions(), m_wsNames(wsNames), m_wsIndexIntervals(), m_spectraNumIntervals(),
      m_wsIndexChoice(), m_spectraNumChoice() {
  checkForSpectraAxes();
  // Generate the intervals allowed to be plotted by the user.
  generateWsIndexIntervals();
  if (m_spectra) {
    generateSpectraNumIntervals();
  }
  init();
}

/**
 * Returns the user-selected options
 * @returns Struct containing user options
 */
MantidWSIndexWidget::UserInput MantidWSIndexWidget::getSelections() {
  UserInput options = UserInput();
  options.plots = getPlots();
  options.simple = is1DPlotSelected();
  options.waterfall = isWaterfallPlotSelected();
  options.tiled = isTiledPlotSelected();
  if (m_advanced) {
    options.surface = isSurfacePlotSelected();
    options.errors = isErrorBarsSelected();
    options.contour = isContourPlotSelected();
  } else {
    options.surface = false;
    options.errors = false;
    options.contour = false;
  }

  // Advanced options
  if (m_advanced && (options.simple || options.waterfall || options.surface || options.contour)) {
    UserInputAdvanced userInputAdvanced = UserInputAdvanced();
    if (options.surface || options.contour) {
      userInputAdvanced.accepted = true;
      userInputAdvanced.plotIndex = getPlotIndex();
      userInputAdvanced.axisName = getAxisName();
    }
    userInputAdvanced.logName = getLogName();
    if (userInputAdvanced.logName == WORKSPACE_NAME || userInputAdvanced.logName == WORKSPACE_INDEX) {
      // We want default names in legend, if log is workspace name or index
      userInputAdvanced.logName = "";
    }
    userInputAdvanced.workspaceNames = m_wsNames;
    if (userInputAdvanced.logName == CUSTOM) {
      userInputAdvanced.customLogValues = getCustomLogValues();
      if (userInputAdvanced.customLogValues.empty()) {
        userInputAdvanced.accepted = false;
      }
    }
    options.isAdvanced = true;
    options.advanced = std::move(userInputAdvanced);
  } else {
    options.isAdvanced = false; // We don't want the view to look at options.advanced.
  }
  return options;
}

/**
 * Returns the workspace index to be plotted
 * @returns Workspace index to be plotted
 */
int MantidWSIndexWidget::getPlotIndex() const {
  int spectrumIndex = 0; // default to 0
  const auto userInput = getPlots();

  if (!userInput.empty()) {
    const auto indexList = userInput.values();
    if (!indexList.empty()) {
      const auto &spectrumIndexes = indexList.at(0);
      if (!spectrumIndexes.empty()) {
        spectrumIndex = *spectrumIndexes.begin();
      }
    }
  }
  return spectrumIndex;
}

/**
 * Displays a message box with the supplied error string.
 * @param message :: [input] Error message to display
 */
void MantidWSIndexWidget::showPlotOptionsError(const QString &message) {
  if (!message.isEmpty()) {
    QMessageBox errorMessage;
    errorMessage.setText(message);
    errorMessage.setIcon(QMessageBox::Critical);
    errorMessage.exec();
  }
}

/**
 * If "Custom" is selected as log, returns the list of values the user has input
 * into the edit box, otherwise returns an empty set.
 * Note that the set is ordered by definition, and values are only added if they
 * are successfully converted to a double.
 * @returns Set of numerical log values
 */
const std::set<double> MantidWSIndexWidget::getCustomLogValues() const {
  std::set<double> logValues;
  if (m_logSelector->currentText() == CUSTOM) {
    QStringList values = m_logValues->lineEdit()->text().split(',');
    foreach (QString value, values) {
      bool ok = false;
      double number = value.toDouble(&ok);
      if (ok) {
        logValues.insert(number);
      }
    }
  }
  return logValues;
}

/**
 * Gets the name that the user gave for the Y axis of the surface plot
 * @returns Name input by user for axis
 */
const QString MantidWSIndexWidget::getAxisName() const { return m_axisNameEdit->lineEdit()->text(); }

/**
 * Gets the log that user selected to plot against
 * @returns Name of log, or "Workspace index"
 */
const QString MantidWSIndexWidget::getLogName() const { return m_logSelector->currentText(); }

/**
 * Returns the user-selected plots
 * @returns Plots selected by user
 */
QMultiMap<QString, std::set<int>> MantidWSIndexWidget::getPlots() const {
  // Map of workspace names to set of indices to be plotted.
  QMultiMap<QString, std::set<int>> plots;

  // If the user typed in the wsField ...
  if (m_wsIndexChoice.getList().size() > 0) {

    for (const auto &wsName : m_wsNames) {
      std::set<int> intSet = m_wsIndexChoice.getIntSet();
      plots.insert(wsName, intSet);
    }
  }
  // Else if the user typed in the spectraField ...
  else if (m_spectraNumChoice.getList().size() > 0) {
    for (const auto &wsName : m_wsNames) {
      // Convert the spectra choices of the user into workspace indices for us
      // to use.
      Mantid::API::MatrixWorkspace_const_sptr ws = std::dynamic_pointer_cast<const Mantid::API::MatrixWorkspace>(
          Mantid::API::AnalysisDataService::Instance().retrieve(wsName.toStdString()));
      if (nullptr == ws)
        continue;

      const Mantid::spec2index_map spec2index = ws->getSpectrumToWorkspaceIndexMap();

      std::set<int> origSet = m_spectraNumChoice.getIntSet();
      std::set<int>::iterator it = origSet.begin();
      std::set<int> convertedSet;

      for (; it != origSet.end(); ++it) {
        int origInt = (*it);
        int convertedInt = static_cast<int>(spec2index.find(origInt)->second);
        convertedSet.insert(convertedInt);
      }

      plots.insert(wsName, convertedSet);
    }
  }

  return plots;
}

/**
 * Whether the user selected "1D plot"
 * @returns True if 1D plot selected
 */
bool MantidWSIndexWidget::is1DPlotSelected() const { return (m_plotOptions->currentText() == SIMPLE_PLOT); }

/**
 * Whether the user selected "waterfall"
 * @returns True if waterfall plot selected
 */
bool MantidWSIndexWidget::isWaterfallPlotSelected() const { return (m_plotOptions->currentText() == WATERFALL_PLOT); }

/**
 * Whether the user selected "tiled"
 * @returns True if tiled plot selected
 */
bool MantidWSIndexWidget::isTiledPlotSelected() const { return (m_plotOptions->currentText() == "Tiled Plot"); }

/**
 * Whether the user selected surface plot
 * @returns True if surfarce plot selected
 */
bool MantidWSIndexWidget::isSurfacePlotSelected() const { return (m_plotOptions->currentText() == SURFACE_PLOT); }

/**
 * Whether the user selected contour plot
 * @returns True if surfarce plot selected
 */
bool MantidWSIndexWidget::isContourPlotSelected() const { return (m_plotOptions->currentText() == CONTOUR_PLOT); }

/**
 * Whether the user has selected plot with error bars
 * @returns True if error bars are selected
 */
bool MantidWSIndexWidget::isErrorBarsSelected() const { return m_showErrorBars->checkState(); }

/**
 * Called when user edits workspace field
 */
void MantidWSIndexWidget::editedWsField() {
  m_spectraField->lineEdit()->clear();
  m_spectraField->setError("");
}

/**
 * Called when user edits spectra field
 */
void MantidWSIndexWidget::editedSpectraField() {
  m_wsField->lineEdit()->clear();
  m_wsField->setError("");
}

/**
 * Called when dialog requests a plot
 * @returns True to accept, false to raise error
 */
bool MantidWSIndexWidget::plotRequested() {
  bool acceptable = false;
  int npos = 0;
  QString wsText = m_wsField->lineEdit()->text();
  QString spectraText = m_spectraField->lineEdit()->text();
  QValidator::State wsState = m_wsField->lineEdit()->validator()->validate(wsText, npos);
  QValidator::State spectraState = m_spectraField->lineEdit()->validator()->validate(spectraText, npos);
  if (wsState == QValidator::Acceptable) {
    m_wsIndexChoice.addIntervals(m_wsField->lineEdit()->text());
    m_usingWsIndexChoice = true;
    m_usingSprectraNumChoice = false;
    acceptable = true;
  }
  // Else if the user typed in the spectraField ...
  else if (spectraState == QValidator::Acceptable) {
    m_spectraNumChoice.addIntervals(m_spectraField->lineEdit()->text());
    m_usingSprectraNumChoice = true;
    m_usingWsIndexChoice = false;
    acceptable = true;
  } else {
    m_usingSprectraNumChoice = false;
    m_usingWsIndexChoice = false;
    QString error_message("Invalid input. It is not in the range available");
    if (!wsText.isEmpty())
      m_wsField->setError(error_message);
    if (!spectraText.isEmpty())
      m_spectraField->setError(error_message);
    if (wsText.isEmpty() && spectraText.isEmpty()) {
      m_wsField->setError("Workspace indices or spectra numbers are needed");
      m_spectraField->setError("Spectra numbers or workspace indices are needed");
    }
  }
  // To give maximum feedback to user, we validate plot options,
  // even if intervals are not acceptable
  return validatePlotOptions() && acceptable;
}

/**
 * Called when dialog requests to plot all
 */
bool MantidWSIndexWidget::plotAllRequested() {
  m_wsIndexChoice = m_wsIndexIntervals;
  m_usingWsIndexChoice = true;
  m_usingSprectraNumChoice = false;
  return validatePlotOptions();
}

/**
 * Validate plot options when a plot is requested
 * set appropriate error if not valid
 */
bool MantidWSIndexWidget::validatePlotOptions() {

  // Only bother is plotting is advanced
  if (!m_advanced)
    return true;

  bool validOptions = true;

  // We only validate the custom log values and
  // only if custom logs are selected, else it's OK.
  if (m_logSelector->currentText() == CUSTOM) {
    QStringList values = m_logValues->lineEdit()->text().split(',');
    bool firstValue = true;
    double previousValue = 0.0;
    for (const auto &value : values) {
      bool ok = false;
      double currentValue = value.toDouble(&ok);
      // Check for non-numeric value
      if (!ok) {
        m_logValues->setError("A custom log value is not valid: " + value);
        validOptions = false;
        break;
      }
      // Check for order
      if (firstValue) {
        firstValue = false;
        previousValue = currentValue;
      } else {
        if (previousValue < currentValue) {
          // cpp-check unreadVariable
          previousValue = currentValue;
        } else {
          m_logValues->setError("The custom log values must be in numerical order and distinct.");
          validOptions = false;
          break;
        }
      }
    }

    if (validOptions) {
      int numCustomLogValues = values.size();
      QString nCustomLogValues;
      nCustomLogValues.setNum(numCustomLogValues);
      int numWorkspaces = m_wsNames.size();
      if (m_plotOptions->currentText() == SURFACE_PLOT || m_plotOptions->currentText() == CONTOUR_PLOT) {
        QString nWorkspaces;
        nWorkspaces.setNum(numWorkspaces);

        if (numCustomLogValues != numWorkspaces) {
          m_logValues->setError("The number of custom log values (" + nCustomLogValues +
                                ") is not equal to the number of workspaces (" + nWorkspaces + ").");
          validOptions = false;
        }
      } else {
        int numSpectra = 0;
        if (m_usingWsIndexChoice)
          numSpectra = m_wsIndexChoice.totalIntervalLength();
        if (m_usingSprectraNumChoice)
          numSpectra = m_spectraNumChoice.totalIntervalLength();
        QString nPlots;
        nPlots.setNum(numWorkspaces * numSpectra);

        if (numCustomLogValues != numWorkspaces * numSpectra) {
          m_logValues->setError("The number of custom log values (" + nCustomLogValues +
                                ") is not equal to the number of plots (" + nPlots + ").");
          validOptions = false;
        }
      }
    }
  }

  if (!validOptions) {
    // Clear record of user choices, because they may change.
    m_wsIndexChoice.clear();
    m_spectraNumChoice.clear();
  }

  return validOptions;
}

/**
 * Set up widget UI
 */
void MantidWSIndexWidget::init() {
  m_outer = new QVBoxLayout;
  initSpectraBox();
  initWorkspaceBox();
  initOptionsBoxes();
  if (m_advanced) {
    initLogs();
  }
  setLayout(m_outer);
}

/**
 * Set up Workspace box UI
 */
void MantidWSIndexWidget::initWorkspaceBox() {
  m_wsBox = new QVBoxLayout;
  const QString wsIndices = m_wsIndexIntervals.toQString();
  const QString label = "Enter Workspace Indices: " + wsIndices;
  m_wsMessage = new QLabel(tr(qPrintable(label)));
  m_wsField = new QLineEditWithErrorMark();

  m_wsField->lineEdit()->setValidator(new IntervalListValidator(this, m_wsIndexIntervals));
  if (wsIndices == "0") { // single spectrum
    m_wsField->lineEdit()->setEnabled(false);
    m_wsField->lineEdit()->setText("0");
  }
  m_wsBox->addWidget(m_wsMessage);
  m_wsBox->addWidget(m_wsField);
  m_outer->addItem(m_wsBox);

  connect(m_wsField->lineEdit(), SIGNAL(textEdited(const QString &)), this, SLOT(editedWsField()));
}

/**
 * Set up Spectra box UI
 */
void MantidWSIndexWidget::initSpectraBox() {
  m_spectraBox = new QVBoxLayout;
  const QString spectraNumbers = m_spectraNumIntervals.toQString();
  const QString label = "Enter Spectra Numbers: " + spectraNumbers;
  m_spectraMessage = new QLabel(tr(qPrintable(label)));
  m_spectraField = new QLineEditWithErrorMark();
  m_orMessage = new QLabel(tr("<br>Or"));

  m_spectraField->lineEdit()->setValidator(new IntervalListValidator(this, m_spectraNumIntervals));
  if (spectraNumbers == "1") { // single spectrum
    m_spectraField->lineEdit()->setEnabled(false);
    m_spectraField->lineEdit()->setText("1");
  }
  m_spectraBox->addWidget(m_spectraMessage);
  m_spectraBox->addWidget(m_spectraField);
  m_spectraBox->addWidget(m_orMessage);

  if (usingSpectraNumbers())
    m_outer->addItem(m_spectraBox);

  connect(m_spectraField->lineEdit(), SIGNAL(textEdited(const QString &)), this, SLOT(editedSpectraField()));
}

/**
 * Set up Options boxes UI
 */
void MantidWSIndexWidget::initOptionsBoxes() {
  m_optionsBox = new QVBoxLayout;

  m_plotOptionLabel = new QLabel(tr("Plot Type:"));
  if (m_waterfall || m_tiled) {
    m_plotOptions = new QComboBox();
    m_plotOptions->addItem(SIMPLE_PLOT);
    if (m_waterfall) {
      m_plotOptions->addItem(WATERFALL_PLOT);
    }
    if (m_tiled) {
      m_plotOptions->addItem(tr("Tiled Plot"));
    }
    if (m_advanced && isSuitableForContourOrSurfacePlot()) {
      m_plotOptions->addItem(SURFACE_PLOT);
      m_plotOptions->addItem(CONTOUR_PLOT);
      connect(m_plotOptions, SIGNAL(currentIndexChanged(const QString &)), this,
              SLOT(onPlotOptionChanged(const QString &)));
    }
    m_optionsBox->addWidget(m_plotOptionLabel);
    m_optionsBox->addWidget(m_plotOptions);
  }

  if (m_advanced) {
    int spacingAboveShowErrorBars = 10;
    m_optionsBox->addSpacing(spacingAboveShowErrorBars);
    m_showErrorBars = new QCheckBox("Show Error Bars");
    m_optionsBox->addWidget(m_showErrorBars);
  }

  m_outer->addItem(m_optionsBox);
}

void MantidWSIndexWidget::initLogs() {
  m_logOptionsGroup = new QGroupBox(tr("Log Options"));
  m_logBox = new QVBoxLayout;

  m_logLabel = new QLabel(tr("Log value to plot against:"));
  m_logSelector = new QComboBox();
  populateLogComboBox();

  m_customLogLabel = new QLabel(tr("<br>Custom log values:"));
  m_logValues = new QLineEditWithErrorMark();

  m_axisLabel = new QLabel(tr("<br>Label for plot axis:"));
  m_axisNameEdit = new QLineEditWithErrorMark();
  m_axisNameEdit->lineEdit()->setText(m_logSelector->currentText());

  m_logBox->addWidget(m_logLabel);
  m_logBox->addWidget(m_logSelector);
  m_logBox->addWidget(m_customLogLabel);
  m_logBox->addWidget(m_logValues);
  m_logBox->addWidget(m_axisLabel);
  m_logBox->addWidget(m_axisNameEdit);

  m_logSelector->setEnabled(true);
  m_logValues->setEnabled(false);
  m_axisNameEdit->setEnabled(false);

  m_logOptionsGroup->setLayout(m_logBox);

  m_outer->addWidget(m_logOptionsGroup);

  connect(m_logSelector, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(onLogSelected(const QString &)));
}

/**
 * Called when log selection changed
 * If "Custom" selected, enable the custom log input box.
 * Otherwise, it is read-only.
 * Also put the log name into the axis name box as a default choice.
 * @param logName :: [input] Text selected in combo box
 */
void MantidWSIndexWidget::onLogSelected(const QString &logName) {
  m_logValues->setEnabled(logName == CUSTOM);
  m_logValues->lineEdit()->clear();
  m_axisNameEdit->lineEdit()->setText(logName);
}

/**
 * Called when plot option is changed
 * @param plotOption :: [input] New plot option
 */
void MantidWSIndexWidget::onPlotOptionChanged(const QString &plotOption) {
  auto useLogNames = m_advanced && isSuitableForLogValues(plotOption);
  auto isLogSelectorCustom = m_logSelector->currentText() == CUSTOM;
  auto isSurfaceOrContourPlot =
      m_plotOptions->currentText() == SURFACE_PLOT || m_plotOptions->currentText() == CONTOUR_PLOT;
  // Enable widgets as appropriate
  m_showErrorBars->setEnabled(!isSurfaceOrContourPlot);
  m_logSelector->setEnabled(useLogNames);
  m_logValues->setEnabled(useLogNames && isLogSelectorCustom);
  m_axisNameEdit->setEnabled(isSurfaceOrContourPlot);
  if (useLogNames) {
    // Make sure an appropriate name is shown for the default log option.
    if (m_plotOptions->currentText() == SURFACE_PLOT || m_plotOptions->currentText() == CONTOUR_PLOT) {
      m_logSelector->setItemText(0, WORKSPACE_INDEX);
      if (m_axisNameEdit->lineEdit()->text() == WORKSPACE_NAME) {
        m_axisNameEdit->lineEdit()->setText(WORKSPACE_INDEX);
      }
    } else {
      m_logSelector->setItemText(0, WORKSPACE_NAME);
    }
  }
}

namespace {
struct LogTestStruct {
  LogTestStruct() : isconstantvalue(true), value(std::numeric_limits<double>::quiet_NaN()) {}
  LogTestStruct(bool isconstantvalue, double value) : isconstantvalue(isconstantvalue), value(value) {}

  bool isconstantvalue;
  double value;
};
} // namespace

/**
 * Populate the log combo box with all log names that
 * have single numeric value per workspace (and occur
 * in every workspace)
 */
void MantidWSIndexWidget::populateLogComboBox() {
  // First item should be "Workspace index"
  m_logSelector->addItem(WORKSPACE_NAME);

  // Create a map of all logs and their double represenation to compare against.
  // Only logs that can be converted to a double and are not all equal will make
  // the final cut
  // it is map[logname] = (isconstantvalue, value)
  std::map<std::string, LogTestStruct> usableLogs;
  // add the logs that are present in the first workspace
  auto ws = getWorkspace(m_wsNames[0]);
  if (ws) {
    const auto runObj = ws->run();
    const std::vector<Mantid::Kernel::Property *> &logData = runObj.getLogData();
    for (auto &log : logData) {
      const std::string &name = log->name();
      try {
        const auto value = runObj.getLogAsSingleValue(name, Mantid::Kernel::Math::TimeAveragedMean);
        usableLogs[name] = LogTestStruct{true, value};
      } catch (std::invalid_argument &) {
        // it can't be represented as a double so ignore it
      }
    }
  }

  // loop over all of the workspaces in the group to see that the value has
  // changed
  for (auto &wsName : m_wsNames) {
    ws = getWorkspace(wsName);
    if (ws) {
      const auto runObj = ws->run();
      for (auto &logItem : usableLogs) {
        if (runObj.hasProperty(logItem.first)) {
          // check the value if it is still considered constant
          if (logItem.second.isconstantvalue) {
            const auto value = runObj.getLogAsSingleValue(logItem.first, Mantid::Kernel::Math::TimeAveragedMean);
            // set the bool to whether the value is the same
            logItem.second.isconstantvalue = (value == logItem.second.value);
          }
        } else { // delete it from the list using the name
          usableLogs.erase(logItem.first);
        }
      }
    }
  }

  // Add the log names to the combo box if they appear in all workspaces
  for (auto &logItem : usableLogs) {
    if (!(logItem.second.isconstantvalue)) { // values are non-constant
      m_logSelector->addItem(logItem.first.c_str());
    }
  }

  // Add "Custom" at the end of the list
  m_logSelector->addItem(CUSTOM);
}

Mantid::API::MatrixWorkspace_const_sptr MantidWSIndexWidget::getWorkspace(const QString &workspaceName) const {
  return std::dynamic_pointer_cast<const Mantid::API::MatrixWorkspace>(
      Mantid::API::AnalysisDataService::Instance().retrieve(workspaceName.toStdString()));
}

// True if selected plot is suitable for plotting as contour of surface plot
bool MantidWSIndexWidget::isSuitableForContourOrSurfacePlot() const { return (m_wsNames.size() > 2); }

// True if selected plot is suitable for putting log values in
bool MantidWSIndexWidget::isSuitableForLogValues(const QString &plotOption) const {
  return (plotOption == SIMPLE_PLOT || plotOption == WATERFALL_PLOT || plotOption == SURFACE_PLOT ||
          plotOption == CONTOUR_PLOT);
}

/**
 * Check to see if *all* workspaces have a spectrum axis.
 * If even one does not have a spectra axis, then we wont
 * ask the user to enter spectra Numberss - only workspace indices.
 */
void MantidWSIndexWidget::checkForSpectraAxes() {
  QList<QString>::const_iterator it = m_wsNames.constBegin();
  m_spectra = true;

  for (; it != m_wsNames.constEnd(); ++it) {
    Mantid::API::MatrixWorkspace_const_sptr ws = std::dynamic_pointer_cast<const Mantid::API::MatrixWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve((*it).toStdString()));
    if (nullptr == ws)
      continue;
    bool hasSpectra = false;
    for (int i = 0; i < ws->axes(); i++) {
      if (ws->getAxis(i)->isSpectra())
        hasSpectra = true;
    }
    if (!hasSpectra) {
      m_spectra = false;
      break;
    }
  }
}

/**
 * Get the available interval for each of the workspaces, and then
 * present the user with interval which is the INTERSECTION of each of
 * those intervals.
 */
void MantidWSIndexWidget::generateWsIndexIntervals() {
  QList<QString>::const_iterator it = m_wsNames.constBegin();

  // Cycle through the workspaces ...
  for (; it != m_wsNames.constEnd(); ++it) {
    Mantid::API::MatrixWorkspace_const_sptr ws = std::dynamic_pointer_cast<const Mantid::API::MatrixWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve((*it).toStdString()));
    if (nullptr == ws)
      continue;

    const int endWs = static_cast<int>(ws->getNumberHistograms() - 1); //= static_cast<int> (end->first);

    Interval interval(0, endWs);
    // If no interval has been added yet, just add it ...
    if (it == m_wsNames.constBegin())
      m_wsIndexIntervals.addInterval(interval);
    // ... else set the list as the intersection of what's already there
    // and what has just been added.
    else
      m_wsIndexIntervals.setIntervalList(IntervalList::intersect(m_wsIndexIntervals, interval));
  }
}

/**
 * Get available intervals for spectra Numbers
 */
void MantidWSIndexWidget::generateSpectraNumIntervals() {
  bool firstWs = true;
  for (const auto &wsName : m_wsNames) {
    Mantid::API::MatrixWorkspace_const_sptr ws = std::dynamic_pointer_cast<const Mantid::API::MatrixWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve(wsName.toStdString()));
    if (!ws)
      continue; // Belt and braces.

    const Mantid::spec2index_map spec2index = ws->getSpectrumToWorkspaceIndexMap();

    IntervalList spectraIntervalList;
    for (const auto &pair : spec2index) {
      spectraIntervalList.addInterval(static_cast<int>(pair.first));
    }

    if (firstWs) {
      m_spectraNumIntervals = spectraIntervalList;
      firstWs = false;
    } else {
      m_spectraNumIntervals.setIntervalList(IntervalList::intersect(m_spectraNumIntervals, spectraIntervalList));
    }
  }
}

/**
 * Whether widget is using spectra IDs or workspace indices
 * @returns True if using spectra IDs
 */
bool MantidWSIndexWidget::usingSpectraNumbers() const {
  return m_spectra && m_spectraNumIntervals.getList().size() > 0;
}

//----------------------------------
// MantidWSIndexDialog public methods
//----------------------------------
/**
 * Construct an object of this type
 * @param parent :: The MantidUI area
 * @param flags :: Window flags that are passed to the QDialog constructor
 * @param wsNames :: the names of the workspaces to be plotted
 * @param showWaterfallOption :: If true the waterfall checkbox is created
 * @param showPlotAll :: If true the "Plot all" button is created
 * @param showTiledOption :: If true the "Tiled" option is created
 * @param isAdvanced :: true if advanced plotting dialog is created
 */
MantidWSIndexDialog::MantidWSIndexDialog(QWidget *parent, const Qt::WindowFlags &flags, const QList<QString> &wsNames,
                                         const bool showWaterfallOption, const bool showPlotAll,
                                         const bool showTiledOption, const bool isAdvanced)
    : QDialog(parent, flags), m_widget(this, flags, wsNames, showWaterfallOption, showTiledOption, isAdvanced),
      m_plotAll(showPlotAll) {
  // Set up UI.
  init(isAdvanced);
}

/**
 * Returns the user-selected options
 * @returns Struct containing user options
 */
MantidWSIndexWidget::UserInput MantidWSIndexDialog::getSelections() { return m_widget.getSelections(); }

/**
 * Returns the user-selected plots from the widget
 * @returns Plots selected by user
 */
QMultiMap<QString, std::set<int>> MantidWSIndexDialog::getPlots() const { return m_widget.getPlots(); }

/**
 * Whether the user selected the simple 1D plot
 * @returns True if waterfall plot selected
 */
bool MantidWSIndexDialog::is1DPlotSelected() const { return m_widget.is1DPlotSelected(); }

/**
 * Whether the user selected the "waterfall" plot
 * @returns True if waterfall plot selected
 */
bool MantidWSIndexDialog::isWaterfallPlotSelected() const { return m_widget.isWaterfallPlotSelected(); }

/**
 * Whether the user selected the "tiled" plot
 * @returns True if tiled plot selected
 */
bool MantidWSIndexDialog::isTiledPlotSelected() const { return m_widget.isTiledPlotSelected(); }

/**
 * Whether the user selected the surface plot
 * @returns True if surface plot selected
 */
bool MantidWSIndexDialog::isSurfacePlotSelected() const { return m_widget.isSurfacePlotSelected(); }

/**
 * Whether the user selected the surface plot
 * @returns True if surface plot selected
 */
bool MantidWSIndexDialog::isContourPlotSelected() const { return m_widget.isContourPlotSelected(); }

/**
 * Whether the user selected error bars
 * @returns True if error bars selected
 */
bool MantidWSIndexDialog::isErrorBarsSelected() const { return m_widget.isErrorBarsSelected(); }

//----------------------------------
// MantidWSIndexDialog private slots
//----------------------------------
/**
 * Called when OK button pressed
 */
void MantidWSIndexDialog::plot() {
  if (m_widget.plotRequested()) {
    accept();
  }
}

/**
 * Called when "Plot all" button pressed
 */
void MantidWSIndexDialog::plotAll() {
  if (m_widget.plotAllRequested()) {
    accept();
  }
}

//----------------------------------
// MantidWSIndexDialog private methods
//----------------------------------
void MantidWSIndexDialog::init(bool isAdvanced) {
  m_outer = new QVBoxLayout;

  if (isAdvanced) {
    setWindowTitle(tr("Plot Advanced"));
  } else {
    setWindowTitle(tr("Plot Spectrum"));
  }
  m_outer->insertWidget(1, &m_widget);
  initButtons();
  setLayout(m_outer);
}

void MantidWSIndexDialog::initButtons() {
  m_buttonBox = new QHBoxLayout;

  m_okButton = new QPushButton("OK");
  m_cancelButton = new QPushButton("Cancel");
  m_plotAllButton = new QPushButton("Plot All");

  m_buttonBox->addWidget(m_okButton);
  m_buttonBox->addWidget(m_cancelButton);
  if (m_plotAll)
    m_buttonBox->addWidget(m_plotAllButton);

  m_outer->addItem(m_buttonBox);

  connect(m_okButton, SIGNAL(clicked()), this, SLOT(plot()));
  connect(m_cancelButton, SIGNAL(clicked()), this, SLOT(close()));
  if (m_plotAll)
    connect(m_plotAllButton, SIGNAL(clicked()), this, SLOT(plotAll()));
}

//----------------------------------
// Interval public methods
//----------------------------------
Interval::Interval(int single) { init(single, single); }

Interval::Interval(int start, int end) { init(start, end); }

Interval::Interval(const QString &intervalString) {
  // Check to see if string is of the correct format, and then parse.
  // An interval can either be "n" or "n-m" where n and m are integers
  const QString patternSingle("^\\d+$");     // E.g. "2" or "712"
  const QString patternRange("^\\d+-\\d+$"); // E.g. "2-4" or "214-200"
  const QRegExp regExpSingle(patternSingle);
  const QRegExp regExpRange(patternRange);

  if (regExpSingle.exactMatch(intervalString)) {
    int single = intervalString.toInt();
    init(single, single);
  } else if (regExpRange.exactMatch(intervalString)) {
    QStringList range = intervalString.split("-");
    int first = range[0].toInt();
    int last = range[1].toInt();
    init(first, last);
  } else {
    throw std::exception();
  }
}

Interval::Interval(const Interval &copy) { init(copy.m_start, copy.m_end); }

bool Interval::merge(const Interval &other) {
  // If cant merge, return false
  if (!canMerge(other))
    return false;

  // Else, merge - e.g "2" into "3-5" to create "2-5":

  if (other.start() < m_start)
    m_start = other.start();

  if (other.end() > m_end)
    m_end = other.end();

  return true;
}

bool Interval::canMerge(const Interval &other) const {
  return !(other.start() > m_end + 1 || other.end() + 1 < m_start);
}

int Interval::start() const { return m_start; }

int Interval::end() const { return m_end; }

// Note that the length of an interval with only one number is 1.
// Ergo, "length" is defined as (1 + (end - start))
int Interval::length() const { return 1 + m_end - m_start; }

std::set<int> Interval::getIntSet() const {
  std::set<int> intSet;

  for (int i = m_start; i <= m_end; i++) {
    intSet.insert(i);
  }

  return intSet;
}

bool Interval::contains(const Interval &other) const { return (other.m_start >= m_start && other.m_end <= m_end); }

std::string Interval::toStdString() const {
  std::string output;

  if (m_start == m_end) {
    output += boost::lexical_cast<std::string>(m_start);
  } else {
    output += boost::lexical_cast<std::string>(m_start) + "-";
    output += boost::lexical_cast<std::string>(m_end);
  }

  return output;
}

QString Interval::toQString() const {
  QString output;

  if (m_start == m_end) {
    output.append(QString("%1").arg(m_start));
  } else {
    output.append(QString("%1").arg(m_start));
    output += "-";
    output.append(QString("%1").arg(m_end));
  }

  return output;
}

//----------------------------------
// Interval private methods
//----------------------------------
void Interval::init(int start, int end) {
  if (start <= end) {
    m_start = start;
    m_end = end;
  }
  // Here we cater for the case where a user sets start to be at say 4 but
  // end at 2.  We redefine the interval to be "2-4".
  else {
    m_start = end;
    m_end = start;
  }
}

//----------------------------------
// IntervalList public methods
//----------------------------------
IntervalList::IntervalList(void) = default;

IntervalList::IntervalList(const QString &intervals) { addIntervals(intervals); }

IntervalList::IntervalList(const Interval &interval) { m_list.append(interval); }

const QList<Interval> &IntervalList::getList() const { return m_list; }

int IntervalList::totalIntervalLength() const {
  // Total up all the individual Interval lengths in the list:
  return std::accumulate(m_list.cbegin(), m_list.cend(), 0,
                         [](int lhs, const auto &interval) { return lhs + interval.length(); });
}

std::string IntervalList::toStdString(int numOfIntervals) const {
  std::string output;

  if (m_list.size() <= numOfIntervals) {
    for (int i = 0; i < m_list.size(); i++) {
      if (i > 0)
        output += ", ";

      output += m_list.at(i).toStdString();
    }
  }
  // If the number of Intervals is over the numOfIntervals, then
  // we only print out the first (numOfIntervals - 1) Intervals,
  // followed by a ", ...", followed by the final Interval.
  // E.g. "0,2,4,6,8,10,12,14,16,18" becomes "0,2,4,6,8,...,18"
  else {
    for (int i = 0; i < numOfIntervals - 1; i++) {
      if (i > 0)
        output += ", ";

      output += m_list[i].toStdString();
    }

    output += ", ..., ";
    output += m_list.back().toStdString();
  }
  return output;
}

QString IntervalList::toQString(int numOfIntervals) const {
  QString output(toStdString(numOfIntervals).c_str());

  return output;
}

void IntervalList::addInterval(int single) {
  Interval interval(single, single);

  IntervalList::addInterval(interval);
}

// Note: this is considerably more efficient in the case where intervals are
// added
// smallest first.
void IntervalList::addInterval(Interval interval) {
  if (m_list.size() == 0) {
    m_list.append(interval);
    return;
  }

  bool added = false;
  QList<int> deleteList;

  for (int i = m_list.size() - 1; i >= 0; i--) {
    // if new interval is completely higher than this interval
    if (interval.start() > m_list.at(i).end() + 1) {
      // add new interval as a seperate interval
      m_list.append(interval);
      added = true;
      break;
    }
    // else if the new interval can be merged with this interval
    else if (m_list.at(i).canMerge(interval)) {
      // for each interval in the list before and including this one
      for (int j = i; j >= 0; j--) {
        // if it can be merged into the new interval
        if (m_list.at(j).canMerge(interval)) {
          // do it
          interval.merge(m_list.at(j));
          // then add its index to the list of intervals to be deleted
          deleteList.append(j);
        }
        // else if it cant, there is no need to continue checking whether
        // any other intervals can alse be merged
        else {
          break;
        }
      }
      // insert the new large interval in the correct place
      m_list.insert(i + 1, interval);
      added = true;
      break;
    }
  }
  // if deleteList has any elements, delete intervals at those indices
  if (deleteList.size() > 0) {
    using std::sort;
    sort(std::begin(deleteList), std::end(deleteList));

    for (int i = deleteList.size() - 1; i >= 0; i--) {
      m_list.removeAt(deleteList[i]);
    }
  }
  // if still not assigned, add to the beginning
  if (!added) {
    m_list.insert(0, interval);
  }
}

void IntervalList::addInterval(int start, int end) {
  Interval interval(start, end);

  IntervalList::addInterval(interval);
}

void IntervalList::addIntervals(QString intervals) {
  // Remove whitespace
  intervals = intervals.simplified();
  intervals = intervals.replace(" ", "");

  // Split the string, and add the intervals to the list.
  QStringList intervalList = intervals.split(",");
  for (int i = 0; i < intervalList.size(); i++) {
    Interval interval(intervalList[i]);
    addInterval(interval);
  }
}

void IntervalList::addIntervalList(const IntervalList &intervals) {
  const QList<Interval> &list = intervals.getList();

  QList<Interval>::const_iterator it = list.constBegin();

  for (; it != list.constEnd(); ++it) {
    addInterval((*it));
  }
}

void IntervalList::setIntervalList(const IntervalList &intervals) { m_list = QList<Interval>(intervals.getList()); }

void IntervalList::clear() { m_list = QList<Interval>(); }

std::set<int> IntervalList::getIntSet() const {
  std::set<int> intSet;

  for (const auto &i : m_list) {
    std::set<int> intervalSet = i.getIntSet();
    intSet.insert(intervalSet.begin(), intervalSet.end());
  }

  return intSet;
}

bool IntervalList::contains(const Interval &other) const {
  const auto it =
      std::find_if(m_list.cbegin(), m_list.cend(), [&other](const auto &interval) { return interval.contains(other); });
  return it != m_list.cend();
}

bool IntervalList::contains(const IntervalList &other) const {
  const auto it = std::find_if((other.m_list).cbegin(), (other.m_list).cend(),
                               [this](const auto &interval) { return !IntervalList::contains(interval); });
  return it == (other.m_list).cend();
}

bool IntervalList::isParsable(const QString &input, const IntervalList &container) {
  try {
    const IntervalList test(input);
    return container.contains(test);
  } catch (std::exception &) {
    return false;
  }
}

bool IntervalList::isParsable(const QString &input) {
  try {
    IntervalList interval(input);
    return true;
  } catch (std::exception &) {
    return false;
  }
}

IntervalList IntervalList::intersect(const IntervalList &aList, const Interval &bInterval) {
  const IntervalList bList(bInterval);

  return IntervalList::intersect(aList, bList);
}

IntervalList IntervalList::intersect(const IntervalList &a, const IntervalList &b) {
  IntervalList output;

  const std::set<int> aInts = a.getIntSet();
  const std::set<int> bInts = b.getIntSet();

  for (const auto &aInt : aInts) {
    const bool inIntervalListB = bInts.find(aInt) != bInts.end();
    if (inIntervalListB)
      output.addInterval(aInt);
  }

  return output;
}

//----------------------------------
// IntervalListValidator public methods
//----------------------------------
IntervalListValidator::IntervalListValidator(QObject *parent, IntervalList intervalList)
    : QValidator(parent), m_intervalList(std::move(intervalList)) {}

QValidator::State IntervalListValidator::validate(QString &input, int &pos) const {
  UNUSED_ARG(pos)
  if (IntervalList::isParsable(input, m_intervalList))
    return QValidator::Acceptable;

  const QString pattern("^(\\d|-|,)*$");
  const QRegExp regExp(pattern);

  if (regExp.exactMatch(input))
    return QValidator::Intermediate;

  return QValidator::Invalid;
}

//////////////////////////////////////
// QLineEditWithErrorMark
//////////////////////////////////////
MantidWSIndexWidget::QLineEditWithErrorMark::QLineEditWithErrorMark(QWidget *parent) : QWidget(parent) {
  auto *layout = new QGridLayout();
  _lineEdit = new QLineEdit();
  m_validLbl = new QLabel("*"); // make it red
  QPalette pal = m_validLbl->palette();
  pal.setColor(QPalette::WindowText, Qt::darkRed);
  m_validLbl->setPalette(pal);
  layout->addWidget(_lineEdit, 0, 0);
  layout->addWidget(m_validLbl, 0, 1);
  m_validLbl->setVisible(false);
  setLayout(layout);
}

void MantidWSIndexWidget::QLineEditWithErrorMark::setError(const QString &error) {
  if (error.isEmpty()) {
    m_validLbl->setVisible(false);
  } else {
    m_validLbl->setVisible(true);
    m_validLbl->setToolTip(error.trimmed());
  }
}
} // namespace MantidQt::MantidWidgets
