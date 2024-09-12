// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "ElwinView.h"
#include "MantidGeometry/Instrument.h"
#include "MantidQtWidgets/Common/ParseKeyValueString.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qteditorfactory.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"
#include "MantidQtWidgets/Common/WorkspaceUtils.h"
#include "MantidQtWidgets/Plotting/RangeSelector.h"
#include "MantidQtWidgets/Spectroscopy/InterfaceUtils.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsHelper.h"

#include <algorithm>

#include "MantidQtWidgets/Common/AddWorkspaceMultiDialog.h"

#include <MantidQtWidgets/Spectroscopy/SettingsWidget/Settings.h>

using namespace Mantid::API;
using namespace MantidQt::API;

namespace {
Mantid::Kernel::Logger g_log("Elwin");
} // namespace

namespace MantidQt::CustomInterfaces {
using namespace Inelastic;
ElwinView::ElwinView(QWidget *parent) : QWidget(parent), m_presenter(), m_elwTree(nullptr) {

  // Create Editor Factories
  m_dblEdFac = new DoubleEditorFactory(this);
  m_blnEdFac = new QtCheckBoxFactory(this);
  m_dblManager = new QtDoublePropertyManager();
  m_blnManager = new QtBoolPropertyManager();
  m_grpManager = new QtGroupPropertyManager();

  m_uiForm.setupUi(parent);
}

ElwinView::~ElwinView() {
  m_elwTree->unsetFactoryForManager(m_dblManager);
  m_elwTree->unsetFactoryForManager(m_blnManager);
}

void ElwinView::setup() {
  // Create QtTreePropertyBrowser object
  m_elwTree = new QtTreePropertyBrowser();
  m_uiForm.properties->addWidget(m_elwTree);

  // Editor Factories
  m_elwTree->setFactoryForManager(m_dblManager, m_dblEdFac);
  m_elwTree->setFactoryForManager(m_blnManager, m_blnEdFac);

  // Number of decimal places in property browsers.
  static const unsigned int NUM_DECIMALS = 6;
  // Create Properties
  m_properties["IntegrationStart"] = m_dblManager->addProperty("IntegrationStart");
  m_dblManager->setDecimals(m_properties["IntegrationStart"], NUM_DECIMALS);
  m_properties["IntegrationEnd"] = m_dblManager->addProperty("IntegrationEnd");
  m_dblManager->setDecimals(m_properties["IntegrationEnd"], NUM_DECIMALS);
  m_properties["BackgroundStart"] = m_dblManager->addProperty("BackgroundStart");
  m_dblManager->setDecimals(m_properties["BackgroundStart"], NUM_DECIMALS);
  m_properties["BackgroundEnd"] = m_dblManager->addProperty("BackgroundEnd");
  m_dblManager->setDecimals(m_properties["BackgroundEnd"], NUM_DECIMALS);

  m_properties["BackgroundSubtraction"] = m_blnManager->addProperty("Background Subtraction");
  m_properties["Normalise"] = m_blnManager->addProperty("Normalise to Lowest Temp");

  m_properties["IntegrationRange"] = m_grpManager->addProperty("Integration Range");
  m_properties["IntegrationRange"]->addSubProperty(m_properties["IntegrationStart"]);
  m_properties["IntegrationRange"]->addSubProperty(m_properties["IntegrationEnd"]);
  m_properties["BackgroundRange"] = m_grpManager->addProperty("Background Range");
  m_properties["BackgroundRange"]->addSubProperty(m_properties["BackgroundStart"]);
  m_properties["BackgroundRange"]->addSubProperty(m_properties["BackgroundEnd"]);

  m_elwTree->addProperty(m_properties["IntegrationRange"]);
  m_elwTree->addProperty(m_properties["BackgroundSubtraction"]);
  m_elwTree->addProperty(m_properties["BackgroundRange"]);
  m_elwTree->addProperty(m_properties["Normalise"]);

  // We always want one range selector... the second one can be controlled from

  auto integrationRangeSelector = m_uiForm.ppPlot->addRangeSelector("ElwinIntegrationRange");
  integrationRangeSelector->setBounds(-DBL_MAX, DBL_MAX);
  connect(integrationRangeSelector, SIGNAL(minValueChanged(double)), this, SLOT(notifyMinChanged(double)));
  connect(integrationRangeSelector, SIGNAL(maxValueChanged(double)), this, SLOT(notifyMaxChanged(double)));
  // create the second range
  auto backgroundRangeSelector = m_uiForm.ppPlot->addRangeSelector("ElwinBackgroundRange");
  backgroundRangeSelector->setColour(Qt::darkGreen); // dark green for background
  backgroundRangeSelector->setBounds(-DBL_MAX, DBL_MAX);

  connect(integrationRangeSelector, SIGNAL(selectionChanged(double, double)), backgroundRangeSelector,
          SLOT(setRange(double, double)));
  connect(backgroundRangeSelector, SIGNAL(minValueChanged(double)), this, SLOT(notifyMinChanged(double)));
  connect(backgroundRangeSelector, SIGNAL(maxValueChanged(double)), this, SLOT(notifyMaxChanged(double)));

  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(notifyDoubleValueChanged(QtProperty *, double)));
  connect(m_blnManager, SIGNAL(valueChanged(QtProperty *, bool)), this,
          SLOT(notifyCheckboxValueChanged(QtProperty *, bool)));
  notifyCheckboxValueChanged(m_properties["BackgroundSubtraction"], false);

