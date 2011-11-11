#include "MantidQtCustomInterfaces/EditableLogView.h"
#include "MantidQtCustomInterfaces/LogPresenter.h"
#include "MantidKernel/System.h"

#include <QGridLayout>
#include <QTableWidget>
#include <QPushButton>
#include <QBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>


using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace MantidQt
{
  namespace CustomInterfaces
  {
    //----------------------------------------------------------------------------------------------
    /** Constructor
    */
    EditableLogView::EditableLogView(boost::shared_ptr<LogPresenter>  presenter) : m_presenter(presenter), m_status(no_change)
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

    /**
    Initalization method.
    @param logs : log values to initalise from.
    */
    void EditableLogView::initalize(std::vector<AbstractMementoItem_sptr> logs)
    {
      m_tableWidget = new QTableWidget(this);
      m_tableWidget->horizontalHeader()->setStretchLastSection(true);
      m_txtName = new QLineEdit();
      m_txtValue = new QLineEdit();

      int logsSize = int(logs.size());
      m_tableWidget->setColumnCount(2);
      //Populate the tree with log names and values
      for(int i = 0; i < logsSize; i++)
      {
        std::string value;
        logs[i]->getValue(value);

        std::string name = logs[i]->getName();

        //Add the row to the QTableWidget.
        addRow(name, value, i);
      }

      QHBoxLayout* editLayout = new QHBoxLayout;
      editLayout->addWidget(new QLabel("Log"));
      editLayout->addWidget(m_txtName);
      editLayout->addWidget(new QLabel("Value"));
      editLayout->addWidget(m_txtValue);

      QPushButton* addBtn = new QPushButton("Add");
      QPushButton* updateBtn = new QPushButton("Update");
      QPushButton* removeBtn = new QPushButton("Remove");

      connect(addBtn, SIGNAL(clicked()), this, SLOT(add()));
      connect(updateBtn, SIGNAL(clicked()), this, SLOT(update()));
      connect(removeBtn, SIGNAL(clicked()), this, SLOT(remove()));

      editLayout->addWidget(addBtn);
      editLayout->addWidget(updateBtn);
      editLayout->addWidget(removeBtn);

      QPushButton* btnCancel = new QPushButton("Cancel");
      connect(btnCancel, SIGNAL(clicked()), this, SLOT(cancel()));

      QPushButton* btnOK = new QPushButton("OK");
      connect(btnOK, SIGNAL(clicked()), this, SLOT(ok()));

      QVBoxLayout* layout = new QVBoxLayout();
      layout->addWidget(m_tableWidget);
      layout->addLayout(editLayout);
      layout->addWidget(btnCancel);
      layout->addWidget(btnOK);
      this->setLayout(layout);
      //Clean/reset output result.
      m_status = no_change;
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

    /// Listener for the edit button click event.
    void EditableLogView::cancel()
    {
      //Remove any new logs!
      m_status = cancelling_mode;
      m_presenter->update();
    }

    /// Listener for the edit button click event.
    void EditableLogView::ok()
    {
      m_status = saving_mode;
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

    /**
    Getter for the LogViewStatus
    @return current LogViewStatus
    */
    LogViewStatus EditableLogView::fetchStatus() const
    {
      return m_status;
    }

    /*
    Helper method to add a row to the QTableWidget.
    @param name : name of the item to add.
    @param value : value of the item to add.
    @param row : row index of QTableWidget.
    */
    void EditableLogView::addRow(const std::string& name, const std::string& value, const int& row)
    {
      QTableWidgetItem *nameItem = new QTableWidgetItem(name.c_str());
      QTableWidgetItem *valueItem = new QTableWidgetItem(value.c_str());
      m_tableWidget->insertRow(row);
      m_tableWidget->setItem(row, 0, nameItem); // Name goes in first column
      m_tableWidget->setItem(row, 1, valueItem); // Value goes in second column
    }

    /// Add a new item
    void EditableLogView::add()
    {
      int rowCount = m_tableWidget->rowCount();
      addRow(m_txtName->text().toStdString(), m_txtValue->text().toStdString(), rowCount);
    }

    /// Edit an existing item
    void EditableLogView::update()
    {
      int row = m_tableWidget->currentRow();
      m_tableWidget->setItem(row, 1, new QTableWidgetItem(m_txtValue->text()));
    }

    /// Remove an existing item
    void EditableLogView::remove()
    {
      //Remove item by name.
    }



  } // namespace Mantid
} // namespace CustomInterfaces
