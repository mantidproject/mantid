#include "MantidQtCustomInterfaces/QtReflMainView.h"
#include "MantidQtCustomInterfaces/QReflTableModel.h"
#include "MantidQtCustomInterfaces/ReflMainViewPresenter.h"
#include "MantidQtMantidWidgets/HintingLineEditFactory.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidKernel/ConfigService.h"
#include <qinputdialog.h>
#include <qmessagebox.h>

namespace MantidQt
{
  namespace CustomInterfaces
  {
    using namespace Mantid::API;

    DECLARE_SUBWINDOW(QtReflMainView);

    //----------------------------------------------------------------------------------------------
    /** Constructor
    */
    QtReflMainView::QtReflMainView(QWidget *parent) : UserSubWindow(parent), m_openMap(new QSignalMapper(this))
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
    */
    QtReflMainView::~QtReflMainView()
    {
    }

    /**
    Initialise the Interface
    */
    void QtReflMainView::initLayout()
    {
      ui.setupUi(this);

      ui.buttonProcess->setDefaultAction(ui.actionProcess);

      //Expand the process runs column at the expense of the search column
      ui.splitterTables->setStretchFactor(0, 0);
      ui.splitterTables->setStretchFactor(1, 1);

      //Allow rows to be reordered
      ui.viewTable->verticalHeader()->setMovable(true);

      //Custom context menu for table
      connect(ui.viewTable, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&)));