  connect(m_uiForm.wkspAdd, SIGNAL(clicked()), this, SLOT(notifyAddWorkspaceDialog()));
  connect(m_uiForm.wkspRemove, SIGNAL(clicked()), this, SLOT(notifyRemoveDataClicked()));
  connect(m_uiForm.pbSelAll, SIGNAL(clicked()), this, SLOT(notifySelectAllClicked()));

  connect(m_uiForm.cbPreviewFile, SIGNAL(currentIndexChanged(int)), this, SLOT(notifyPreviewIndexChanged(int)));
  connect(m_uiForm.spPlotSpectrum, SIGNAL(valueChanged(int)), this, SLOT(notifySelectedSpectrumChanged()));
  connect(m_uiForm.cbPlotSpectrum, SIGNAL(currentIndexChanged(int)), this, SLOT(notifySelectedSpectrumChanged()));
  connect(m_uiForm.ckCollapse, SIGNAL(stateChanged(int)), this, SLOT(notifyRowModeChanged()));

  // Handle plot and save
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(notifySaveClicked()));
  connect(m_uiForm.pbPlotPreview, SIGNAL(clicked()), this, SLOT(notifyPlotPreviewClicked()));

  // Set any default values
  m_dblManager->setValue(m_properties["IntegrationStart"], -0.02);
  m_dblManager->setValue(m_properties["IntegrationEnd"], 0.02);

  m_dblManager->setValue(m_properties["BackgroundStart"], -0.24);
  m_dblManager->setValue(m_properties["BackgroundEnd"], -0.22);

  setHorizontalHeaders();
}

void ElwinView::subscribePresenter(IElwinPresenter *presenter) { m_presenter = presenter; }

void ElwinView::notifySaveClicked() { m_presenter->handleSaveClicked(); }

void ElwinView::notifyPlotPreviewClicked() { m_presenter->handlePlotPreviewClicked(); }

void ElwinView::notifySelectedSpectrumChanged() { m_presenter->handlePreviewSpectrumChanged(getPreviewSpec()); }

void ElwinView::notifyPreviewIndexChanged(int index) { m_presenter->handlePreviewIndexChanged(index); }

void ElwinView::notifyRowModeChanged() { m_presenter->handleRowModeChanged(); }

void ElwinView::notifyRemoveDataClicked() { m_presenter->handleRemoveSelectedData(); }

void ElwinView::notifySelectAllClicked() { selectAllRows(); }

void ElwinView::notifyAddWorkspaceDialog() { showAddWorkspaceDialog(); }

void ElwinView::showAddWorkspaceDialog() {
  auto dialog = new MantidWidgets::AddWorkspaceMultiDialog(parentWidget());
  connect(dialog, SIGNAL(addData(MantidWidgets::IAddWorkspaceDialog *)), this,
          SLOT(notifyAddData(MantidWidgets::IAddWorkspaceDialog *)));
  auto const tabName("Elwin");
  dialog->setup();
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWSSuffices(InterfaceUtils::getSampleWSSuffixes(tabName));
  dialog->setFBSuffices(InterfaceUtils::getSampleFBSuffixes(tabName));
  dialog->setLoadProperty("LoadHistory", SettingsHelper::loadHistory());
  dialog->show();
}

