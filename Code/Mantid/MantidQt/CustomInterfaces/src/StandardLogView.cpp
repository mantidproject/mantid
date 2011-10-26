//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/StandardLogView.h"
#include "MantidQtCustomInterfaces/LogPresenter.h"
#include <qgridlayout.h>
#include <qlistwidget.h>

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
      QListWidget *listWidget = new QListWidget(this);

      for(int i = 0; i < logs.size(); i++)
      {
        QListWidgetItem *newItem = new QListWidgetItem;
        std::string temp;
        logs[i]->getValue(temp);
        newItem->setText(temp.c_str());
        listWidget->insertItem(i, newItem);
      }
      QGridLayout* layout = new QGridLayout();
      layout->addWidget(listWidget);
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