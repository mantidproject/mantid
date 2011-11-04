//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/StandardLogView.h"
#include "MantidQtCustomInterfaces/LogPresenter.h"
#include <qtablewidget.h>
#include <qpushbutton.h>
#include <qboxlayout.h>

namespace MantidQt
{
  namespace CustomInterfaces
  {
    /// Constructor
    StandardLogView::StandardLogView(boost::shared_ptr<LogPresenter> presenter) : m_presenter(presenter), m_request_edit(false), m_tableWidget(new QTableWidget(this))
    {
      presenter->acceptView(this);
    }

    /// Destructor
    StandardLogView::~StandardLogView()
    {
    }

    /// Indicate that the view has been modified.
    void StandardLogView::indicateModified()
    {
    }

    /// Indicate that the view is unmodified.
    void StandardLogView::indicateDefault()
    {
    }

    /// Initalization method.
    void StandardLogView::initalize(std::vector<AbstractMementoItem_sptr> logs)
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

      QPushButton* btnEdit = new QPushButton("Edit");
      connect(btnEdit, SIGNAL(clicked()), this, SLOT(edited()));

      QVBoxLayout* layout = new QVBoxLayout();
      layout->addWidget(m_tableWidget);
      layout->addWidget(btnEdit);
      this->setLayout(layout);
      m_request_edit = false;
    }

    /** Getter for the log data.
    @return LogDataMap containing the log data.
    */
    LogDataMap StandardLogView::getLogData() const
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
    bool StandardLogView::getRequestEdit() const
    {
      return m_request_edit;
    }

    /// Listener for the edit button click event.
    void StandardLogView::edited()
    {
      m_request_edit = true;
      m_presenter->update();
    }

  }
}