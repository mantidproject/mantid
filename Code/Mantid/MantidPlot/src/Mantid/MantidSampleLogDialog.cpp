//----------------------------------
// Includes
//----------------------------------
#include "MantidSampleLogDialog.h"
#include "MantidUI.h"

#include "MantidKernel/TimeSeriesProperty.h"

#include <QTreeWidgetItem>
#include <QTreeWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QMenu>
#include <QAction>
#include <QGroupBox>
#include <QRadioButton>

#include <QMessageBox>
#include <iostream>
#include <sstream>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;

//----------------------------------
// Public methods
//----------------------------------
/**
 * Construct an object of this type
 * @param wsname :: The name of the workspace object from which to retrieve the log files
 * @param mui :: The MantidUI area
 * @param flags :: Window flags that are passed the the QDialog constructor
 */
MantidSampleLogDialog::MantidSampleLogDialog(const QString & wsname, MantidUI* mui, Qt::WFlags flags)  :
  QDialog(mui->appWindow(), flags), m_wsname(wsname), m_mantidUI(mui)
{
  setWindowTitle(tr("MantidPlot - " + wsname + " sample log files"));
  
  m_tree = new QTreeWidget;
  QStringList titles;
  titles << "Name" << "Type" << "Value" << "Units";
  m_tree->setHeaderLabels(titles);
  m_tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
  QHeaderView* hHeader = (QHeaderView*)m_tree->header();
  hHeader->setResizeMode(2,QHeaderView::Stretch);
  hHeader->setStretchLastSection(false);

  QHBoxLayout *uiLayout = new QHBoxLayout;
  uiLayout->addWidget(m_tree);

  // ----- Filtering options --------------
  QGroupBox *groupBox = new QGroupBox(tr("Filter log values by"));

  filterNone = new QRadioButton("None");
  filterStatus = new QRadioButton("Status");
  filterPeriod = new QRadioButton("Period");
  filterStatusPeriod = new QRadioButton("Status + Period");
  filterStatusPeriod->setChecked(true);

  QVBoxLayout *vbox = new QVBoxLayout;
  vbox->addWidget(filterNone);
  vbox->addWidget(filterStatus);
  vbox->addWidget(filterPeriod);
  vbox->addWidget(filterStatusPeriod);
  //vbox->addStretch(1);
  groupBox->setLayout(vbox);

  // -------------- Statistics on logs ------------------------
  std::string stats[5] = {"Min:", "Max:", "Mean:", "Median:" ,"Std Dev:"};
  QGroupBox * statsBox = new QGroupBox("Log Statistics");
  QFormLayout * statsBoxLayout = new QFormLayout;
  for (size_t i=0; i<5; i++)
  {
    statLabels[i] = new QLabel(stats[i].c_str());
    statValues[i] = new QLineEdit("");
    statValues[i]->setReadOnly(1);
    statsBoxLayout->addRow(statLabels[i], statValues[i]);
  }
  statsBox->setLayout(statsBoxLayout);


  QHBoxLayout *bottomButtons = new QHBoxLayout;
  buttonPlot = new QPushButton(tr("&Import selected log"));
  buttonPlot->setAutoDefault(true);
  buttonPlot->setToolTip("Import log file as a table and construct a 1D graph if appropriate");
  bottomButtons->addWidget(buttonPlot);

  buttonClose = new QPushButton(tr("Close"));
  buttonClose->setToolTip("Close dialog");
  bottomButtons->addWidget(buttonClose);

  QVBoxLayout *hbox = new QVBoxLayout;
  hbox->addLayout(bottomButtons);
  hbox->addWidget(groupBox);
  hbox->addWidget(statsBox);
  hbox->addStretch(1);
     

  //--- Main layout With 2 sides -----
  QHBoxLayout *mainLayout = new QHBoxLayout(this);
  mainLayout->addLayout(uiLayout, 1); // the tree
  mainLayout->addLayout(hbox, 0);
  //mainLayout->addLayout(bottomButtons);
  this->setLayout(mainLayout);

  init();

  resize(750,400);
  
  connect(buttonPlot, SIGNAL(clicked()), this, SLOT(importSelectedFiles()));
  connect(buttonClose, SIGNAL(clicked()), this, SLOT(close()));
  //want a custom context menu
  m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_tree, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(popupMenu(const QPoint &)));

  //Double-click imports a log file
  connect(m_tree, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(importItem(QTreeWidgetItem *)));

  //Selecting shows the stats of it
  connect(m_tree, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(showLogStatistics()));

  //Selecting shows the stats of it
  connect(m_tree, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)), this, SLOT(showLogStatistics()));
}

//----------------------------------
// Private methods
//----------------------------------
/**
 * Plot the selected log entries (TimeSeriesProperty or PropertyWithValue)
 */
