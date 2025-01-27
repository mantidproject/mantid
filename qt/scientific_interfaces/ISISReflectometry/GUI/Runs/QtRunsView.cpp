// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtRunsView.h"
#include "MantidKernel/UsageService.h"
#include "MantidQtIcons/Icon.h"
#include "MantidQtWidgets/Common/FileDialogHandler.h"
#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidQtWidgets/Common/HintingLineEditFactory.h"
#include "MantidQtWidgets/Common/QtAlgorithmRunner.h"
#include <QMenu>
#include <QMessageBox>
#include <QSignalBlocker>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;
using namespace MantidQt::Icons;

//----------------------------------------------------------------------------------------------
/** Constructor
 * @param parent :: The parent of this view
 * @param makeRunsTableView :: The factory for the RunsTableView.
 */
QtRunsView::QtRunsView(QWidget *parent, const RunsTableViewFactory &makeRunsTableView)
    : MantidWidget(parent), m_notifyee(nullptr), m_timerNotifyee(nullptr), m_searchNotifyee(nullptr), m_searchModel(),
      m_tableView(makeRunsTableView()), m_timer() {
  initLayout();
  m_ui.tableSearchResults->setModel(&m_searchModel);
}

void QtRunsView::subscribe(RunsViewSubscriber *notifyee) { m_notifyee = notifyee; }

void QtRunsView::subscribeTimer(RunsViewTimerSubscriber *notifyee) { m_timerNotifyee = notifyee; }

void QtRunsView::subscribeSearch(RunsViewSearchSubscriber *notifyee) { m_searchNotifyee = notifyee; }

IRunsTableView *QtRunsView::table() const { return m_tableView; }

/**
Initialise the Interface
*/
void QtRunsView::initLayout() {
  m_ui.setupUi(this);

  m_ui.buttonTransfer->setDefaultAction(m_ui.actionTransfer);
  m_ui.buttonExport->setDefaultAction(m_ui.actionExport);

  // Expand the process runs column at the expense of the search column
  m_ui.splitterTables->setStretchFactor(0, 0);
  m_ui.splitterTables->setStretchFactor(1, 1);
  m_ui.tablePane->layout()->addWidget(m_tableView);

  // Add Icons to the buttons
  m_ui.actionAutoreducePause->setIcon(getIcon("mdi.pause", "red", 1.3));
  m_ui.buttonAutoreduce->setIcon(getIcon("mdi.play", "green", 1.3));
  m_ui.buttonAutoreducePause->setIcon(getIcon("mdi.pause", "red", 1.3));
  m_ui.buttonMonitor->setIcon(getIcon("mdi.play", "green", 1.3));
  m_ui.buttonStopMonitor->setIcon(getIcon("mdi.pause", "red", 1.3));
  m_ui.actionAutoreduce->setIcon(getIcon("mdi.play", "green", 1.3));
  m_ui.actionSearch->setIcon(getIcon("mdi.folder", "black", 1.3));
  m_ui.actionTransfer->setIcon(getIcon("mdi.file-move", "black", 1.3));
  m_ui.actionExport->setIcon(getIcon("mdi.content-save", "black", 1.3));

  m_algoRunner = std::make_shared<MantidQt::API::QtAlgorithmRunner>(this);
  m_monitorAlgoRunner = std::make_shared<MantidQt::API::QtAlgorithmRunner>(this);

  // Custom context menu for table
  connect(m_ui.searchPane, SIGNAL(customContextMenuRequested(const QPoint &)), this,
          SLOT(onShowSearchContextMenuRequested(const QPoint &)));
  // Synchronize the slit calculator
  connect(m_ui.comboSearchInstrument, SIGNAL(currentIndexChanged(int)), this, SLOT(onInstrumentChanged(int)));
  // Connect signal for when search algorithm completes
  connect(m_algoRunner.get(), SIGNAL(algorithmComplete(bool)), this, SLOT(onSearchComplete()), Qt::UniqueConnection);
  // Connect signal for when user edits the search results table
  connect(&m_searchModel, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this,
          SLOT(onSearchResultsChanged(const QModelIndex &, const QModelIndex &)));
}

/**
 * Updates actions in the menus to be enabled or disabled
 * according to whether processing is running or not.
 * @param isProcessing: Whether processing is running
 */
