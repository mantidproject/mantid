#include "MantidQtCustomInterfaces/Reflectometry/QtReflMainView.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtAPI/FileDialogHandler.h"
#include "MantidQtAPI/HelpWindow.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflTablePresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/QReflTableModel.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflCommandAdapter.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflMainViewPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflTableSchema.h"
#include "MantidQtMantidWidgets/HintingLineEditFactory.h"
#include <QSignalMapper>
#include <qinputdialog.h>
#include <qmessagebox.h>

namespace {
const QString ReflSettingsGroup = "Mantid/CustomInterfaces/ISISReflectometry";
}

namespace MantidQt {
namespace CustomInterfaces {
using namespace Mantid::API;

DECLARE_SUBWINDOW(QtReflMainView)

//----------------------------------------------------------------------------------------------
/** Constructor
*/
QtReflMainView::QtReflMainView(QWidget *parent)
    : UserSubWindow(parent),
      m_calculator(new MantidWidgets::SlitCalculator(this)),
      m_openMap(new QSignalMapper(this)) {}

//----------------------------------------------------------------------------------------------
/** Destructor
*/
QtReflMainView::~QtReflMainView() {}

/**
Initialise the Interface
*/
void QtReflMainView::initLayout() {
  ui.setupUi(this);

  ui.buttonTransfer->setDefaultAction(ui.actionTransfer);

  // Expand the process runs column at the expense of the search column
  ui.splitterTables->setStretchFactor(0, 0);
  ui.splitterTables->setStretchFactor(1, 1);

  // Custom context menu for table
  connect(ui.tableSearchResults,
          SIGNAL(customContextMenuRequested(const QPoint &)), this,
          SLOT(showSearchContextMenu(const QPoint &)));
  // Synchronize the two instrument selection widgets
  connect(ui.comboSearchInstrument, SIGNAL(currentIndexChanged(int)),
          ui.qReflTableView,
          SLOT(on_comboProcessInstrument_currentIndexChanged(int)));
  connect(ui.qReflTableView,
          SIGNAL(comboProcessInstrument_currentIndexChanged(int)),
          ui.comboSearchInstrument, SLOT(setCurrentIndex(int)));

  // Needed to Import/Export TBL, plot row and plot group
  connect(ui.qReflTableView, SIGNAL(runAsPythonScript(const QString &, bool)),
          this, SIGNAL(runAsPythonScript(const QString &, bool)));

  m_presenter = boost::make_shared<ReflMainViewPresenter>(
      this /*main view*/, ui.qReflTableView->getTablePresenter().get(),
      this /*currently this concrete view is also responsibile for prog reporting*/);
  m_algoRunner = boost::make_shared<MantidQt::API::AlgorithmRunner>(this);

  // Adds actions (New Table, Save Table, ...) to the "Reflectometry" menu
  createReflectometryActions();
  // Adds actions (Process, Expand Selection, ...) to the "Edit" menu
  createEditActions();
}

/**
* Add a command (action) to a menu
* @param menu : [input] The menu where actions will be added
* @param command : [input] The command (action) to add
*/
void QtReflMainView::addToMenu(QMenu *menu, ReflCommandBase_uptr command) {

  QAction *action = new QAction(QString::fromStdString(command->name()), this);
  action->setIcon(QIcon(QString::fromStdString(command->icon())));
  menu->addAction(action);
  ReflCommandAdapter *adapter = new ReflCommandAdapter(std::move(command));
  connect(action, SIGNAL(triggered()), adapter, SLOT(call()));
}

/**
* Adds actions to the "Reflectometry" menu
*/
void QtReflMainView::createReflectometryActions() {

  auto commands = ui.qReflTableView->getTablePresenter()->publishCommands();

  if (commands.size() != 19)
    throw std::runtime_error("Invalid number of commands");

  // New Table, Save Table, Save Table As
  addToMenu(ui.menuTable, std::move(commands.at(1)));
  addToMenu(ui.menuTable, std::move(commands.at(2)));
  addToMenu(ui.menuTable, std::move(commands.at(3)));
  ui.menuTable->addSeparator();
  // Import .TBL, Export .TBL
  addToMenu(ui.menuTable, std::move(commands.at(4)));
  addToMenu(ui.menuTable, std::move(commands.at(5)));
  ui.menuTable->addSeparator();
  // Options
  addToMenu(ui.menuTable, std::move(commands.at(6)));
}

/**
* Adds actions to the "Edit" menu
*/
void QtReflMainView::createEditActions() {

  // Options in the "Edit" menu
  auto commands = ui.qReflTableView->getTablePresenter()->publishCommands();

  if (commands.size() != 19)
    throw std::runtime_error("Invalid number of commands");

  // Process, Expand Selection
  addToMenu(ui.menuRows, std::move(commands.at(7)));
  addToMenu(ui.menuRows, std::move(commands.at(8)));
  ui.menuRows->addSeparator();
  // Plot Selected Rows, Plot Selected Groups
  addToMenu(ui.menuRows, std::move(commands.at(9)));
  addToMenu(ui.menuRows, std::move(commands.at(10)));
  ui.menuRows->addSeparator();
  // Insert Row Before, Insert Row After
  addToMenu(ui.menuRows, std::move(commands.at(11)));
  addToMenu(ui.menuRows, std::move(commands.at(12)));
  ui.menuRows->addSeparator();
  // Group Selected, Copy Selected, Cut Selected
  // Paste Selected, Clear Selected
  addToMenu(ui.menuRows, std::move(commands.at(13)));
  addToMenu(ui.menuRows, std::move(commands.at(14)));
  addToMenu(ui.menuRows, std::move(commands.at(15)));
  addToMenu(ui.menuRows, std::move(commands.at(16)));
  addToMenu(ui.menuRows, std::move(commands.at(17)));
  ui.menuRows->addSeparator();
  // Delete Row
  addToMenu(ui.menuRows, std::move(commands.at(18)));
}
/**
* Set all possible tranfer methods
* @param methods : All possible transfer methods.
*/
void QtReflMainView::setTransferMethods(const std::set<std::string> &methods) {
  for (auto method = methods.begin(); method != methods.end(); ++method) {
    ui.comboTransferMethod->addItem((*method).c_str());
  }
}

/**
Set the list of available instruments to search for and updates the list of
available instruments in the table view
@param instruments : The list of instruments available
@param defaultInstrument : The instrument to have selected by default
*/
void QtReflMainView::setInstrumentList(
    const std::vector<std::string> &instruments,
    const std::string &defaultInstrument) {
  ui.comboSearchInstrument->clear();

  for (auto it = instruments.begin(); it != instruments.end(); ++it) {
    QString instrument = QString::fromStdString(*it);
    ui.comboSearchInstrument->addItem(instrument);
  }

  int index = ui.comboSearchInstrument->findData(
      QString::fromStdString(defaultInstrument), Qt::DisplayRole);
  ui.comboSearchInstrument->setCurrentIndex(index);
}

/**
Set the range of the progress bar
@param min : The minimum value of the bar
@param max : The maxmimum value of the bar
*/
void QtReflMainView::setProgressRange(int min, int max) {
  ui.progressBar->setRange(min, max);
}

/**
Set the status of the progress bar
@param progress : The current value of the bar
*/
void QtReflMainView::setProgress(int progress) {
  ui.progressBar->setValue(progress);
}

/**
* Clear the progress
*/
void QtReflMainView::clearProgress() { ui.progressBar->reset(); }

/**
Set a new model for search results
@param model : the model to be attached to the search results
*/
void QtReflMainView::showSearch(ReflSearchModel_sptr model) {
  m_searchModel = model;
  ui.tableSearchResults->setModel(m_searchModel.get());
  ui.tableSearchResults->resizeColumnsToContents();
}

/**
This slot notifies the presenter that the ICAT search was completed
*/
void QtReflMainView::icatSearchComplete() {
  m_presenter->notify(IReflPresenter::ICATSearchCompleteFlag);
}

/**
This slot notifies the presenter that the "search" button has been pressed
*/
void QtReflMainView::on_actionSearch_triggered() {
  m_presenter->notify(IReflPresenter::SearchFlag);
  connect(m_algoRunner.get(), SIGNAL(algorithmComplete(bool)), this,
          SLOT(icatSearchComplete()));
}

/**
This slot notifies the presenter that the "transfer" button has been pressed
*/
void QtReflMainView::on_actionTransfer_triggered() {
  m_presenter->notify(IReflPresenter::Flag::TransferFlag);
}

/**
This slot opens the documentation when the "help" button has been pressed
*/
void QtReflMainView::on_actionHelp_triggered() {
  MantidQt::API::HelpWindow::showPage(
      this,
      QString(
          "qthelp://org.mantidproject/doc/interfaces/ISIS_Reflectometry.html"));
}

/**
This slot shows the slit calculator
*/
void QtReflMainView::on_actionSlitCalculator_triggered() {
  m_calculator->setCurrentInstrumentName(
      ui.comboSearchInstrument->currentText().toStdString());
  m_calculator->show();
}

/**
This slot is triggered when the user right clicks on the search results table
@param pos : The position of the right click within the table
*/
void QtReflMainView::showSearchContextMenu(const QPoint &pos) {
  if (!ui.tableSearchResults->indexAt(pos).isValid())
    return;

  // parent widget takes ownership of QMenu
  QMenu *menu = new QMenu(this);
  menu->addAction(ui.actionTransfer);
  menu->popup(ui.tableSearchResults->viewport()->mapToGlobal(pos));
}

/**
Show an information dialog
@param prompt : The prompt to appear on the dialog
@param title : The text for the title bar of the dialog
*/
void QtReflMainView::giveUserInfo(std::string prompt, std::string title) {
  QMessageBox::information(this, QString(title.c_str()),
                           QString(prompt.c_str()), QMessageBox::Ok,
                           QMessageBox::Ok);
}

/**
Show an critical error dialog
@param prompt : The prompt to appear on the dialog
@param title : The text for the title bar of the dialog
*/
void QtReflMainView::giveUserCritical(std::string prompt, std::string title) {
  QMessageBox::critical(this, QString(title.c_str()), QString(prompt.c_str()),
                        QMessageBox::Ok, QMessageBox::Ok);
}

/**
Ask the user to enter a string.
@param prompt : The prompt to appear on the dialog
@param title : The text for the title bar of the dialog
@param defaultValue : The default value entered.
@returns The user's string if submitted, or an empty string
*/
std::string QtReflMainView::askUserString(const std::string &prompt,
                                          const std::string &title,
                                          const std::string &defaultValue) {
  bool ok;
  QString text = QInputDialog::getText(
      QString::fromStdString(title), QString::fromStdString(prompt),
      QLineEdit::Normal, QString::fromStdString(defaultValue), &ok);
  if (ok)
    return text.toStdString();
  return "";
}

/**
Show the user the dialog for an algorithm
*/
void QtReflMainView::showAlgorithmDialog(const std::string &algorithm) {
  std::stringstream pythonSrc;
  pythonSrc << "try:\n";
  pythonSrc << "  algm = " << algorithm << "Dialog()\n";
  pythonSrc << "except:\n";
  pythonSrc << "  pass\n";
  runPythonCode(QString::fromStdString(pythonSrc.str()), false);
}

/**
Get the selected instrument for searching
@returns the selected instrument to search for
*/
std::string QtReflMainView::getSearchInstrument() const {
  return ui.comboSearchInstrument->currentText().toStdString();
}

/**
Get the indices of the highlighted search result rows
@returns a set of ints containing the selected row numbers
*/
std::set<int> QtReflMainView::getSelectedSearchRows() const {
  std::set<int> rows;
  auto selectionModel = ui.tableSearchResults->selectionModel();
  if (selectionModel) {
    auto selectedRows = selectionModel->selectedRows();
    for (auto it = selectedRows.begin(); it != selectedRows.end(); ++it)
      rows.insert(it->row());
  }
  return rows;
}

/**
Get a pointer to the presenter that's currently controlling this view.
@returns A pointer to the presenter
*/
boost::shared_ptr<IReflPresenter> QtReflMainView::getPresenter() const {
  return m_presenter;
}

boost::shared_ptr<MantidQt::API::AlgorithmRunner>
QtReflMainView::getAlgorithmRunner() const {
  return m_algoRunner;
}

/**
Get the string the user wants to search for.
@returns The search string
*/
std::string QtReflMainView::getSearchString() const {
  return ui.textSearch->text().toStdString();
}

/**
* @return the transfer method selected.
*/
std::string QtReflMainView::getTransferMethod() const {
  return ui.comboTransferMethod->currentText().toStdString();
}

/**
Set the list of tables the user is offered to open
@param tables : the names of the tables in the ADS
*/
void QtReflMainView::setTableList(const std::set<std::string> &tables) {
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
This slot loads a table workspace model and changes to a LoadedMainView
presenter
@param name : the string name of the workspace to be grabbed
*/
void QtReflMainView::setModel(QString name) {
	m_toOpen = name;
  m_presenter->notify(IReflPresenter::OpenTableFlag);
}

/**
Get the name of the workspace that the user wishes to open as a table
@returns The name of the workspace to open
*/
std::string QtReflMainView::getWorkspaceToOpen() const { return m_toOpen; }

} // namespace CustomInterfaces
} // namespace Mantid
