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
#include <QMenu>
#include <QAction>

#include <QMessageBox>
#include <iostream>
//----------------------------------
// Public methods
//----------------------------------
/**
 * Construct an object of this type
 * @param wsname The name of the workspace object from which to retrieve the log files
 * @param mui The MantidUI area
 * @param flags Window flags that are passed the the QDialog constructor
 */
MantidSampleLogDialog::MantidSampleLogDialog(const QString & wsname, MantidUI* mui, Qt::WFlags flags)  :
  QDialog(mui->appWindow(), flags), m_wsname(wsname), m_mantidUI(mui)
{
  setWindowTitle(tr("MantidPlot - " + wsname + " sample log files"));
  
  m_tree = new QTreeWidget;
  QStringList titles;
  titles << "File name" << "Type";
  m_tree->setHeaderLabels(titles);
  m_tree->setSelectionMode(QAbstractItemView::ExtendedSelection);

  QHBoxLayout *uiLayout = new QHBoxLayout;
  uiLayout->addWidget(m_tree);

  QHBoxLayout *bottomButtons = new QHBoxLayout;
  
  buttonPlot = new QPushButton(tr("&Import selected log"));
  buttonPlot->setAutoDefault(true);
  buttonPlot->setToolTip("Import log file as a table and construct a 1D graph if appropriate");
  bottomButtons->addWidget(buttonPlot);

  buttonClose = new QPushButton(tr("Close"));
  buttonClose->setToolTip("Close dialog");
  bottomButtons->addWidget(buttonClose);

  //Main layout
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->addLayout(uiLayout);
  mainLayout->addLayout(bottomButtons);

  init();

  resize(500,400);
  
  connect(buttonPlot, SIGNAL(clicked()), this, SLOT(importSelectedFiles()));
  connect(buttonClose, SIGNAL(clicked()), this, SLOT(close()));
  //want a custom context menu
  m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_tree, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(popupMenu(const QPoint &)));

  //Double-click imports a log file
  connect(m_tree, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(importItem(QTreeWidgetItem *)));
}

//----------------------------------
// Private methods
//----------------------------------
/**
 * Plot the selected log files
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
* Import an item
* @param item The item to be imported
*/
void MantidSampleLogDialog::importItem(QTreeWidgetItem * item)
{
  m_mantidUI->importSampleLog(item->text(0), item->data(0, Qt::UserRole).toString(), 
			      item->data(1, Qt::UserRole).toBool());
}


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

/**
 * Initialize everything
 */
void MantidSampleLogDialog::init()
{
  m_tree->clear();
  const std::vector< Mantid::Kernel::Property * > & logData = m_mantidUI->getWorkspace(m_wsname)->getSample()->getLogData();
  std::vector< Mantid::Kernel::Property * >::const_iterator pEnd = logData.end();
  int max_length(0);
  for( std::vector< Mantid::Kernel::Property * >::const_iterator pItr = logData.begin();
       pItr != pEnd; ++pItr )
  {
    //name() contains the full path, so strip to file name
    QString filename = QFileInfo((**pItr).name().c_str()).fileName();
    if( filename.size() > max_length ) max_length = filename.size();
    QTreeWidgetItem *treeItem = new QTreeWidgetItem(QStringList(filename));
    treeItem->setData(0, Qt::UserRole, QString::fromStdString((*pItr)->value()));
    //See what type of data we have    
    if( dynamic_cast<Mantid::Kernel::TimeSeriesProperty<double> *>(*pItr) )
    {
      treeItem->setText(1, "numeric");
      treeItem->setData(1, Qt::UserRole, true);
    }
    else if( dynamic_cast<Mantid::Kernel::TimeSeriesProperty<std::string> *>(*pItr) )
    {
      treeItem->setText(1, "string");
      treeItem->setData(1, Qt::UserRole, false);
    }

    //Add tree item
    m_tree->addTopLevelItem(treeItem);
  }
  
  m_tree->header()->resizeSection(0, max_length*10);
  m_tree->header()->resizeSection(1, 100);
  m_tree->header()->setMovable(false);
  m_tree->setSortingEnabled(true);
}