void ElwinView::notifyAddData(MantidWidgets::IAddWorkspaceDialog *dialog) { addData(dialog); }

/** This method checks whether a Workspace or a File is being uploaded through the AddWorkspaceDialog
 *
 */
void ElwinView::addData(MantidWidgets::IAddWorkspaceDialog const *dialog) {
  try {
    const auto indirectDialog = dynamic_cast<MantidWidgets::AddWorkspaceMultiDialog const *>(dialog);
    if (indirectDialog) {
      if (!indirectDialog->isEmpty())
        m_presenter->handleAddData(dialog);
      else
        (throw std::runtime_error("Unable to access data: No available workspaces or not selected"));
    }
  } catch (const std::runtime_error &ex) {
    QMessageBox::warning(this->parentWidget(), "Warning! ", ex.what());
  }
}

IRunView *ElwinView::getRunView() const { return m_uiForm.runWidget; }

IOutputPlotOptionsView *ElwinView::getPlotOptions() const { return m_uiForm.ipoPlotOptions; }

void ElwinView::setHorizontalHeaders() {
  QStringList headers;
  headers << "Workspace"
          << "WS Index";
  m_uiForm.tbElwinData->setColumnCount(headers.size());
  m_uiForm.tbElwinData->setHorizontalHeaderLabels(headers);
  auto header = m_uiForm.tbElwinData->horizontalHeader();
  header->setSectionResizeMode(0, QHeaderView::Stretch);
  m_uiForm.tbElwinData->verticalHeader()->setVisible(false);
  m_uiForm.tbElwinData->setSelectionBehavior(QAbstractItemView::SelectRows);
}

void ElwinView::clearDataTable() { m_uiForm.tbElwinData->setRowCount(0); }

void ElwinView::addTableEntry(int row, std::string const &name, std::string const &wsIndexes) {
  m_uiForm.tbElwinData->insertRow(static_cast<int>(row));
  auto cell = std::make_unique<QTableWidgetItem>(QString::fromStdString(name));
  auto flags = cell->flags();
  flags ^= Qt::ItemIsEditable;
  cell->setFlags(flags);
  setCell(std::move(cell), row, 0);

  cell = std::make_unique<QTableWidgetItem>(QString::fromStdString(wsIndexes));
  cell->setFlags(flags);
  setCell(std::move(cell), row, 1);
}

void ElwinView::updatePreviewWorkspaceNames(const std::vector<std::string> &names) {
  m_uiForm.cbPreviewFile->clear();
  m_uiForm.cbPreviewFile->addItems(stdVectorToQStringList(names));
  m_uiForm.cbPreviewFile->setCurrentIndex(static_cast<int>(names.size()) - 1);
}

void ElwinView::setCell(std::unique_ptr<QTableWidgetItem> cell, int row, int column) {
  m_uiForm.tbElwinData->setItem(static_cast<int>(row), column, cell.release());
}

QModelIndexList ElwinView::getSelectedData() { return m_uiForm.tbElwinData->selectionModel()->selectedRows(); }

void ElwinView::selectAllRows() { m_uiForm.tbElwinData->selectAll(); }

void ElwinView::setDefaultSampleLog(const Mantid::API::MatrixWorkspace_const_sptr &ws) {
  auto inst = ws->getInstrument();
  // Set sample environment log name
  auto log = inst->getStringParameter("Workflow.SE-log");
  QString logName("sample");
  if (log.size() > 0) {
    logName = QString::fromStdString(log[0]);
  }
  m_uiForm.leLogName->setText(logName);
  // Set sample environment log value
  auto logval = inst->getStringParameter("Workflow.SE-log-value");
  if (logval.size() > 0) {
    auto logValue = QString::fromStdString(logval[0]);
    int index = m_uiForm.leLogValue->findText(logValue);
    if (index >= 0) {
      m_uiForm.leLogValue->setCurrentIndex(index);
    }
  }
}

