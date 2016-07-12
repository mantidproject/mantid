#include "MantidQtCustomInterfaces/Reflectometry/QtReflMainView.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtAPI/FileDialogHandler.h"
#include "MantidQtAPI/HelpWindow.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflGenericDataProcessorPresenterFactory.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflMainViewPresenter.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorCommandAdapter.h"
#include "MantidQtMantidWidgets/DataProcessorUI/QDataProcessorWidget.h"
#include "MantidQtMantidWidgets/HintingLineEditFactory.h"
#include "MantidQtMantidWidgets/SlitCalculator.h"
#include <qinputdialog.h>
#include <qmessagebox.h>

namespace {
const QString ReflSettingsGroup = "Mantid/CustomInterfaces/ISISReflectometry";
}

namespace MantidQt {
namespace CustomInterfaces {
using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

DECLARE_SUBWINDOW(QtReflMainView)

//----------------------------------------------------------------------------------------------
/** Constructor
*/
QtReflMainView::QtReflMainView(QWidget *parent)
    : UserSubWindow(parent), m_calculator(new SlitCalculator(this)) {}

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

  ReflGenericDataProcessorPresenterFactory presenterFactory;

  boost::shared_ptr<DataProcessorPresenter> processorPresenter =
      presenterFactory.create();

  QDataProcessorWidget *qDataProcessorWidget =
      new QDataProcessorWidget(processorPresenter, this);
  ui.layoutProcessPane->addWidget(qDataProcessorWidget);

  // Custom context menu for table
  connect(ui.tableSearchResults,
          SIGNAL(customContextMenuRequested(const QPoint &)), this,
          SLOT(showSearchContextMenu(const QPoint &)));
  // Synchronize the two instrument selection widgets
  connect(ui.comboSearchInstrument, SIGNAL(currentIndexChanged(int)),
          qDataProcessorWidget,
          SLOT(on_comboProcessInstrument_currentIndexChanged(int)));
  connect(qDataProcessorWidget,
          SIGNAL(comboProcessInstrument_currentIndexChanged(int)),
          ui.comboSearchInstrument, SLOT(setCurrentIndex(int)));
  // Synchronize the slit calculator
  connect(ui.comboSearchInstrument, SIGNAL(currentIndexChanged(int)), this,
          SLOT(instrumentChanged(int)));
  connect(qDataProcessorWidget,
          SIGNAL(comboProcessInstrument_currentIndexChanged(int)), this,
          SLOT(instrumentChanged(int)));
  // Needed to Import/Export TBL, plot row and plot group
  connect(qDataProcessorWidget,
          SIGNAL(runAsPythonScript(const QString &, bool)), this,
          SIGNAL(runAsPythonScript(const QString &, bool)));

  m_presenter = boost::make_shared<ReflMainViewPresenter>(
      this /*main view*/,
      this /*currently this concrete view is also responsibile for prog reporting*/,
      processorPresenter /*the table presenter*/);
  m_algoRunner = boost::make_shared<MantidQt::API::AlgorithmRunner>(this);
}

/**
* Add a command (action) to a menu
* @param menu : [input] The menu where actions will be added
* @param command : [input] The command (action) to add
*/
void QtReflMainView::addToMenu(QMenu *menu, DataProcessorCommand_uptr command) {

  m_commands.push_back(Mantid::Kernel::make_unique<DataProcessorCommandAdapter>(
      menu, std::move(command)));
}

/**
* Adds actions to the "Reflectometry" menu
* @param tableCommands : [input] The list of commands to add to the
* "Reflectometry" menu
*/
void QtReflMainView::setTableCommands(
    std::vector<DataProcessorCommand_uptr> tableCommands) {

  ui.menuTable->clear();
  for (auto &command : tableCommands) {
    addToMenu(ui.menuTable, std::move(command));
  }

  // Slit calculator
  QAction *slitCalc = ui.menuTable->addAction(
      QIcon(QString::fromStdString(":/param_range_btn.png")),
      QString("Slit Calculator"));
  connect(slitCalc, SIGNAL(triggered()), this, SLOT(slitCalculatorTriggered()));
}

/**
* Adds actions to the "Edit" menu
* @param rowCommands : [input] The list of commands to add to the "Edit" menu
*/
void QtReflMainView::setRowCommands(
    std::vector<DataProcessorCommand_uptr> rowCommands) {

  ui.menuRows->clear();
  for (auto &command : rowCommands) {
    addToMenu(ui.menuRows, std::move(command));
  }
}

/**
* Clears all the actions (commands)
*/
void QtReflMainView::clearCommands() { m_commands.clear(); }

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
This slot shows the slit calculator
*/
void QtReflMainView::slitCalculatorTriggered() {
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

/** This is slot is triggered when any of the instrument combo boxes changes. It
 * is used to update the Slit Calculator
 * @param index : The index of the combo box
 */
void QtReflMainView::instrumentChanged(int index) {
  m_calculator->setCurrentInstrumentName(
      ui.comboSearchInstrument->itemText(index).toStdString());
  m_calculator->processInstrumentHasBeenChanged();
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
      this, QString::fromStdString(title), QString::fromStdString(prompt),
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

} // namespace CustomInterfaces
} // namespace Mantid
