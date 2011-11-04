#include "MantidQtCustomInterfaces/EditableLogView.h"
#include "MantidQtCustomInterfaces/LogPresenter.h"
#include "MantidKernel/System.h"
#include <qboxlayout.h>
#include <qpushbutton.h>
#include <qtablewidget.h>


using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace MantidQt
{
  namespace CustomInterfaces
  {
    //----------------------------------------------------------------------------------------------
    /** Constructor
    */
    EditableLogView::EditableLogView(boost::shared_ptr<LogPresenter>  presenter) : m_presenter(presenter), m_request_close(false), m_tableWidget(new QTableWidget(this))
    {
      presenter->acceptEditableView(this);
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
    */
    EditableLogView::~EditableLogView()
    {
    }

    /// Indicate that the view has been modified.
    void EditableLogView::indicateModified()
    {
      //TODO
    }

    /// Indicate that the view is unmodified.
    void EditableLogView::indicateDefault()
    {
      //TODO
    }

    /// Initalization method.
    void EditableLogView::initalize(std::vector<AbstractMementoItem_sptr> logs)
    {
      int logsSize = int(logs.size());
      m_tableWidget->setRowCount(logsSize);
      m_tableWidget->setColumnCount(2);

      //Populate the tree with log names and values
      for(int i = 0; i < logsSize; i++)
      {
        std::string value;
        logs[i]->getValue(value);

        std::string name = logs[i]->getName();

        QTableWidgetItem *nameItem = new QTableWidgetItem(name.c_str());
        QTableWidgetItem *valueItem = new QTableWidgetItem(value.c_str());

        m_tableWidget->setItem(i, 0, nameItem);
        m_tableWidget->setItem(i, 1, valueItem);
      }

      QPushButton* btnEdit = new QPushButton("Close");
      connect(btnEdit, SIGNAL(clicked()), this, SLOT(close()));

      QVBoxLayout* layout = new QVBoxLayout();
      layout->addWidget(m_tableWidget);
      layout->addWidget(btnEdit);
      this->setLayout(layout);
      m_request_close = false;
    }

    /** Getter for the log data.
    @return LogDataMap containing the log data.
    */
    LogDataMap EditableLogView::getLogData() const
    {
      LogDataMap result;
      for(int i = 0; i < m_tableWidget->rowCount(); i++)
      {
        std::string name = m_tableWidget->item(i, 0)->text().toStdString();
        std::string value = m_tableWidget->item(i, 1)->text().toStdString();
        result.insert(std::make_pair(name, value));
      }
      return result;
    }

    /** Getter for the edit request status.
    @return true if an edit request has been made.
    */
    bool EditableLogView::swapMode() const
    {
      return m_request_close;
    }

    /// Listener for the edit button click event.
    void EditableLogView::close()
    {
      m_request_close = true;
      m_presenter->update();
    }

    /// Show the widget.
    void EditableLogView::show()
    {
      QWidget::show();
    }

    /// Hide the widget
    void EditableLogView::hide()
    {
      QWidget::hide();
    }



  } // namespace Mantid
} // namespace CustomInterfaces