void ElwinView::updateSelectorRange(const MatrixWorkspace_sptr &inputWs) {
  const auto range = WorkspaceUtils::getXRangeFromWorkspace(inputWs);

  setRangeSelector(m_uiForm.ppPlot->getRangeSelector("ElwinIntegrationRange"), m_properties["IntegrationStart"],
                   m_properties["IntegrationEnd"], range);
  setRangeSelector(m_uiForm.ppPlot->getRangeSelector("ElwinBackgroundRange"), m_properties["BackgroundStart"],
                   m_properties["BackgroundEnd"], range);
}

/**
 * Plots the selected spectrum of the input workspace.
 *
 * @param previewPlot The preview plot widget in which to plot the input
 *                    input workspace.
 */
void ElwinView::plotInput(MatrixWorkspace_sptr inputWS, int spectrum) {
  m_uiForm.ppPlot->clear();

  if (inputWS && inputWS->x(spectrum).size() > 1) {
    m_uiForm.ppPlot->addSpectrum("Sample", inputWS, spectrum);
  }
  setDefaultSampleLog(inputWS);
}

void ElwinView::notifyCheckboxValueChanged(QtProperty *prop, bool enabled) {

  m_presenter->handleValueChanged(prop->propertyName().toStdString(), enabled);

  if (prop == m_properties["BackgroundSubtraction"]) {
    auto integrationRangeSelector = m_uiForm.ppPlot->getRangeSelector("ElwinIntegrationRange");
    auto backgroundRangeSelector = m_uiForm.ppPlot->getRangeSelector("ElwinBackgroundRange");
    backgroundRangeSelector->setVisible(enabled);
    m_properties["BackgroundStart"]->setEnabled(enabled);
    m_properties["BackgroundEnd"]->setEnabled(enabled);

    disconnect(integrationRangeSelector, SIGNAL(selectionChanged(double, double)), backgroundRangeSelector,
               SLOT(setRange(double, double)));
    if (!enabled) {
      backgroundRangeSelector->setRange(integrationRangeSelector->getRange());
      connect(integrationRangeSelector, SIGNAL(selectionChanged(double, double)), backgroundRangeSelector,
              SLOT(setRange(double, double)));
    }
  }
}

void ElwinView::notifyMinChanged(double val) {
  MantidWidgets::RangeSelector *from = qobject_cast<MantidWidgets::RangeSelector *>(sender());
  auto prop = (from == m_uiForm.ppPlot->getRangeSelector("ElwinIntegrationRange")) ? m_properties["IntegrationStart"]
                                                                                   : m_properties["BackgroundStart"];
  m_dblManager->setValue(prop, val);
}

void ElwinView::notifyMaxChanged(double val) {
  MantidWidgets::RangeSelector *from = qobject_cast<MantidWidgets::RangeSelector *>(sender());
  auto const prop = (from == m_uiForm.ppPlot->getRangeSelector("ElwinIntegrationRange"))
                        ? m_properties["IntegrationEnd"]
                        : m_properties["BackgroundEnd"];
  m_dblManager->setValue(prop, val);
}

void ElwinView::notifyDoubleValueChanged(QtProperty *prop, double val) {
  auto const integrationRangeSelector = m_uiForm.ppPlot->getRangeSelector("ElwinIntegrationRange");
  auto const backgroundRangeSelector = m_uiForm.ppPlot->getRangeSelector("ElwinBackgroundRange");
  m_presenter->handleValueChanged(prop->propertyName().toStdString(), val);

  disconnectSignals();
  if (prop == m_properties["IntegrationStart"])
    setRangeSelectorMin(m_properties["IntegrationStart"], m_properties["IntegrationEnd"], integrationRangeSelector,
                        val);
  else if (prop == m_properties["IntegrationEnd"])
    setRangeSelectorMax(m_properties["IntegrationStart"], m_properties["IntegrationEnd"], integrationRangeSelector,
                        val);
  else if (prop == m_properties["BackgroundStart"])
    setRangeSelectorMin(m_properties["BackgroundStart"], m_properties["BackgroundEnd"], backgroundRangeSelector, val);
  else if (prop == m_properties["BackgroundEnd"])
    setRangeSelectorMax(m_properties["BackgroundStart"], m_properties["BackgroundEnd"], backgroundRangeSelector, val);
  connectSignals();
}