void QtRunsView::updateMenuEnabledState(bool isProcessing) { UNUSED_ARG(isProcessing) }

/**
 * Sets the "Autoreduce" button enabled or disabled
 * @param enabled : Whether to enable or disable the button
 */
void QtRunsView::setAutoreduceButtonEnabled(bool enabled) { m_ui.buttonAutoreduce->setEnabled(enabled); }

/**
 * Sets the "Autoreduce" button enabled or disabled
 * @param enabled : Whether to enable or disable the button
 */
void QtRunsView::setAutoreducePauseButtonEnabled(bool enabled) { m_ui.buttonAutoreducePause->setEnabled(enabled); }

/**
 * Sets the "Transfer" button enabled or disabled
 * @param enabled : Whether to enable or disable the button
 */
void QtRunsView::setTransferButtonEnabled(bool enabled) { m_ui.buttonTransfer->setEnabled(enabled); }

/**
 * Sets the "Instrument" combo box enabled or disabled
 * @param enabled : Whether to enable or disable the button
 */
void QtRunsView::setInstrumentComboEnabled(bool enabled) { m_ui.comboSearchInstrument->setEnabled(enabled); }

/**
 * Sets the search text box enabled or disabled
 * @param enabled : Whether to enable or disable the button
 */
void QtRunsView::setSearchTextEntryEnabled(bool enabled) {

  m_ui.textSearch->setEnabled(enabled);
  m_ui.textCycle->setEnabled(enabled);
}

/**
 * Sets the search button enabled or disabled
 * @param enabled : Whether to enable or disable the button
 */
void QtRunsView::setSearchButtonEnabled(bool enabled) { m_ui.buttonSearch->setEnabled(enabled); }

/**
 * Sets editing the search results table enabled or disabled
 * @param enabled : Whether to enable or disable the button
 */
void QtRunsView::setSearchResultsEnabled(bool enabled) {
  static const auto editTriggers = m_ui.tableSearchResults->editTriggers();
  if (enabled)
    m_ui.tableSearchResults->setEditTriggers(editTriggers);
  else
    m_ui.tableSearchResults->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

/**
 * Sets the start-monitor button enabled or disabled
 * @param enabled : Whether to enable or disable the button
 */
void QtRunsView::setStartMonitorButtonEnabled(bool enabled) { m_ui.buttonMonitor->setEnabled(enabled); }

/**
 * Sets the stop-monitor enabled or disabled
 * @param enabled : Whether to enable or disable the button
 */
void QtRunsView::setStopMonitorButtonEnabled(bool enabled) { m_ui.buttonStopMonitor->setEnabled(enabled); }

/**
 * Sets the update interval enabled or disabled
 * @param enabled : Whether to enable or disable the spin box
 */
void QtRunsView::setUpdateIntervalSpinBoxEnabled(bool enabled) { m_ui.spinBoxUpdateInterval->setEnabled(enabled); }

/**
Set the list of available instruments to search for and updates the list of
available instruments in the table view. The selected instrument will be the first item added to the combobox,
unless a valid value for selectedInstrument is provided.
@param instruments : The list of instruments available
@param selectedInstrument : The name of an instrument to select from the dropdown
default
*/
void QtRunsView::setInstrumentList(const std::vector<std::string> &instruments, const std::string &selectedInstrument) {
  // We block signals while populating the list and setting the selected instrument because adding the first item
  // will trigger a currentIndexChanged signal. This causes existing batch settings to be overwritten when we're
  // initialising a new batch for an instrument that isn't the first in the list.
  const QSignalBlocker blocker(m_ui.comboSearchInstrument);

  m_ui.comboSearchInstrument->clear();
  for (auto &&instrument : instruments)
    m_ui.comboSearchInstrument->addItem(QString::fromStdString(instrument));
  setSearchInstrument(selectedInstrument);
}

/**
Set the range of the progress bar
@param min : The minimum value of the bar
@param max : The maxmimum value of the bar
*/
void QtRunsView::setProgressRange(int min, int max) {
  m_ui.progressBar->setRange(min, max);
  ProgressableView::setProgressRange(min, max);
}

/**
Set the status of the progress bar
@param progress : The current value of the bar
*/
void QtRunsView::setProgress(int progress) { m_ui.progressBar->setValue(progress); }

/**
 * Clear the progress
 */
void QtRunsView::clearProgress() { m_ui.progressBar->reset(); }

/**
 * Resize the search results table columns
 */
void QtRunsView::resizeSearchResultsColumnsToContents() { m_ui.tableSearchResults->resizeColumnsToContents(); }

/** Get the width of the search results table
 */
int QtRunsView::getSearchResultsTableWidth() const { return m_ui.tableSearchResults->width(); }

/** Get the width of a particular column in the search results table
 */
int QtRunsView::getSearchResultsColumnWidth(int column) const { return m_ui.tableSearchResults->columnWidth(column); }

/** Set the width of column in the search results table
 */
void QtRunsView::setSearchResultsColumnWidth(int column, int width) {
  m_ui.tableSearchResults->setColumnWidth(column, width);
}

/**
 * Get the model containing the search results
 */
ISearchModel const &QtRunsView::searchResults() const { return m_searchModel; }

ISearchModel &QtRunsView::mutableSearchResults() { return m_searchModel; }

/**
This slot notifies the presenter that the user has modified some values in the
search results table
*/
void QtRunsView::onSearchResultsChanged(const QModelIndex &, const QModelIndex &) {
  m_searchNotifyee->notifySearchResultsChanged();
}

/**
This slot notifies the presenter that the search was completed
*/
void QtRunsView::onSearchComplete() { m_searchNotifyee->notifySearchComplete(); }

/**
This slot notifies the presenter that the "search" button has been pressed
*/
void QtRunsView::on_actionSearch_triggered() {
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(Mantid::Kernel::FeatureType::Feature,
                                                                {"ISIS Reflectometry", "RunsTab", "Search"}, false);
  m_notifyee->notifySearch();
}

/**
This slot conducts a search operation before notifying the presenter that the
"autoreduce" button has been pressed
*/
void QtRunsView::on_actionAutoreduce_triggered() {
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      Mantid::Kernel::FeatureType::Feature, {"ISIS Reflectometry", "RunsTab", "StartAutoprocessing"}, false);
  m_notifyee->notifyResumeAutoreductionRequested();
}

