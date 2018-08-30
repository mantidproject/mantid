#include "QtReflRunsTabView.h"
#include "IReflRunsTabPresenter.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtWidgets/Common/AlgorithmRunner.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorPresenter.h"
#include "MantidQtWidgets/Common/DataProcessorUI/QDataProcessorWidget.h"
#include "MantidQtWidgets/Common/DataProcessorUI/QtCommandAdapter.h"
#include "MantidQtWidgets/Common/FileDialogHandler.h"
#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidQtWidgets/Common/HintingLineEditFactory.h"
#include "MantidQtWidgets/Common/SlitCalculator.h"
#include "ReflRunsTabPresenter.h"
#include "ReflSearchModel.h"
#include "MantidQtWidgets/Common/DataProcessorUI/QtCommandAdapter.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorPresenter.h"
#include "MantidQtWidgets/Common/DataProcessorUI/QDataProcessorWidget.h"
#include "MantidQtWidgets/Common/HintingLineEditFactory.h"
#include "MantidQtWidgets/Common/SlitCalculator.h"
#include <QMessageBox>

namespace MantidQt {
namespace CustomInterfaces {
using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

//----------------------------------------------------------------------------------------------
/** Constructor
* @param parent :: The parent of this view
* @param makeRunsTableView :: The factory for the RunsTableView.
*/
QtReflRunsTabView::QtReflRunsTabView(QWidget *parent,
                                     RunsTableViewFactory makeRunsTableView)
    : m_presenter(nullptr), m_calculator(new SlitCalculator(this)),
      m_tableView(makeRunsTableView()) {

  UNUSED_ARG(parent);
  initLayout();
}
void QtReflRunsTabView::loginFailed(std::string const &fullError) {
  QMessageBox::critical(this, QString::fromStdString(fullError),
                        "Login Failed!");
}

void QtReflRunsTabView::subscribe(IReflRunsTabPresenter *presenter) {
  m_presenter = presenter;
}

IRunsTableView *QtReflRunsTabView::table() const { return m_tableView; }

/**
Initialise the Interface
*/
void QtReflRunsTabView::initLayout() {
  ui.setupUi(this);

  ui.buttonTransfer->setDefaultAction(ui.actionTransfer);

  // Expand the process runs column at the expense of the search column
  ui.splitterTables->setStretchFactor(0, 0);
  ui.splitterTables->setStretchFactor(1, 1);
  ui.tablePane->layout()->addWidget(m_tableView);

  m_algoRunner = boost::make_shared<MantidQt::API::AlgorithmRunner>(this);

  // Custom context menu for table
  connect(ui.searchPane, SIGNAL(customContextMenuRequested(const QPoint &)),
          this, SLOT(showSearchContextMenu(const QPoint &)));
  // Synchronize the slit calculator
  connect(ui.comboSearchInstrument, SIGNAL(currentIndexChanged(int)), this,
          SLOT(instrumentChanged(int)));

  // Synchronize the instrument selection widgets
  // Processing table in group 1
  // connect(ui.comboSearchInstrument, SIGNAL(currentIndexChanged(int)),
  //        qDataProcessorWidget_1,
  //        SLOT(on_comboProcessInstrument_currentIndexChanged(int)));
  // connect(qDataProcessorWidget_1,
  //        SIGNAL(comboProcessInstrument_currentIndexChanged(int)),
  //        ui.comboSearchInstrument, SLOT(setCurrentIndex(int)));
  // connect(qDataProcessorWidget_1,
  //        SIGNAL(comboProcessInstrument_currentIndexChanged(int)), this,
  //        SLOT(instrumentChanged(int)));
  // Processing table in group 2
  // connect(ui.comboSearchInstrument, SIGNAL(currentIndexChanged(int)),
  //        qDataProcessorWidget_2,
  //        SLOT(on_comboProcessInstrument_currentIndexChanged(int)));
  // connect(qDataProcessorWidget_2,
  //        SIGNAL(comboProcessInstrument_currentIndexChanged(int)),
  //        ui.comboSearchInstrument, SLOT(setCurrentIndex(int)));
  // connect(qDataProcessorWidget_2,
  //        SIGNAL(comboProcessInstrument_currentIndexChanged(int)), this,
  //        SLOT(instrumentChanged(int)));
}

void QtReflRunsTabView::noActiveICatSessions() {
  QMessageBox::information(
      this, "Login Failed",
      "Error Logging in: Please press 'Search' to try again.");
}

void QtReflRunsTabView::missingRunsToTransfer() {
  QMessageBox::critical(this, "No runs selected",
                        "Error: Please select at least one run to transfer.");
}

/**
 * Updates actions in the menus to be enabled or disabled
 * according to whether processing is running or not.
 * @param isProcessing: Whether processing is running
 */
void QtReflRunsTabView::updateMenuEnabledState(bool isProcessing) {

  for (auto &command : m_commands) {
    command->updateEnabledState(isProcessing);
  }
}

/**
 * Sets the "Autoreduce" button enabled or disabled
 * @param enabled : Whether to enable or disable the button
 */
void QtReflRunsTabView::setAutoreduceButtonEnabled(bool enabled) {

  ui.buttonAutoreduce->setEnabled(enabled);
}

/**
 * Sets the "Autoreduce" button enabled or disabled
 * @param enabled : Whether to enable or disable the button
 */
void QtReflRunsTabView::setAutoreducePauseButtonEnabled(bool enabled) {

  ui.buttonAutoreducePause->setEnabled(enabled);
}

/**
 * Sets the "Transfer" button enabled or disabled
 * @param enabled : Whether to enable or disable the button
 */
void QtReflRunsTabView::setTransferButtonEnabled(bool enabled) {

  ui.buttonTransfer->setEnabled(enabled);
}

/**
 * Sets the "Instrument" combo box enabled or disabled
 * @param enabled : Whether to enable or disable the button
 */
void QtReflRunsTabView::setInstrumentComboEnabled(bool enabled) {

  ui.comboSearchInstrument->setEnabled(enabled);
}

/**
* Sets the search text box enabled or disabled
* @param enabled : Whether to enable or disable the button
*/
void QtReflRunsTabView::setSearchTextEntryEnabled(bool enabled) {

  ui.textSearch->setEnabled(enabled);
}

/**
 * Sets the search button enabled or disabled
 * @param enabled : Whether to enable or disable the button
 */
void QtReflRunsTabView::setSearchButtonEnabled(bool enabled) {

  ui.buttonSearch->setEnabled(enabled);
}

/**
Set the list of available instruments to search for and updates the list of
available instruments in the table view
@param instruments : The list of instruments available
@param defaultInstrumentIndex : The index of the instrument to have selected by
default
*/
void QtReflRunsTabView::setInstrumentList(
    const std::vector<std::string> &instruments, int defaultInstrumentIndex) {
  ui.comboSearchInstrument->clear();
  for (auto &&instrument : instruments)
    ui.comboSearchInstrument->addItem(QString::fromStdString(instrument));
  ui.comboSearchInstrument->setCurrentIndex(defaultInstrumentIndex);
}

/**
Set the range of the progress bar
@param min : The minimum value of the bar
@param max : The maxmimum value of the bar
*/
void QtReflRunsTabView::setProgressRange(int min, int max) {
  ui.progressBar->setRange(min, max);
  ProgressableView::setProgressRange(min, max);
}

/**
Set the status of the progress bar
@param progress : The current value of the bar
*/
void QtReflRunsTabView::setProgress(int progress) {
  ui.progressBar->setValue(progress);
}

/**
 * Clear the progress
 */
void QtReflRunsTabView::clearProgress() { ui.progressBar->reset(); }

/**
Set a new model for search results
@param model : the model to be attached to the search results
*/
void QtReflRunsTabView::showSearch(ReflSearchModel_sptr model) {
  m_searchModel = model;
  ui.tableSearchResults->setModel(m_searchModel.get());
  ui.tableSearchResults->resizeColumnsToContents();
}

/** Start an icat search
 */
void QtReflRunsTabView::startIcatSearch() {
  m_algoRunner.get()->disconnect(); // disconnect any other connections
  m_presenter->notify(IReflRunsTabPresenter::SearchFlag);
  connect(m_algoRunner.get(), SIGNAL(algorithmComplete(bool)), this,
          SLOT(icatSearchComplete()), Qt::UniqueConnection);
}

/**
This slot notifies the presenter that the ICAT search was completed
*/
void QtReflRunsTabView::icatSearchComplete() {
  m_presenter->notify(IReflRunsTabPresenter::ICATSearchCompleteFlag);
}

/**
This slot notifies the presenter that the "search" button has been pressed
*/
void QtReflRunsTabView::on_actionSearch_triggered() { startIcatSearch(); }

/**
This slot conducts a search operation before notifying the presenter that the
"autoreduce" button has been pressed
*/
void QtReflRunsTabView::on_actionAutoreduce_triggered() {
  m_presenter->notify(IReflRunsTabPresenter::StartAutoreductionFlag);
}

/**
This slot conducts a search operation before notifying the presenter that the
"pause autoreduce" button has been pressed
*/
void QtReflRunsTabView::on_actionAutoreducePause_triggered() {
  m_presenter->notify(IReflRunsTabPresenter::PauseAutoreductionFlag);
}

/**
This slot notifies the presenter that the "transfer" button has been pressed
*/
void QtReflRunsTabView::on_actionTransfer_triggered() {
  m_presenter->notify(IReflRunsTabPresenter::Flag::TransferFlag);
}

/**
   This slot is called each time the timer times out
*/
void QtReflRunsTabView::timerEvent(QTimerEvent *event) {
  if (event->timerId() == m_timer.timerId()) {
    m_presenter->notify(IReflRunsTabPresenter::TimerEventFlag);
  } else {
    QWidget::timerEvent(event);
  }
}

/** start the timer
 */
void QtReflRunsTabView::startTimer(const int millisecs) {
  m_timer.start(millisecs, this);
}

/** stop
 */
void QtReflRunsTabView::stopTimer() { m_timer.stop(); }

/**
This slot shows the slit calculator
*/
void QtReflRunsTabView::slitCalculatorTriggered() {
  m_calculator->setCurrentInstrumentName(
      ui.comboSearchInstrument->currentText().toStdString());
  m_calculator->show();
}

/**
This slot is triggered when the user right clicks on the search results table
@param pos : The position of the right click within the table
*/
void QtReflRunsTabView::showSearchContextMenu(const QPoint &pos) {
  if (!ui.tableSearchResults->indexAt(pos).isValid())
    return;

  // parent widget takes ownership of QMenu
  QMenu *menu = new QMenu(this);
  menu->addAction(ui.actionTransfer);
  menu->popup(ui.tableSearchResults->viewport()->mapToGlobal(pos));
}

/** This is slot is triggered when any of the instrument combo boxes changes. It
 * notifies the main presenter and updates the Slit Calculator
 * @param index : The index of the combo box
 */
void QtReflRunsTabView::instrumentChanged(int index) {
  ui.textSearch->clear();
  if (m_searchModel)
    m_searchModel->clear();
  m_calculator->setCurrentInstrumentName(
      ui.comboSearchInstrument->itemText(index).toStdString());
  m_calculator->processInstrumentHasBeenChanged();
  // m_presenter->notify(IReflRunsTabPresenter::InstrumentChangedFlag);
}

/**
Get the selected instrument for searching
@returns the selected instrument to search for
*/
std::string QtReflRunsTabView::getSearchInstrument() const {
  return ui.comboSearchInstrument->currentText().toStdString();
}

/**
Get the indices of the highlighted search result rows
@returns a set of ints containing the selected row numbers
*/
std::set<int> QtReflRunsTabView::getSelectedSearchRows() const {
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
Get the indices of all search result rows
@returns a set of ints containing the row numbers
*/
std::set<int> QtReflRunsTabView::getAllSearchRows() const {
  std::set<int> rows;
  if (!ui.tableSearchResults || !ui.tableSearchResults->model())
    return rows;
  auto const rowCount = ui.tableSearchResults->model()->rowCount();
  for (auto row = 0; row < rowCount; ++row)
    rows.insert(row);
  return rows;
}

/**
Get a pointer to the presenter that's currently controlling this view.
@returns A pointer to the presenter
*/
IReflRunsTabPresenter *QtReflRunsTabView::getPresenter() const {
  return m_presenter;
}

boost::shared_ptr<MantidQt::API::AlgorithmRunner>
QtReflRunsTabView::getAlgorithmRunner() const {
  return m_algoRunner;
}

/**
Get the string the user wants to search for.
@returns The search string
*/
std::string QtReflRunsTabView::getSearchString() const {
  return ui.textSearch->text().toStdString();
}

} // namespace CustomInterfaces
} // namespace MantidQt