void ElwinView::disconnectSignals() const {
  disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
             SLOT(notifyDoubleValueChanged(QtProperty *, double)));
  disconnect(m_uiForm.ppPlot->getRangeSelector("ElwinIntegrationRange"), SIGNAL(maxValueChanged(double)), this,
             SLOT(notifyMaxChanged(double)));
  disconnect(m_uiForm.ppPlot->getRangeSelector("ElwinIntegrationRange"), SIGNAL(minValueChanged(double)), this,
             SLOT(notifyMinChanged(double)));
  disconnect(m_uiForm.ppPlot->getRangeSelector("ElwinBackgroundRange"), SIGNAL(maxValueChanged(double)), this,
             SLOT(notifyMaxChanged(double)));
  disconnect(m_uiForm.ppPlot->getRangeSelector("ElwinBackgroundRange"), SIGNAL(minValueChanged(double)), this,
             SLOT(notifyMinChanged(double)));
}

void ElwinView::connectSignals() const {
  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(notifyDoubleValueChanged(QtProperty *, double)));
  connect(m_uiForm.ppPlot->getRangeSelector("ElwinIntegrationRange"), SIGNAL(maxValueChanged(double)), this,
          SLOT(notifyMaxChanged(double)));
  connect(m_uiForm.ppPlot->getRangeSelector("ElwinIntegrationRange"), SIGNAL(minValueChanged(double)), this,
          SLOT(notifyMinChanged(double)));
  connect(m_uiForm.ppPlot->getRangeSelector("ElwinBackgroundRange"), SIGNAL(maxValueChanged(double)), this,
          SLOT(notifyMaxChanged(double)));
  connect(m_uiForm.ppPlot->getRangeSelector("ElwinBackgroundRange"), SIGNAL(minValueChanged(double)), this,
          SLOT(notifyMinChanged(double)));
}

void ElwinView::setIntegrationStart(double value) { m_dblManager->setValue(m_properties["IntegrationStart"], value); }

double ElwinView::getIntegrationStart() { return m_dblManager->value(m_properties["IntegrationStart"]); }

void ElwinView::setIntegrationEnd(double value) { m_dblManager->setValue(m_properties["IntegrationEnd"], value); }

double ElwinView::getIntegrationEnd() { return m_dblManager->value(m_properties["IntegrationEnd"]); }

void ElwinView::setBackgroundStart(double value) { m_dblManager->setValue(m_properties["BackgroundStart"], value); }

double ElwinView::getBackgroundStart() { return m_dblManager->value(m_properties["BackgroundStart"]); }

void ElwinView::setBackgroundEnd(double value) { m_dblManager->setValue(m_properties["BackgroundEnd"], value); }

double ElwinView::getBackgroundEnd() { return m_dblManager->value(m_properties["BackgroundEnd"]); }

/**
 * Set the position of the range selectors on the mini plot
 *
 * @param rs :: Pointer to the RangeSelector
 * @param lower :: The lower bound property in the property browser
 * @param upper :: The upper bound property in the property browser
 * @param bounds :: The upper and lower bounds to be set
 * @param range :: The range to set the range selector to.
 */
void ElwinView::setRangeSelector(RangeSelector *rs, QtProperty *lower, QtProperty *upper,
                                 const QPair<double, double> &range,
                                 const std::optional<QPair<double, double>> &bounds) {
  m_dblManager->setValue(lower, range.first);
  m_dblManager->setValue(upper, range.second);
  rs->setRange(range.first, range.second);
  if (bounds) {
    // clamp the bounds of the selector
    const auto value = bounds.value();
    rs->setBounds(value.first, value.second);
  }
}

/**
 * Set the minimum of a range selector if it is less than the maximum value.
 * To be used when changing the min or max via the Property table
 *
 * @param minProperty :: The property managing the minimum of the range
 * @param maxProperty :: The property managing the maximum of the range
 * @param rangeSelector :: The range selector
 * @param newValue :: The new value for the minimum
 */
void ElwinView::setRangeSelectorMin(QtProperty *minProperty, QtProperty *maxProperty, RangeSelector *rangeSelector,
                                    double newValue) {
  if (newValue <= maxProperty->valueText().toDouble())
    rangeSelector->setMinimum(newValue);
  else
    m_dblManager->setValue(minProperty, rangeSelector->getMinimum());
}

