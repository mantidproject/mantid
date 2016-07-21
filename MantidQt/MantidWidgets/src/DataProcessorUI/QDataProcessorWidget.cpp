#include "MantidQtMantidWidgets/DataProcessorUI/QDataProcessorWidget.h"
#include "MantidQtAPI/FileDialogHandler.h"
#include "MantidQtAPI/HelpWindow.h"
#include "MantidQtAPI/MantidWidget.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPresenter.h"
#include "MantidQtMantidWidgets/DataProcessorUI/QDataProcessorTreeModel.h"
#include "MantidQtMantidWidgets/HintingLineEditFactory.h"

#include <QWidget>
namespace {
const QString DataProcessorSettingsGroup =
    "Mantid/MantidWidgets/ISISDataProcessorUI";
}

namespace MantidQt {
namespace MantidWidgets {
using namespace Mantid::API;

//----------------------------------------------------------------------------------------------
/** Constructor
*/
QDataProcessorWidget::QDataProcessorWidget(
    boost::shared_ptr<DataProcessorPresenter> presenter, QWidget *parent)
    : MantidWidget(parent), m_presenter(presenter),
      m_openMap(new QSignalMapper(this)) {

  createTable();

  m_presenter->acceptViews(this, this);
}

//----------------------------------------------------------------------------------------------
/** Destructor
*/
QDataProcessorWidget::~QDataProcessorWidget() {}

/**
Initialise the Interface
*/
void QDataProcessorWidget::createTable() {
  ui.setupUi(this);

  ui.buttonProcess->setDefaultAction(ui.actionProcess);

  // Create a whats this button
  ui.rowToolBar->addAction(QWhatsThis::createAction(this));

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
  m_toOpen = name;
}

/**
Set a new model in the tableview
@param model : the model to be attached to the tableview
*/
void QDataProcessorWidget::showTable(QDataProcessorTreeModel_sptr model) {
  m_model = model;
  // So we can notify the presenter when the user updates the table
  connect(m_model.get(),
          SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this,
          SLOT(tableUpdated(const QModelIndex &, const QModelIndex &)));
  ui.viewTable->setModel(m_model.get());
}

/**
Set the list of tables the user is offered to open
@param tables : the names of the tables in the ADS
*/
void QDataProcessorWidget::setTableList(const std::set<std::string> &tables) {
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
This slot notifies the presenter that the "append row" button has been pressed
*/
void QDataProcessorWidget::on_actionAppendRow_triggered() {
  m_presenter->notify(DataProcessorPresenter::AppendRowFlag);
}

/**
This slot notifies the presenter that the "append group" button has been pressed
*/
void QDataProcessorWidget::on_actionAppendGroup_triggered() {
  m_presenter->notify(DataProcessorPresenter::AppendGroupFlag);
}

/**
This slot notifies the presenter that the "delete row" button has been pressed
*/
void QDataProcessorWidget::on_actionDeleteRow_triggered() {
  m_presenter->notify(DataProcessorPresenter::DeleteRowFlag);
}

/**
This slot notifies the presenter that the "delete group" button has been pressed
*/
void QDataProcessorWidget::on_actionDeleteGroup_triggered() {
  m_presenter->notify(DataProcessorPresenter::DeleteGroupFlag);
}

/**
This slot notifies the presenter that the "process" button has been pressed
*/
void QDataProcessorWidget::on_actionProcess_triggered() {
  m_presenter->notify(DataProcessorPresenter::ProcessFlag);
}

/**
This slot notifies the presenter that the "group rows" button has been pressed
*/
void QDataProcessorWidget::on_actionGroupRows_triggered() {
  m_presenter->notify(DataProcessorPresenter::GroupRowsFlag);
}

/**
This slot notifies the presenter that the "clear selected" button has been
pressed
*/
void QDataProcessorWidget::on_actionClearSelected_triggered() {
  m_presenter->notify(DataProcessorPresenter::ClearSelectedFlag);
}

/**
This slot notifies the presenter that the "copy selection" button has been
pressed
*/
void QDataProcessorWidget::on_actionCopySelected_triggered() {
  m_presenter->notify(DataProcessorPresenter::CopySelectedFlag);
}

/**
This slot notifies the presenter that the "cut selection" button has been
pressed
*/
void QDataProcessorWidget::on_actionCutSelected_triggered() {
  m_presenter->notify(DataProcessorPresenter::CutSelectedFlag);
}

/**
This slot notifies the presenter that the "paste selection" button has been
pressed
*/
void QDataProcessorWidget::on_actionPasteSelected_triggered() {
  m_presenter->notify(DataProcessorPresenter::PasteSelectedFlag);
}

/**
This slot notifies the presenter that the "expand selection" button has been
pressed
*/
void QDataProcessorWidget::on_actionExpandSelection_triggered() {
  m_presenter->notify(DataProcessorPresenter::ExpandSelectionFlag);
}

/**
This slot opens the documentation when the "help" button has been pressed
*/
void QDataProcessorWidget::on_actionHelp_triggered() {
  MantidQt::API::HelpWindow::showPage(
      this,
      QString(
          "qthelp://org.mantidproject/doc/interfaces/ISIS_Reflectometry.html"));
}

/**
This slot notifies the presenter that the "plot selected rows" button has been
pressed
*/
void QDataProcessorWidget::on_actionPlotRow_triggered() {
  m_presenter->notify(DataProcessorPresenter::PlotRowFlag);
}

/**
This slot notifies the presenter that the "plot selected groups" button has been
pressed
*/
void QDataProcessorWidget::on_actionPlotGroup_triggered() {
  m_presenter->notify(DataProcessorPresenter::PlotGroupFlag);
}

/** This slot is used to update the instrument*/
void QDataProcessorWidget::on_comboProcessInstrument_currentIndexChanged(
    int index) {
  ui.comboProcessInstrument->setCurrentIndex(index);
}

/**
This slot notifies the presenter that the table has been updated/changed by the
user
*/
void QDataProcessorWidget::tableUpdated(const QModelIndex &topLeft,
                                        const QModelIndex &bottomRight) {
  Q_UNUSED(topLeft);
  Q_UNUSED(bottomRight);
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

  // parent widget takes ownership of QMenu
  QMenu *menu = new QMenu(this);
  menu->addAction(ui.actionProcess);
  menu->addAction(ui.actionExpandSelection);
  menu->addSeparator();
  menu->addAction(ui.actionPlotRow);
  menu->addAction(ui.actionPlotGroup);
  menu->addSeparator();
  menu->addAction(ui.actionAppendRow);
  menu->addAction(ui.actionAppendGroup);
  menu->addSeparator();
  menu->addAction(ui.actionGroupRows);
  menu->addAction(ui.actionCopySelected);
  menu->addAction(ui.actionCutSelected);
  menu->addAction(ui.actionPasteSelected);
  menu->addAction(ui.actionClearSelected);
  menu->addSeparator();
  menu->addAction(ui.actionDeleteRow);
  menu->addAction(ui.actionDeleteGroup);

  menu->popup(ui.viewTable->viewport()->mapToGlobal(pos));
}

/**
Show an critical error dialog
@param prompt : The prompt to appear on the dialog
@param title : The text for the title bar of the dialog
*/
void QDataProcessorWidget::giveUserCritical(std::string prompt,
                                            std::string title) {
  QMessageBox::critical(this, QString(title.c_str()), QString(prompt.c_str()),
                        QMessageBox::Ok, QMessageBox::Ok);
}

/**
Show a warning dialog
@param prompt : The prompt to appear on the dialog
@param title : The text for the title bar of the dialog
*/
void QDataProcessorWidget::giveUserWarning(std::string prompt,
                                           std::string title) {
  QMessageBox::warning(this, QString(title.c_str()), QString(prompt.c_str()),
                       QMessageBox::Ok, QMessageBox::Ok);
}

/**
Ask the user a Yes/No question
@param prompt : The prompt to appear on the dialog
@param title : The text for the title bar of the dialog
@returns a boolean true if Yes, false if No
*/
bool QDataProcessorWidget::askUserYesNo(std::string prompt, std::string title) {
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

/**
Show the user the dialog for an algorithm
* @param algorithm : [input] The algorithm
*/
void QDataProcessorWidget::showAlgorithmDialog(const std::string &algorithm) {
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
void QDataProcessorWidget::showImportDialog() {
  std::stringstream pythonSrc;
  pythonSrc << "try:\n";
  pythonSrc << "  algm = "
            << "LoadTBL"
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
  m_presenter->notify(DataProcessorPresenter::OpenTableFlag);
}

/**
Show the user file dialog to choose save location of notebook
*/
std::string QDataProcessorWidget::requestNotebookPath() {

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
Plot a set of workspaces
* @param workspaces : [input] The list of workspaces as a set
*/
void QDataProcessorWidget::plotWorkspaces(
    const std::set<std::string> &workspaces) {
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
void QDataProcessorWidget::setInstrumentList(
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
@param hintStrategy : The hinting strategy to use
@param column : The index of the 'Options' column
*/
void QDataProcessorWidget::setOptionsHintStrategy(HintStrategy *hintStrategy,
                                                  int column) {
  ui.viewTable->setItemDelegateForColumn(
      column, new HintingLineEditFactory(hintStrategy));
}

/**
* Adds the specified HintingLineEdit widgets to this view. A hinting line edit
* comes with a label and an algorithm's name. Headings are also shown.
* @param stages : The stages, pre-process, process or post-process, as a vector
* @param algNames : The algorithm names as a vector
* @param hints : The hints for each algorithm as a vector
*/
void QDataProcessorWidget::setGlobalOptions(
    const std::vector<std::string> &stages,
    const std::vector<std::string> &algNames,
    const std::vector<std::map<std::string, std::string>> &hints) {
  // Headers
  QLabel *stageHeader = new QLabel(QString::fromStdString("<b>Stage</b>"));
  QLabel *algorithmHeader =
      new QLabel(QString::fromStdString("<b>Algorithm</b>"));
  QLabel *optionsHeader =
      new QLabel(QString::fromStdString("<b>Global Options</b>"));
  stageHeader->setMinimumHeight(30);
  ui.processLayout->addWidget(stageHeader, 0, 0);
  ui.processLayout->addWidget(algorithmHeader, 0, 1);
  ui.processLayout->addWidget(optionsHeader, 0, 2);

  int rows = static_cast<int>(stages.size());

  for (int row = 0; row < rows; row++) {

    // The title
    QLabel *stageLabel =
        new QLabel(QString::fromStdString(stages.at(row)), this);
    stageLabel->setMinimumSize(100, 10);
    ui.processLayout->addWidget(stageLabel, row + 1, 0);
    // The name
    QLabel *nameLabel =
        new QLabel(QString::fromStdString(algNames.at(row)), this);
    ui.processLayout->addWidget(new HintingLineEdit(this, hints.at(row)),
                                row + 1, 2);
    // The content
    ui.processLayout->addWidget(nameLabel, row + 1, 1);
  }
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
Get the indices of the highlighted runs (rows)
@returns :: a map where keys are group indices and values are sets containing
the highlighted row numbers
*/
std::map<int, std::set<int>> QDataProcessorWidget::getSelectedRows() const {
  std::map<int, std::set<int>> rows;
  auto selectionModel = ui.viewTable->selectionModel();
  if (selectionModel) {
    auto selectedRows = selectionModel->selectedRows();
    for (auto it = selectedRows.begin(); it != selectedRows.end(); ++it) {

      if (it->parent().isValid()) {
        // This item is a run (row)
        // Add run and corresponding group
        int run = it->row();
        int group = it->parent().row();
        rows[group].insert(run);
      }
      // else :
      // A group was selected, selected groups can be retrieved using
      // getSelectedGroups()
    }
  }
  return rows;
}

/**
Get the indices of the highlighted groups
@returns :: a sets containing
the highlighted row numbers
*/
std::set<int> QDataProcessorWidget::getSelectedGroups() const {
  std::set<int> groups;
  auto selectionModel = ui.viewTable->selectionModel();
  if (selectionModel) {
    auto selectedRows = selectionModel->selectedRows();
    for (auto it = selectedRows.begin(); it != selectedRows.end(); ++it) {
      if (!it->parent().isValid()) {
        // This group was selected
        groups.insert(it->row());
      }
    }
  }
  return groups;
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
boost::shared_ptr<DataProcessorPresenter>
QDataProcessorWidget::getTablePresenter() const {
  return m_presenter;
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

/**
* Returns the processing instructions for the specified algorithm
* @param name : The name of the algorithm
* @return : The processing instructions specified by the user
*/
std::string
QDataProcessorWidget::getProcessingOptions(const std::string &name) const {

  const int nrows = ui.processLayout->rowCount();
  for (int r = 0; r < nrows; r++) {

    auto widget = ui.processLayout->itemAtPosition(r + 1, 1)->widget();
    auto text = static_cast<QLabel *>(widget)->text().toStdString();
    if (text == name) {
      // This is the algorithm for which we need the processing instructions
      // (options)
      auto widget = ui.processLayout->itemAtPosition(r + 1, 2)->widget();
      auto text = static_cast<HintingLineEdit *>(widget)->text().toStdString();
      return text;
    }
  }
  return "";
}

} // namespace MantidWidgets
} // namespace Mantid
