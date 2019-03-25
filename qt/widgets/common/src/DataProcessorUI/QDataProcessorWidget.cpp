// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/DataProcessorUI/QDataProcessorWidget.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorMainPresenter.h"
#include "MantidQtWidgets/Common/DataProcessorUI/GenericDataProcessorPresenter.h"
#include "MantidQtWidgets/Common/DataProcessorUI/GridDelegate.h"
#include "MantidQtWidgets/Common/DataProcessorUI/QtCommandAdapter.h"
#include "MantidQtWidgets/Common/HintingLineEditFactory.h"
#include "MantidQtWidgets/Common/MantidWidget.h"

#include <QClipboard>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QSettings>
#include <QWhatsThis>
#include <QWidget>

namespace {
const QString DataProcessorSettingsGroup =
    "Mantid/MantidWidgets/ISISDataProcessorUI";
}

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
using namespace Mantid::API;

/** Constructor
 * @param presenter :: [input] A unique ptr to the presenter
 * @param parent :: [input] The parent of this view
 */
QDataProcessorWidget::QDataProcessorWidget(
    std::unique_ptr<DataProcessorPresenter> presenter, QWidget *parent)
    : MantidWidget(parent), m_presenter(std::move(presenter)),
      m_openMap(new QSignalMapper(this)) {

  createTable();

  m_presenter->acceptViews(this, this);
}

/** Delegating constructor
 * @param whitelist :: [input] The white list
 * @param parent :: [input] The parent of this view
 * @param group :: [input] The zero based index of this widget within the parent
 * presenter (reflectometry).
 */
QDataProcessorWidget::QDataProcessorWidget(const WhiteList &whitelist,
                                           QWidget *parent, int group)
    : QDataProcessorWidget(
          Mantid::Kernel::make_unique<GenericDataProcessorPresenter>(whitelist,
                                                                     group),
          parent) {}

/** Delegating constructor
 * @param whitelist :: [input] The white list
 * @param algorithm :: [input] The processing algorithm
 * @param parent :: [input] The parent of this view
 * @param group :: [input] The zero based index of this widget within the parent
 * presenter (reflectometry).
 */
QDataProcessorWidget::QDataProcessorWidget(const WhiteList &whitelist,
                                           const ProcessingAlgorithm &algorithm,
                                           QWidget *parent, int group)
    : QDataProcessorWidget(
          Mantid::Kernel::make_unique<GenericDataProcessorPresenter>(
              whitelist, algorithm, group),
          parent) {}

/** Delegating constructor: pre-processing, no post-processing
 * @param whitelist :: [input] The white list
 * @param preprocessMap :: [input] Pre-processing instructions as a map
 * @param algorithm :: [input] The processing algorithm
 * @param parent :: [input] The parent of this view
 * @param group :: [input] The zero based index of this widget within the parent
 * presenter (reflectometry).
 */
QDataProcessorWidget::QDataProcessorWidget(const WhiteList &whitelist,
                                           const PreprocessMap &preprocessMap,
                                           const ProcessingAlgorithm &algorithm,
                                           QWidget *parent, int group)
    : QDataProcessorWidget(
          Mantid::Kernel::make_unique<GenericDataProcessorPresenter>(
              whitelist, preprocessMap.asMap(), algorithm, group),
          parent) {}

/** Delegating constructor: no pre-processing, post-processing
 * @param whitelist :: [input] The white list
 * @param algorithm :: [input] The processing algorithm
 * @param postprocessor :: [input] The post-processing algorithm
 * @param parent :: [input] The parent of this view
 * @param group :: [input] The zero based index of this widget within the parent
 * presenter (reflectometry).
 */
QDataProcessorWidget::QDataProcessorWidget(
    const WhiteList &whitelist, const ProcessingAlgorithm &algorithm,
    const PostprocessingAlgorithm &postprocessor, QWidget *parent, int group)
    : QDataProcessorWidget(
          Mantid::Kernel::make_unique<GenericDataProcessorPresenter>(
              whitelist, algorithm, postprocessor, group),
          parent) {}

