#include "MantidQtCustomInterfaces/Reflectometry/QReflTableView.h"
#include "MantidQtAPI/FileDialogHandler.h"
#include "MantidQtAPI/HelpWindow.h"
#include "MantidQtAPI/MantidWidget.h"
#include "MantidQtCustomInterfaces/Reflectometry/QReflTableModel.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflTableViewPresenter.h"
#include "MantidQtMantidWidgets/HintingLineEditFactory.h"

#include <QWidget>
namespace {
const QString ReflSettingsGroup = "Mantid/CustomInterfaces/ISISReflectometry";
}

namespace MantidQt {
namespace CustomInterfaces {
using namespace Mantid::API;

//----------------------------------------------------------------------------------------------
/** Constructor
*/
QReflTableView::QReflTableView(QWidget *parent)
    : MantidWidget(parent), m_openMap(new QSignalMapper(this)) {

  createTable();
}

//----------------------------------------------------------------------------------------------
/** Destructor
*/
QReflTableView::~QReflTableView() {}

/**
Initialise the Interface
*/
void QReflTableView::createTable() {
  ui.setupUi(this);

  ui.buttonProcess->setDefaultAction(ui.actionProcess);

  // Create a whats this button
  ui.rowToolBar->addAction(QWhatsThis::createAction(this));

  // Allow rows and columns to be reordered
  ui.viewTable->verticalHeader()->setMovable(true);
  ui.viewTable->horizontalHeader()->setMovable(true);

  // Re-emit a signal when the instrument changes
  connect(ui.comboProcessInstrument, SIGNAL(currentIndexChanged(int)), this,
          SIGNAL(comboProcessInstrument_currentIndexChanged(int)));
  // Custom context menu for table
  connect(ui.viewTable, SIGNAL(customContextMenuRequested(const QPoint &)),
          this, SLOT(showContextMenu(const QPoint &)));
  // Finally, create the presenters to do the thinking for us
  m_tablePresenter = boost::make_shared<ReflTableViewPresenter>(
      this /*table view*/,
      this /*currently this concrete view is also responsibile for prog reporting*/);
}

/**
This slot loads a table workspace model and changes to a LoadedMainView
presenter
@param name : the string name of the workspace to be grabbed
*/
void QReflTableView::setModel(QString name) {
  m_toOpen = name.toStdString();
  m_tablePresenter->notify(IReflTablePresenter::OpenTableFlag);
}

/**
This method loads a table workspace model. Unlike
QReflTableView::setModel(QString name),
this method is public and takes a std::string as argument. The reason is that
this method is intended to be called by the presenter
@param name : the string name of the workspace to be grabbed
*/
void QReflTableView::setModel(const std::string &name) {
  m_toOpen = name;
  m_tablePresenter->notify(IReflTablePresenter::OpenTableFlag);
}

/**
Set a new model in the tableview
@param model : the model to be attached to the tableview
*/
void QReflTableView::showTable(QReflTableModel_sptr model) {
  m_model = model;
  // So we can notify the presenter when the user updates the table
  connect(m_model.get(),
          SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this,
          SLOT(tableUpdated(const QModelIndex &, const QModelIndex &)));
  ui.viewTable->setModel(m_model.get());
  ui.viewTable->resizeColumnsToContents();
  std::string windowTitle = "ISIS Reflectometry (Polref) - " + m_toOpen;
  auto mainWindowWidget = this->topLevelWidget();
  mainWindowWidget->setWindowTitle(QString::fromStdString(windowTitle + "[*]"));
  this->setWindowModified(false);
}

/**
Set the list of tables the user is offered to open
@param tables : the names of the tables in the ADS
*/
void QReflTableView::setTableList(const std::set<std::string> &tables) {
  ui.menuOpenTable->clear();
  for (auto it = tables.begin(); it != tables.end(); ++it) {
    QAction *openTable =
        ui.menuOpenTable->addAction(QString::fromStdString(*it));
    openTable->setIcon(QIcon("://worksheet.png"));

    // Map this action to the table name
    m_openMap->setMapping(openTable, QString::fromStdString(*it));
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

/**
This slot notifies the presenter that the "save" button has been pressed
*/
void QReflTableView::on_actionSaveTable_triggered() {
  m_tablePresenter->notify(IReflTablePresenter::SaveFlag);
}

/**
This slot notifies the presenter that the "save as" button has been pressed
*/
void QReflTableView::on_actionSaveTableAs_triggered() {
  m_tablePresenter->notify(IReflTablePresenter::SaveAsFlag);
}

/**
This slot notifies the presenter that the "append row" button has been pressed
*/
void QReflTableView::on_actionAppendRow_triggered() {
  m_tablePresenter->notify(IReflTablePresenter::AppendRowFlag);
}

/**
This slot notifies the presenter that the "prepend row" button has been pressed
*/
void QReflTableView::on_actionPrependRow_triggered() {
  m_tablePresenter->notify(IReflTablePresenter::PrependRowFlag);
}

/**
This slot notifies the presenter that the "delete" button has been pressed
*/
void QReflTableView::on_actionDeleteRow_triggered() {
  m_tablePresenter->notify(IReflTablePresenter::DeleteRowFlag);
}

/**
This slot notifies the presenter that the "process" button has been pressed
*/
void QReflTableView::on_actionProcess_triggered() {
  m_tablePresenter->notify(IReflTablePresenter::ProcessFlag);
}

/**
This slot notifies the presenter that the "group rows" button has been pressed
*/
void QReflTableView::on_actionGroupRows_triggered() {
  m_tablePresenter->notify(IReflTablePresenter::GroupRowsFlag);
}

/**
This slot notifies the presenter that the "clear selected" button has been
pressed
*/
void QReflTableView::on_actionClearSelected_triggered() {
  m_tablePresenter->notify(IReflTablePresenter::ClearSelectedFlag);
}

/**
This slot notifies the presenter that the "copy selection" button has been
pressed
*/
void QReflTableView::on_actionCopySelected_triggered() {
  m_tablePresenter->notify(IReflTablePresenter::CopySelectedFlag);
}

/**
This slot notifies the presenter that the "cut selection" button has been
pressed
*/
void QReflTableView::on_actionCutSelected_triggered() {
  m_tablePresenter->notify(IReflTablePresenter::CutSelectedFlag);
}

/**
This slot notifies the presenter that the "paste selection" button has been
pressed
*/
void QReflTableView::on_actionPasteSelected_triggered() {
  m_tablePresenter->notify(IReflTablePresenter::PasteSelectedFlag);
}

/**
This slot notifies the presenter that the "new table" button has been pressed
*/
void QReflTableView::on_actionNewTable_triggered() {
  m_tablePresenter->notify(IReflTablePresenter::NewTableFlag);
}

/**
This slot notifies the presenter that the "expand selection" button has been
pressed
*/
void QReflTableView::on_actionExpandSelection_triggered() {
  m_tablePresenter->notify(IReflTablePresenter::ExpandSelectionFlag);
}

/**
This slot notifies the presenter that the "options..." button has been pressed
*/
void QReflTableView::on_actionOptionsDialog_triggered() {
  m_tablePresenter->notify(IReflTablePresenter::OptionsDialogFlag);
}

/**
This slot notifies the presenter that the "export table" button has been pressed
*/
void QReflTableView::on_actionExportTable_triggered() {
  m_tablePresenter->notify(IReflTablePresenter::ExportTableFlag);
}

/**
This slot notifies the presenter that the "import table" button has been pressed
*/
void QReflTableView::on_actionImportTable_triggered() {
  m_tablePresenter->notify(IReflTablePresenter::ImportTableFlag);
}

/**
This slot opens the documentation when the "help" button has been pressed
*/
void QReflTableView::on_actionHelp_triggered() {
  MantidQt::API::HelpWindow::showPage(
      this,
      QString(
          "qthelp://org.mantidproject/doc/interfaces/ISIS_Reflectometry.html"));
}

/**
This slot notifies the presenter that the "plot selected rows" button has been
pressed
*/
void QReflTableView::on_actionPlotRow_triggered() {
  m_tablePresenter->notify(IReflTablePresenter::PlotRowFlag);
}

/**
This slot notifies the presenter that the "plot selected groups" button has been
pressed
*/
void QReflTableView::on_actionPlotGroup_triggered() {
  m_tablePresenter->notify(IReflTablePresenter::PlotGroupFlag);
}

/** This slot is used to update the instrument*/
void QReflTableView::on_comboProcessInstrument_currentIndexChanged(int index) {
  ui.comboProcessInstrument->setCurrentIndex(index);
}

/**
This slot notifies the presenter that the table has been updated/changed by the
user
*/
void QReflTableView::tableUpdated(const QModelIndex &topLeft,
                                  const QModelIndex &bottomRight) {
  Q_UNUSED(topLeft);
  Q_UNUSED(bottomRight);
  m_tablePresenter->notify(IReflTablePresenter::TableUpdatedFlag);
}

/**
This slot is triggered when the user right clicks on the table
@param pos : The position of the right click within the table
*/
void QReflTableView::showContextMenu(const QPoint &pos) {
  // If the user didn't right-click on anything, don't show a context menu.
  if (!ui.viewTable->indexAt(pos).isValid())
    return;

  // parent widget takes ownership of QMenu
  QMenu *menu = new QMenu(this);
  menu->addAction(ui.actionProcess);
  menu->addAction(ui.actionExpandSelection);
  menu->addSeparator();
  menu->addAction(ui.actionPlotRow);
  menu->addAction(ui.actionPlotGroup);
  menu->addSeparator();
  menu->addAction(ui.actionPrependRow);
  menu->addAction(ui.actionAppendRow);
  menu->addSeparator();
  menu->addAction(ui.actionGroupRows);
  menu->addAction(ui.actionCopySelected);
  menu->addAction(ui.actionCutSelected);
  menu->addAction(ui.actionPasteSelected);
  menu->addAction(ui.actionClearSelected);
  menu->addSeparator();
  menu->addAction(ui.actionDeleteRow);

  menu->popup(ui.viewTable->viewport()->mapToGlobal(pos));
}

/**
Show an critical error dialog
@param prompt : The prompt to appear on the dialog
@param title : The text for the title bar of the dialog
*/
void QReflTableView::giveUserCritical(std::string prompt, std::string title) {
  QMessageBox::critical(this, QString(title.c_str()), QString(prompt.c_str()),
                        QMessageBox::Ok, QMessageBox::Ok);
}

/**
Show a warning dialog
@param prompt : The prompt to appear on the dialog
@param title : The text for the title bar of the dialog
*/
void QReflTableView::giveUserWarning(std::string prompt, std::string title) {
  QMessageBox::warning(this, QString(title.c_str()), QString(prompt.c_str()),
                       QMessageBox::Ok, QMessageBox::Ok);
}

/**
Ask the user a Yes/No question
@param prompt : The prompt to appear on the dialog
@param title : The text for the title bar of the dialog
@returns a boolean true if Yes, false if No
*/
bool QReflTableView::askUserYesNo(std::string prompt, std::string title) {
  auto response = QMessageBox::question(
      this, QString(title.c_str()), QString(prompt.c_str()),
      QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
  if (response == QMessageBox::Yes) {
    return true;
  }
  return false;
}

/**
Ask the user to enter a string.
@param prompt : The prompt to appear on the dialog
@param title : The text for the title bar of the dialog
@param defaultValue : The default value entered.
@returns The user's string if submitted, or an empty string
*/
std::string QReflTableView::askUserString(const std::string &prompt,
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

/**
Show the user the dialog for an algorithm
* @param algorithm : [input] The algorithm
*/
void QReflTableView::showAlgorithmDialog(const std::string &algorithm) {
  std::stringstream pythonSrc;
  pythonSrc << "try:\n";
  pythonSrc << "  algm = " << algorithm << "Dialog()\n";
  pythonSrc << "except:\n";
  pythonSrc << "  pass\n";
  runPythonCode(QString::fromStdString(pythonSrc.str()), false);
}

/**
Show the user the dialog for "LoadReflTBL"
*/
void QReflTableView::showImportDialog() {
  std::stringstream pythonSrc;
  pythonSrc << "try:\n";
  pythonSrc << "  algm = "
            << "LoadReflTBL"
            << "Dialog()\n";
  pythonSrc << "  print algm.getPropertyValue(\"OutputWorkspace\")\n";
  pythonSrc << "except:\n";
  pythonSrc << "  pass\n";
  // outputWorkspaceName will hold the name of the workspace
  // otherwise this should be an empty string.
  QString outputWorkspaceName =
      runPythonCode(QString::fromStdString(pythonSrc.str()), false);
  m_toOpen = outputWorkspaceName.trimmed().toStdString();
  // notifying the presenter that a new table should be opened
  // The presenter will ask about any unsaved changes etc
  // before opening the new table
  m_tablePresenter->notify(IReflTablePresenter::OpenTableFlag);
}

/**
Show the user file dialog to choose save location of notebook
*/
std::string QReflTableView::requestNotebookPath() {

  // We won't use QFileDialog directly here as using the NativeDialog option
  // causes problems on MacOS.
  QString qfilename = API::FileDialogHandler::getSaveFileName(
      this, "Save notebook file", QDir::currentPath(),
      "IPython Notebook files (*.ipynb);;All files (*.*)",
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
Save settings
@param options : map of user options to save
*/
void QReflTableView::saveSettings(
    const std::map<std::string, QVariant> &options) {
  QSettings settings;
  settings.beginGroup(ReflSettingsGroup);
  for (auto it = options.begin(); it != options.end(); ++it)
    settings.setValue(QString::fromStdString(it->first), it->second);
  settings.endGroup();
}

/**
Load settings
@param options : map of user options to load into
*/
void QReflTableView::loadSettings(std::map<std::string, QVariant> &options) {
  QSettings settings;
  settings.beginGroup(ReflSettingsGroup);
  QStringList keys = settings.childKeys();
  for (auto it = keys.begin(); it != keys.end(); ++it)
    options[it->toStdString()] = settings.value(*it);
  settings.endGroup();
}

/**
Plot a set of workspaces
* @param workspaces : [input] The list of workspaces as a set
*/
void QReflTableView::plotWorkspaces(const std::set<std::string> &workspaces) {
  if (workspaces.empty())
    return;

  std::stringstream pythonSrc;
  pythonSrc << "base_graph = None\n";
  for (auto ws = workspaces.begin(); ws != workspaces.end(); ++ws)
    pythonSrc << "base_graph = plotSpectrum(\"" << *ws
              << "\", 0, True, window = base_graph)\n";

  pythonSrc << "base_graph.activeLayer().logLogAxes()\n";

  runPythonCode(QString::fromStdString(pythonSrc.str()));
}

/**
Set the range of the progress bar
@param min : The minimum value of the bar
@param max : The maxmimum value of the bar
*/
void QReflTableView::setProgressRange(int min, int max) {
  ui.progressBar->setRange(min, max);
}

/**
Set the status of the progress bar
@param progress : The current value of the bar
*/
void QReflTableView::setProgress(int progress) {
  ui.progressBar->setValue(progress);
}

/**
Get status of checkbox which determines whether an ipython notebook is produced
@return true if a notebook should be output on process, false otherwise
*/
bool QReflTableView::getEnableNotebook() {
  return ui.checkEnableNotebook->isChecked();
}

/**
Set which rows are selected
@param rows : The set of rows to select
*/
void QReflTableView::setSelection(const std::set<int> &rows) {
  ui.viewTable->clearSelection();
  auto selectionModel = ui.viewTable->selectionModel();

  for (auto row = rows.begin(); row != rows.end(); ++row)
    selectionModel->select(ui.viewTable->model()->index((*row), 0),
                           QItemSelectionModel::Select |
                               QItemSelectionModel::Rows);
}

/**
Set the list of available instruments to process
@param instruments : The list of instruments available
@param defaultInstrument : The instrument to have selected by default
*/
void QReflTableView::setInstrumentList(
    const std::vector<std::string> &instruments,
    const std::string &defaultInstrument) {
  ui.comboProcessInstrument->clear();

  for (auto it = instruments.begin(); it != instruments.end(); ++it) {
    QString instrument = QString::fromStdString(*it);
    ui.comboProcessInstrument->addItem(instrument);
  }

  int index = ui.comboProcessInstrument->findData(
      QString::fromStdString(defaultInstrument), Qt::DisplayRole);
  ui.comboProcessInstrument->setCurrentIndex(index);
}

/**
Set the strategy used for generating hints for the autocompletion in the options
column.
@param hintStrategy The hinting strategy to use
*/
void QReflTableView::setOptionsHintStrategy(HintStrategy *hintStrategy) {
  ui.viewTable->setItemDelegateForColumn(
      ReflTableSchema::COL_OPTIONS, new HintingLineEditFactory(hintStrategy));
}

/**
Sets the contents of the system's clipboard
@param text The contents of the clipboard
*/
void QReflTableView::setClipboard(const std::string &text) {
  QApplication::clipboard()->setText(QString::fromStdString(text));
}

/**
Get the selected instrument for processing
@returns the selected instrument to process with
*/
std::string QReflTableView::getProcessInstrument() const {
  return ui.comboProcessInstrument->currentText().toStdString();
}

/**
Get the indices of the highlighted rows
@returns a set of ints containing the highlighted row numbers
*/
std::set<int> QReflTableView::getSelectedRows() const {
  std::set<int> rows;
  auto selectionModel = ui.viewTable->selectionModel();
  if (selectionModel) {
    auto selectedRows = selectionModel->selectedRows();
    for (auto it = selectedRows.begin(); it != selectedRows.end(); ++it)
      rows.insert(it->row());
  }
  return rows;
}

/**
Get the name of the workspace that the user wishes to open as a table
@returns The name of the workspace to open
*/
std::string QReflTableView::getWorkspaceToOpen() const { return m_toOpen; }

/**
Get a pointer to the presenter that's currently controlling this view.
@returns A pointer to the presenter
*/
boost::shared_ptr<IReflTablePresenter>
QReflTableView::getTablePresenter() const {
  return m_tablePresenter;
}

/**
Gets the contents of the system's clipboard
@returns The contents of the clipboard
*/
std::string QReflTableView::getClipboard() const {
  return QApplication::clipboard()->text().toStdString();
}

/**
* Clear the progress
*/
void QReflTableView::clearProgress() { ui.progressBar->reset(); }

/**
Loads some runs into the table
* @param runs : [input] The set of runs to be transferred to the table
*/
void QReflTableView::transferRuns(
    const std::vector<std::map<std::string, std::string>> &runs) {
  m_tablePresenter->transfer(runs);
}

} // namespace CustomInterfaces
} // namespace Mantid
