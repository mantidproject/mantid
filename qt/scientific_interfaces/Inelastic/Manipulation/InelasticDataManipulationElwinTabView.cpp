// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "InelasticDataManipulationElwinTabView.h"
#include "Common/InterfaceUtils.h"
#include "Common/WorkspaceUtils.h"
#include "MantidGeometry/Instrument.h"
#include "MantidQtWidgets/Common/ParseKeyValueString.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qteditorfactory.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"
#include "MantidQtWidgets/Plotting/RangeSelector.h"

#include <QFileInfo>

#include <algorithm>

#include "MantidQtWidgets/Common/AddWorkspaceMultiDialog.h"

using namespace Mantid::API;
using namespace MantidQt::API;

namespace {
Mantid::Kernel::Logger g_log("Elwin");

namespace Regexes {
const QString EMPTY = "^$";
const QString SPACE = "(\\s)*";
const QString COMMA = SPACE + "," + SPACE;
const QString NATURAL_NUMBER = "(0|[1-9][0-9]*)";
const QString REAL_NUMBER = "(-?" + NATURAL_NUMBER + "(\\.[0-9]*)?)";
const QString REAL_RANGE = "(" + REAL_NUMBER + COMMA + REAL_NUMBER + ")";
const QString MASK_LIST = "(" + REAL_RANGE + "(" + COMMA + REAL_RANGE + ")*" + ")|" + EMPTY;
} // namespace Regexes

class ExcludeRegionDelegate : public QItemDelegate {
public:
  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem & /*option*/,
                        const QModelIndex & /*index*/) const override {
    auto lineEdit = std::make_unique<QLineEdit>(parent);
    auto validator = std::make_unique<QRegExpValidator>(QRegExp(Regexes::MASK_LIST), parent);
    lineEdit->setValidator(validator.release());
    return lineEdit.release();
  }

  void setEditorData(QWidget *editor, const QModelIndex &index) const override {
    const auto value = index.model()->data(index, Qt::EditRole).toString();
    static_cast<QLineEdit *>(editor)->setText(value);
  }

  void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override {
    auto *lineEdit = static_cast<QLineEdit *>(editor);
    model->setData(index, lineEdit->text(), Qt::EditRole);
  }

  void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                            const QModelIndex & /*index*/) const override {
    editor->setGeometry(option.rect);
  }
};

} // namespace

