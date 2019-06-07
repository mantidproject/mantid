// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALCDataLoadingView.h"

#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidQtWidgets/Common/LogValueSelector.h"

#include <QMessageBox>

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {
/// This is the string "Auto", used for last file
const std::string ALCDataLoadingView::g_autoString = "Auto";

ALCDataLoadingView::ALCDataLoadingView(QWidget *widget) : m_widget(widget) {}

ALCDataLoadingView::~ALCDataLoadingView() {}

void ALCDataLoadingView::initialize() {
  m_ui.setupUi(m_widget);
  m_ui.logValueSelector->setCheckboxShown(false);
  m_ui.logValueSelector->setVisible(true);
  m_ui.logValueSelector->setEnabled(true);
  connect(m_ui.load, SIGNAL(clicked()), SIGNAL(loadRequested()));
  connect(m_ui.firstRun, SIGNAL(fileFindingFinished()),
          SIGNAL(firstRunSelected()));
  connect(m_ui.firstRun, SIGNAL(filesFoundChanged()), this,
          SLOT(handleFirstFileChanged()));
  connect(m_ui.help, SIGNAL(clicked()), this, SLOT(help()));
  connect(m_ui.lastRunAuto, SIGNAL(stateChanged(int)), this,
          SLOT(checkBoxAutoChanged(int)));

  m_ui.dataPlot->setCanvasColour(QColor(240, 240, 240));

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
  if (m_ui.firstRun->isValid()) {
    return m_ui.firstRun->getFirstFilename().toStdString();
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
  std::string toReturn("");

  if (m_ui.lastRun->isValid()) {
    toReturn = m_ui.lastRun->getFirstFilename().toStdString();
    QString userInput = m_ui.lastRun->getText();
    if (0 ==
        userInput.compare(QString(autoString().c_str()), Qt::CaseInsensitive)) {
      toReturn = autoString();
    }
  }
  return toReturn;
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

void ALCDataLoadingView::setDataCurve(MatrixWorkspace_sptr &workspace,
                                      std::size_t const &workspaceIndex) {
  // These kwargs ensure only the data points are plotted with no line
  QHash<QString, QVariant> kwargs;
  kwargs.insert("linestyle", QString("None").toLatin1().constData());
  kwargs.insert("marker", QString(".").toLatin1().constData());

  // Error bars on the plot
  QStringList plotsWithErrors{"Data"};
  m_ui.dataPlot->setLinesWithErrors(plotsWithErrors);

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  m_ui.dataPlot->setCurveStyle("Data", -1);
  m_ui.dataPlot->setCurveSymbol("Data", 0);
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

/**
 * Called when the check state of the "Auto" checkbox changes.
 * Set text before setting read-only to validate the right text.
 * @param state :: [input] Check state - member of Qt::CheckState enum
 */
void ALCDataLoadingView::checkBoxAutoChanged(int state) {
  // Tell the presenter about the change
  emit lastRunAutoCheckedChanged(state);
  if (state == Qt::Checked) {
    // Auto mode on
    m_ui.lastRun->setText(autoString().c_str());
    m_ui.lastRun->setReadOnly(true);
  } else {
    // Replace "auto" with the currently loaded file
    // The search is necessary to clear the validator
    m_ui.lastRun->setFileTextWithSearch(m_currentAutoFile.c_str());
    m_ui.lastRun->setReadOnly(false);
  }
}

/**
 * Called when the "first run" file has changed.
 * Sets the "last run" box to look in the same directory.
 */
void ALCDataLoadingView::handleFirstFileChanged() {
  QString directory = m_ui.firstRun->getLastDirectory();
  if (!directory.isEmpty()) {
    m_ui.lastRun->setLastDirectory(directory);
  }
}

} // namespace CustomInterfaces
} // namespace MantidQt
