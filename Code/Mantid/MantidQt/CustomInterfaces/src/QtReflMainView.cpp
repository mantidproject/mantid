#include "MantidQtCustomInterfaces/QtReflMainView.h"
#include "MantidQtCustomInterfaces/QReflTableModel.h"
#include "MantidQtCustomInterfaces/ReflNullMainViewPresenter.h"
#include "MantidQtCustomInterfaces/ReflMainViewPresenter.h"
#include "MantidQtCustomInterfaces/ReflBlankMainViewPresenter.h"
#include "MantidQtCustomInterfaces/ReflLoadedMainViewPresenter.h"
#include "MantidAPI/ITableWorkspace.h"
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
    QtReflMainView::QtReflMainView(QWidget *parent) : UserSubWindow(parent), m_save_flag(false), m_saveAs_flag(false),
    m_addRow_flag(false), m_deleteRow_flag(false), m_process_flag(false), m_presenter(new ReflNullMainViewPresenter())
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
      ui.workspaceSelector->refresh();
      connect(ui.workspaceSelector,SIGNAL(activated(QString)),this,SLOT(setModel(QString)));
      connect(ui.buttonSave, SIGNAL(clicked()),this, SLOT(saveButton()));
      connect(ui.buttonSaveAs, SIGNAL(clicked()),this, SLOT(saveAsButton()));
      connect(ui.buttonNew, SIGNAL(clicked()),this, SLOT(setNew()));
      connect(ui.buttonAddRow, SIGNAL(clicked()),this, SLOT(addRowButton()));
      connect(ui.buttonDeleteRow, SIGNAL(clicked()),this, SLOT(deleteRowButton()));
      connect(ui.buttonProcess, SIGNAL(clicked()),this, SLOT(processButton()));
      setNew();
    }

    /**
    Start a new table
    */
    void QtReflMainView::setNew()
    {
      boost::scoped_ptr<IReflPresenter> newPtr(new ReflBlankMainViewPresenter(this));
      m_presenter.swap(newPtr);
    }

    /**
    Laod a different model
    @param name : the string name of the workspace to be grabbed
    */
    void QtReflMainView::setModel(QString name)
    {
      boost::scoped_ptr<IReflPresenter> newPtr(new ReflLoadedMainViewPresenter(name.toStdString(), this));
      m_presenter.swap(newPtr);
      m_presenter->notify();
    }

    /**
    Set a new model in the tableview
    @param model : the model to be attached to the tableview
    */
    void QtReflMainView::showTable(ITableWorkspace_sptr model)
    {
      ui.viewTable->setModel(new QReflTableModel(model));
      ui.viewTable->resizeColumnsToContents();
    }

    /**
    Set the save flag then notify the presenter
    */
    void QtReflMainView::saveButton()
    {
      m_save_flag = true;
      m_presenter->notify();
    }

    /**
    Set the save as flag then notify the presenter
    */
    void QtReflMainView::saveAsButton()
    {
      m_saveAs_flag = true;
      m_presenter->notify();
    }

    /**
    Set the add row flag then notify the presenter
    */
    void QtReflMainView::addRowButton()
    {
      m_addRow_flag = true;
      m_presenter->notify();
    }

    /**
    Set the delete flag then notify the presenter
    */
    void QtReflMainView::deleteRowButton()
    {
      m_deleteRow_flag = true;
      m_presenter->notify();
    }

    /**
    Set the process flag then notify the presenter
    */
    void QtReflMainView::processButton()
    {
      m_process_flag = true;
      m_presenter->notify();
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
    ask user a Yes/No question
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
    ask user a Yes/No question
    @param prompt : The prompt to appear on the dialog
    @param title : The text for the title bar of the dialog
    @returns a boolean true if Yes, false if No
    */
    bool QtReflMainView::askUserString()
    {
      bool ok;
      QString text = QInputDialog::getText(QString("Enter a value"), QString("Workspace name:"), QLineEdit::Normal, QString("Workspace"), &ok);
      if (ok && !text.isEmpty())
      {
        m_UserString = text.toStdString();
      }
      return ok;
    }

    /**
    get the indeces of the highlighted rows
    @returns a vector of unsigned ints contianing the highlighted row numbers
    */
    std::vector<size_t> QtReflMainView::getSelectedRowIndexes() const
    {
      auto selectedRows = ui.viewTable->selectionModel()->selectedRows();
      //auto selectedType = ui.viewTable->selectionModel()->;
      std::vector<size_t> rowIndexes;
      for (auto idx = selectedRows.begin(); idx != selectedRows.end(); ++idx)
      {
        rowIndexes.push_back(idx->row());
      }
      return rowIndexes;
    }

    /**
    Clear all notification flags
    */
    void QtReflMainView::clearNotifyFlags()
    {
      m_save_flag = false;
      m_saveAs_flag = false;
      m_addRow_flag = false;
      m_deleteRow_flag = false;
      m_process_flag = false;
    }
  } // namespace CustomInterfaces
} // namespace Mantid
