#include "QtReflRunsTabView.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtWidgets/Common/AlgorithmRunner.h"
#include "MantidQtWidgets/Common/FileDialogHandler.h"
#include "MantidQtWidgets/Common/HelpWindow.h"
#include "IReflRunsTabPresenter.h"
#include "ReflGenericDataProcessorPresenterFactory.h"
#include "ReflRunsTabPresenter.h"
#include "ReflSearchModel.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorCommandAdapter.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorPresenter.h"
#include "MantidQtWidgets/Common/DataProcessorUI/QDataProcessorWidget.h"
#include "MantidQtWidgets/Common/HintingLineEditFactory.h"
#include "MantidQtWidgets/Common/SlitCalculator.h"

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
      std::unique_ptr<DataProcessorPresenter>(presenterFactory.create()), this);
  ui.toolbox->addItem(qDataProcessorWidget_1, "Group 1");
  connect(qDataProcessorWidget_1,
          SIGNAL(runAsPythonScript(const QString &, bool)), this,
          SIGNAL(runAsPythonScript(const QString &, bool)));

  QDataProcessorWidget *qDataProcessorWidget_2 = new QDataProcessorWidget(
      std::unique_ptr<DataProcessorPresenter>(presenterFactory.create()), this);
  ui.toolbox->addItem(qDataProcessorWidget_2, "Group 2");
  connect(qDataProcessorWidget_2,
          SIGNAL(runAsPythonScript(const QString &, bool)), this,
          SIGNAL(runAsPythonScript(const QString &, bool)));

  std::vector<DataProcessorPresenter *> processingWidgets;
  processingWidgets.push_back(qDataProcessorWidget_1->getPresenter());
  processingWidgets.push_back(qDataProcessorWidget_2->getPresenter());

  // Create the presenter
  m_presenter = std::make_shared<ReflRunsTabPresenter>(
      this /* main view */, this /* Currently this concrete view is also
                                    responsible for prog reporting */
      ,
      processingWidgets /* The data processor presenters */);
  m_algoRunner = boost::make_shared<MantidQt::API::AlgorithmRunner>(this);

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
                                  DataProcessorCommand_uptr command) {

  m_commands.push_back(Mantid::Kernel::make_unique<DataProcessorCommandAdapter>(
      menu, std::move(command)));
}

