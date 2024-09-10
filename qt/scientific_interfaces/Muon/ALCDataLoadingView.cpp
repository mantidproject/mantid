// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALCDataLoadingView.h"
#include "ALCLatestFileFinder.h"

#include "MantidKernel/ConfigService.h"
#include "MantidQtWidgets/Common/FileFinderWidget.h"
#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidQtWidgets/Common/LogValueSelector.h"
#include "MantidQtWidgets/Common/ManageUserDirectories.h"

#include <Poco/File.h>
#include <QMessageBox>
#include <algorithm>

using namespace Mantid::API;

const QString DEFAULT_LOG("run_number");
const std::vector<std::string> INSTRUMENTS{"ARGUS", "CHRONUS", "EMU", "HIFI", "MUSR"};

namespace MantidQt::CustomInterfaces {

ALCDataLoadingView::ALCDataLoadingView(QWidget *widget)
    : m_widget(widget), m_selectedLog(DEFAULT_LOG), m_numPeriods(0) {}

ALCDataLoadingView::~ALCDataLoadingView() = default;

void ALCDataLoadingView::initialize() {
  m_ui.setupUi(m_widget);
  initInstruments();
  m_ui.logValueSelector->setCheckboxShown(false);
  m_ui.logValueSelector->setVisible(true);
  m_ui.logValueSelector->setEnabled(true);
  enableLoad(false);
  enableRunsAutoAdd(false);
  enableAlpha(false);
  showAlphaMessage(false);
  connect(m_ui.load, SIGNAL(clicked()), SIGNAL(loadRequested()));
  connect(m_ui.help, SIGNAL(clicked()), this, SLOT(help()));
  connect(m_ui.instrument, SIGNAL(currentTextChanged(QString)), this, SLOT(instrumentChanged(QString)));
  connect(m_ui.runs, SIGNAL(fileTextChanged(const QString &)), SIGNAL(runsEditingSignal()));
  connect(m_ui.runs, SIGNAL(findingFiles()), SIGNAL(runsEditingFinishedSignal()));
  connect(m_ui.runs, SIGNAL(fileFindingFinished()), SIGNAL(runsFoundSignal()));
  connect(m_ui.manageDirectoriesButton, SIGNAL(clicked()), SIGNAL(manageDirectoriesClicked()));
  connect(m_ui.runsAutoAdd, SIGNAL(toggled(bool)), this, SLOT(runsAutoAddToggled(bool)));
  connect(m_ui.periodInfo, SIGNAL(clicked()), SIGNAL(periodInfoClicked()));

  m_ui.dataPlot->setCanvasColour(QColor(240, 240, 240));

  // Error bars on the plot
  QStringList plotsWithErrors{"Data"};
  m_ui.dataPlot->setLinesWithErrors(plotsWithErrors);
  m_ui.dataPlot->showLegend(false);
  // The following lines disable the groups' titles when the
  // group is disabled
  QPalette palette;
  palette.setColor(QPalette::Disabled, QPalette::WindowText,
                   QApplication::palette().color(QPalette::Disabled, QPalette::WindowText));
  m_ui.dataGroup->setPalette(palette);
  m_ui.deadTimeGroup->setPalette(palette);
  m_ui.detectorGroupingGroup->setPalette(palette);
  m_ui.periodsGroup->setPalette(palette);
  m_ui.calculationGroup->setPalette(palette);
  m_ui.subtractCheckbox->setEnabled(false);

  // Regex validator for runs
  QRegExp re("[0-9]+(,[0-9]+)*(-[0-9]+(($)|(,[0-9]+))+)*");
  QValidator *validator = new QRegExpValidator(re, this);
  m_ui.runs->setTextValidator(validator);

  // Alpha to only accept positive doubles
  m_ui.alpha->setValidator(new QDoubleValidator(0, 99999, 10, this));

  m_ui.runs->doButtonOpt(MantidQt::API::FileFinderWidget::ButtonOpts::None);
}

/**
 * Initialised instrument combo box with Muon instruments and sets index to user
 * defualt instrument if available otherwise set as HIFI
 */
void ALCDataLoadingView::initInstruments() {
  // Initialising so do not want to send signals here
  m_ui.instrument->blockSignals(true);
  for (const auto &instrument : INSTRUMENTS) {
    m_ui.instrument->addItem(QString::fromStdString(instrument));
  }
  const auto userInstrument = Mantid::Kernel::ConfigService::Instance().getString("default.instrument");
  const auto index = m_ui.instrument->findText(QString::fromStdString(userInstrument));
  if (index != -1)
    m_ui.instrument->setCurrentIndex(index);
  else
    m_ui.instrument->setCurrentIndex(3);
  m_ui.instrument->blockSignals(false);
  setInstrument(m_ui.instrument->currentText().toStdString());
}

/**
 * Returns string of selected instrument
 */
std::string ALCDataLoadingView::getInstrument() const { return m_ui.instrument->currentText().toStdString(); }

/**
 * Returns string of path
 */
std::string ALCDataLoadingView::getPath() const { return m_ui.path->text().toStdString(); }

std::string ALCDataLoadingView::log() const { return m_ui.logValueSelector->getLog().toStdString(); }

std::string ALCDataLoadingView::function() const { return m_ui.logValueSelector->getFunctionText().toStdString(); }

std::string ALCDataLoadingView::calculationType() const {
  // XXX: "text" property of the buttons should be set correctly, as accepted by
  //      PlotAsymmetryByLogValue
  return m_ui.calculationType->checkedButton()->text().toStdString();
}

std::string ALCDataLoadingView::deadTimeType() const {
  std::string checkedButton = m_ui.deadTimeCorrType->checkedButton()->text().toStdString();
  if (checkedButton == "From Data File") {
    return std::string("FromRunData");
  } else if (checkedButton == "From Custom File") {
    return std::string("FromSpecifiedFile");
  } else {
    return checkedButton;
  }
}

std::string ALCDataLoadingView::deadTimeFile() const {
  if (deadTimeType() == "FromSpecifiedFile") {
    return m_ui.deadTimeFile->getFirstFilename().toStdString();
  } else {
    return "";
  }
}

std::string ALCDataLoadingView::detectorGroupingType() const {
  std::string checkedButton = m_ui.detectorGroupingType->checkedButton()->text().toStdString();
  return checkedButton;
}

std::string ALCDataLoadingView::getForwardGrouping() const { return m_ui.forwardEdit->text().toStdString(); }

std::string ALCDataLoadingView::getBackwardGrouping() const { return m_ui.backwardEdit->text().toStdString(); }

std::string ALCDataLoadingView::redPeriod() const { return m_ui.redPeriod->currentText().toStdString(); }

std::string ALCDataLoadingView::greenPeriod() const { return m_ui.greenPeriod->currentText().toStdString(); }

bool ALCDataLoadingView::subtractIsChecked() const { return m_ui.subtractCheckbox->isChecked(); }

std::optional<std::pair<double, double>> ALCDataLoadingView::timeRange() const {
  auto range = std::make_pair(m_ui.minTime->value(), m_ui.maxTime->value());
  return std::make_optional(range);
}

void ALCDataLoadingView::setDataCurve(MatrixWorkspace_sptr workspace, std::size_t const &workspaceIndex) {
  // These kwargs ensure only the data points are plotted with no line
  QHash<QString, QVariant> kwargs;
  kwargs.insert("linestyle", QString("None").toLatin1().constData());
  kwargs.insert("marker", QString(".").toLatin1().constData());
  kwargs.insert("distribution", QString("False").toLatin1().constData());

  m_ui.dataPlot->clear();

  // If x scale is run number, ensure plain format
  if (log() == "run_number")
    m_ui.dataPlot->tickLabelFormat("x", "plain", false);
  else
    m_ui.dataPlot->tickLabelFormat("x", "sci", true);

  m_ui.dataPlot->addSpectrum("Data", workspace, workspaceIndex, Qt::black, kwargs);
}

void ALCDataLoadingView::displayError(const std::string &error) {
  QMessageBox::critical(m_widget, "ALC Loading error", QString::fromStdString(error));
}

bool ALCDataLoadingView::displayWarning(const std::string &warning) {
  auto reply = QMessageBox::warning(m_widget, "ALC Warning", QString::fromStdString(warning),
                                    QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
  if (reply == QMessageBox::Yes)
    return true;
  else
    return false;
}

/**
 * Set list of available log values
 * @param logs :: [input] List of log values
 */
void ALCDataLoadingView::setAvailableLogs(const std::vector<std::string> &logs) {
  const auto currentLog = m_ui.logValueSelector->getLog();
  if (!currentLog.isEmpty())
    m_selectedLog = currentLog;

  setAvailableItems(m_ui.logValueSelector->getLogComboBox(), logs);

  if (!setCurrentLog(m_selectedLog))
    setCurrentLog(DEFAULT_LOG);
}

/**
 * Set the currently selected log
 * @param log :: The log to search for and select.
 * @returns true if the log was found and selected.
 */
bool ALCDataLoadingView::setCurrentLog(const QString &log) {
  const auto index = m_ui.logValueSelector->getLogComboBox()->findText(log);
  if (index >= 0) {
    m_ui.logValueSelector->getLogComboBox()->setCurrentIndex(index);
    m_selectedLog = log;
  }
  return index >= 0;
}

/**
 * Set list of available periods in both boxes
 * @param periods :: [input] List of periods
 */
void ALCDataLoadingView::setAvailablePeriods(const std::vector<std::string> &periods) {
  setAvailableItems(m_ui.redPeriod, periods);
  setAvailableItems(m_ui.greenPeriod, periods);

  // Reset subtraction if single period as not possible
  if (periods.size() < 2)
    m_ui.subtractCheckbox->setChecked(false);

  // If single period, disable "Subtract" checkbox and green period box
  const bool multiPeriod = periods.size() > 1;
  m_ui.subtractCheckbox->setEnabled(multiPeriod);
  m_ui.greenPeriod->setEnabled(multiPeriod);

  // If two or more periods and number of periods has changed, default to 1
  // minus 2
  if (periods.size() >= 2 && m_numPeriods != periods.size()) {
    m_ui.subtractCheckbox->setChecked(true);
    m_ui.redPeriod->setCurrentText("1");
    m_ui.greenPeriod->setCurrentText("2");
  }
  m_numPeriods = periods.size();
}

/**
 * Sets available items in a combo box from given list.
 * If the current value is in the new list, keep it.
 * @param comboBox :: [input] Pointer to combo box to populate
 * @param items :: [input] Vector of items to populate box with
 */
void ALCDataLoadingView::setAvailableItems(QComboBox *comboBox, const std::vector<std::string> &items) {
  if (!comboBox) {
    throw std::invalid_argument("No combobox to set items in: this should never happen");
  }

  // Keep the current value
  const auto previousValue = comboBox->currentText().toStdString();

  // Clear previous list
  comboBox->clear();

  // If previous value is in the list, add it at the beginning
  if (std::find(items.begin(), items.end(), previousValue) != items.end()) {
    comboBox->addItem(QString::fromStdString(previousValue));
  }

  // Add new items
  for (const auto &item : items) {
    if (item != previousValue) { // has already been added
      comboBox->addItem(QString::fromStdString(item));
    }
  }
}

void ALCDataLoadingView::setTimeLimits(double tMin, double tMax) {
  // Set initial values
  m_ui.minTime->setValue(tMin);
  m_ui.maxTime->setValue(tMax);
}

void ALCDataLoadingView::setTimeRange(double tMin, double tMax) {
  // Set range for minTime
  m_ui.minTime->setMinimum(tMin);
  m_ui.minTime->setMaximum(tMax);
  // Set range for maxTime
  m_ui.maxTime->setMinimum(tMin);
  m_ui.maxTime->setMaximum(tMax);
}

void ALCDataLoadingView::help() {
  MantidQt::API::HelpWindow::showCustomInterface(QString("Muon ALC"), QString("muon"));
}

void ALCDataLoadingView::disableAll() {

  // Disable all the widgets in the view
  m_ui.plotByLogGroup->setEnabled(false);
  m_ui.dataGroup->setEnabled(false);
  m_ui.deadTimeGroup->setEnabled(false);
  m_ui.detectorGroupingGroup->setEnabled(false);
  m_ui.periodsGroup->setEnabled(false);
  m_ui.calculationGroup->setEnabled(false);
  m_ui.load->setEnabled(false);
}

void ALCDataLoadingView::enableAll() {

  // Enable all the widgets in the view
  m_ui.plotByLogGroup->setEnabled(true);
  m_ui.deadTimeGroup->setEnabled(true);
  m_ui.dataGroup->setEnabled(true);
  m_ui.detectorGroupingGroup->setEnabled(true);
  m_ui.periodsGroup->setEnabled(true);
  m_ui.calculationGroup->setEnabled(true);
  m_ui.load->setEnabled(true);
}

void ALCDataLoadingView::instrumentChanged(QString instrument) {
  emit instrumentChangedSignal(instrument.toStdString());
  if (!m_ui.runs->getText().isEmpty()) {
    m_ui.runs->findFiles(); // Re-search for files with new instrument
  }
}

void ALCDataLoadingView::enableLoad(bool enable) { m_ui.load->setEnabled(enable); }

void ALCDataLoadingView::setPath(const std::string &path) { m_ui.path->setText(QString::fromStdString(path)); }

void ALCDataLoadingView::enableRunsAutoAdd(bool enable) { m_ui.runsAutoAdd->setEnabled(enable); }

void ALCDataLoadingView::setInstrument(const std::string &instrument) {
  m_ui.runs->setInstrumentOverride(QString::fromStdString(instrument));
}

std::string ALCDataLoadingView::getRunsError() { return m_ui.runs->getFileProblem().toStdString(); }

std::vector<std::string> ALCDataLoadingView::getFiles() {
  const auto QFiles = m_ui.runs->getFilenames();
  std::vector<std::string> files;
  std::transform(QFiles.cbegin(), QFiles.cend(), std::back_inserter(files),
                 [](const auto &file) { return file.toStdString(); });
  return files;
}

void ALCDataLoadingView::setFileExtensions(const std::vector<std::string> &extensions) {
  QStringList exts;
  for (std::string value : extensions) {
    exts << QString::fromStdString(value);
  }
  m_ui.runs->setFileExtensions(exts);
}

std::string ALCDataLoadingView::getFirstFile() { return m_ui.runs->getFirstFilename().toStdString(); }

void ALCDataLoadingView::setAvailableInfoToEmpty() {
  setAvailableLogs(std::vector<std::string>());    // Empty logs list
  setAvailablePeriods(std::vector<std::string>()); // Empty period list
  setTimeLimits(0, 0);                             // "Empty" time limits
}

std::string ALCDataLoadingView::getRunsText() const { return m_ui.runs->getText().toStdString(); }

void ALCDataLoadingView::setLoadStatus(const std::string &status, const std::string &colour) {
  m_ui.loadStatusLabel->setText(QString::fromStdString("Status: " + status));
  m_ui.loadStatusLabel->setStyleSheet(QString::fromStdString("color: " + colour));
  m_ui.loadStatusLabel->adjustSize();
}

void ALCDataLoadingView::runsAutoAddToggled(bool on) {
  if (on) {
    m_ui.runs->setReadOnly(true);
    m_ui.load->setEnabled(false);
    setLoadStatus("Auto Add", "orange");
    emit autoAddToggledSignal(true);
  } else {
    m_ui.runs->setReadOnly(false);
    m_ui.load->setEnabled(true);
    setLoadStatus("Waiting", "orange");
    emit autoAddToggledSignal(false);
  }
}

void ALCDataLoadingView::setRunsTextWithoutSearch(const std::string &text) {
  m_ui.runs->setFileTextWithoutSearch(QString::fromStdString(text));
}

void ALCDataLoadingView::toggleRunsAutoAdd(const bool autoAdd) { m_ui.runsAutoAdd->setChecked(autoAdd); }

std::string ALCDataLoadingView::getRunsFirstRunText() const {
  std::string text = m_ui.runs->getText().toStdString();

  auto commaSearchResult = text.find_first_of(",");
  auto rangeSearchResult = text.find_first_of("-");

  if (commaSearchResult == std::string::npos && rangeSearchResult == std::string::npos) {
    return text; // Only one run
  }

  if (commaSearchResult == std::string::npos)
    return text.substr(0, rangeSearchResult); // Must have range
  return text.substr(0, commaSearchResult);   // Must have comma
}

void ALCDataLoadingView::enableAlpha(const bool alpha) {
  m_ui.alpha->setEnabled(alpha);
  m_ui.alphaLabel->setEnabled(alpha);
}

bool ALCDataLoadingView::isAlphaEnabled() const { return m_ui.alpha->isEnabled(); }

void ALCDataLoadingView::setAlphaValue(const std::string &alpha) { m_ui.alpha->setText(QString::fromStdString(alpha)); }

// Get alpha value, defualt value 1
std::string ALCDataLoadingView::getAlphaValue() const {
  if (!m_ui.alpha->text().isEmpty())
    return m_ui.alpha->text().toStdString();
  return "1.0";
}

void ALCDataLoadingView::showAlphaMessage(const bool alpha) { m_ui.alphaMessage->setVisible(alpha); }

} // namespace MantidQt::CustomInterfaces
