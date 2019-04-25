// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtReflRunsTabView.h"
#include "IReflRunsTabPresenter.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtWidgets/Common/AlgorithmRunner.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorPresenter.h"
#include "MantidQtWidgets/Common/DataProcessorUI/QDataProcessorWidget.h"
#include "MantidQtWidgets/Common/DataProcessorUI/QtCommandAdapter.h"
#include "MantidQtWidgets/Common/FileDialogHandler.h"
#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidQtWidgets/Common/HintingLineEditFactory.h"
#include "MantidQtWidgets/Common/SlitCalculator.h"
#include "ReflGenericDataProcessorPresenterFactory.h"
#include "ReflRunsTabPresenter.h"
#include "ReflSearchModel.h"

namespace MantidQt {
namespace CustomInterfaces {
using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

//----------------------------------------------------------------------------------------------
/** Constructor
 * @param parent :: The parent of this view
 */
QtReflRunsTabView::QtReflRunsTabView(QWidget *parent)
    : m_presenter(), m_calculator(new SlitCalculator(this)) {

  UNUSED_ARG(parent);
  initLayout();
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
QtReflRunsTabView::~QtReflRunsTabView() {}

/**
Initialise the Interface
*/
void QtReflRunsTabView::initLayout() {
  ui.setupUi(this);

  ui.buttonTransfer->setDefaultAction(ui.actionTransfer);

  // Expand the process runs column at the expense of the search column
  ui.splitterTables->setStretchFactor(0, 0);
  ui.splitterTables->setStretchFactor(1, 1);

  // Create the DataProcessor presenter
  ReflGenericDataProcessorPresenterFactory presenterFactory;

  QDataProcessorWidget *qDataProcessorWidget_1 = new QDataProcessorWidget(
      std::unique_ptr<DataProcessor::DataProcessorPresenter>(
          presenterFactory.create(0)),
      this);
  ui.toolbox->addItem(qDataProcessorWidget_1, "Group 1");
  connect(qDataProcessorWidget_1,
          SIGNAL(runAsPythonScript(const QString &, bool)), this,
          SIGNAL(runAsPythonScript(const QString &, bool)));

  QDataProcessorWidget *qDataProcessorWidget_2 = new QDataProcessorWidget(
      std::unique_ptr<DataProcessor::DataProcessorPresenter>(
          presenterFactory.create(1)),
      this);
  ui.toolbox->addItem(qDataProcessorWidget_2, "Group 2");
  connect(qDataProcessorWidget_2,
          SIGNAL(runAsPythonScript(const QString &, bool)), this,
          SIGNAL(runAsPythonScript(const QString &, bool)));

  std::vector<DataProcessor::DataProcessorPresenter *> processingWidgets;
  processingWidgets.push_back(qDataProcessorWidget_1->getPresenter());
  processingWidgets.push_back(qDataProcessorWidget_2->getPresenter());

  // Create the presenter
  m_presenter = std::make_shared<ReflRunsTabPresenter>(
      this /* main view */,
      this /* Currently this concrete view is also responsible for prog
              reporting */
      ,
      processingWidgets /* The data processor presenters */);
  m_algoRunner = boost::make_shared<MantidQt::API::AlgorithmRunner>(this);
  m_monitorAlgoRunner =
      boost::make_shared<MantidQt::API::AlgorithmRunner>(this);

  // Custom context menu for table
  connect(ui.tableSearchResults,
          SIGNAL(customContextMenuRequested(const QPoint &)), this,
          SLOT(showSearchContextMenu(const QPoint &)));
  // Synchronize the slit calculator
  connect(ui.comboSearchInstrument, SIGNAL(currentIndexChanged(int)), this,
          SLOT(instrumentChanged(int)));
  // Selected group changed
  connect(ui.toolbox, SIGNAL(currentChanged(int)), this, SLOT(groupChanged()));

  // Synchronize the instrument selection widgets
  // Processing table in group 1
  connect(ui.comboSearchInstrument, SIGNAL(currentIndexChanged(int)),
          qDataProcessorWidget_1,
          SLOT(on_comboProcessInstrument_currentIndexChanged(int)));
  connect(qDataProcessorWidget_1,
          SIGNAL(comboProcessInstrument_currentIndexChanged(int)),
          ui.comboSearchInstrument, SLOT(setCurrentIndex(int)));
  connect(qDataProcessorWidget_1,
          SIGNAL(comboProcessInstrument_currentIndexChanged(int)), this,
          SLOT(instrumentChanged(int)));
  // Processing table in group 2
  connect(ui.comboSearchInstrument, SIGNAL(currentIndexChanged(int)),
          qDataProcessorWidget_2,
          SLOT(on_comboProcessInstrument_currentIndexChanged(int)));
  connect(qDataProcessorWidget_2,
          SIGNAL(comboProcessInstrument_currentIndexChanged(int)),
          ui.comboSearchInstrument, SLOT(setCurrentIndex(int)));
  connect(qDataProcessorWidget_2,
          SIGNAL(comboProcessInstrument_currentIndexChanged(int)), this,
          SLOT(instrumentChanged(int)));
}

/**
 * Add a command (action) to a menu
 * @param menu : [input] The menu where actions will be added
 * @param command : [input] The command (action) to add
 */
void QtReflRunsTabView::addToMenu(QMenu *menu,
                                  DataProcessor::Command_uptr command) {

  m_commands.push_back(
      Mantid::Kernel::make_unique<QtCommandAdapter>(menu, std::move(command)));
}

/**
 * Adds actions to the "Reflectometry" menu
 * @param tableCommands : [input] The list of commands to add to the
 * "Reflectometry" menu
 */
void QtReflRunsTabView::setTableCommands(
    std::vector<DataProcessor::Command_uptr> tableCommands) {

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
void QtReflRunsTabView::setRowCommands(
    std::vector<DataProcessor::Command_uptr> rowCommands) {

  for (auto &command : rowCommands) {
    addToMenu(ui.menuRows, std::move(command));
  }
}

/**
 * Clears all the actions (commands)
 */
void QtReflRunsTabView::clearCommands() {
  ui.menuRows->clear();
  ui.menuTable->clear();
  m_commands.clear();
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
 * Sets the transfer method combo box enabled or disabled
 * @param enabled : Whether to enable or disable the button
 */
void QtReflRunsTabView::setTransferMethodComboEnabled(bool enabled) {

  ui.comboTransferMethod->setEnabled(enabled);
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
 * Sets the start-monitor button enabled or disabled
 * @param enabled : Whether to enable or disable the button
 */
void QtReflRunsTabView::setStartMonitorButtonEnabled(bool enabled) {

  ui.buttonMonitor->setEnabled(enabled);
}

/**
 * Sets the stop-monitor enabled or disabled
 * @param enabled : Whether to enable or disable the button
 */
void QtReflRunsTabView::setStopMonitorButtonEnabled(bool enabled) {

  ui.buttonStopMonitor->setEnabled(enabled);
}

/**
 * Set all possible tranfer methods
 * @param methods : All possible transfer methods.
 */
void QtReflRunsTabView::setTransferMethods(
    const std::set<std::string> &methods) {
  for (const auto &method : methods) {
    ui.comboTransferMethod->addItem(method.c_str());
  }
}

/**
Set the list of available instruments to search for and updates the list of
available instruments in the table view
@param instruments : The list of instruments available
@param defaultInstrument : The instrument to have selected by default
*/
void QtReflRunsTabView::setInstrumentList(
    const std::vector<std::string> &instruments,
    const std::string &defaultInstrument) {
  ui.comboSearchInstrument->clear();

  for (const auto &it : instruments) {
    QString instrument = QString::fromStdString(it);
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
  m_presenter->notify(IReflRunsTabPresenter::InstrumentChangedFlag);
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
    for (auto &selectedRow : selectedRows)
      rows.insert(selectedRow.row());
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
  return m_presenter.get();
}

boost::shared_ptr<MantidQt::API::AlgorithmRunner>
QtReflRunsTabView::getAlgorithmRunner() const {
  return m_algoRunner;
}

boost::shared_ptr<MantidQt::API::AlgorithmRunner>
QtReflRunsTabView::getMonitorAlgorithmRunner() const {
  return m_monitorAlgoRunner;
}

/**
Get the string the user wants to search for.
@returns The search string
*/
std::string QtReflRunsTabView::getSearchString() const {
  return ui.textSearch->text().toStdString();
}

/**
 * @return the transfer method selected.
 */
std::string QtReflRunsTabView::getTransferMethod() const {
  return ui.comboTransferMethod->currentText().toStdString();
}

/**
 * @return the selected group
 */
int QtReflRunsTabView::getSelectedGroup() const {
  return ui.toolbox->currentIndex();
}

/** This is slot is triggered when the selected group changes.
 *
 */
void QtReflRunsTabView::groupChanged() {
  m_presenter->notify(IReflRunsTabPresenter::GroupChangedFlag);
}

void MantidQt::CustomInterfaces::QtReflRunsTabView::on_buttonMonitor_clicked() {
  startMonitor();
}

void MantidQt::CustomInterfaces::QtReflRunsTabView::
    on_buttonStopMonitor_clicked() {
  stopMonitor();
}

/** Start live data monitoring
 */
void QtReflRunsTabView::startMonitor() {
  m_monitorAlgoRunner.get()->disconnect(); // disconnect any other connections
  m_presenter->notify(IReflRunsTabPresenter::StartMonitorFlag);
  connect(m_monitorAlgoRunner.get(), SIGNAL(algorithmComplete(bool)), this,
          SLOT(startMonitorComplete()), Qt::UniqueConnection);
}

/**
This slot notifies the presenter that the monitoring algorithm finished
*/
void QtReflRunsTabView::startMonitorComplete() {
  m_presenter->notify(IReflRunsTabPresenter::StartMonitorCompleteFlag);
}

/** Stop live data monitoring
 */
void QtReflRunsTabView::stopMonitor() {
  m_presenter->notify(IReflRunsTabPresenter::StopMonitorFlag);
}

} // namespace CustomInterfaces
} // namespace MantidQt
