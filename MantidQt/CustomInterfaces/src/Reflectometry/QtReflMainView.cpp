#include "MantidQtCustomInterfaces/Reflectometry/QtReflMainView.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtAPI/FileDialogHandler.h"
#include "MantidQtAPI/HelpWindow.h"
#include "MantidQtCustomInterfaces/Reflectometry/QReflTableModel.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflMainViewPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflTableSchema.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflTableViewPresenter.h"
#include "MantidQtMantidWidgets/HintingLineEditFactory.h"
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
      m_calculator(new MantidWidgets::SlitCalculator(this)) {}

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
  // Finally, create the presenters to do the thinking for us
  m_presenter = boost::make_shared<ReflMainViewPresenter>(this /*main view*/);
  m_algoRunner = boost::make_shared<MantidQt::API::AlgorithmRunner>(this);
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
This slot requests the list of runs to transfer to ReflMainViewPresenter
Then updates ReflTableViewPresenter
*/
void QtReflMainView::on_actionTransfer_triggered() {
  // m_presenter->notify(IReflPresenter::TransferFlag);
  // TODO: see below
  // auto runs = m_presenter->getRunsToTransfer();
  // m_tablePresenter->transfer(runs);
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
  // TODO: see below
  // m_calculator->setCurrentInstrumentName(
  //    ui.comboProcessInstrument->currentText().toStdString());
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

} // namespace CustomInterfaces
} // namespace Mantid