/**
 * Set the maximum of a range selector if it is greater than the minimum value
 * To be used when changing the min or max via the Property table
 *
 * @param minProperty :: The property managing the minimum of the range
 * @param maxProperty :: The property managing the maximum of the range
 * @param rangeSelector :: The range selector
 * @param newValue :: The new value for the maximum
 */
void ElwinView::setRangeSelectorMax(QtProperty *minProperty, QtProperty *maxProperty, RangeSelector *rangeSelector,
                                    double newValue) {
  if (newValue >= minProperty->valueText().toDouble())
    rangeSelector->setMaximum(newValue);
  else
    m_dblManager->setValue(maxProperty, rangeSelector->getMaximum());
}

void ElwinView::setRunIsRunning(const bool running) {
  setSaveResultEnabled(!running);
  m_uiForm.ppPlot->watchADS(!running);
}

void ElwinView::setSaveResultEnabled(const bool enabled) { m_uiForm.pbSave->setEnabled(enabled); }

void ElwinView::setAvailableSpectra(WorkspaceIndex minimum, WorkspaceIndex maximum) {
  m_uiForm.elwinPreviewSpec->setCurrentIndex(0);
  m_uiForm.spPlotSpectrum->setMinimum(boost::numeric_cast<int>(minimum.value));
  m_uiForm.spPlotSpectrum->setMaximum(boost::numeric_cast<int>(maximum.value));
  m_uiForm.spPlotSpectrum->setValue(m_uiForm.spPlotSpectrum->minimum());
}

void ElwinView::setAvailableSpectra(const std::vector<WorkspaceIndex>::const_iterator &from,
                                    const std::vector<WorkspaceIndex>::const_iterator &to) {
  disconnect(m_uiForm.cbPlotSpectrum, SIGNAL(currentIndexChanged(int)), this, SLOT(notifySelectedSpectrumChanged()));
  m_uiForm.elwinPreviewSpec->setCurrentIndex(1);
  m_uiForm.cbPlotSpectrum->clear();

  for (auto spectrum = from; spectrum < to; ++spectrum)
    m_uiForm.cbPlotSpectrum->addItem(QString::number(spectrum->value));

  m_uiForm.cbPlotSpectrum->setCurrentIndex(0);

  connect(m_uiForm.cbPlotSpectrum, SIGNAL(currentIndexChanged(int)), this, SLOT(notifySelectedSpectrumChanged()));
}

std::string ElwinView::getPreviewWorkspaceName(int index) const {
  return m_uiForm.cbPreviewFile->itemText(index).toStdString();
}

void ElwinView::setPreviewWorkspaceName(int index) { m_uiForm.cbPreviewFile->setCurrentIndex(index); }

int ElwinView::getPreviewSpec() const {
  int tabIndex = m_uiForm.elwinPreviewSpec->currentIndex();
  return tabIndex == 0 ? m_uiForm.spPlotSpectrum->value() : m_uiForm.cbPlotSpectrum->currentText().toInt();
}

std::string ElwinView::getCurrentPreview() const { return m_uiForm.cbPreviewFile->currentText().toStdString(); }

bool ElwinView::isGroupInput() const { return m_uiForm.ckGroupOutput->isChecked(); }
bool ElwinView::isRowCollapsed() const { return m_uiForm.ckCollapse->isChecked(); }
bool ElwinView::isTableEmpty() const { return m_uiForm.tbElwinData->rowCount() == 0; }
bool ElwinView::getNormalise() { return m_blnManager->value(m_properties["Normalise"]); }

bool ElwinView::getBackgroundSubtraction() { return m_blnManager->value(m_properties["BackgroundSubtraction"]); }

std::string ElwinView::getLogName() { return m_uiForm.leLogName->text().toStdString(); }

std::string ElwinView::getLogValue() { return m_uiForm.leLogValue->currentText().toStdString(); }

void ElwinView::showMessageBox(std::string const &message) const {
  QMessageBox::information(parentWidget(), this->windowTitle(), QString::fromStdString(message));
}
} // namespace MantidQt::CustomInterfaces
