//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/StandardLogView.h"
#include "MantidQtCustomInterfaces/LogPresenter.h"
#include <qgridlayout.h>
#include <qtablewidget.h>

namespace MantidQt
{
  namespace CustomInterfaces
  {
    /// Constructor
    StandardLogView::StandardLogView(LogPresenter* presenter) : m_presenter(presenter)
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
     QTableWidget* tableWidget = new QTableWidget(this);
     tableWidget->setRowCount(logs.size());
     tableWidget->setColumnCount(2);

      for(int i = 0; i < logs.size(); i++)
      {
        std::string value;
        logs[i]->getValue(value);

        std::string name = logs[i]->getName();

        QTableWidgetItem *nameItem = new QTableWidgetItem(name.c_str());
        QTableWidgetItem *valueItem = new QTableWidgetItem(value.c_str());

        tableWidget->setItem(i, 0, nameItem);
        tableWidget->setItem(i, 1, valueItem);
      }

      QGridLayout* layout = new QGridLayout();
      layout->addWidget(tableWidget);
      this->setLayout(layout);
    }

    /// Getter for the log data.
    LogDataMap StandardLogView::getLogData() const
    {
      LogDataMap map; //HACK
      return map;
    }

    void StandardLogView::edited()
    {
    }

  }
}