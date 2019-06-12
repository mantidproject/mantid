// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "RunsView.h"
#include "GUI/Common/SearchModel.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidQtIcons/Icon.h"
#include "MantidQtWidgets/Common/AlgorithmRunner.h"
#include "MantidQtWidgets/Common/FileDialogHandler.h"
#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidQtWidgets/Common/HintingLineEditFactory.h"
#include "MantidQtWidgets/Common/SlitCalculator.h"
#include <QMenu>
#include <QMessageBox>

namespace MantidQt {
namespace CustomInterfaces {
using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;
using namespace MantidQt::Icons;

//----------------------------------------------------------------------------------------------
/** Constructor
 * @param parent :: The parent of this view
 * @param makeRunsTableView :: The factory for the RunsTableView.
 */
RunsView::RunsView(QWidget *parent, RunsTableViewFactory makeRunsTableView)
    : MantidWidget(parent), m_notifyee(nullptr), m_timerNotifyee(nullptr),
      m_calculator(new SlitCalculator(this)), m_tableView(makeRunsTableView()) {
  initLayout();
}
void RunsView::loginFailed(std::string const &fullError) {
  QMessageBox::critical(this, QString::fromStdString(fullError),
                        "Login Failed!");
}

void RunsView::subscribe(RunsViewSubscriber *notifyee) {
  m_notifyee = notifyee;
}

void RunsView::subscribeTimer(RunsViewTimerSubscriber *notifyee) {
  m_timerNotifyee = notifyee;
}

IRunsTableView *RunsView::table() const { return m_tableView; }

/**
Initialise the Interface
*/
void RunsView::initLayout() {
  ui.setupUi(this);

  ui.buttonTransfer->setDefaultAction(ui.actionTransfer);

  // Expand the process runs column at the expense of the search column
  ui.splitterTables->setStretchFactor(0, 0);
  ui.splitterTables->setStretchFactor(1, 1);
  ui.tablePane->layout()->addWidget(m_tableView);

  // Add Icons to the buttons
  ui.actionAutoreducePause->setIcon(getIcon("mdi.pause", "red", 1.3));
  ui.buttonAutoreduce->setIcon(getIcon("mdi.play", "green", 1.3));
  ui.buttonAutoreducePause->setIcon(getIcon("mdi.pause", "red", 1.3));
  ui.buttonMonitor->setIcon(getIcon("mdi.play", "green", 1.3));
  ui.buttonStopMonitor->setIcon(getIcon("mdi.pause", "red", 1.3));
  ui.actionAutoreduce->setIcon(getIcon("mdi.play", "green", 1.3));
  ui.actionSearch->setIcon(getIcon("mdi.folder", "black", 1.3));
  ui.actionTransfer->setIcon(getIcon("mdi.file-move", "black", 1.3));

  m_algoRunner = boost::make_shared<MantidQt::API::AlgorithmRunner>(this);
  m_monitorAlgoRunner =
      boost::make_shared<MantidQt::API::AlgorithmRunner>(this);

  // Custom context menu for table
  connect(ui.searchPane, SIGNAL(customContextMenuRequested(const QPoint &)),
          this, SLOT(onShowSearchContextMenuRequested(const QPoint &)));
  // Synchronize the slit calculator
  connect(ui.comboSearchInstrument, SIGNAL(currentIndexChanged(int)), this,
          SLOT(onInstrumentChanged(int)));

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

void RunsView::noActiveICatSessions() {
  QMessageBox::information(
      this, "Login Failed",
      "Error Logging in: Please press 'Search' to try again.");
}

void RunsView::missingRunsToTransfer() {
  QMessageBox::critical(this, "No runs selected",
                        "Error: Please select at least one run to transfer.");
}

/**
 * Updates actions in the menus to be enabled or disabled
 * according to whether processing is running or not.
 * @param isProcessing: Whether processing is running
 */
void RunsView::updateMenuEnabledState(bool isProcessing) {
  UNUSED_ARG(isProcessing)
}

/**
 * Sets the "Autoreduce" button enabled or disabled
 * @param enabled : Whether to enable or disable the button
 */
void RunsView::setAutoreduceButtonEnabled(bool enabled) {

  ui.buttonAutoreduce->setEnabled(enabled);
}

/**
 * Sets the "Autoreduce" button enabled or disabled
 * @param enabled : Whether to enable or disable the button
 */
void RunsView::setAutoreducePauseButtonEnabled(bool enabled) {

  ui.buttonAutoreducePause->setEnabled(enabled);
}

/**
 * Sets the "Transfer" button enabled or disabled
 * @param enabled : Whether to enable or disable the button
 */
void RunsView::setTransferButtonEnabled(bool enabled) {

  ui.buttonTransfer->setEnabled(enabled);
}

/**
 * Sets the "Instrument" combo box enabled or disabled
 * @param enabled : Whether to enable or disable the button
 */
void RunsView::setInstrumentComboEnabled(bool enabled) {

  ui.comboSearchInstrument->setEnabled(enabled);
}

/**
 * Sets the search text box enabled or disabled
 * @param enabled : Whether to enable or disable the button
 */
void RunsView::setSearchTextEntryEnabled(bool enabled) {

  ui.textSearch->setEnabled(enabled);
}

/**
 * Sets the search button enabled or disabled
 * @param enabled : Whether to enable or disable the button
 */
void RunsView::setSearchButtonEnabled(bool enabled) {

  ui.buttonSearch->setEnabled(enabled);
}

/**
 * Sets the start-monitor button enabled or disabled
 * @param enabled : Whether to enable or disable the button
 */
void RunsView::setStartMonitorButtonEnabled(bool enabled) {

  ui.buttonMonitor->setEnabled(enabled);
}

/**
 * Sets the stop-monitor enabled or disabled
 * @param enabled : Whether to enable or disable the button
 */
void RunsView::setStopMonitorButtonEnabled(bool enabled) {

  ui.buttonStopMonitor->setEnabled(enabled);
}

/**
Set the list of available instruments to search for and updates the list of
available instruments in the table view
@param instruments : The list of instruments available
@param defaultInstrumentIndex : The index of the instrument to have selected by
default
*/
void RunsView::setInstrumentList(const std::vector<std::string> &instruments,
                                 int defaultInstrumentIndex) {
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
void RunsView::setProgressRange(int min, int max) {
  ui.progressBar->setRange(min, max);
  ProgressableView::setProgressRange(min, max);
}

/**
Set the status of the progress bar
@param progress : The current value of the bar
*/
void RunsView::setProgress(int progress) { ui.progressBar->setValue(progress); }

/**
 * Clear the progress
 */
void RunsView::clearProgress() { ui.progressBar->reset(); }

/**
Set a new model for search results
@param model : the model to be attached to the search results
*/
void RunsView::showSearch(SearchModel_sptr model) {
  m_searchModel = model;
  ui.tableSearchResults->setModel(m_searchModel.get());
  ui.tableSearchResults->resizeColumnsToContents();
}

/** Start an icat search
 */
void RunsView::startIcatSearch() {
  m_algoRunner.get()->disconnect(); // disconnect any other connections
  m_notifyee->notifySearch();
  connect(m_algoRunner.get(), SIGNAL(algorithmComplete(bool)), this,
          SLOT(onSearchComplete()), Qt::UniqueConnection);
}

/**
This slot notifies the presenter that the ICAT search was completed
*/
void RunsView::onSearchComplete() { m_notifyee->notifyICATSearchComplete(); }

/**
This slot notifies the presenter that the "search" button has been pressed
*/
void RunsView::on_actionSearch_triggered() { startIcatSearch(); }

/**
This slot conducts a search operation before notifying the presenter that the
"autoreduce" button has been pressed
*/
void RunsView::on_actionAutoreduce_triggered() {
  m_notifyee->notifyAutoreductionResumed();
}

/**
This slot conducts a search operation before notifying the presenter that the
"pause autoreduce" button has been pressed
*/
void RunsView::on_actionAutoreducePause_triggered() {
  m_notifyee->notifyAutoreductionPaused();
}

/**
This slot notifies the presenter that the "transfer" button has been pressed
*/
void RunsView::on_actionTransfer_triggered() { m_notifyee->notifyTransfer(); }

/**
This slot shows the slit calculator
*/
void RunsView::onShowSlitCalculatorRequested() {
  m_calculator->setCurrentInstrumentName(
      ui.comboSearchInstrument->currentText().toStdString());
  m_calculator->show();
}

/**
This slot is triggered when the user right clicks on the search results table
@param pos : The position of the right click within the table
*/
void RunsView::onShowSearchContextMenuRequested(const QPoint &pos) {
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
void RunsView::onInstrumentChanged(int index) {
  ui.textSearch->clear();
  if (m_searchModel)
    m_searchModel->clear();
  m_calculator->setCurrentInstrumentName(
      ui.comboSearchInstrument->itemText(index).toStdString());
  m_calculator->processInstrumentHasBeenChanged();
  m_notifyee->notifyInstrumentChanged();
}

/**
Get the selected instrument for searching
@returns the selected instrument to search for
*/
std::string RunsView::getSearchInstrument() const {
  return ui.comboSearchInstrument->currentText().toStdString();
}

void RunsView::setSearchInstrument(std::string const &instrumentName) {
  setSelected(*ui.comboSearchInstrument, instrumentName);
}

/**
Get the indices of the highlighted search result rows
@returns a set of ints containing the selected row numbers
*/
std::set<int> RunsView::getSelectedSearchRows() const {
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
std::set<int> RunsView::getAllSearchRows() const {
  std::set<int> rows;
  if (!ui.tableSearchResults || !ui.tableSearchResults->model())
    return rows;
  auto const rowCount = ui.tableSearchResults->model()->rowCount();
  for (auto row = 0; row < rowCount; ++row)
    rows.insert(row);
  return rows;
}

boost::shared_ptr<MantidQt::API::AlgorithmRunner>
RunsView::getAlgorithmRunner() const {
  return m_algoRunner;
}

boost::shared_ptr<MantidQt::API::AlgorithmRunner>
RunsView::getMonitorAlgorithmRunner() const {
  return m_monitorAlgoRunner;
}

/**
Get the string the user wants to search for.
@returns The search string
*/
std::string RunsView::getSearchString() const {
  return ui.textSearch->text().toStdString();
}

void MantidQt::CustomInterfaces::RunsView::on_buttonMonitor_clicked() {
  startMonitor();
}

void MantidQt::CustomInterfaces::RunsView::on_buttonStopMonitor_clicked() {
  stopMonitor();
}

/** Start live data monitoring
 */
void RunsView::startMonitor() {
  m_monitorAlgoRunner.get()->disconnect(); // disconnect any other connections
  m_notifyee->notifyStartMonitor();
  connect(m_monitorAlgoRunner.get(), SIGNAL(algorithmComplete(bool)), this,
          SLOT(onStartMonitorComplete()), Qt::UniqueConnection);
}

/**
This slot notifies the presenter that the monitoring algorithm finished
*/
void RunsView::onStartMonitorComplete() {
  m_notifyee->notifyStartMonitorComplete();
}

/** Stop live data monitoring
 */
void RunsView::stopMonitor() { m_notifyee->notifyStopMonitor(); }

/** Set a combo box to the given value
 */
void RunsView::setSelected(QComboBox &box, std::string const &str) {
  auto const index = box.findText(QString::fromStdString(str));
  if (index != -1)
    box.setCurrentIndex(index);
}

/**
   This slot is called each time the timer times out
*/
void RunsView::timerEvent(QTimerEvent *event) {
  if (event->timerId() == m_timer.timerId()) {
    if (m_timerNotifyee)
      m_timerNotifyee->notifyTimerEvent();
  } else {
    QWidget::timerEvent(event);
  }
}

/** start the timer
 */
void RunsView::startTimer(const int millisecs) {
  m_timer.start(millisecs, this);
}

/** stop
 */
void RunsView::stopTimer() { m_timer.stop(); }
} // namespace CustomInterfaces
} // namespace MantidQt
