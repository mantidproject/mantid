// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALCDataLoadingView.h"

#include "ALCLatestFileFinder.h"
#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidQtWidgets/Common/LogValueSelector.h"
#include <Poco/File.h>

#include <QMessageBox>

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {

ALCDataLoadingView::ALCDataLoadingView(QWidget *widget) : m_widget(widget) {}

ALCDataLoadingView::~ALCDataLoadingView() {}

void ALCDataLoadingView::initialize() {
  m_ui.setupUi(m_widget);
  m_ui.logValueSelector->setCheckboxShown(false);
  m_ui.logValueSelector->setVisible(true);
  m_ui.logValueSelector->setEnabled(true);
  connect(m_ui.load, SIGNAL(clicked()), SIGNAL(loadRequested()));
  connect(m_ui.runs, SIGNAL(fileFindingFinished()), SIGNAL(runsSelected()));
  connect(m_ui.help, SIGNAL(clicked()), this, SLOT(help()));
  connect(m_ui.runAuto, SIGNAL(stateChanged(int)), this,
          SLOT(checkBoxAutoChanged(int)));

  m_ui.dataPlot->setCanvasColour(QColor(240, 240, 240));

  // Error bars on the plot
  QStringList plotsWithErrors{"Data"};
  m_ui.dataPlot->setLinesWithErrors(plotsWithErrors);
  m_ui.dataPlot->showLegend(false);
  // The following lines disable the groups' titles when the
  // group is disabled
  QPalette palette;
  palette.setColor(
      QPalette::Disabled, QPalette::WindowText,
      QApplication::palette().color(QPalette::Disabled, QPalette::WindowText));
  m_ui.dataGroup->setPalette(palette);
  m_ui.deadTimeGroup->setPalette(palette);
  m_ui.detectorGroupingGroup->setPalette(palette);
  m_ui.periodsGroup->setPalette(palette);
  m_ui.calculationGroup->setPalette(palette);
}

std::string ALCDataLoadingView::firstRun() const {
  if (m_ui.runs->isValid()) {
    return m_ui.runs->getFirstFilename().toStdString();
  } else {
    return "";
  }
}

/**
 * If the last run is valid, return the filename.
 * If user entered "Auto", return this.
 * Otherwise, return an empty string.
 */
std::string ALCDataLoadingView::lastRun() const {
  if (m_ui.runs->isValid()) {
    const auto files = m_ui.runs->getFilenames();
    if (!files.empty())
      return files.back().toStdString();
  }
  return "";
}

std::vector<std::string> ALCDataLoadingView::getRuns() const {
  std::vector<std::string> returnFiles;
  if (m_ui.runs->isValid()) {
    const auto fileNames = m_ui.runs->getFilenames();
    for (const auto &file : fileNames)
      returnFiles.emplace_back(file.toStdString());
  }
  return returnFiles;
}

std::string ALCDataLoadingView::log() const {
  return m_ui.logValueSelector->getLog().toStdString();
}

std::string ALCDataLoadingView::function() const {
  return m_ui.logValueSelector->getFunctionText().toStdString();
}

std::string ALCDataLoadingView::calculationType() const {
  // XXX: "text" property of the buttons should be set correctly, as accepted by
  //      PlotAsymmetryByLogValue
  return m_ui.calculationType->checkedButton()->text().toStdString();
}

std::string ALCDataLoadingView::deadTimeType() const {
  std::string checkedButton =
      m_ui.deadTimeCorrType->checkedButton()->text().toStdString();
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
  std::string checkedButton =
      m_ui.detectorGroupingType->checkedButton()->text().toStdString();
  return checkedButton;
}

std::string ALCDataLoadingView::getForwardGrouping() const {
  return m_ui.forwardEdit->text().toStdString();
}

std::string ALCDataLoadingView::getBackwardGrouping() const {
  return m_ui.backwardEdit->text().toStdString();
}

std::string ALCDataLoadingView::redPeriod() const {
  return m_ui.redPeriod->currentText().toStdString();
}

std::string ALCDataLoadingView::greenPeriod() const {
  return m_ui.greenPeriod->currentText().toStdString();
}

bool ALCDataLoadingView::subtractIsChecked() const {
  return m_ui.subtractCheckbox->isChecked();
}

boost::optional<std::pair<double, double>>
ALCDataLoadingView::timeRange() const {
  auto range = std::make_pair(m_ui.minTime->value(), m_ui.maxTime->value());
  return boost::make_optional(range);
}

void ALCDataLoadingView::setDataCurve(MatrixWorkspace_sptr workspace,
                                      std::size_t const &workspaceIndex) {
  // These kwargs ensure only the data points are plotted with no line
  QHash<QString, QVariant> kwargs;
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  m_ui.dataPlot->setCurveStyle("Data", -1);
  m_ui.dataPlot->setCurveSymbol("Data", 0);
#else
  kwargs.insert("linestyle", QString("None").toLatin1().constData());
  kwargs.insert("marker", QString(".").toLatin1().constData());
#endif

  m_ui.dataPlot->clear();
  m_ui.dataPlot->addSpectrum("Data", workspace, workspaceIndex, Qt::black,
                             kwargs);
}

void ALCDataLoadingView::displayError(const std::string &error) {
  QMessageBox::critical(m_widget, "Loading error",
                        QString::fromStdString(error));
}

/**
 * Set list of available log values
 * @param logs :: [input] List of log values
 */
void ALCDataLoadingView::setAvailableLogs(
    const std::vector<std::string> &logs) {
  setAvailableItems(m_ui.logValueSelector->getLogComboBox(), logs);
}

/**
 * Set list of available periods in both boxes
 * @param periods :: [input] List of periods
 */
void ALCDataLoadingView::setAvailablePeriods(
    const std::vector<std::string> &periods) {
  setAvailableItems(m_ui.redPeriod, periods);
  setAvailableItems(m_ui.greenPeriod, periods);

  // If single period, disable "Subtract" checkbox and green period box
  const bool multiPeriod = periods.size() > 1;
  m_ui.subtractCheckbox->setEnabled(multiPeriod);
  m_ui.greenPeriod->setEnabled(multiPeriod);
}

/**
 * Sets available items in a combo box from given list.
 * If the current value is in the new list, keep it.
 * @param comboBox :: [input] Pointer to combo box to populate
 * @param items :: [input] Vector of items to populate box with
 */
void ALCDataLoadingView::setAvailableItems(
    QComboBox *comboBox, const std::vector<std::string> &items) {
  if (!comboBox) {
    throw std::invalid_argument(
        "No combobox to set items in: this should never happen");
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
  MantidQt::API::HelpWindow::showCustomInterface(nullptr, QString("Muon ALC"));
}

void ALCDataLoadingView::disableAll() {

  // Disable all the widgets in the view
  m_ui.dataGroup->setEnabled(false);
  m_ui.deadTimeGroup->setEnabled(false);
  m_ui.detectorGroupingGroup->setEnabled(false);
  m_ui.periodsGroup->setEnabled(false);
  m_ui.calculationGroup->setEnabled(false);
  m_ui.load->setEnabled(false);
}

void ALCDataLoadingView::enableAll() {

  // Enable all the widgets in the view
  m_ui.deadTimeGroup->setEnabled(true);
  m_ui.dataGroup->setEnabled(true);
  m_ui.detectorGroupingGroup->setEnabled(true);
  m_ui.periodsGroup->setEnabled(true);
  m_ui.calculationGroup->setEnabled(true);
  m_ui.load->setEnabled(true);
}

void ALCDataLoadingView::updateRunsTextFromAuto() {

  const int currentLastRun = extractRunNumber(lastRun());
  auto currentInput = m_ui.runs->getText().toStdString();

  // Only make changes if found run > user last run
  if (m_currentAutoRun < currentLastRun)
    return;

  // Save old input
  m_oldInput = currentInput;

  // Check if range at end
  std::size_t findRange = currentInput.find_last_of("-");
  std::size_t findComma = currentInput.find_last_of(",");
  QString newInput;

  // Remove ending range if at end of input
  if (findRange != -1 && (findComma == -1 || findRange > findComma)) {
    currentInput.erase(findRange, currentInput.length() - 1);
  }

  // Initialise new input
  newInput = QString::fromStdString(currentInput);

  // Will hold the base path for all runs, used to check which run numbers
  // exist
  auto basePath = firstRun();

  // Strip the extension
  size_t findExt = basePath.find_last_of(".");
  const auto ext = basePath.substr(findExt);
  basePath.erase(findExt);

  // Remove the run number part so we are left with just the base path
  const std::string numPart = std::to_string(currentLastRun);
  basePath.erase(basePath.length() - numPart.length());

  bool fnf = false; // file not found

  // Check all files valid between current last and auto, remove bad ones
  for (int i = currentLastRun + 1; i < m_currentAutoRun; ++i) {

    // Try creating a file from base, i and extension
    Poco::File testFile(basePath + std::to_string(i) + ext);

    // If doesn't exist add range to previous run
    if (testFile.exists()) {

      if (fnf) { // Means next file found since a file not found
        // Add comma
        newInput.append(",");
        newInput.append(QString::number(i));
        fnf = false;
      }
    } else {
      newInput.append("-");
      newInput.append(QString::number(i - 1));
      fnf = true;
    }
  }

  // If true then need a comma instead as file before last is missing
  if (fnf)
    newInput.append(",");
  else
    newInput.append("-");
  newInput.append(QString::number(m_currentAutoRun));

  // Update it
  m_ui.runs->setFileTextWithSearch(newInput);
}

/**
 * Called when the check state of the "Auto" checkbox changes.
 * Set text before setting read-only to validate the right text.
 * @param state :: [input] Check state - member of Qt::CheckState enum
 */
void ALCDataLoadingView::checkBoxAutoChanged(int state) {

  // Try to auto fill in rest of runs
  if (state == Qt::Checked) {
    // Set read only
    m_ui.runs->setReadOnly(true);

    // Update auto run
    emit runAutoChecked();

    // Check for failure
    if (m_currentAutoRun == -1) {
      return; // Error displayed from presenter
    }

    // Update runs
    updateRunsTextFromAuto();

  } else {
    // Remove read only
    m_ui.runs->setReadOnly(false);

    // Reset text as before auto checked
    m_ui.runs->setFileTextWithSearch(QString::fromStdString(m_oldInput));
  }
}

/**
 * Remove the run number from a full file path
 * @param file :: [input] full path which contains a run number
 * @return An integer representation of the run number 
 */
int ALCDataLoadingView::extractRunNumber(const std::string &file) {
  if (file.empty())
    return -1;

  auto returnVal = file;
  // Strip beginning of path to just the run (e.g. MUSR00015189.nxs)
  std::size_t found = returnVal.find_last_of("/\\");
  returnVal = returnVal.substr(found + 1);

  // Remove all non-digits
  returnVal.erase(std::remove_if(returnVal.begin(), returnVal.end(),
                               [](auto c) { return !std::isdigit(c); }),
                returnVal.end());

  // Return run number as int (removes leading 0's)
  return std::stoi(returnVal);
}

} // namespace CustomInterfaces
} // namespace MantidQt