void MantidSampleLogDialog::importSelectedFiles()
{
  QList<QTreeWidgetItem *> items = m_tree->selectedItems();
  QListIterator<QTreeWidgetItem *> pItr(items);
  while( pItr.hasNext() )
  {
    importItem(pItr.next());
  }
}



/**
 * Show Log Statistics when a line is selected
 */
void MantidSampleLogDialog::showLogStatistics()
{
  QList<QTreeWidgetItem *> items = m_tree->selectedItems();
  QListIterator<QTreeWidgetItem *> pItr(items);
  if( pItr.hasNext() )
  {
    // Show only the first one
    showLogStatisticsOfItem(pItr.next());
  }
}


//------------------------------------------------------------------------------------------------
/**
* Show the stats of the log for the selected item
*
* @param item :: The item to be imported
* @throw invalid_argument if format identifier for the item is wrong
*/
void MantidSampleLogDialog::showLogStatisticsOfItem(QTreeWidgetItem * item)
{
  // Assume that you can't show the stats
  for (size_t i=0; i<5; i++)
  {
    statValues[i]->setText( QString(""));
  }

  //used in numeric time series below, the default filter value
  switch (item->data(1, Qt::UserRole).toInt())
  {
    case numeric :
    case string :
    case stringTSeries :
      return;
      break;

    case numTSeries :
      // Calculate the stats

      // Get the workspace
      if (!AnalysisDataService::Instance().doesExist(m_wsname.toStdString()))  return;
      Mantid::API::MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
          AnalysisDataService::Instance().retrieve(m_wsname.toStdString() ));
      if (!ws) return;

      // Now the log
      Mantid::Kernel::TimeSeriesPropertyStatistics stats;
      Mantid::Kernel::Property * logData = ws->run().getLogData(item->text(0).toStdString());
      // Get the stas if its a series of int or double; fail otherwise
      Mantid::Kernel::TimeSeriesProperty<double> * tspd = dynamic_cast<TimeSeriesProperty<double> *>(logData);
      Mantid::Kernel::TimeSeriesProperty<int> * tspi = dynamic_cast<TimeSeriesProperty<int> *>(logData);
      if (tspd)
        stats = tspd->getStatistics();
      else if (tspi)
        stats = tspi->getStatistics();
      else
        return;

      // --- Show the stats ---
      statValues[0]->setText( QString::number( stats.minimum ));
      statValues[1]->setText( QString::number( stats.maximum ));
      statValues[2]->setText( QString::number( stats.mean ));
      statValues[3]->setText( QString::number( stats.median ));
      statValues[4]->setText( QString::number( stats.standard_deviation ));
      return;
      break;
  }
  throw std::invalid_argument("Error importing log entry, wrong data type");
}

//------------------------------------------------------------------------------------------------
/**
* Import an item from sample logs
*
* @param item :: The item to be imported
* @throw invalid_argument if format identifier for the item is wrong
*/
void MantidSampleLogDialog::importItem(QTreeWidgetItem * item)
{
  //used in numeric time series below, the default filter value
  int filter = 0;

  switch (item->data(1, Qt::UserRole).toInt())
  {
    case numeric :
    case string :
      m_mantidUI->importString(item->text(0),
        item->data(0, Qt::UserRole).toString()); //Pretty much just print out the string
      break;
    case numTSeries :
      if (filterStatus->isChecked()) filter = 1;
      if (filterPeriod->isChecked()) filter = 2;
      if (filterStatusPeriod->isChecked()) filter = 3;
      m_mantidUI->importNumSeriesLog(m_wsname, item->text(0), filter);
      break;
    case stringTSeries :
      m_mantidUI->importStrSeriesLog(item->text(0),
                                     item->data(0, Qt::UserRole).toString());
      break;
    default :
      throw std::invalid_argument("Error importing log entry, wrong data type");
  }
}


//------------------------------------------------------------------------------------------------
/**
 * Popup a custom context menu
 */
void MantidSampleLogDialog::popupMenu(const QPoint & pos)
{
  if( !m_tree->itemAt(pos) ) 
  {
    m_tree->selectionModel()->clear();
    return;
  }

  QMenu *menu = new QMenu(m_tree);
  
  QAction *action = new QAction("Import", m_tree);
  connect(action, SIGNAL(triggered()), this, SLOT(importSelectedFiles()));
  menu->addAction(action);
  
  menu->popup(QCursor::pos());

}

//------------------------------------------------------------------------------------------------
/**
 * Initialize everything
 */
