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

#include "MantidKernel/ConfigService.h"

#include <QMessageBox>

using namespace Mantid::API;

const std::vector<std::string> INSTRUMENTS{"ARGUS", "CHRONUS", "EMU", "HIFI",
                                           "MUSR"};

namespace MantidQt {
namespace CustomInterfaces {

ALCDataLoadingView::ALCDataLoadingView(QWidget *widget) : m_widget(widget) {}

ALCDataLoadingView::~ALCDataLoadingView() {}

void ALCDataLoadingView::initialize() {
  m_ui.setupUi(m_widget);
  initInstruments();
  m_ui.logValueSelector->setCheckboxShown(false);
  m_ui.logValueSelector->setVisible(true);
  m_ui.logValueSelector->setEnabled(true);
  enableLoad(false);
  connect(m_ui.load, SIGNAL(clicked()), SIGNAL(loadRequested()));
  connect(m_ui.help, SIGNAL(clicked()), this, SLOT(help()));
  connect(m_ui.instrument, SIGNAL(currentTextChanged(QString)), this,
          SLOT(instrumentChanged(QString)));
  connect(m_ui.path, SIGNAL(textChanged(QString)), this,
          SLOT(pathChanged(QString)));
  connect(m_ui.runs, SIGNAL(returnPressed()), this,
          SIGNAL(runsChangedSignal()));

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
  m_ui.subtractCheckbox->setEnabled(false);

  // Regex for runs
  QRegExp re("[0-9]+(\,[0-9]+)*(\-[0-9]+(($)|(\,[0-9]+))+)*");
  QValidator *validator = new QRegExpValidator(re, this);
  m_ui.runs->setValidator(validator);
}

/**
 * Initialised instrument combo box with Muon instruments and sets index to user
 * defualt instrument if available otherwise set as HIFI
 */
void ALCDataLoadingView::initInstruments() {
  for (const auto &instrument : INSTRUMENTS) {
    m_ui.instrument->addItem(QString::fromStdString(instrument));
  }
  const auto userInstrument =
      Mantid::Kernel::ConfigService::Instance().getString("default.instrument");
  const auto index =
      m_ui.instrument->findText(QString::fromStdString(userInstrument));
  if (index != -1)
    m_ui.instrument->setCurrentIndex(index);
  else
    m_ui.instrument->setCurrentIndex(3);
}

/**
 * Returns string of selected instrument
 */
std::string ALCDataLoadingView::getInstrument() const {
  return m_ui.instrument->currentText().toStdString();
}

/**
 * Returns string of path
 */
std::string ALCDataLoadingView::getPath() const {
  return m_ui.path->text().toStdString();
}

std::string ALCDataLoadingView::getRunsExpression() const {
  return m_ui.runs->text().toStdString();
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
  auto _log = log();
  m_ui.dataPlot->setOverrideAxisLabel(MantidQt::MantidWidgets::AxisID::XBottom,
                                      _log.c_str());
  // If x scale is run number, ensure plain format
  if (log() == "run_number")
    m_ui.dataPlot->tickLabelFormat("x", "plain", false);
  else
    m_ui.dataPlot->tickLabelFormat("x", "sci", true);

  m_ui.dataPlot->addSpectrum("Data", workspace, workspaceIndex, Qt::black,
                             kwargs);
}

void ALCDataLoadingView::displayError(const std::string &error) {
  QMessageBox::critical(m_widget, "Loading error",
                        QString::fromStdString(error));
}

bool ALCDataLoadingView::displayWarning(const std::string &warning) {
  auto reply = QMessageBox::warning(
      m_widget, "Warning", QString::fromStdString(warning),
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

void ALCDataLoadingView::instrumentChanged(QString instrument) {
  emit instrumentChangedSignal(instrument.toStdString());
}
void ALCDataLoadingView::pathChanged(QString path) {
  emit pathChangedSignal(path.toStdString());
}

void ALCDataLoadingView::handleRunsEditingFinsihed() {
  // emit runsChangedSignal(m_ui.runs->text().toStdString());
}

void ALCDataLoadingView::enableLoad(bool enable) {
  m_ui.load->setEnabled(enable);
}

void ALCDataLoadingView::setPath(std::string &path) {
  m_ui.path->setText(QString::fromStdString(path));
}

} // namespace CustomInterfaces
} // namespace MantidQt