/** Delegating constructor: pre-processing, post-processing
 * @param whitelist :: [input] The white list
 * @param preprocessMap :: [input] Pre-processing instructions as a map
 * @param algorithm :: [input] The processing algorithm
 * @param postprocessor :: [input] The post-processing algorithm
 * @param parent :: [input] The parent of this view
 * @param group :: [input] The zero based index of this widget within the parent
 * presenter (reflectometry).
 */
QDataProcessorWidget::QDataProcessorWidget(
    const WhiteList &whitelist, const PreprocessMap &preprocessMap,
    const ProcessingAlgorithm &algorithm,
    const PostprocessingAlgorithm &postprocessor, QWidget *parent, int group)
    : QDataProcessorWidget(
          Mantid::Kernel::make_unique<GenericDataProcessorPresenter>(
              whitelist, preprocessMap.asMap(), algorithm, postprocessor,
              group),
          parent) {}

/** Destructor
 */
QDataProcessorWidget::~QDataProcessorWidget() {}

/**
Initialise the Interface
*/
void QDataProcessorWidget::createTable() {
  ui.setupUi(this);

  // Allow rows and columns to be reordered
  QHeaderView *header = new QHeaderView(Qt::Horizontal);
  header->setStretchLastSection(true);
  header->setStyleSheet("QHeaderView {font-size:11pt;}");
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  header->setMovable(true);
  header->setResizeMode(QHeaderView::ResizeToContents);
#else
  header->setSectionsMovable(true);
  header->setSectionResizeMode(QHeaderView::ResizeToContents);
#endif
  ui.viewTable->setHeader(header);

  // Re-emit a signal when the instrument changes
  connect(ui.comboProcessInstrument, SIGNAL(currentIndexChanged(int)), this,
          SIGNAL(comboProcessInstrument_currentIndexChanged(int)));
  // Custom context menu for table
  connect(ui.viewTable, SIGNAL(customContextMenuRequested(const QPoint &)),
          this, SLOT(showContextMenu(const QPoint &)));
  // Process button
  connect(ui.buttonProcess, SIGNAL(clicked()), this, SLOT(processClicked()));
}

/** Add actions to the toolbar
 * @param commands :: A vector of actions (commands)
 */
void QDataProcessorWidget::addActions(
    std::vector<std::unique_ptr<Command>> commands) {

  // Put the commands in the toolbar
  for (auto &command : commands) {
    m_commands.push_back(Mantid::Kernel::make_unique<QtCommandAdapter>(
        ui.rowToolBar, std::move(command)));
  }

  // Add actions to context menu
  m_contextMenu = new QMenu(this);
  for (const auto &command : m_commands) {
    if (command->hasAction())
      m_contextMenu->addAction(command->getAction());
  }

  // Add a whats this button
  ui.rowToolBar->addAction(QWhatsThis::createAction(this));
}

/** This slot notifies the presenter that the 'Process' button has been
 * clicked
 */
void QDataProcessorWidget::processClicked() {

  m_presenter->notify(DataProcessorPresenter::ProcessFlag);
}

void QDataProcessorWidget::settingsChanged() { m_presenter->settingsChanged(); }

/**
This slot loads a table workspace model and changes to a LoadedMainView
presenter
@param name : the string name of the workspace to be grabbed
*/
void QDataProcessorWidget::setModel(QString const &name) {
  m_toOpen = name;
  m_presenter->notify(DataProcessorPresenter::OpenTableFlag);
}

/**
Set a new model in the tableview
@param model : the model to be attached to the tableview
*/
void QDataProcessorWidget::showTable(
    boost::shared_ptr<AbstractTreeModel> model) {
  m_model = model;
  // So we can notify the presenter when the user updates the table
  connect(m_model.get(),
          SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this,
          SLOT(rowDataUpdated(const QModelIndex &, const QModelIndex &)));
  connect(m_model.get(),
          SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this,
          SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)));
  connect(m_model.get(), SIGNAL(rowsInserted(const QModelIndex &, int, int)),
          this, SLOT(rowsUpdated(const QModelIndex &, int, int)));
  connect(m_model.get(), SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
          this, SLOT(rowsUpdated(const QModelIndex &, int, int)));
  ui.viewTable->setModel(m_model.get());
  ui.viewTable->setStyleSheet("QTreeView {font-size:11pt;}");
  ui.viewTable->setAlternatingRowColors(false);

  // Hide the Hidden Options column
  ui.viewTable->hideColumn(m_model->columnCount() - 1);
}