void MantidSampleLogDialog::init()
{
  m_tree->clear();
  Mantid::API::MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(m_mantidUI->getWorkspace(m_wsname));
  if (!ws)
  {
      throw std::runtime_error("Wrong type of a Workspace");
  }
  const std::vector< Mantid::Kernel::Property * > & logData = ws->run().getLogData();
  std::vector< Mantid::Kernel::Property * >::const_iterator pEnd = logData.end();
  int max_length(0);
  for( std::vector< Mantid::Kernel::Property * >::const_iterator pItr = logData.begin();
       pItr != pEnd; ++pItr )
  {
    //name() contains the full path, so strip to file name
    QString filename = QFileInfo((**pItr).name().c_str()).fileName();
    if( filename.size() > max_length ) max_length = filename.size();
    QTreeWidgetItem *treeItem = new QTreeWidgetItem(QStringList(filename));

    //store the log contents in the treeItem
    //treeItem->setData(0, Qt::UserRole, QString::fromStdString((*pItr)->value()));

    //NOTE: The line above appears to be completely unused since it is overwritten. And it is real slow.
    //  So commented out, and putting this placeholder instead
    treeItem->setData(0, Qt::UserRole, "value");

    //Set the units text
    treeItem->setText(3, QString::fromStdString((*pItr)->units()));

    //this specifies the format of the data it should be overridden below or there is a problem
    treeItem->setData(1, Qt::UserRole, -1);

    Mantid::Kernel::TimeSeriesProperty<double> *tspd = dynamic_cast<Mantid::Kernel::TimeSeriesProperty<double> *>(*pItr);
    Mantid::Kernel::TimeSeriesProperty<int>    *tspi = dynamic_cast<Mantid::Kernel::TimeSeriesProperty<int> *>(*pItr);
    Mantid::Kernel::TimeSeriesProperty<bool>   *tspb = dynamic_cast<Mantid::Kernel::TimeSeriesProperty<bool> *>(*pItr);

    //See what type of data we have    
    if( tspd || tspi || tspb )
    {
      treeItem->setText(1, "num. series");
      //state that the string we passed into data[0] is a time series -multiple lines with a time and then a number
      treeItem->setData(1, Qt::UserRole, static_cast<int>(numTSeries));
      std::ostringstream msg;
      if ((*pItr)->size() == 1)
      {
        //Print out the only entry
        if (tspd)
          msg << tspd->nthValue(0);
        else if (tspi)
          msg << tspi->nthValue(0);
        else if (tspb)
          msg << tspb->nthValue(0);
      }
      else
      {
        //Show the # of entries
        msg << "(" << (*pItr)->size() << " entries)";
      }
      treeItem->setText(2, QString::fromStdString( msg.str()) );
    }
    else if( dynamic_cast<Mantid::Kernel::TimeSeriesProperty<std::string> *>(*pItr) )
    {
      treeItem->setText(1, "str. series");
      treeItem->setData(1, Qt::UserRole, static_cast<int>(stringTSeries));
      treeItem->setData(0, Qt::UserRole, QString::fromStdString((*pItr)->value()));
      std::ostringstream msg;
      if ((*pItr)->size() == 1)
      {
        //Print out the only entry
        (dynamic_cast<Mantid::Kernel::TimeSeriesProperty<std::string> *>(*pItr))->nthValue(1);
      }
      else
      {
        //Show the # of entries
        msg << "(" << (*pItr)->size() << " entries)";
      }
      treeItem->setText(2, QString::fromStdString( msg.str()) );
    }
    else if( dynamic_cast<Mantid::Kernel::PropertyWithValue<std::string> *>(*pItr) )
    {
      treeItem->setText(1, "string");
      treeItem->setData(1, Qt::UserRole, static_cast<int>(string));
      treeItem->setData(0, Qt::UserRole, QString::fromStdString((*pItr)->value()));
      treeItem->setText(2, QString::fromStdString((*pItr)->value()));

    }
    else if( dynamic_cast<Mantid::Kernel::PropertyWithValue<int> *>(*pItr) ||
             dynamic_cast<Mantid::Kernel::PropertyWithValue<double> *>(*pItr))
    {
      treeItem->setText(1, "numeric");
      treeItem->setData(1, Qt::UserRole, static_cast<int>(numeric)); //Save the "role" as numeric.
      treeItem->setData(0, Qt::UserRole, QString::fromStdString((*pItr)->value()));
      treeItem->setText(2, QString::fromStdString((*pItr)->value()));
    }

    //Add tree item
    m_tree->addTopLevelItem(treeItem);
  }
  
  //Resize the columns
  m_tree->header()->resizeSection(0, max_length*10);
  m_tree->header()->resizeSection(1, 100);
  m_tree->header()->resizeSection(2, 170);
  m_tree->header()->resizeSection(3, 90); //units column
  m_tree->header()->setMovable(false);
  m_tree->setSortingEnabled(true);
}