/**
This slot conducts a search operation before notifying the presenter that the
"pause autoreduce" button has been pressed
*/
void QtRunsView::on_actionAutoreducePause_triggered() {
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      Mantid::Kernel::FeatureType::Feature, {"ISIS Reflectometry", "RunsTab", "PauseAutoprocessing"}, false);
  m_notifyee->notifyPauseAutoreductionRequested();
}

/**
This slot notifies the presenter that the "transfer" button has been pressed
*/
void QtRunsView::on_actionTransfer_triggered() {
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(Mantid::Kernel::FeatureType::Feature,
                                                                {"ISIS Reflectometry", "RunsTab", "Transfer"}, false);
  m_notifyee->notifyTransfer();
}

/**
 * This slot notifies the presenter that the "Export" button has been pressed
 */
void QtRunsView::on_actionExport_triggered() {
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(Mantid::Kernel::FeatureType::Feature,
                                                                {"ISIS Reflectometry", "RunsTab", "Export"}, false);
  m_notifyee->notifyExportSearchResults();
}

/**
This slot is triggered when the user right clicks on the search results table
@param pos : The position of the right click within the table
*/
void QtRunsView::onShowSearchContextMenuRequested(const QPoint &pos) {
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      Mantid::Kernel::FeatureType::Feature, {"ISIS Reflectometry", "RunsTab", "ShowSearchContextMenu"}, false);
  if (!m_ui.tableSearchResults->indexAt(pos).isValid())
    return;

  // parent widget takes ownership of QMenu
  QMenu *menu = new QMenu(this);
  menu->addAction(m_ui.actionTransfer);
  menu->popup(m_ui.tableSearchResults->viewport()->mapToGlobal(pos));
}

/** This is slot is triggered when any of the instrument combo boxes changes. It
 * notifies the main presenter and updates the Slit Calculator
 * @param index : The index of the combo box
 */
void QtRunsView::onInstrumentChanged(int index) {
  UNUSED_ARG(index);
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      Mantid::Kernel::FeatureType::Feature, {"ISIS Reflectometry", "RunsTab", "InstrumentChanged"}, false);
  m_notifyee->notifyChangeInstrumentRequested();
}

