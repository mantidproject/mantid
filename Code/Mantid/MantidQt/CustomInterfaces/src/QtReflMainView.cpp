#include "MantidQtCustomInterfaces/QtReflMainView.h"
#include "MantidQtCustomInterfaces/QReflTableModel.h"
#include "MantidQtCustomInterfaces/ReflNullMainViewPresenter.h"
#include "MantidQtCustomInterfaces/ReflMainViewPresenter.h"
#include "MantidQtCustomInterfaces/ReflBlankMainViewPresenter.h"
#include "MantidQtCustomInterfaces/ReflLoadedMainViewPresenter.h"
#include "MantidAPI/ITableWorkspace.h"
#include <qinputdialog.h>

namespace MantidQt
{
  namespace CustomInterfaces
  {
    using namespace Mantid::API;

    DECLARE_SUBWINDOW(QtReflMainView);

    //----------------------------------------------------------------------------------------------
    /** Constructor
    */
    QtReflMainView::QtReflMainView(QWidget *parent) : UserSubWindow(parent), m_presenter(new ReflNullMainViewPresenter()), m_save_flag(false), m_saveAs_flag(false)
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
    */
    QtReflMainView::~QtReflMainView()
    {
    }

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

    void QtReflMainView::setNew()
    {
      m_presenter.swap(boost::scoped_ptr<IReflPresenter>(new ReflBlankMainViewPresenter(this)));
    }

    void QtReflMainView::setModel(QString name)
    {
      m_presenter.swap(boost::scoped_ptr<IReflPresenter>(new ReflLoadedMainViewPresenter(name.toStdString(),this)));
      m_presenter->notify();
    }

    void QtReflMainView::showTable(ITableWorkspace_sptr model)
    {
      ui.viewTable->setModel(new QReflTableModel(model));
      ui.viewTable->resizeColumnsToContents();
    }

    void QtReflMainView::saveButton()
    {
      m_save_flag = true;
      m_presenter->notify();
    }

    void QtReflMainView::saveAsButton()
    {
      m_saveAs_flag = true;
      m_presenter->notify();
    }

    void QtReflMainView::addRowButton()
    {
      m_addRow_flag = true;
      m_presenter->notify();
    }

    void QtReflMainView::deleteRowButton()
    {
      m_deleteRow_flag = true;
      m_presenter->notify();
    }

    void QtReflMainView::processButton()
    {
      m_process_flag = true;
      m_presenter->notify();
    }

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