void QDataProcessorWidget::setItemDelegate() {
  ui.viewTable->setItemDelegate(new GridDelegate(ui.viewTable));
}

/** This slot is used to update the instrument*/
void QDataProcessorWidget::on_comboProcessInstrument_currentIndexChanged(
    int index) {
  ui.comboProcessInstrument->setCurrentIndex(index);
  emit instrumentHasChanged();
}

/**
This slot updates the 'process' status of affected groups and notifies the
presenter that the table has been updated. This is called when rows are added
or removed from the table.
*/
void QDataProcessorWidget::rowsUpdated(const QModelIndex &parent, int start,
                                       int end) {
  Q_UNUSED(start);
  Q_UNUSED(end);

  if (parent.isValid()) {
    // Changing the number of rows in a group will set the containing group
    // unprocessed
    m_model->setProcessed(false, parent.row());
  }

  m_presenter->notify(DataProcessorPresenter::TableUpdatedFlag);
}

/**
This slot updates the 'process' status of affected rows and groups and notifies
the presenter that the table has been updated. This is called when data within
the rows is updated.
*/
void QDataProcessorWidget::rowDataUpdated(const QModelIndex &topLeft,
                                          const QModelIndex &bottomRight) {
  Q_UNUSED(bottomRight);

  if (!m_presenter->isProcessing()) {
    auto pIndex = m_model->parent(topLeft);
    if (pIndex.isValid()) {
      // Changes made to rows outside of processing will set the containing
      // group and all changed row unprocessed
      m_model->setProcessed(false, pIndex.row());
      m_model->setProcessed(false, topLeft.row(), pIndex);
    }
  }

  m_presenter->notify(DataProcessorPresenter::TableUpdatedFlag);
}

/**
This slot is triggered when the user right clicks on the table
@param pos : The position of the right click within the table
*/
void QDataProcessorWidget::showContextMenu(const QPoint &pos) {
  // If the user didn't right-click on anything, don't show a context menu.
  if (ui.viewTable->indexAt(pos).isValid())
    m_contextMenu->popup(ui.viewTable->viewport()->mapToGlobal(pos));
}

void QDataProcessorWidget::ensureHasExtension(QString &filename) const {
  if (!filename.endsWith(".ipynb")) {
    filename.append(".ipynb");
  }
}

/**
Show the user file dialog to choose save location of notebook
*/
QString QDataProcessorWidget::requestNotebookPath() {

  // We won't use QFileDialog directly here as using the NativeDialog option
  // causes problems on MacOS.
  QString qfilename = QFileDialog::getSaveFileName(
      this, "Save notebook file", QDir::currentPath(),
      "IPython Notebook files (*.ipynb);;All files (*)",
      new QString("IPython Notebook files (*.ipynb)"));

  // There is a Qt bug (QTBUG-27186) which means the filename returned
  // from the dialog doesn't always the file extension appended.
  // So we'll have to ensure this ourselves.
  // Important, notebooks can't be loaded without this extension.
  ensureHasExtension(qfilename);
  return qfilename;
}

/**
Expand all currently closed groups
*/
void QDataProcessorWidget::expandAll() { ui.viewTable->expandAll(); }

/**
Collapse all currently expanded groups
*/
void QDataProcessorWidget::collapseAll() { ui.viewTable->collapseAll(); }

/**
Select all rows/groups
*/
void QDataProcessorWidget::selectAll() { ui.viewTable->selectAll(); }