/**
Get the selected instrument for searching
@returns the selected instrument to search for
*/
std::string QtRunsView::getSearchInstrument() const { return m_ui.comboSearchInstrument->currentText().toStdString(); }

void QtRunsView::setSearchInstrument(std::string const &instrumentName) {
  setSelected(*m_ui.comboSearchInstrument, instrumentName);
}

/**
Get the indices of the highlighted search result rows
@returns a set of ints containing the selected row numbers
*/
std::set<int> QtRunsView::getSelectedSearchRows() const {
  std::set<int> rows;
  auto selectionModel = m_ui.tableSearchResults->selectionModel();
  if (selectionModel) {
    auto selectedIndexes = selectionModel->selectedIndexes();
    for (auto it = selectedIndexes.begin(); it != selectedIndexes.end(); ++it)
      rows.insert(it->row());
  }
  return rows;
}

/**
Get the indices of all search result rows
@returns a set of ints containing the row numbers
*/
std::set<int> QtRunsView::getAllSearchRows() const {
  std::set<int> rows;
  if (!m_ui.tableSearchResults || !m_ui.tableSearchResults->model())
    return rows;
  auto const rowCount = m_ui.tableSearchResults->model()->rowCount();
  for (auto row = 0; row < rowCount; ++row)
    rows.insert(row);
  return rows;
}

std::shared_ptr<MantidQt::API::QtAlgorithmRunner> QtRunsView::getAlgorithmRunner() const { return m_algoRunner; }

std::shared_ptr<MantidQt::API::QtAlgorithmRunner> QtRunsView::getMonitorAlgorithmRunner() const {
  return m_monitorAlgoRunner;
}

/**
Get the string the user wants to search for.
@returns The search string
*/
std::string QtRunsView::getSearchString() const { return m_ui.textSearch->text().toStdString(); }

/**
Get the string the user wants to search for.
@returns The search string
*/
std::string QtRunsView::getSearchCycle() const { return m_ui.textCycle->text().toStdString(); }

/**
Get the live data update interval value given by the user.
@returns The live data update interval
*/
int QtRunsView::getLiveDataUpdateInterval() const { return m_ui.spinBoxUpdateInterval->value(); }

void QtRunsView::on_buttonMonitor_clicked() {
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      Mantid::Kernel::FeatureType::Feature, {"ISIS Reflectometry", "RunsTab", "StartMonitor"}, false);
  startMonitor();
}

void QtRunsView::on_buttonStopMonitor_clicked() {
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      Mantid::Kernel::FeatureType::Feature, {"ISIS Reflectometry", "RunsTab", "StopMonitor"}, false);
  stopMonitor();
}

/** Start live data monitoring
 */
void QtRunsView::startMonitor() {
  m_monitorAlgoRunner.get()->disconnect(); // disconnect any other connections
  m_notifyee->notifyStartMonitor();
  connect(m_monitorAlgoRunner.get(), SIGNAL(algorithmComplete(bool)), this, SLOT(onStartMonitorComplete()),
          Qt::UniqueConnection);
}

/**
This slot notifies the presenter that the monitoring algorithm finished
*/
void QtRunsView::onStartMonitorComplete() { m_notifyee->notifyStartMonitorComplete(); }

/** Stop live data monitoring
 */
void QtRunsView::stopMonitor() { m_notifyee->notifyStopMonitor(); }

/** Set a combo box to the given value
 */
void QtRunsView::setSelected(QComboBox &box, std::string const &str) {
  auto const index = box.findText(QString::fromStdString(str));
  if (index != -1)
    box.setCurrentIndex(index);
}

/**
   This slot is called each time the timer times out
*/
void QtRunsView::timerEvent(QTimerEvent *event) {
  if (event->timerId() == m_timer.timerId()) {
    if (m_timerNotifyee)
      m_timerNotifyee->notifyTimerEvent();
  } else {
    QWidget::timerEvent(event);
  }
}

/** start the timer
 */
void QtRunsView::startTimer(const int millisecs) { m_timer.start(millisecs, this); }

/** stop
 */
void QtRunsView::stopTimer() { m_timer.stop(); }
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