namespace MantidQt::CustomInterfaces {
using namespace IDA;
InelasticDataManipulationElwinTabView::InelasticDataManipulationElwinTabView(QWidget *parent)
    : m_presenter(), m_elwTree(nullptr) {

  // Create Editor Factories
  m_dblEdFac = new DoubleEditorFactory(this);
  m_blnEdFac = new QtCheckBoxFactory(this);
  m_dblManager = new QtDoublePropertyManager();
  m_blnManager = new QtBoolPropertyManager();
  m_grpManager = new QtGroupPropertyManager();

  m_uiForm.setupUi(parent);
}

InelasticDataManipulationElwinTabView::~InelasticDataManipulationElwinTabView() {
  m_elwTree->unsetFactoryForManager(m_dblManager);
  m_elwTree->unsetFactoryForManager(m_blnManager);
}

void InelasticDataManipulationElwinTabView::setup() {
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

  connect(m_uiForm.cbPreviewFile, SIGNAL(currentIndexChanged(int)), this, SLOT(notifyPreviewIndexChanged(int)));
  connect(m_uiForm.spPlotSpectrum, SIGNAL(valueChanged(int)), this, SLOT(notifySelectedSpectrumChanged(int)));
  connect(m_uiForm.cbPlotSpectrum, SIGNAL(currentIndexChanged(int)), this, SLOT(notifySelectedSpectrumChanged(int)));
  connect(m_uiForm.ckCollapse, SIGNAL(stateChanged(int)), this, SLOT(notifyRowModeChanged()));

  // Handle plot and save
  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(notifyRunClicked()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(notifySaveClicked()));
  connect(m_uiForm.pbPlotPreview, SIGNAL(clicked()), this, SLOT(notifyPlotPreviewClicked()));

  // Set any default values
  m_dblManager->setValue(m_properties["IntegrationStart"], -0.02);
  m_dblManager->setValue(m_properties["IntegrationEnd"], 0.02);

  m_dblManager->setValue(m_properties["BackgroundStart"], -0.24);
  m_dblManager->setValue(m_properties["BackgroundEnd"], -0.22);

  setHorizontalHeaders();
}

void InelasticDataManipulationElwinTabView::subscribePresenter(IElwinPresenter *presenter) { m_presenter = presenter; }

void InelasticDataManipulationElwinTabView::notifyRunClicked() { m_presenter->handleRunClicked(); }

void InelasticDataManipulationElwinTabView::notifySaveClicked() { m_presenter->handleSaveClicked(); }

void InelasticDataManipulationElwinTabView::notifyPlotPreviewClicked() { m_presenter->handlePlotPreviewClicked(); }

void InelasticDataManipulationElwinTabView::notifySelectedSpectrumChanged(int index) {
  m_presenter->handlePreviewSpectrumChanged(index);
}

void InelasticDataManipulationElwinTabView::notifyPreviewIndexChanged(int index) {
  m_presenter->handlePreviewIndexChanged(index);
}

void InelasticDataManipulationElwinTabView::notifyRowModeChanged() { m_presenter->handleRowModeChanged(); }

void InelasticDataManipulationElwinTabView::notifyRemoveDataClicked() { m_presenter->handleRemoveSelectedData(); }

void InelasticDataManipulationElwinTabView::notifyAddWorkspaceDialog() { showAddWorkspaceDialog(); }

void InelasticDataManipulationElwinTabView::showAddWorkspaceDialog() {
  auto dialog = new MantidWidgets::AddWorkspaceMultiDialog(parentWidget());
  connect(dialog, SIGNAL(addData(MantidWidgets::IAddWorkspaceDialog *)), this,
          SLOT(notifyAddData(MantidWidgets::IAddWorkspaceDialog *)));
  auto const tabName("Elwin");
  dialog->setup();
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWSSuffices(InterfaceUtils::getSampleWSSuffixes(tabName));
  dialog->setFBSuffices(InterfaceUtils::getSampleFBSuffixes(tabName));
  dialog->show();
}

void InelasticDataManipulationElwinTabView::notifyAddData(MantidWidgets::IAddWorkspaceDialog *dialog) {
  addData(dialog);
}

/** This method checks whether a Workspace or a File is being uploaded through the AddWorkspaceDialog
 *
 */
void InelasticDataManipulationElwinTabView::addData(MantidWidgets::IAddWorkspaceDialog const *dialog) {
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

OutputPlotOptionsView *InelasticDataManipulationElwinTabView::getPlotOptions() const { return m_uiForm.ipoPlotOptions; }

void InelasticDataManipulationElwinTabView::setHorizontalHeaders() {
  QStringList headers;
  headers << "Workspace"
          << "WS Index";
  m_uiForm.tbElwinData->setColumnCount(headers.size());
  m_uiForm.tbElwinData->setHorizontalHeaderLabels(headers);
  auto header = m_uiForm.tbElwinData->horizontalHeader();
  header->setSectionResizeMode(0, QHeaderView::Stretch);
  m_uiForm.tbElwinData->setItemDelegateForColumn(headers.size() - 1,
                                                 std::make_unique<ExcludeRegionDelegate>().release());
  m_uiForm.tbElwinData->verticalHeader()->setVisible(false);
}

void InelasticDataManipulationElwinTabView::clearDataTable() { m_uiForm.tbElwinData->setRowCount(0); }

void InelasticDataManipulationElwinTabView::addTableEntry(int row, std::string const &name,
                                                          std::string const &wsIndexes) {
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

void InelasticDataManipulationElwinTabView::setCell(std::unique_ptr<QTableWidgetItem> cell, int row, int column) {
  m_uiForm.tbElwinData->setItem(static_cast<int>(row), column, cell.release());
}

QModelIndexList InelasticDataManipulationElwinTabView::getSelectedData() {
  return m_uiForm.tbElwinData->selectionModel()->selectedIndexes();
}

void InelasticDataManipulationElwinTabView::setDefaultSampleLog(const Mantid::API::MatrixWorkspace_const_sptr &ws) {
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

/**
 * Handles a new set of input files being entered.
 *
 * Updates preview selection combo box.
 */
void InelasticDataManipulationElwinTabView::newInputDataFromDialog(std::vector<std::string> const &names) {
  // Populate the combo box with the filenames
  QString workspaceNames;
  QString filename;
  m_uiForm.cbPreviewFile->addItems(MantidWidgets::stdVectorToQStringList(names));

  // Default to the first file
  setPreviewToDefault();
}

void InelasticDataManipulationElwinTabView::clearPreviewFile() { m_uiForm.cbPreviewFile->clear(); }

void InelasticDataManipulationElwinTabView::setPreviewToDefault() {
  m_uiForm.cbPreviewFile->setCurrentIndex(0);
  QString const wsname = m_uiForm.cbPreviewFile->currentText();
  auto const inputWs = WorkspaceUtils::getADSWorkspace(wsname.toStdString());
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
void InelasticDataManipulationElwinTabView::plotInput(MatrixWorkspace_sptr inputWS, int spectrum) {
  m_uiForm.ppPlot->clear();

  if (inputWS && inputWS->x(spectrum).size() > 1) {
    m_uiForm.ppPlot->addSpectrum("Sample", inputWS, spectrum);
  }
  setDefaultSampleLog(inputWS);
}

void InelasticDataManipulationElwinTabView::notifyCheckboxValueChanged(QtProperty *prop, bool enabled) {

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

void InelasticDataManipulationElwinTabView::notifyMinChanged(double val) {
  MantidWidgets::RangeSelector *from = qobject_cast<MantidWidgets::RangeSelector *>(sender());
  auto prop = (from == m_uiForm.ppPlot->getRangeSelector("ElwinIntegrationRange")) ? m_properties["IntegrationStart"]
                                                                                   : m_properties["BackgroundStart"];
  m_dblManager->setValue(prop, val);
}

void InelasticDataManipulationElwinTabView::notifyMaxChanged(double val) {
  MantidWidgets::RangeSelector *from = qobject_cast<MantidWidgets::RangeSelector *>(sender());
  auto prop = (from == m_uiForm.ppPlot->getRangeSelector("ElwinIntegrationRange")) ? m_properties["IntegrationEnd"]
                                                                                   : m_properties["BackgroundEnd"];
  m_dblManager->setValue(prop, val);
}

void InelasticDataManipulationElwinTabView::notifyDoubleValueChanged(QtProperty *prop, double val) {
  auto integrationRangeSelector = m_uiForm.ppPlot->getRangeSelector("ElwinIntegrationRange");
  auto backgroundRangeSelector = m_uiForm.ppPlot->getRangeSelector("ElwinBackgroundRange");
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

void InelasticDataManipulationElwinTabView::disconnectSignals() {
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

void InelasticDataManipulationElwinTabView::connectSignals() {
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

void InelasticDataManipulationElwinTabView::setIntegrationStart(double value) {
  m_dblManager->setValue(m_properties["IntegrationStart"], value);
}

double InelasticDataManipulationElwinTabView::getIntegrationStart() {
  return m_dblManager->value(m_properties["IntegrationStart"]);
}

void InelasticDataManipulationElwinTabView::setIntegrationEnd(double value) {
  m_dblManager->setValue(m_properties["IntegrationEnd"], value);
}

double InelasticDataManipulationElwinTabView::getIntegrationEnd() {
  return m_dblManager->value(m_properties["IntegrationEnd"]);
}

void InelasticDataManipulationElwinTabView::setBackgroundStart(double value) {
  m_dblManager->setValue(m_properties["BackgroundStart"], value);
}

double InelasticDataManipulationElwinTabView::getBackgroundStart() {
  return m_dblManager->value(m_properties["BackgroundStart"]);
}

void InelasticDataManipulationElwinTabView::setBackgroundEnd(double value) {
  m_dblManager->setValue(m_properties["BackgroundEnd"], value);
}

double InelasticDataManipulationElwinTabView::getBackgroundEnd() {
  return m_dblManager->value(m_properties["BackgroundEnd"]);
}

/**
 * Set the position of the range selectors on the mini plot
 *
 * @param rs :: Pointer to the RangeSelector
 * @param lower :: The lower bound property in the property browser
 * @param upper :: The upper bound property in the property browser
 * @param bounds :: The upper and lower bounds to be set
 * @param range :: The range to set the range selector to.
 */
void InelasticDataManipulationElwinTabView::setRangeSelector(RangeSelector *rs, QtProperty *lower, QtProperty *upper,
                                                             const QPair<double, double> &range,
                                                             const boost::optional<QPair<double, double>> &bounds) {
  m_dblManager->setValue(lower, range.first);
  m_dblManager->setValue(upper, range.second);
  rs->setRange(range.first, range.second);
  if (bounds) {
    // clamp the bounds of the selector
    rs->setBounds(bounds.get().first, bounds.get().second);
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
void InelasticDataManipulationElwinTabView::setRangeSelectorMin(QtProperty *minProperty, QtProperty *maxProperty,
                                                                RangeSelector *rangeSelector, double newValue) {
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
void InelasticDataManipulationElwinTabView::setRangeSelectorMax(QtProperty *minProperty, QtProperty *maxProperty,
                                                                RangeSelector *rangeSelector, double newValue) {
  if (newValue >= minProperty->valueText().toDouble())
    rangeSelector->setMaximum(newValue);
  else
    m_dblManager->setValue(maxProperty, rangeSelector->getMaximum());
}

void InelasticDataManipulationElwinTabView::setRunIsRunning(const bool running) {
  m_uiForm.pbRun->setText(running ? "Running..." : "Run");
  setButtonsEnabled(!running);
  m_uiForm.ppPlot->watchADS(!running);
}

void InelasticDataManipulationElwinTabView::setButtonsEnabled(const bool enabled) {
  setRunEnabled(enabled);
  setSaveResultEnabled(enabled);
}

void InelasticDataManipulationElwinTabView::setRunEnabled(const bool enabled) { m_uiForm.pbRun->setEnabled(enabled); }

void InelasticDataManipulationElwinTabView::setSaveResultEnabled(const bool enabled) {
  m_uiForm.pbSave->setEnabled(enabled);
}

void InelasticDataManipulationElwinTabView::setAvailableSpectra(WorkspaceIndex minimum, WorkspaceIndex maximum) {
  m_uiForm.elwinPreviewSpec->setCurrentIndex(0);
  m_uiForm.spPlotSpectrum->setMinimum(boost::numeric_cast<int>(minimum.value));
  m_uiForm.spPlotSpectrum->setMaximum(boost::numeric_cast<int>(maximum.value));
}

void InelasticDataManipulationElwinTabView::setAvailableSpectra(const std::vector<WorkspaceIndex>::const_iterator &from,
                                                                const std::vector<WorkspaceIndex>::const_iterator &to) {
  disconnect(m_uiForm.cbPlotSpectrum, SIGNAL(currentIndexChanged(int)), this, SLOT(notifySelectedSpectrumChanged(int)));
  m_uiForm.elwinPreviewSpec->setCurrentIndex(1);
  m_uiForm.cbPlotSpectrum->clear();

  for (auto spectrum = from; spectrum < to; ++spectrum)
    m_uiForm.cbPlotSpectrum->addItem(QString::number(spectrum->value));
  connect(m_uiForm.cbPlotSpectrum, SIGNAL(currentIndexChanged(int)), this, SLOT(notifySelectedSpectrumChanged(int)));
}

std::string InelasticDataManipulationElwinTabView::getPreviewWorkspaceName(int index) const {
  return m_uiForm.cbPreviewFile->itemText(index).toStdString();
}

std::string InelasticDataManipulationElwinTabView::getPreviewFilename(int index) const {
  return m_uiForm.cbPreviewFile->itemData(index).toString().toStdString();
}

int InelasticDataManipulationElwinTabView::getPreviewSpec() const {
  int tabIndex = m_uiForm.elwinPreviewSpec->currentIndex();
  return tabIndex == 0 ? m_uiForm.spPlotSpectrum->value() : m_uiForm.cbPlotSpectrum->currentText().toInt();
}

std::string InelasticDataManipulationElwinTabView::getCurrentPreview() const {
  return m_uiForm.cbPreviewFile->currentText().toStdString();
}

bool InelasticDataManipulationElwinTabView::isGroupInput() const { return m_uiForm.ckGroupOutput->isChecked(); }
bool InelasticDataManipulationElwinTabView::isRowCollapsed() const { return m_uiForm.ckCollapse->isChecked(); }
bool InelasticDataManipulationElwinTabView::isTableEmpty() const { return m_uiForm.tbElwinData->rowCount() == 0; }
bool InelasticDataManipulationElwinTabView::getNormalise() { return m_blnManager->value(m_properties["Normalise"]); }

bool InelasticDataManipulationElwinTabView::getBackgroundSubtraction() {
  return m_blnManager->value(m_properties["BackgroundSubtraction"]);
}

std::string InelasticDataManipulationElwinTabView::getLogName() { return m_uiForm.leLogName->text().toStdString(); }

std::string InelasticDataManipulationElwinTabView::getLogValue() {
  return m_uiForm.leLogValue->currentText().toStdString();
}

void InelasticDataManipulationElwinTabView::showMessageBox(std::string const &message) const {
  QMessageBox::information(parentWidget(), this->windowTitle(), QString::fromStdString(message));
}
} // namespace MantidQt::CustomInterfaces