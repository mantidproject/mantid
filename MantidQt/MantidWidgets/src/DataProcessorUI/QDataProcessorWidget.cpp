#include "MantidQtMantidWidgets/DataProcessorUI/QDataProcessorWidget.h"
#include "MantidQtAPI/MantidWidget.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorCommandAdapter.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorMainPresenter.h"
#include "MantidQtMantidWidgets/DataProcessorUI/GenericDataProcessorPresenter.h"
#include "MantidQtMantidWidgets/HintingLineEditFactory.h"

#include <QWidget>
#include <qinputdialog.h>
#include <qmessagebox.h>

namespace {
const QString DataProcessorSettingsGroup =
    "Mantid/MantidWidgets/ISISDataProcessorUI";
}

namespace MantidQt {
namespace MantidWidgets {
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
* @param algorithm :: [input] The processing algorithm
* @param parent :: [input] The parent of this view
*/
QDataProcessorWidget::QDataProcessorWidget(
    const DataProcessorWhiteList &whitelist,
    const DataProcessorProcessingAlgorithm &algorithm, QWidget *parent)
    : QDataProcessorWidget(
          Mantid::Kernel::make_unique<GenericDataProcessorPresenter>(whitelist,
                                                                     algorithm),
          parent) {}

/** Delegating constructor: pre-processing, no post-processing
* @param whitelist :: [input] The white list
* @param preprocessMap :: [input] Pre-processing instructions as a map
* @param algorithm :: [input] The processing algorithm
* @param parent :: [input] The parent of this view
*/
QDataProcessorWidget::QDataProcessorWidget(
    const DataProcessorWhiteList &whitelist,
    const DataProcessorPreprocessMap &preprocessMap,
    const DataProcessorProcessingAlgorithm &algorithm, QWidget *parent)
    : QDataProcessorWidget(
          Mantid::Kernel::make_unique<GenericDataProcessorPresenter>(
              whitelist, preprocessMap.asMap(), algorithm),
          parent) {}

/** Delegating constructor: no pre-processing, post-processing
* @param whitelist :: [input] The white list
* @param algorithm :: [input] The processing algorithm
* @param postprocessor :: [input] The post-processing algorithm
* @param parent :: [input] The parent of this view
*/
QDataProcessorWidget::QDataProcessorWidget(
    const DataProcessorWhiteList &whitelist,
    const DataProcessorProcessingAlgorithm &algorithm,
    const DataProcessorPostprocessingAlgorithm &postprocessor, QWidget *parent)
    : QDataProcessorWidget(
          Mantid::Kernel::make_unique<GenericDataProcessorPresenter>(
              whitelist, algorithm, postprocessor),
          parent) {}

/** Delegating constructor: pre-processing, post-processing
* @param whitelist :: [input] The white list
* @param preprocessMap :: [input] Pre-processing instructions as a map
* @param algorithm :: [input] The processing algorithm
* @param postprocessor :: [input] The post-processing algorithm
* @param parent :: [input] The parent of this view
*/
QDataProcessorWidget::QDataProcessorWidget(
    const DataProcessorWhiteList &whitelist,
    const DataProcessorPreprocessMap &preprocessMap,
    const DataProcessorProcessingAlgorithm &algorithm,
    const DataProcessorPostprocessingAlgorithm &postprocessor, QWidget *parent)
    : QDataProcessorWidget(
          Mantid::Kernel::make_unique<GenericDataProcessorPresenter>(
              whitelist, preprocessMap.asMap(), algorithm, postprocessor),
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
  header->setMovable(true);
  header->setStretchLastSection(true);
  header->setResizeMode(QHeaderView::ResizeToContents);
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
    std::vector<std::unique_ptr<DataProcessorCommand>> commands) {

  // Put the commands in the toolbar
  for (auto &command : commands) {
    m_commands.push_back(
        Mantid::Kernel::make_unique<DataProcessorCommandAdapter>(
            ui.rowToolBar, std::move(command)));
  }

  // Add actions to context menu
  m_contextMenu = new QMenu(this);
  for (const auto &command : m_commands) {
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

/**
This slot loads a table workspace model and changes to a LoadedMainView
presenter
@param name : the string name of the workspace to be grabbed
*/
void QDataProcessorWidget::setModel(QString name) {
  m_toOpen = name.toStdString();
  m_presenter->notify(DataProcessorPresenter::OpenTableFlag);
}

/**
This method loads a table workspace model. Unlike
QDataProcessorWidget::setModel(QString name),
this method is public and takes a std::string as argument. The reason is that
this method is intended to be called by the presenter
@param name : the string name of the workspace to be grabbed
*/
void QDataProcessorWidget::setModel(const std::string &name) {
  setModel(QString::fromStdString(name));
}

/**
Set a new model in the tableview
@param model : the model to be attached to the tableview
*/
void QDataProcessorWidget::showTable(
    boost::shared_ptr<AbstractDataProcessorTreeModel> model) {
  m_model = model;
  // So we can notify the presenter when the user updates the table
  connect(m_model.get(),
          SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this,
          SLOT(rowDataUpdated(const QModelIndex &, const QModelIndex &)));
  connect(m_model.get(), SIGNAL(rowsInserted(const QModelIndex &, int, int)),
          this, SLOT(rowsUpdated(const QModelIndex &, int, int)));
  connect(m_model.get(), SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
          this, SLOT(rowsUpdated(const QModelIndex &, int, int)));
  ui.viewTable->setModel(m_model.get());
}

/**
Set the list of tables the user is offered to open
@param tables : the names of the tables in the ADS
*/
void QDataProcessorWidget::setTableList(const QSet<QString> &tables) {
  ui.menuOpenTable->clear();
  for (auto it = tables.begin(); it != tables.end(); ++it) {
    QAction *openTable = ui.menuOpenTable->addAction(*it);
    openTable->setIcon(QIcon("://worksheet.png"));

    // Map this action to the table name
    m_openMap->setMapping(openTable, *it);
    // When repeated corrections happen the QMessageBox from openTable()
    // method in ReflMainViewPresenter will be called multiple times
    // when 'no' is clicked.
    // ConnectionType = UniqueConnection ensures that
    // each object has only one of these signals.
    connect(openTable, SIGNAL(triggered()), m_openMap, SLOT(map()),
            Qt::UniqueConnection);
    connect(m_openMap, SIGNAL(mapped(QString)), this, SLOT(setModel(QString)),
            Qt::UniqueConnection);
  }
}

/** This slot is used to update the instrument*/
void QDataProcessorWidget::on_comboProcessInstrument_currentIndexChanged(
    int index) {
  ui.comboProcessInstrument->setCurrentIndex(index);
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
  if (!ui.viewTable->indexAt(pos).isValid())
    return;

  m_contextMenu->popup(ui.viewTable->viewport()->mapToGlobal(pos));
}

/**
Show the user file dialog to choose save location of notebook
*/
std::string QDataProcessorWidget::requestNotebookPath() {

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
  std::string filename = qfilename.toStdString();
  if (filename.size() > 6) {
    if (filename.substr(filename.size() - 6) != ".ipynb") {
      filename.append(".ipynb");
    }
  } else {
    filename.append(".ipynb");
  }

  return filename;
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
Handle interface when data reduction paused
*/
void QDataProcessorWidget::pause() {

  // Enable 'resume' buttons
  ui.rowToolBar->actions()[0]->setEnabled(true);
  m_contextMenu->actions()[0]->setEnabled(true);
  ui.buttonProcess->setEnabled(true);

  // Disable 'pause' buttons
  ui.rowToolBar->actions()[1]->setEnabled(false);
  m_contextMenu->actions()[1]->setEnabled(false);
}

/**
Handle interface when data reduction resumed
*/
void QDataProcessorWidget::resume() {

  // Enable 'resume' buttons
  ui.rowToolBar->actions()[0]->setEnabled(false);
  m_contextMenu->actions()[0]->setEnabled(false);
  ui.buttonProcess->setEnabled(false);

  // Disable 'pause' buttons
  ui.rowToolBar->actions()[1]->setEnabled(true);
  m_contextMenu->actions()[1]->setEnabled(true);
}

/**
Save settings
@param options : map of user options to save
*/
void QDataProcessorWidget::saveSettings(
    const std::map<std::string, QVariant> &options) {
  QSettings settings;
  settings.beginGroup(DataProcessorSettingsGroup);
  for (auto it = options.begin(); it != options.end(); ++it)
    settings.setValue(QString::fromStdString(it->first), it->second);
  settings.endGroup();
}

/**
Load settings
@param options : map of user options to load into
*/
void QDataProcessorWidget::loadSettings(
    std::map<std::string, QVariant> &options) {
  QSettings settings;
  settings.beginGroup(DataProcessorSettingsGroup);
  QStringList keys = settings.childKeys();
  for (auto it = keys.begin(); it != keys.end(); ++it)
    options[it->toStdString()] = settings.value(*it);
  settings.endGroup();
}

/**
Set the range of the progress bar
@param min : The minimum value of the bar
@param max : The maxmimum value of the bar
*/
void QDataProcessorWidget::setProgressRange(int min, int max) {
  ui.progressBar->setRange(min, max);
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

  for (auto group = groups.begin(); group != groups.end(); ++group) {
    selectionModel->select(ui.viewTable->model()->index((*group), 0),
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
  for (auto it = instrList.begin(); it != instrList.end(); ++it) {
    ui.comboProcessInstrument->addItem((*it).trimmed());
  }

  int index =
      ui.comboProcessInstrument->findData(defaultInstrument, Qt::DisplayRole);
  ui.comboProcessInstrument->setCurrentIndex(index);
}

/**
Set the strategy used for generating hints for the autocompletion in the options
column.
@param hintStrategy : The hinting strategy to use
@param column : The index of the 'Options' column
*/
void QDataProcessorWidget::setOptionsHintStrategy(HintStrategy *hintStrategy,
                                                  int column) {
  ui.viewTable->setItemDelegateForColumn(
      column, new HintingLineEditFactory(hintStrategy));
}

/**
Sets the contents of the system's clipboard
@param text The contents of the clipboard
*/
void QDataProcessorWidget::setClipboard(const std::string &text) {
  QApplication::clipboard()->setText(QString::fromStdString(text));
}

/**
Get the selected instrument for processing
@returns the selected instrument to process with
*/
std::string QDataProcessorWidget::getProcessInstrument() const {
  return ui.comboProcessInstrument->currentText().toStdString();
}

/**
Get the indices of the highlighted items that have a valid parent
@returns :: a map where keys are parents of selected items and values are sets
containing the highlighted children
*/
std::map<int, std::set<int>> QDataProcessorWidget::getSelectedChildren() const {
  std::map<int, std::set<int>> rows;
  auto selectionModel = ui.viewTable->selectionModel();
  if (selectionModel) {
    auto selectedRows = selectionModel->selectedRows();
    for (auto it = selectedRows.begin(); it != selectedRows.end(); ++it) {

      if (it->parent().isValid()) {
        int children = it->row();
        int parent = it->parent().row();
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
    for (auto it = selectedRows.begin(); it != selectedRows.end(); ++it) {
      if (!it->parent().isValid()) {
        parents.insert(it->row());
      }
    }
  }
  return parents;
}

/**
Get the name of the workspace that the user wishes to open as a table
@returns The name of the workspace to open
*/
std::string QDataProcessorWidget::getWorkspaceToOpen() const {
  return m_toOpen;
}

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
std::string QDataProcessorWidget::getClipboard() const {
  return QApplication::clipboard()->text().toStdString();
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
void QDataProcessorWidget::giveUserCritical(std::string prompt,
                                            std::string title) {

  QMessageBox::critical(this, QString::fromStdString(title),
                        QString::fromStdString(prompt), QMessageBox::Ok,
                        QMessageBox::Ok);
}

/** Shows a warning dialog
*
* @param prompt : The prompt to appear on the dialog
* @param title : The text for the title bar of the dialog
*/
void QDataProcessorWidget::giveUserWarning(std::string prompt,
                                           std::string title) {

  QMessageBox::warning(this, QString::fromStdString(title),
                       QString::fromStdString(prompt), QMessageBox::Ok,
                       QMessageBox::Ok);
}

/** Asks the user a Yes/No question
*
* @param prompt : The prompt to appear on the dialog
* @param title : The text for the title bar of the dialog
* @returns a boolean true if Yes, false if No
*/
bool QDataProcessorWidget::askUserYesNo(std::string prompt, std::string title) {

  auto response = QMessageBox::question(
      this, QString::fromStdString(title), QString::fromStdString(prompt),
      QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
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
std::string
QDataProcessorWidget::askUserString(const std::string &prompt,
                                    const std::string &title,
                                    const std::string &defaultValue) {

  bool ok;
  QString text = QInputDialog::getText(
      this, QString::fromStdString(title), QString::fromStdString(prompt),
      QLineEdit::Normal, QString::fromStdString(defaultValue), &ok);
  if (ok)
    return text.toStdString();
  return "";
}

/** Runs python code
*
* @param pythonCode :: the python code to run
* @return :: output from execution
*/
std::string
QDataProcessorWidget::runPythonAlgorithm(const std::string &pythonCode) {

  QString output = runPythonCode(QString::fromStdString(pythonCode));

  emit runPythonAlgorithm(QString::fromStdString(pythonCode));

  return output.toStdString();
}

/** Transfer runs to the table
 *
 */
void QDataProcessorWidget::transfer(const QList<QString> &runs) {

  std::vector<std::map<std::string, std::string>> runsMap(runs.size());
  size_t row = 0;

  for (QList<QString>::const_iterator it = runs.begin(); it != runs.end();
       ++it) {

    QStringList map = (*it).split(",");

    for (QList<QString>::iterator jt = map.begin(); jt != map.end(); ++jt) {

      QStringList pair = (*jt).split(":");

      if (pair.size() != 2) {
        giveUserCritical("Could not transfer runs to processing table",
                         "Transfer failed");
        return;
      }

      runsMap[row][pair[0].toStdString()] = pair[1].toStdString();
    }

    row++;
  }

  m_presenter->transfer(runsMap);
}

} // namespace MantidWidgets
} // namespace Mantid