/**
 * Update menu items to be enabled/disabled according to whether processing
 * is in progress or not
 * @param isProcessing :: true if processing is in progress
 */
void QDataProcessorWidget::updateMenuEnabledState(const bool isProcessing) {
  for (const auto &command : m_commands) {
    command->updateEnabledState(isProcessing);
  }
}

/**
 * Sets the "Process" button to be enabled or disabled
 * @param enabled :: true if it should be enabled
 */
void QDataProcessorWidget::setProcessButtonEnabled(const bool enabled) {
  ui.buttonProcess->setEnabled(enabled);
}

/**
 * Sets the "Instrument" combo to be enabled or disabled
 * @param enabled :: true if it should be enabled
 */
void QDataProcessorWidget::setInstrumentComboEnabled(const bool enabled) {
  ui.comboProcessInstrument->setEnabled(enabled);
}

/**
 * Sets the table/tree widget to be enabled or disabled
 * @param enabled :: true if it should be enabled
 */
void QDataProcessorWidget::setTreeEnabled(const bool enabled) {
  // Remember the original edit triggers so that we can revert
  // back to them when re-enabling
  static const auto editTriggers = ui.viewTable->editTriggers();

  if (enabled)
    ui.viewTable->setEditTriggers(editTriggers);
  else
    ui.viewTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

/**
 * Sets the "Output Notebook" widget to be enabled or disabled
 * @param enabled :: true if it should be enabled
 */
void QDataProcessorWidget::setOutputNotebookEnabled(const bool enabled) {
  ui.checkEnableNotebook->setEnabled(enabled);
}

/**
Save settings
@param options : map of user options to save
*/
void QDataProcessorWidget::saveSettings(
    const std::map<QString, QVariant> &options) {
  QSettings settings;
  settings.beginGroup(DataProcessorSettingsGroup);
  for (const auto & option : options)
    settings.setValue(option.first, option.second);
  settings.endGroup();
}

/**
Load settings
@param options : map of user options to load into
*/
void QDataProcessorWidget::loadSettings(std::map<QString, QVariant> &options) {
  QSettings settings;
  settings.beginGroup(DataProcessorSettingsGroup);
  QStringList keys = settings.childKeys();
  for (auto & key : keys)
    options[key] = settings.value(key);
  settings.endGroup();
}

/**
Set the range of the progress bar
@param min : The minimum value of the bar
@param max : The maxmimum value of the bar
*/
void QDataProcessorWidget::setProgressRange(int min, int max) {
  ui.progressBar->setRange(min, max);
  ProgressableView::setProgressRange(min, max);
}

/**
Set the status of the progress bar
@param progress : The current value of the bar
*/
void QDataProcessorWidget::setProgress(int progress) {
  ui.progressBar->setValue(progress);
}

/**
Get status of checkbox which determines whether an ipython notebook is produced
@return true if a notebook should be output on process, false otherwise
*/
bool QDataProcessorWidget::getEnableNotebook() {
  return ui.checkEnableNotebook->isChecked();
}

/**
Set which groups are selected
@param groups : The set of groups to select
*/
void QDataProcessorWidget::setSelection(const std::set<int> &groups) {

  ui.viewTable->clearSelection();
  auto selectionModel = ui.viewTable->selectionModel();

  for (const auto &group : groups) {
    selectionModel->select(ui.viewTable->model()->index(group, 0),
                           QItemSelectionModel::Select |
                               QItemSelectionModel::Rows);
  }
}

/**
Set the list of available instruments to process
@param instruments : The list of instruments available
@param defaultInstrument : The instrument to have selected by default
*/
void QDataProcessorWidget::setInstrumentList(const QString &instruments,
                                             const QString &defaultInstrument) {

  ui.comboProcessInstrument->clear();

  QStringList instrList = instruments.split(",");
  for (auto &instrument : instrList) {
    ui.comboProcessInstrument->addItem(instrument.trimmed());
  }

  int index =
      ui.comboProcessInstrument->findData(defaultInstrument, Qt::DisplayRole);
  ui.comboProcessInstrument->setCurrentIndex(index);
  emit instrumentHasChanged();
}

/**
Set the strategy used for generating hints for the autocompletion in the options
column.
@param hintStrategy : The hinting strategy to use
@param column : The index of the 'Options' column
*/
void QDataProcessorWidget::setOptionsHintStrategy(
    MantidQt::MantidWidgets::HintStrategy *hintStrategy, int column) {
  auto delegate_pointer = ui.viewTable->itemDelegate();
  ui.viewTable->setItemDelegateForColumn(
      column, new HintingLineEditFactory(
                  delegate_pointer, std::unique_ptr<HintStrategy>(hintStrategy),
                  ui.viewTable));
}

/**
Sets the contents of the system's clipboard
@param text The contents of the clipboard
*/
void QDataProcessorWidget::setClipboard(const QString &text) {
  QApplication::clipboard()->setText(text);
}

/**
Get the selected instrument for processing
@returns the selected instrument to process with
*/
QString QDataProcessorWidget::getProcessInstrument() const {
  return ui.comboProcessInstrument->currentText();
}

/**
Get the indices of the highlighted items that have a valid parent
.what()@returns :: a map where keys are parents of selected items and values are
sets
containing the highlighted children
*/
std::map<int, std::set<int>> QDataProcessorWidget::getSelectedChildren() const {
  std::map<int, std::set<int>> rows;
  auto selectionModel = ui.viewTable->selectionModel();
  if (selectionModel) {
    auto selectedRows = selectionModel->selectedRows();
    for (auto & selectedRow : selectedRows) {
      if (selectedRow.parent().isValid()) {
        int children = selectedRow.row();
        int parent = selectedRow.parent().row();
        rows[parent].insert(children);
      }
    }
  }
  return rows;
}

/**
Get the indices of the highlighted items that have invalid parent
@returns :: a set containing the highlighted item numbers
*/
std::set<int> QDataProcessorWidget::getSelectedParents() const {
  std::set<int> parents;
  auto selectionModel = ui.viewTable->selectionModel();
  if (selectionModel) {
    auto selectedRows = selectionModel->selectedRows();
    for (auto & selectedRow : selectedRows) {
      if (!selectedRow.parent().isValid()) {
        parents.insert(selectedRow.row());
      }
    }
  }
  return parents;
}

/**
Get the name of the workspace that the user wishes to open as a table
@returns The name of the workspace to open
*/
QString QDataProcessorWidget::getWorkspaceToOpen() const { return m_toOpen; }

/**
Get a pointer to the presenter that's currently controlling this view.
@returns A pointer to the presenter
*/
DataProcessorPresenter *QDataProcessorWidget::getPresenter() const {
  return m_presenter.get();
}

/**
Gets the contents of the system's clipboard
@returns The contents of the clipboard
*/
QString QDataProcessorWidget::getClipboard() const {
  return QApplication::clipboard()->text();
}

/**
 * Clear the progress
 */
void QDataProcessorWidget::clearProgress() { ui.progressBar->reset(); }

/** Forward a main presenter to this view's presenter
 * @param mainPresenter :: the main presenter
 */
void QDataProcessorWidget::accept(DataProcessorMainPresenter *mainPresenter) {

  m_presenter->accept(mainPresenter);
}

/** Shows a critical error dialog
 *
 * @param prompt : The prompt to appear on the dialog
 * @param title : The text for the title bar of the dialog
 */
void QDataProcessorWidget::giveUserCritical(QString prompt, QString title) {

  QMessageBox::critical(this, title, prompt, QMessageBox::Ok, QMessageBox::Ok);
}

/** Shows a warning dialog
 *
 * @param prompt : The prompt to appear on the dialog
 * @param title : The text for the title bar of the dialog
 */
void QDataProcessorWidget::giveUserWarning(QString prompt, QString title) {

  QMessageBox::warning(this, title, prompt, QMessageBox::Ok, QMessageBox::Ok);
}

/** Asks the user a Yes/No question
 *
 * @param prompt : The prompt to appear on the dialog
 * @param title : The text for the title bar of the dialog
 * @returns a boolean true if Yes, false if No
 */
bool QDataProcessorWidget::askUserYesNo(QString prompt, QString title) {

  auto response = QMessageBox::question(this, title, prompt,
                                        QMessageBox::Yes | QMessageBox::No,
                                        QMessageBox::Yes);
  if (response == QMessageBox::Yes) {
    return true;
  }
  return false;
}

/** Asks the user to enter a string.
 *
 * @param prompt : The prompt to appear on the dialog
 * @param title : The text for the title bar of the dialog
 * @param defaultValue : The default value entered.
 * @returns The user's string if submitted, or an empty string
 */
QString QDataProcessorWidget::askUserString(const QString &prompt,
                                            const QString &title,
                                            const QString &defaultValue) {

  bool ok;
  QString text = QInputDialog::getText(this, title, prompt, QLineEdit::Normal,
                                       defaultValue, &ok);
  return ok ? text : QString("");
}

/** Runs python code
 *
 * @param pythonCode :: the python code to run
 * @return :: output from execution
 */
QString QDataProcessorWidget::runPythonAlgorithm(const QString &pythonCode) {

  QString output = runPythonCode(pythonCode);
  emit ranPythonAlgorithm(pythonCode);
  return output;
}

/** Transfer runs to the table
 *
 */
void QDataProcessorWidget::transfer(const QList<QString> &runs) {

  std::vector<std::map<QString, QString>> runsMap(runs.size());
  size_t row = 0;

  for (auto it = runs.constBegin(); it != runs.constEnd(); ++it) {
    QStringList map = (*it).split(",");
    for (auto & jt : map) {
      QStringList pair = jt.split(":");

      // The entry can be of the for "key:value" or of the form "key:" if
      // nothing is to be set in the column.
      if (pair.size() == 1) {
        runsMap[row][pair[0]] = "";
      } else if (pair.size() == 2) {
        runsMap[row][pair[0]] = pair[1];
      } else {
        giveUserCritical("Could not transfer runs to processing table",
                         "Transfer failed");
        return;
      }
    }
    row++;
  }

  m_presenter->transfer(runsMap);
}

/** Get a cell from the table
 *
 * @param row : the row index
 * @param column : the column index
 * @param parentRow : the row index of the parent
 * @param parentColumn : the row index of the parent
 * @return : the value in the cell as a string
 */
QString QDataProcessorWidget::getCell(int row, int column, int parentRow,
                                      int parentColumn) {

  return QString::fromStdString(
      m_presenter->getCell(row, column, parentRow, parentColumn));
}

/** Set a value in the table
 *
 * @param value : the new value
 * @param row : the row index
 * @param column : the column index
 * @param parentRow : the row index of the parent
 * @param parentColumn : the row index of the parent
 */
void QDataProcessorWidget::setCell(const QString &value, int row, int column,
                                   int parentRow, int parentColumn) {

  m_presenter->setCell(row, column, parentRow, parentColumn,
                       value.toStdString());
}

int QDataProcessorWidget::getNumberOfRows() {
  return m_presenter->getNumberOfRows();
}

void QDataProcessorWidget::clearTable() {
  const auto numberOfRows = getNumberOfRows();
  std::set<int> groups;
  for (int index = 0; index < numberOfRows; ++index) {
    groups.insert(groups.end(), index);
  }
  setSelection(groups);
  m_presenter->clearTable();
}

void QDataProcessorWidget::setForcedReProcessing(bool forceReProcessing) {
  m_presenter->setForcedReProcessing(forceReProcessing);
}

QString QDataProcessorWidget::getCurrentInstrument() const {
  return ui.comboProcessInstrument->currentText();
}

void QDataProcessorWidget::skipProcessing() { m_presenter->skipProcessing(); }

void QDataProcessorWidget::enableGrouping() {
  ui.viewTable->setRootIsDecorated(true);
}
void QDataProcessorWidget::disableGrouping() {
  ui.viewTable->setRootIsDecorated(false);
}
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