/**
* Adds actions to the "Reflectometry" menu
* @param tableCommands : [input] The list of commands to add to the
* "Reflectometry" menu
*/
void QtReflRunsTabView::setTableCommands(
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
void QtReflRunsTabView::setRowCommands(
    std::vector<DataProcessorCommand_uptr> rowCommands) {

  ui.menuRows->clear();
  for (auto &command : rowCommands) {
    addToMenu(ui.menuRows, std::move(command));
  }
}

/**
* Sets all rows in the table view to be selected
*/
void QtReflRunsTabView::setAllSearchRowsSelected() {

  ui.tableSearchResults->selectAll();
}

/**
* Clears all the actions (commands)
*/
void QtReflRunsTabView::clearCommands() { m_commands.clear(); }

void QtReflRunsTabView::enable(QAction &toEnable) { toEnable.setEnabled(true); }

void QtReflRunsTabView::disable(QAction &toDisable) {
  toDisable.setEnabled(false);
}

int QtReflRunsTabView::toRowIndex(DataProcessorAction action) {
  switch (action) {
  case DataProcessorAction::PROCESS:
    return 0;
  case DataProcessorAction::PAUSE:
    return 1;
  case DataProcessorAction::SELECT_GROUP:
    return 3;
  case DataProcessorAction::EXPAND_GROUP:
    return 4;
  case DataProcessorAction::COLAPSE_GROUP:
    return 5;
  case DataProcessorAction::PLOT_RUNS:
    return 7;
  case DataProcessorAction::PLOT_GROUP:
    return 8;
  case DataProcessorAction::INSERT_ROW_AFTER:
    return 10;
  case DataProcessorAction::INSERT_GROUP_AFTER:
    return 11;
  case DataProcessorAction::GROUP_SELECTED:
    return 13;
  case DataProcessorAction::COPY_SELECTED:
    return 14;
  case DataProcessorAction::CUT_SELECTED:
    return 15;
  case DataProcessorAction::PASTE_SELECTED:
    return 16;
  case DataProcessorAction::CLEAR_SELECTED:
    return 17;
  case DataProcessorAction::DELETE_ROW:
    return 19;
  case DataProcessorAction::DELETE_GROUP:
    return 20;
  case DataProcessorAction::WHATS_THIS:
    return 21;
  default:
    throw std::logic_error("Unknown action specified.");
  }
}

int QtReflRunsTabView::toMenuIndex(ReflectometryAction action) {
  switch (action) {
  case ReflectometryAction::OPEN_TABLE:
    return 0;
  case ReflectometryAction::NEW_TABLE:
    return 1;
  case ReflectometryAction::SAVE_TABLE:
    return 2;
  case ReflectometryAction::SAVE_TABLE_AS:
    return 3;
  case ReflectometryAction::IMPORT_TBL:
    return 5;
  case ReflectometryAction::EXPORT_TBL:
    return 6;
  default:
    throw std::logic_error("Unknown action specified.");
  }
}

/**
* Enables a specific action in the "Reflectometry" menu.
* @param action : The action in the "Reflectometry" menu to enable
*/
void QtReflRunsTabView::disableAction(ReflectometryAction action) {
  disable(*(ui.menuTable->actions()[toMenuIndex(action)]));
}

/**
* Enables a specific action in the "Reflectometry" menu.
* @param action : The action in the "Reflectometry" menu to disable
*/
void QtReflRunsTabView::enableAction(ReflectometryAction action) {
  enable(*(ui.menuTable->actions()[toMenuIndex(action)]));
}

/**
* Disables a specific action in the "Edit" menu.
* @param action : The action in the "Edit" menu to disable
*/
void QtReflRunsTabView::disableAction(DataProcessorAction action) {
  disable(*(ui.menuRows->actions()[toRowIndex(action)]));
}

/**
* Enables a specific action in the "Edit" menu.
* @param action : The action in the "Edit" menu to enable
*/
void QtReflRunsTabView::enableAction(DataProcessorAction action) {
  enable(*(ui.menuRows->actions()[toRowIndex(action)]));
}

void QtReflRunsTabView::disableAutoreduce() { setAutoreduceEnabled(false); }

void QtReflRunsTabView::enableAutoreduce() { setAutoreduceEnabled(true); }

void QtReflRunsTabView::setTransferEnabled(bool enabled) {
  ui.buttonTransfer->setEnabled(enabled);
}

void QtReflRunsTabView::disableTransfer() { setTransferEnabled(false); }

void QtReflRunsTabView::enableTransfer() { setTransferEnabled(true); }

/**
* Sets the "Autoreduce" button enabled or disabled
* @param enabled : Whether to enable or disable the button
*/
void QtReflRunsTabView::setAutoreduceEnabled(bool enabled) {
  ui.buttonAutoreduce->setEnabled(enabled);
}

/**
* Set all possible tranfer methods
* @param methods : All possible transfer methods.
*/
void QtReflRunsTabView::setTransferMethods(
    const std::set<std::string> &methods) {
  for (const auto &method : methods)
    ui.comboTransferMethod->addItem(method.c_str());
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

  for (const auto &instrumentName : instruments) {
    QString instrument = QString::fromStdString(instrumentName);
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

/**
This slot notifies the presenter that the ICAT search was completed
*/
void QtReflRunsTabView::icatSearchComplete() {
  m_presenter->notify(IReflRunsTabPresenter::ICATSearchCompleteFlag);
}

/**
This slot notifies the presenter that the "search" button has been pressed
*/
void QtReflRunsTabView::on_actionSearch_triggered() {
  m_algoRunner.get()->disconnect(); // disconnect any other connections
  m_presenter->notify(IReflRunsTabPresenter::SearchFlag);
  connect(m_algoRunner.get(), SIGNAL(algorithmComplete(bool)), this,
          SLOT(icatSearchComplete()), Qt::UniqueConnection);
}

/**
This slot conducts a search operation before notifying the presenter that the
"autoreduce" button has been pressed
*/
void QtReflRunsTabView::on_actionAutoreduce_triggered() {
  // No need to search first if not starting a new autoreduction
  if (m_presenter->startNewAutoreduction()) {
    m_algoRunner.get()->disconnect(); // disconnect any other connections
    m_presenter->notify(IReflRunsTabPresenter::SearchFlag);
    connect(m_algoRunner.get(), SIGNAL(algorithmComplete(bool)), this,
            SLOT(newAutoreduction()), Qt::UniqueConnection);
  } else {
    m_presenter->notify(IReflRunsTabPresenter::ResumeAutoreductionFlag);
  }
}

/**
This slot notifies the presenter that the "transfer" button has been pressed
*/
void QtReflRunsTabView::on_actionTransfer_triggered() {
  m_presenter->notify(IReflRunsTabPresenter::Flag::TransferFlag);
}

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
This notifies the presenter that a new autoreduction has been started
*/
void QtReflRunsTabView::newAutoreduction() {
  m_presenter->notify(IReflRunsTabPresenter::NewAutoreductionFlag);
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

} // namespace CustomInterfaces
} // namespace Mantid