      //Finally, create a presenter to do the thinking for us
      m_presenter = boost::shared_ptr<IReflPresenter>(new ReflMainViewPresenter(this));
    }

    /**
    This slot loads a table workspace model and changes to a LoadedMainView presenter
    @param name : the string name of the workspace to be grabbed
    */
    void QtReflMainView::setModel(QString name)
    {
      m_toOpen = name.toStdString();
      m_presenter->notify(OpenTableFlag);
    }

    /**
    Set a new model in the tableview
    @param model : the model to be attached to the tableview
    */
    void QtReflMainView::showTable(QReflTableModel_sptr model)
    {
      m_model = model;
      //So we can notify the presenter when the user updates the table
      connect(m_model.get(), SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(tableUpdated(const QModelIndex&, const QModelIndex&)));
      ui.viewTable->setModel(m_model.get());
      ui.viewTable->resizeColumnsToContents();
    }

    /**
    Set the list of tables the user is offered to open
    @param tables : the names of the tables in the ADS
    */
    void QtReflMainView::setTableList(const std::set<std::string>& tables)
    {
      ui.menuOpenTable->clear();

      for(auto it = tables.begin(); it != tables.end(); ++it)
      {
        QAction* openTable = ui.menuOpenTable->addAction(QString::fromStdString(*it));
        openTable->setIcon(QIcon("://worksheet.png"));

        //Map this action to the table name
        m_openMap->setMapping(openTable, QString::fromStdString(*it));

        connect(openTable, SIGNAL(triggered()), m_openMap, SLOT(map()));
        connect(m_openMap, SIGNAL(mapped(QString)), this, SLOT(setModel(QString)));
      }
    }

    /**
    This slot notifies the presenter that the "save" button has been pressed
    */
    void QtReflMainView::on_actionSaveTable_triggered()
    {
      m_presenter->notify(SaveFlag);
    }

    /**
    This slot notifies the presenter that the "save as" button has been pressed
    */
    void QtReflMainView::on_actionSaveTableAs_triggered()
    {
      m_presenter->notify(SaveAsFlag);
    }

    /**
    This slot notifies the presenter that the "append row" button has been pressed
    */
    void QtReflMainView::on_actionAppendRow_triggered()
    {
      m_presenter->notify(AppendRowFlag);
    }

    /**
    This slot notifies the presenter that the "prepend row" button has been pressed
    */
    void QtReflMainView::on_actionPrependRow_triggered()
    {
      m_presenter->notify(PrependRowFlag);
    }

    /**
    This slot notifies the presenter that the "delete" button has been pressed
    */
    void QtReflMainView::on_actionDeleteRow_triggered()
    {
      m_presenter->notify(DeleteRowFlag);
    }

    /**
    This slot notifies the presenter that the "process" button has been pressed
    */
    void QtReflMainView::on_actionProcess_triggered()
    {
      m_presenter->notify(ProcessFlag);
    }

    /**
    This slot notifies the presenter that the "group rows" button has been pressed
    */
    void QtReflMainView::on_actionGroupRows_triggered()
    {
      m_presenter->notify(GroupRowsFlag);
    }

    /**
    This slot notifies the presenter that the "clear selected" button has been pressed
    */
    void QtReflMainView::on_actionClearSelected_triggered()
    {
      m_presenter->notify(ClearSelectedFlag);
    }

    /**
    This slot notifies the presenter that the "copy selection" button has been pressed
    */
    void QtReflMainView::on_actionCopySelected_triggered()
    {
      m_presenter->notify(CopySelectedFlag);
    }

    /**
    This slot notifies the presenter that the "cut selection" button has been pressed
    */
    void QtReflMainView::on_actionCutSelected_triggered()
    {
      m_presenter->notify(CutSelectedFlag);
    }

    /**
    This slot notifies the presenter that the "paste selection" button has been pressed
    */
    void QtReflMainView::on_actionPasteSelected_triggered()
    {
      m_presenter->notify(PasteSelectedFlag);
    }

    /**
    This slot notifies the presenter that the "new table" button has been pressed
    */
    void QtReflMainView::on_actionNewTable_triggered()
    {
      m_presenter->notify(NewTableFlag);
    }

    /**
    This slot notifies the presenter that the "expand selection" button has been pressed
    */
    void QtReflMainView::on_actionExpandSelection_triggered()
    {
      m_presenter->notify(ExpandSelectionFlag);
    }

    /**
    This slot notifies the presenter that the "options..." button has been pressed
    */
    void QtReflMainView::on_actionOptionsDialog_triggered()
    {
      m_presenter->notify(OptionsDialogFlag);
    }

    /**
    This slot notifies the presenter that the table has been updated/changed by the user
    */
    void QtReflMainView::tableUpdated(const QModelIndex& topLeft, const QModelIndex& bottomRight)
    {
      Q_UNUSED(topLeft);
      Q_UNUSED(bottomRight);
      m_presenter->notify(TableUpdatedFlag);
    }

    /**
    This slot is triggered when the user right clicks on the table
    @param pos : The position of the right click within the table
    */
    void QtReflMainView::showContextMenu(const QPoint& pos)
    {
      //parent widget takes ownership of QMenu
      QMenu* menu = new QMenu(this);
      menu->addAction(ui.actionProcess);
      menu->addAction(ui.actionExpandSelection);
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
    Show an information dialog
    @param prompt : The prompt to appear on the dialog
    @param title : The text for the title bar of the dialog
    */
    void QtReflMainView::giveUserInfo(std::string prompt, std::string title)
    {
      QMessageBox::information(this,QString(title.c_str()),QString(prompt.c_str()),QMessageBox::Ok, QMessageBox::Ok);
    }

    /**
    Show an critical error dialog
    @param prompt : The prompt to appear on the dialog
    @param title : The text for the title bar of the dialog
    */
    void QtReflMainView::giveUserCritical(std::string prompt, std::string title)
    {
      QMessageBox::critical(this,QString(title.c_str()),QString(prompt.c_str()),QMessageBox::Ok, QMessageBox::Ok);
    }

    /**
    Show a warning dialog
    @param prompt : The prompt to appear on the dialog
    @param title : The text for the title bar of the dialog
    */
    void QtReflMainView::giveUserWarning(std::string prompt, std::string title)
    {
      QMessageBox::warning(this,QString(title.c_str()),QString(prompt.c_str()),QMessageBox::Ok, QMessageBox::Ok);
    }

    /**
    Ask the user a Yes/No question
    @param prompt : The prompt to appear on the dialog
    @param title : The text for the title bar of the dialog
    @returns a boolean true if Yes, false if No
    */
    bool QtReflMainView::askUserYesNo(std::string prompt, std::string title)
    {
      auto response = QMessageBox::question(this,QString(title.c_str()),QString(prompt.c_str()),QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes);
      if (response == QMessageBox::Yes)
      {
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
    std::string QtReflMainView::askUserString(const std::string& prompt, const std::string& title, const std::string& defaultValue)
    {
      bool ok;
      QString text = QInputDialog::getText(QString::fromStdString(title), QString::fromStdString(prompt), QLineEdit::Normal, QString::fromStdString(defaultValue), &ok);
      if(ok)
        return text.toStdString();
      return "";
    }

    /**
    Set the range of the progress bar
    @param min : The minimum value of the bar
    @param max : The maxmimum value of the bar
    */
    void QtReflMainView::setProgressRange(int min, int max)
    {
      ui.progressBar->setRange(min, max);
    }

    /**
    Set the status of the progress bar
    @param progress : The current value of the bar
    */
    void QtReflMainView::setProgress(int progress)
    {
      ui.progressBar->setValue(progress);
    }

    /**
    Set which rows are selected
    @param rows : The set of rows to select
    */
    void QtReflMainView::setSelection(const std::set<int>& rows)
    {
      ui.viewTable->clearSelection();
      auto selectionModel = ui.viewTable->selectionModel();

      for(auto row = rows.begin(); row != rows.end(); ++row)
        selectionModel->select(ui.viewTable->model()->index((*row), 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
    }

    /**
    Set the list of available instruments to search and process for
    @param instruments : The list of instruments available
    @param defaultInstrument : The instrument to have selected by default
    */
    void QtReflMainView::setInstrumentList(const std::vector<std::string>& instruments, const std::string& defaultInstrument)
    {
      ui.comboSearchInstrument->clear();
      ui.comboProcessInstrument->clear();

      for(auto it = instruments.begin(); it != instruments.end(); ++it)
      {
        QString instrument = QString::fromStdString(*it);
        ui.comboSearchInstrument->addItem(instrument);
        ui.comboProcessInstrument->addItem(instrument);
      }

      int index = ui.comboSearchInstrument->findData(QString::fromStdString(defaultInstrument), Qt::DisplayRole);
      ui.comboSearchInstrument->setCurrentIndex(index);
      ui.comboProcessInstrument->setCurrentIndex(index);
    }

    /**
    Set the strategy used for generating hints for the autocompletion in the options column.
    @param hintStrategy The hinting strategy to use
    */
    void QtReflMainView::setOptionsHintStrategy(HintStrategy* hintStrategy)
    {
      ui.viewTable->setItemDelegateForColumn(ReflMainViewPresenter::COL_OPTIONS, new HintingLineEditFactory(hintStrategy));
    }

    /**
    Get the selected instrument for searching
    @returns the selected instrument to search for
    */
    std::string QtReflMainView::getSearchInstrument() const
    {
      return ui.comboSearchInstrument->currentText().toStdString();
    }

    /**
    Get the selected instrument for processing
    @returns the selected instrument to process with
    */
    std::string QtReflMainView::getProcessInstrument() const
    {
      return ui.comboProcessInstrument->currentText().toStdString();
    }

    /**
    Get the indices of the highlighted rows
    @returns a vector of unsigned ints contianing the highlighted row numbers
    */
    std::set<int> QtReflMainView::getSelectedRows() const
    {
      auto selectedRows = ui.viewTable->selectionModel()->selectedRows();
      std::set<int> rows;
      for(auto it = selectedRows.begin(); it != selectedRows.end(); ++it)
        rows.insert(it->row());

      return rows;
    }

    /**
    Get the name of the workspace that the user wishes to open as a table
    @returns The name of the workspace to open
    */
    std::string QtReflMainView::getWorkspaceToOpen() const
    {
      return m_toOpen;
    }

    /**
    Get a pointer to the presenter that's currently controlling this view.
    @returns A pointer to the presenter
    */
    boost::shared_ptr<IReflPresenter> QtReflMainView::getPresenter() const
    {
      return m_presenter;
    }

  } // namespace CustomInterfaces
} // namespace Mantid
