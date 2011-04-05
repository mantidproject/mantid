#include "MantidDock.h"
#include "MantidUI.h"
#include "../ApplicationWindow.h"
#include "../pixmaps.h"
#include <MantidAPI/AlgorithmFactory.h>
#include <MantidAPI/MemoryManager.h>
#include <MantidAPI/IEventWorkspace.h>
#include <MantidAPI/IMDEventWorkspace.h>
#include <MantidAPI/IMDWorkspace.h>
#include <MantidGeometry/MDGeometry/IMDDimension.h>
#include "MantidMatrix.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QTextEdit>
#include <QListWidget>
#include <QApplication>
#include <QMap>
#include <QMenu>
#include <QAction>
#include <QLineEdit>
#include <QPushButton>
#include <QMutexLocker>
#include <QProgressBar>
#include <QSignalMapper>

#include <map>
#include <iostream>
#include <sstream>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

Mantid::Kernel::Logger& MantidDockWidget::logObject=Mantid::Kernel::Logger::get("mantidDockWidget");
Mantid::Kernel::Logger& MantidTreeWidget::logObject=Mantid::Kernel::Logger::get("MantidTreeWidget");

MantidDockWidget::MantidDockWidget(MantidUI *mui, ApplicationWindow *parent) :
QDockWidget(tr("Workspaces"),parent), m_mantidUI(mui), m_known_groups()
{
  setObjectName("exploreMantid"); // this is needed for QMainWindow::restoreState()
  setMinimumHeight(150);
  setMinimumWidth(200);
  parent->addDockWidget( Qt::RightDockWidgetArea, this );

  QFrame *f = new QFrame(this);
  setWidget(f);

  m_tree = new MantidTreeWidget(f,m_mantidUI);
  m_tree->setHeaderLabel("Workspaces");

  QHBoxLayout * buttonLayout = new QHBoxLayout();
  m_loadButton = new QPushButton("Load");
  m_deleteButton = new QPushButton("Delete");
  m_groupButton= new QPushButton("Group");
  if(m_groupButton)
    m_groupButton->setEnabled(false);
  buttonLayout->addWidget(m_loadButton);
  buttonLayout->addWidget(m_deleteButton);
  buttonLayout->addWidget(m_groupButton);
  buttonLayout->addStretch();
  //
  QVBoxLayout * layout = new QVBoxLayout();
  f->setLayout(layout);
  layout->addLayout(buttonLayout);
  layout->addWidget(m_tree);
  //

  m_loadMenu = new QMenu(this);
  QAction* loadFileAction = new QAction("File",this);
  QAction *loadDAEAction = new QAction("from DAE",this);
  m_loadMapper = new QSignalMapper(this);
  m_loadMapper->setMapping(loadDAEAction,"LoadDAE");
  m_loadMapper->setMapping(loadFileAction,"Load");
  connect(loadDAEAction,SIGNAL(activated()), m_loadMapper, SLOT(map()));
  connect(loadFileAction,SIGNAL(activated()),m_loadMapper,SLOT(map()));
  connect(m_loadMapper, SIGNAL(mapped(const QString &)), m_mantidUI, SLOT(executeAlgorithm(const QString&)));
  m_loadMenu->addAction(loadFileAction);
  m_loadMenu->addAction(loadDAEAction);
  m_loadButton->setMenu(m_loadMenu);

  createWorkspaceMenuActions();

  connect(m_deleteButton,SIGNAL(clicked()),this,SLOT(deleteWorkspaces()));
  connect(m_tree,SIGNAL(itemClicked(QTreeWidgetItem*, int)),this,SLOT(clickedWorkspace(QTreeWidgetItem*, int)));
  connect(m_tree,SIGNAL(itemSelectionChanged()),this,SLOT(workspaceSelected()));
  connect(m_groupButton,SIGNAL(clicked()),this,SLOT(groupingButtonClick()));

  m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_tree, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(popupMenu(const QPoint &)));

  connect(m_mantidUI, SIGNAL(workspace_added(const QString &, Mantid::API::Workspace_sptr)),
    this, SLOT(addTreeEntry(const QString &, Mantid::API::Workspace_sptr)), Qt::QueuedConnection);
  connect(m_mantidUI, SIGNAL(workspace_replaced(const QString &, Mantid::API::Workspace_sptr)),
    this, SLOT(replaceTreeEntry(const QString &, Mantid::API::Workspace_sptr)),Qt::QueuedConnection);

  connect(m_mantidUI, SIGNAL(workspace_ungrouped(const QString &, Mantid::API::Workspace_sptr)),
    this, SLOT(unrollWorkspaceGroup(const QString &,Mantid::API::Workspace_sptr)),Qt::QueuedConnection);

  connect(m_mantidUI, SIGNAL(workspace_removed(const QString &)),
    this, SLOT(removeWorkspaceEntry(const QString &)),Qt::QueuedConnection);

  connect(m_mantidUI, SIGNAL(workspaces_cleared()), m_tree, SLOT(clear()),Qt::QueuedConnection);
  connect(m_tree,SIGNAL(itemSelectionChanged()),this,SLOT(treeSelectionChanged()));

  connect(m_tree, SIGNAL(itemExpanded(QTreeWidgetItem*)), this, SLOT(populateChildData(QTreeWidgetItem*)));
}

/** Returns the name of the selected workspace
*  (the first one if more than one is selected)
*/
QString MantidDockWidget::getSelectedWorkspaceName() const
{
  QList<QTreeWidgetItem*> items = m_tree->selectedItems();
  QString str("");
  if( !items.empty() )
  {
    QTreeWidgetItem *item = items[0];
    if (item) str = item->text(0);
  }
  return str;
}

/// Returns a pointer to the selected workspace (the first if multiple workspaces selected)
Mantid::API::Workspace_sptr MantidDockWidget::getSelectedWorkspace() const
{
  QString workspaceName = getSelectedWorkspaceName();
  if (AnalysisDataService::Instance().doesExist(workspaceName.toStdString()))
  {
    return AnalysisDataService::Instance().retrieve(workspaceName.toStdString());
  }
  else
  {
    return Mantid::API::Workspace_sptr();
  }
}

/**
* Add an item to the tree list
* @ws_name The name of the workspace
* @param workspace :: A pointer to the workspace
*/
void MantidDockWidget::addTreeEntry(const QString & ws_name, Mantid::API::Workspace_sptr workspace)
{
  QString group_name = findParentName(ws_name, workspace);
  if( !group_name.isEmpty() )
  {
    QList<QTreeWidgetItem *> matches = m_tree->findItems(group_name, Qt::MatchFixedString, 0);
    if( matches.empty() ) return;
    QTreeWidgetItem * item = matches[0];
    if( item->isExpanded() )
    {
      populateChildData(item);
    }
    return;
  }
  QTreeWidgetItem *ws_item = createEntry(ws_name, workspace);
  setItemIcon(ws_item, workspace);
  m_tree->addTopLevelItem(ws_item);
}

/**
* Replace an item in the tree list
* @ws_name The name of the workspace
* @param workspace :: A pointer to the workspace
*/
void MantidDockWidget::replaceTreeEntry(const QString & ws_name, Mantid::API::Workspace_sptr workspace)
{
  QString group_name = findParentName(ws_name, workspace);

  QList<QTreeWidgetItem *> matches = m_tree->findItems(ws_name, Qt::MatchFixedString, 0);
  if( matches.empty() )
  {
    if(isInvisibleWorkspaceOptionSet())
    {
      addTreeEntry(ws_name,workspace);
    }
    return;
  }
  QTreeWidgetItem * item = matches[0];

  if( !group_name.isEmpty() )
  {
    m_tree->takeTopLevelItem(m_tree->indexOfTopLevelItem(item));
  }
  else
  {
    setItemIcon(item, workspace);
  }
  if( item->isExpanded() )
  {
    populateChildData(item);
  }
}

// returns true if the workspaces option is set
bool MantidDockWidget::isInvisibleWorkspaceOptionSet()
{
  bool invisible_ws = AnalysisDataService::Instance().isInvisbleWorkspaceOptionsSet();
  return (invisible_ws?true:false);
}

/**
* Create the action items associated with the dock
*/
void MantidDockWidget::createWorkspaceMenuActions()
{
  m_showData = new QAction(tr("Show data"),this);
  connect(m_showData,SIGNAL(triggered()),m_mantidUI,SLOT(importWorkspace()));

  m_showInst = new QAction(tr("Show instrument"),this);
  connect(m_showInst,SIGNAL(triggered()),m_mantidUI,SLOT(showMantidInstrumentSelected()));

  m_plotSpec = new QAction(tr("Plot spectrum..."),this);
  connect(m_plotSpec,SIGNAL(triggered()),this,SLOT(plotSpectra()));

  m_colorFill = new QAction(tr("Color fill plot"), this);
  connect(m_colorFill, SIGNAL(triggered()), this, SLOT(drawColorFillPlot()));

  m_showLogs = new QAction(tr("Sample Logs..."), this);
  connect(m_showLogs,SIGNAL(triggered()),m_mantidUI,SLOT(showLogFileWindow()));

  m_showHist = new QAction(tr("Show History"), this);
  connect(m_showHist,SIGNAL(triggered()),m_mantidUI,SLOT(showAlgorithmHistory()));

  m_saveNexus = new QAction(tr("Save Nexus"),this);
  connect(m_saveNexus,SIGNAL(activated()),m_mantidUI,SLOT(saveNexusWorkspace()));

  m_rename = new QAction(tr("Rename"),this);
  connect(m_rename,SIGNAL(activated()),this,SLOT(renameWorkspace()));

  m_delete = new QAction(tr("Delete"),this);
  connect(m_delete,SIGNAL(triggered()),this,SLOT(deleteWorkspaces()));
}

/**
* Check if the given workspace is part of a known group
*/
QString MantidDockWidget::findParentName(const QString & ws_name, Mantid::API::Workspace_sptr workspace)
{
  QString name("");
  if(Mantid::API::WorkspaceGroup_sptr ws_group = boost::dynamic_pointer_cast<WorkspaceGroup>(workspace) )
  {
    m_known_groups.insert(QString::fromStdString(workspace->getName()));
  }
  else
  {
    QSet<QString>::const_iterator iend = m_known_groups.constEnd();
    for( QSet<QString>::const_iterator itr = m_known_groups.constBegin(); itr != iend; ++itr )
    {
      Mantid::API::Workspace *worksp = NULL;
      try
      {
        worksp = Mantid::API::AnalysisDataService::Instance().retrieve((*itr).toStdString()).get();
      }
      catch(Mantid::Kernel::Exception::NotFoundError&)
      {
        continue;
      }
      if( Mantid::API::WorkspaceGroup *grouped = dynamic_cast<Mantid::API::WorkspaceGroup *>(worksp) )
      {
        if( grouped->contains(ws_name.toStdString()) )
        {
          name = *itr;
          break;
        }
      }
    }
  }
  return name;
}

/**
* When an item is expanded, populate the child data for this item
* @param item :: The item being expanded
*/
void MantidDockWidget::populateChildData(QTreeWidgetItem* item)
{
  // Clear it first
  while( item->childCount() > 0 )
  {
    item->takeChild(0);
  }

  // Retrieve the workspace from the ADS
  Mantid::API::Workspace_sptr workspace;
  try
  {
    workspace = Mantid::API::AnalysisDataService::Instance().retrieve(item->text(0).toStdString());
  }
  catch(Exception::NotFoundError&)
  {
    return;
  }
  QTreeWidgetItem *wsid_item = new QTreeWidgetItem(QStringList(QString::fromStdString(workspace->id())));
  wsid_item->setFlags(Qt::NoItemFlags);
  item->addChild(wsid_item);

  if( Mantid::API::MatrixWorkspace_sptr matrix = boost::dynamic_pointer_cast<MatrixWorkspace>(workspace) )
  {
    populateMatrixWorkspaceData(matrix, item);
  }
  else if( Mantid::API::IMDEventWorkspace_sptr imdew = boost::dynamic_pointer_cast<IMDEventWorkspace>(workspace) )
  {
    populateMDEventWorkspaceData(imdew, item);
  }
  else if( Mantid::API::IMDWorkspace_sptr imdew = boost::dynamic_pointer_cast<IMDWorkspace>(workspace) )
  {
    populateMDWorkspaceData(imdew, item);
  }
  else if(Mantid::API::WorkspaceGroup_sptr ws_group = boost::dynamic_pointer_cast<WorkspaceGroup>(workspace) )
  {
    populateWorkspaceGroupData(ws_group, item);
  }
  else if(Mantid::API::ITableWorkspace_sptr table_ws = boost::dynamic_pointer_cast<ITableWorkspace>(workspace) )
  {
    populateTableWorkspaceData(table_ws, item);
  }
  else return;
}

void MantidDockWidget::setItemIcon(QTreeWidgetItem* ws_item,  Mantid::API::Workspace_sptr workspace)
{
  if( boost::dynamic_pointer_cast<MatrixWorkspace>(workspace) )
  {
    ws_item->setIcon(0,QIcon(getQPixmap("mantid_matrix_xpm")));
  }
  else if( boost::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(workspace) )
  {
    ws_item->setIcon(0,QIcon(getQPixmap("mantid_wsgroup_xpm")));
  }
  // Assume it is a table workspace
  else if( boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(workspace) )
  {
    ws_item->setIcon(0,QIcon(getQPixmap("worksheet_xpm")));
  }
  else
  {
  }
}

/**
* Create a tree item for the given workspace
* @param ws_name :: The worksapce name
* @param workspace :: A pointer to the workspace
*/
QTreeWidgetItem * MantidDockWidget::createEntry(const QString & ws_name, Mantid::API::Workspace_sptr workspace)
{
  QTreeWidgetItem *ws_item = new QTreeWidgetItem(QStringList(ws_name));

  // Need to add a child so that it becomes expandable. Using the correct ID is needed when plotting from non-expanded groups.
  std::string workspace_type = workspace->id();
  QTreeWidgetItem *wsid_item = new QTreeWidgetItem(QStringList(workspace_type.c_str()));
  wsid_item->setFlags(Qt::NoItemFlags);
  ws_item->addChild(wsid_item);
  return ws_item;
}

/*
* This slot handles the notification sent by the UnGroupWorkspace algorithm.
* @param group_name :: The name of the group
* @param ws_group :: The pointer to the workspace group
*/
void MantidDockWidget::unrollWorkspaceGroup(const QString &group_name, Mantid::API::Workspace_sptr ws_group)
{
  QList<QTreeWidgetItem*> matches = m_tree->findItems(group_name, Qt::MatchFixedString);
  if( matches.empty() ) return;

  m_tree->removeItemWidget(matches[0],0);
  if(Mantid::API::WorkspaceGroup_sptr group_ptr = boost::dynamic_pointer_cast<WorkspaceGroup>(ws_group) )
  {
    const std::vector<std::string> group_names = group_ptr->getNames();
    std::vector<std::string>::const_iterator sitr = group_names.begin();
    std::vector<std::string>::const_iterator send = group_names.end();
    for( ; sitr != send; ++sitr )
    {
      std::string name = *sitr;
      Workspace_sptr member_ws;
      try
      {
        member_ws = AnalysisDataService::Instance().retrieve(name);
      }
      catch(Exception::NotFoundError&)
      {
        continue;
      }
      QTreeWidgetItem *item = createEntry(QString::fromStdString(name), member_ws);
      setItemIcon(item, member_ws);
      m_tree->addTopLevelItem(item);
    }
  }

}



/** Populate the tree with some details about a MDWorkspace
 *
 * @param workspace :: Workspace
 * @param ws_item :: tree item
 */
void MantidDockWidget::populateMDWorkspaceData(Mantid::API::IMDWorkspace_sptr workspace, QTreeWidgetItem* ws_item)
{
  QTreeWidgetItem* data_item = new QTreeWidgetItem(QStringList("Title: "+QString::fromStdString(workspace->getTitle())));
  data_item->setFlags(Qt::NoItemFlags);
  ws_item->addChild(data_item);

  // Now add each dimension
  for (size_t i=0; i < workspace->getNumDims(); i++)
  {
    std::ostringstream mess;
    IMDDimension_sptr dim = workspace->getDimensionNum(i);
    mess << "Dim " << i << ": (" << dim->getName() << ") " << dim->getMinimum() << " to " << dim->getMaximum() << " in " << dim->getNBins() << " bins";
    std::string s = mess.str();
    QTreeWidgetItem* sub_data_item = new QTreeWidgetItem(QStringList(QString::fromStdString(s)));
    sub_data_item->setFlags(Qt::NoItemFlags);
    ws_item->addChild(sub_data_item);
  }
}



/** Populate the tree with some details about a MDEventWorkspace
 *
 * @param workspace :: Workspace
 * @param ws_item :: tree item
 */
void MantidDockWidget::populateMDEventWorkspaceData(Mantid::API::IMDEventWorkspace_sptr workspace, QTreeWidgetItem* ws_item)
{
  QTreeWidgetItem* data_item = new QTreeWidgetItem(QStringList("Title: "+QString::fromStdString(workspace->getTitle())));
  data_item->setFlags(Qt::NoItemFlags);
  ws_item->addChild(data_item);

  //data_item = new QTreeWidgetItem(QStringList("Dimensions: "));
  //data_item->setFlags(Qt::NoItemFlags);
  //ws_item->addChild(data_item);

  // Now add each dimension
  for (size_t i=0; i < workspace->getNumDims(); i++)
  {
    std::ostringstream mess;
    IMDDimension_sptr dim = workspace->getDimension(i);
    mess << "Dim " << i << ": (" << dim->getName() << ") " << dim->getMinimum() << " to " << dim->getMaximum() << " " << dim->getUnits();
    std::string s = mess.str();
    QTreeWidgetItem* sub_data_item = new QTreeWidgetItem(QStringList(QString::fromStdString(s)));
    sub_data_item->setFlags(Qt::NoItemFlags);
    ws_item->addChild(sub_data_item);
  }

  // Now box controller details
  std::vector<std::string> stats = workspace->getBoxControllerStats();
  for (size_t i=0; i < stats.size(); i++)
  {
    QTreeWidgetItem* sub_data_item = new QTreeWidgetItem(QStringList(QString::fromStdString( stats[i] )));
    sub_data_item->setFlags(Qt::NoItemFlags);
    ws_item->addChild(sub_data_item);
  }


  data_item = new QTreeWidgetItem(QStringList("Events: "+QString::number(workspace->getNPoints())));
  data_item->setFlags(Qt::NoItemFlags);
  ws_item->addChild(data_item);

  data_item = new QTreeWidgetItem(QStringList("Memory used: "+QString::number(workspace->getMemorySize()/1024)+" KB"));
  data_item->setFlags(Qt::NoItemFlags);
  ws_item->addChild(data_item);

}


/**
* Populate the children of this item with data relevant to the MatrixWorkspace object
* @param workspace :: A pointer to the MatrixWorkspace object to inspect
* @param ws_item :: The tree item that has been expanded
*/
void MantidDockWidget::populateMatrixWorkspaceData(Mantid::API::MatrixWorkspace_sptr workspace, QTreeWidgetItem* ws_item)
{
  QTreeWidgetItem* data_item = new QTreeWidgetItem(QStringList("Title: "+QString::fromStdString(workspace->getTitle())));
  data_item->setFlags(Qt::NoItemFlags);
  ws_item->addChild(data_item);

  data_item = new QTreeWidgetItem(QStringList("Histograms: "+QString::number(workspace->getNumberHistograms())));
  data_item->setFlags(Qt::NoItemFlags);
  ws_item->addChild(data_item);

  data_item = new QTreeWidgetItem(QStringList("Bins: "+QString::number(workspace->blocksize())));
  data_item->setFlags(Qt::NoItemFlags);
  ws_item->addChild(data_item);

  data_item = new QTreeWidgetItem(QStringList(workspace->isHistogramData() ? "Histogram" : "Data points"));
  data_item->setFlags(Qt::NoItemFlags);
  ws_item->addChild(data_item);

  std::string s = "X axis: ";
  if (workspace->axes() > 0 )
  {
    Mantid::API::Axis *ax = workspace->getAxis(0);
    if ( ax && ax->unit() ) s += ax->unit()->caption() + " / " + ax->unit()->label();
    else s += "Not set";
  }
  else
  {
    s += "N/A";
  }
  data_item = new QTreeWidgetItem(QStringList(QString::fromStdString(s)));
  data_item->setFlags(Qt::NoItemFlags);
  ws_item->addChild(data_item);
  s = "Y axis: " + workspace->YUnitLabel();

  data_item = new QTreeWidgetItem(QStringList(QString::fromStdString(s)));
  data_item->setFlags(Qt::NoItemFlags);
  ws_item->addChild(data_item);

  data_item = new QTreeWidgetItem(QStringList("Memory used: "+QString::number(workspace->getMemorySize()/1024)+" KB"));
  data_item->setFlags(Qt::NoItemFlags);
  ws_item->addChild(data_item);

  //Extra stuff for EventWorkspace
  Mantid::API::IEventWorkspace_sptr eventWS = boost::dynamic_pointer_cast<Mantid::API::IEventWorkspace> ( workspace );
  if (eventWS)
  {
    std::string extra("");
    switch (eventWS->getEventType())
    {
    case Mantid::API::WEIGHTED:
      extra = " (weighted)";
      break;
    case Mantid::API::WEIGHTED_NOTIME:
      extra = " (weighted, no times)";
      break;
    case Mantid::API::TOF:
      extra = "";
      break;
    }
    data_item = new QTreeWidgetItem(QStringList("Number of events: "+QString::number(eventWS->getNumberEvents()) + extra.c_str() ));
    data_item->setFlags(Qt::NoItemFlags);
    ws_item->addChild(data_item);
  }
}

/**
* Populate the children of this item with data relevant to the WorkspaceGroup object
* @param workspace :: A pointer to the WorkspaceGroup object to inspect
* @param ws_item :: The tree item that has been expanded
*/
void MantidDockWidget::populateWorkspaceGroupData(Mantid::API::WorkspaceGroup_sptr workspace, QTreeWidgetItem* ws_item)
{
  const std::vector<std::string> group_names = workspace->getNames();
  if (group_names.size() < 1) return;
  std::vector<std::string>::const_iterator sitr = group_names.begin();
  std::vector<std::string>::const_iterator send = group_names.end();
  for( ; sitr != send; ++sitr )
  {
    std::string name = *sitr;
    Workspace_sptr member_ws;
    try
    {
      member_ws = AnalysisDataService::Instance().retrieve(name);
    }
    catch(Exception::NotFoundError&)
    {
      continue;
    }
    QTreeWidgetItem *item = createEntry(QString::fromStdString(name), member_ws);
    setItemIcon(item, member_ws);
    ws_item->addChild(item);
  }
}

/**
* Populate the children of this item with data relevant to the TableWorkspace object
* @param workspace :: A pointer to the TableWorkspace object to inspect
* @param ws_item :: The tree item that has been expanded
*/
void MantidDockWidget::populateTableWorkspaceData(Mantid::API::ITableWorkspace_sptr workspace, QTreeWidgetItem* ws_item)
{
  QTreeWidgetItem* data_item = new QTreeWidgetItem(QStringList("Columns: "+QString::number(workspace->columnCount())));
  data_item->setFlags(Qt::NoItemFlags);
  ws_item->addChild(data_item);

  data_item = new QTreeWidgetItem(QStringList("Rows: "+QString::number(workspace->rowCount())));
  data_item->setFlags(Qt::NoItemFlags);
  ws_item->addChild(data_item);

}

void MantidDockWidget::removeWorkspaceEntry(const QString & ws_name)
{
  //This will only ever be of size zero or one
  QList<QTreeWidgetItem *> name_matches = m_tree->findItems(ws_name,Qt::MatchFixedString);
  QTreeWidgetItem *parent_item(NULL);
  if( name_matches.isEmpty() )
  {	 
    //   if there are  no toplevel items in three matching the workspace name ,loop through
    //  all child elements 
    int topitemCounts = m_tree->topLevelItemCount();
    for (int index = 0; index < topitemCounts; ++index)
    {
      QTreeWidgetItem* topItem = m_tree->topLevelItem(index);
      if(!topItem)
      {
        return;
      }
      int childCounts=topItem->childCount();
      for(int chIndex=0;chIndex<childCounts;++chIndex)
      {
        QTreeWidgetItem* childItem= topItem->child(chIndex);
        if(!childItem)
        {
          return;
        }
        //if the workspace  exists as child workspace
        if(!ws_name.compare(childItem->text(0)))
        {
          topItem->takeChild(chIndex);
		  parent_item = topItem;
		 
        }
      }
    }
    if( parent_item && parent_item->isExpanded() )
    {
      populateChildData(parent_item);
    }
  }
  else
  {
    if( m_known_groups.contains(ws_name) )
    {
      m_known_groups.remove(ws_name);
    }
    m_tree->takeTopLevelItem(m_tree->indexOfTopLevelItem(name_matches[0]));
  }

}



/**
 * Add the actions that are appropriate for a MatrixWorspace
 * @param menu :: The menu to store the items
 * @param matrixWS :: The workspace related to the menu
 */
void MantidDockWidget::addMatrixWorspaceMenuItems(QMenu *menu, Mantid::API::MatrixWorkspace_const_sptr matrixWS) const
{
  // Add all options except plot of we only have 1 value
  menu->addAction(m_showData);
  menu->addAction(m_showInst);
  menu->addAction(m_plotSpec);
  // Don't plot a spectrum if only one X value
  m_plotSpec->setEnabled ( matrixWS->blocksize() > 1 );
  menu->addAction(m_colorFill);
  // Show the color fill plot if you have more than one histogram
  m_colorFill->setEnabled( ( matrixWS->axes() > 1 && matrixWS->getNumberHistograms() > 1) );
  menu->addAction(m_showLogs);
  menu->addAction(m_showHist);
  menu->addAction(m_saveNexus);
}

/**
 * Add the actions that are appropriate for a MDEventWorkspace
 * @param menu :: The menu to store the items
 * @param matrixWS :: The workspace related to the menu
 */
void MantidDockWidget::addMDEventWorspaceMenuItems(QMenu *menu, Mantid::API::IMDEventWorkspace_const_sptr mdeventWS) const
{
  (void) mdeventWS;

  //menu->addAction(m_showData); // Show data
  //menu->addAction(m_showInst); // Show instrument
  //menu->addAction(m_plotSpec); // Plot spectra
  //menu->addAction(m_colorFill);
  //menu->addAction(m_showLogs); // Sample logs
  menu->addAction(m_showHist);
  //menu->addAction(m_saveNexus);
}

/**
 * Add the actions that are appropriate for a MatrixWorspace
 * @param menu :: The menu to store the items
 */
void MantidDockWidget::addWorkspaceGroupMenuItems(QMenu *menu) const
{
  m_plotSpec->setEnabled(true);
  menu->addAction(m_plotSpec);
  menu->addAction(m_colorFill);
  m_colorFill->setEnabled(true);
  menu->addAction(m_saveNexus);
}

/**
 * Add the actions that are appropriate for a MatrixWorspace
 * @param menu :: The menu to store the items
 */
void MantidDockWidget::addTableWorkspaceMenuItems(QMenu * menu) const
{
  menu->addAction(m_showData);
  menu->addAction(m_showHist);
}

void MantidDockWidget::clickedWorkspace(QTreeWidgetItem* item, int)
{
  (void) item; //Avoid unused warning
}

void MantidDockWidget::workspaceSelected()
{ 
  QList<QTreeWidgetItem*> selectedItems=m_tree->selectedItems();
  if(selectedItems.isEmpty()) return;
  QString wsName=selectedItems[0]->text(0);
  if(Mantid::API::AnalysisDataService::Instance().doesExist(wsName.toStdString()))
  {
    m_mantidUI->enableSaveNexus(wsName);
  }
}

/**
deleteWorkspaces
*/
void MantidDockWidget::deleteWorkspaces()
{
  QList<QTreeWidgetItem*> items = m_tree->selectedItems();
  if(items.empty())
  {
    MantidMatrix* m = (MantidMatrix*) m_mantidUI->appWindow()->activeWindow();
    if (!m || !m->isA("MantidMatrix")) return;
    if(m->workspaceName().isEmpty()) return;

    if(Mantid::API::AnalysisDataService::Instance().doesExist(m->workspaceName().toStdString()))
    {	
      m_mantidUI->deleteWorkspace(m->workspaceName());
    }
    return;
  }
  //loop through multiple items selected from the mantid tree
  QList<QTreeWidgetItem*>::iterator itr=items.begin();
  for (itr = items.begin(); itr != items.end(); ++itr)
  {
    m_mantidUI->deleteWorkspace((*itr)->text(0));
  }//end of for loop for selected items
}

void MantidDockWidget::renameWorkspace()
{
  //get selected workspace
  QList<QTreeWidgetItem*>selectedItems=m_tree->selectedItems();
  QString selctedwsName;
  if(!selectedItems.empty())
  {
    selctedwsName=selectedItems[0]->text(0);
  }
  m_mantidUI->renameWorkspace(selctedwsName);
}

void MantidDockWidget::popupMenu(const QPoint & pos)
{
  QTreeWidgetItem* treeItem = m_tree->itemAt(pos);
  QString selectedWsName("");
  if( treeItem ) selectedWsName = treeItem->text(0);
  else m_tree->selectionModel()->clear();
  QMenu *menu(NULL);

  //If no workspace is here then have load raw and dae
  if( selectedWsName.isEmpty() )
  {
    menu = m_loadMenu;
  }
  //else show instrument, sample logs and delete
  else
  {
    // Fresh menu
    menu = new QMenu(this);
    Mantid::API::Workspace_const_sptr ws;
    try
    {
      ws = Mantid::API::AnalysisDataService::Instance().retrieve(selectedWsName.toStdString());
    }
    catch(Mantid::Kernel::Exception::NotFoundError &)
    {
      // Nothing to do
      return;
    }

    // Add the items that are appropriate for the type
    if( MatrixWorkspace_const_sptr matrixWS = boost::dynamic_pointer_cast<const Mantid::API::MatrixWorkspace>(ws) ) 
    {
      addMatrixWorspaceMenuItems(menu, matrixWS);
    }
    else if( IMDEventWorkspace_const_sptr mdeventWS = boost::dynamic_pointer_cast<const IMDEventWorkspace>(ws) )
    {
      addMDEventWorspaceMenuItems(menu, mdeventWS);
    }
    else if( boost::dynamic_pointer_cast<const WorkspaceGroup>(ws) ) 
    {
      addWorkspaceGroupMenuItems(menu);
    }
    else if( boost::dynamic_pointer_cast<const Mantid::API::ITableWorkspace>(ws) )
    {
      addTableWorkspaceMenuItems(menu);
    }
    else {}
    
    //Rename is valid for all workspace types
    menu->addAction(m_rename);
    //separate delete
    menu->addSeparator();
    menu->addAction(m_delete);
  }

  //Show the menu at the cursor's current position
  menu->popup(QCursor::pos());
}

void MantidDockWidget::groupingButtonClick()
{
  if(m_groupButton)
  {
    QString qButtonName=m_groupButton->text();
    if(qButtonName == "Group")
    {
      m_mantidUI->groupWorkspaces();
    }
    else if(qButtonName == "UnGroup")
    {
      m_mantidUI->ungroupWorkspaces();
    }
  }
}

/// Plots a single spectrum from each selected workspace
void MantidDockWidget::plotSpectra()
{
  const QMultiMap<QString,int> toPlot = m_tree->chooseSpectrumFromSelected();
  // An empty map will be returned if the user clicks cancel in the spectrum selection
  if (toPlot.empty()) return;
  m_mantidUI->plotSpectraList( toPlot, false );
}

/**
 * Draw a color fill plot of the workspaces that are currently selected.
 * NOTE: The drawing of 2D plots is currently intimately linked with MantidMatrix meaning
 * that one of these must be generated first!
 */
void MantidDockWidget::drawColorFillPlot()
{
  // Get the selected workspaces
  QStringList wsNames = m_tree->getSelectedWorkspaceNames();
  if( wsNames.empty() ) return;
  m_mantidUI->drawColorFillPlots(wsNames);

}

void MantidDockWidget::treeSelectionChanged()
{
  //get selected workspaces
  if(m_groupButton)
  {
    QList<QTreeWidgetItem*>Items=m_tree->selectedItems();
    if(Items.size()==1)
    {
      //check it's group
      QList<QTreeWidgetItem*>::const_iterator itr=Items.begin();
      std::string selectedWSName=(*itr)->text(0).toStdString();
      if(Mantid::API::AnalysisDataService::Instance().doesExist(selectedWSName))
      {
        Workspace_sptr wsSptr=Mantid::API::AnalysisDataService::Instance().retrieve(selectedWSName);
        WorkspaceGroup_sptr grpSptr=boost::dynamic_pointer_cast<WorkspaceGroup>(wsSptr);
        if(grpSptr)
        {
          m_groupButton->setText("UnGroup");
          m_groupButton->setEnabled(true);
        }
        else
          m_groupButton->setEnabled(false);


      }

    }
    else if(Items.size()>=2)
    {
      m_groupButton->setText("Group");
      m_groupButton->setEnabled(true);
    }
    else if(Items.size()==0)
    {
      m_groupButton->setText("Group");
      m_groupButton->setEnabled(false);
    }
  }

}



//------------ MantidTreeWidget -----------------------//

MantidTreeWidget::MantidTreeWidget(QWidget *w, MantidUI *mui):QTreeWidget(w),m_mantidUI(mui)
{
  setObjectName("WorkspaceTree");
  setSelectionMode(QAbstractItemView::ExtendedSelection);
}

void MantidTreeWidget::mousePressEvent (QMouseEvent *e)
{
  if (e->button() == Qt::LeftButton)
  {
    if( !itemAt(e->pos()) ) selectionModel()->clear();
    m_dragStartPosition = e->pos();
  }

  QTreeWidget::mousePressEvent(e);
}

void MantidTreeWidget::mouseMoveEvent(QMouseEvent *e)
{
  if (!(e->buttons() & Qt::LeftButton))
    return;
  if ((e->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance())
    return;

  // Start dragging
  QDrag *drag = new QDrag(this);
  QMimeData *mimeData = new QMimeData;

  mimeData->setText("Workspace");
  drag->setMimeData(mimeData);

  Qt::DropAction dropAction = drag->exec(Qt::CopyAction | Qt::MoveAction);
  (void) dropAction;
}

void MantidTreeWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
  try
  {
    QString wsName = m_mantidUI->getSelectedWorkspaceName();
    Mantid::API::WorkspaceGroup_sptr grpWSPstr;
    grpWSPstr=boost::dynamic_pointer_cast< WorkspaceGroup>
      (Mantid::API::AnalysisDataService::Instance().retrieve(wsName.toStdString()));
    if(!grpWSPstr)
    {
      if ( ! wsName.isEmpty() )
      {
        m_mantidUI->importWorkspace(wsName,false);
        return;
      }
    }
  }
  catch(Mantid::Kernel::Exception::NotFoundError &)
  {
    return;
  }
  QTreeWidget::mouseDoubleClickEvent(e);
}

/** Returns a list of all selected workspaces
*  (including members of groups if appropriate)
*/
QStringList MantidTreeWidget::getSelectedWorkspaceNames() const
{
  QStringList names;
  const QList<QTreeWidgetItem*> items = this->selectedItems();
  QList<QTreeWidgetItem*>::const_iterator it;
  // Need to look for workspace groups and add all children if found
  for (it = items.constBegin(); it != items.constEnd(); ++it)
  {
    // Look for children (workspace groups)
    if ( (*it)->child(0)->text(0) == "WorkspaceGroup" )
    {
      // Have to populate the group's children if it hasn't been expanded
      if (!(*it)->isExpanded()) static_cast<MantidDockWidget*>(parentWidget())->populateChildData(*it);
      const int count = (*it)->childCount();
      for ( int i=1; i < count; ++i )
      {
        names.append((*it)->child(i)->text(0));
      }
    }
    else
    {
      // Add entries that aren't groups
      if (*it) names.append((*it)->text(0));
    }
  }//end of for loop for selected items

  return names;
}

/** Allows the user to select a spectrum from the selected workspaces.
*  Automatically chooses spectrum 0 if all are single-spectrum workspaces.
*  @return A map of workspace name - spectrum index pairs
*/
QMultiMap<QString,int> MantidTreeWidget::chooseSpectrumFromSelected() const
{
  // Get hold of the names of all the selected workspaces
  QList<QString> wsNames = this->getSelectedWorkspaceNames();
  QList<int> wsSizes;

  // Find out if they are all single-spectrum workspaces
  QList<QString>::const_iterator it = wsNames.constBegin();
  int maxHists = -1;
  for ( ; it != wsNames.constEnd(); ++it )
  {
    MatrixWorkspace_const_sptr ws = boost::dynamic_pointer_cast<const MatrixWorkspace>(AnalysisDataService::Instance().retrieve((*it).toStdString()));
    if ( !ws ) continue;
    const int currentHists = ws->getNumberHistograms();
    wsSizes.append(currentHists);
    if ( currentHists > maxHists ) maxHists = currentHists;
  }
  // If not all single spectrum, ask which one to plot
  QMultiMap<QString,int> toPlot;
  int spec = 0;
  if ( maxHists > 1 )
  {
    bool goAhead;
    spec = QInputDialog::getInteger(m_mantidUI->appWindow(),tr("MantidPlot"),tr("Enter the workspace index to plot"),0,0,maxHists-1,1,&goAhead);
    if (!goAhead) return toPlot;
  }

  // Now need to go around inserting workspace-spectrum pairs into a map
  // and checking whether the requested spectrum is too large for any workspaces
  for ( int i = 0; i < wsNames.size(); ++i )
  {
    if (spec >= wsSizes[i])
    {
      logObject.warning() << wsNames[i].toStdString() << " has only "
        << wsSizes[i] << (wsSizes[i]==1 ? " spectrum" : " spectra") << " - not plotted.\n";
      continue;
    }
    toPlot.insert(wsNames[i],spec);
  }

  return toPlot;
}

//-------------------- AlgorithmDockWidget ----------------------//

void FindAlgComboBox::keyPressEvent(QKeyEvent *e)
{
  if (e->key() == Qt::Key_Return)
  {
    if (currentIndex() >= 0) emit enterPressed();
    return;
  }
  QComboBox::keyPressEvent(e);
}

AlgorithmDockWidget::AlgorithmDockWidget(MantidUI *mui, ApplicationWindow *w):
QDockWidget(w),m_progressBar(NULL),m_algID(NULL),m_mantidUI(mui)
{
  setWindowTitle(tr("Algorithms"));
  setObjectName("exploreAlgorithms"); // this is needed for QMainWindow::restoreState()
  setMinimumHeight(150);
  setMinimumWidth(200);
  w->addDockWidget( Qt::RightDockWidgetArea, this );//*/

  QFrame *f = new QFrame(this);

  m_tree = new AlgorithmTreeWidget(f,mui);
  m_tree->setHeaderLabel("Algorithms");
  connect(m_tree,SIGNAL(itemSelectionChanged()),this,SLOT(treeSelectionChanged()));

  QHBoxLayout * buttonLayout = new QHBoxLayout();
  buttonLayout->setName("testC");
  QPushButton *execButton = new QPushButton("Execute");
  m_findAlg = new FindAlgComboBox;
  m_findAlg->setEditable(true);
  connect(m_findAlg,SIGNAL(editTextChanged(const QString&)),this,SLOT(findAlgTextChanged(const QString&)));
  connect(m_findAlg,SIGNAL(enterPressed()),m_mantidUI,SLOT(executeAlgorithm()));
  connect(execButton,SIGNAL(clicked()),m_mantidUI,SLOT(executeAlgorithm()));

  buttonLayout->addWidget(execButton);
  buttonLayout->addWidget(m_findAlg);
  buttonLayout->addStretch();

  m_runningLayout = new QHBoxLayout();
  m_runningLayout->setName("testA");

  m_runningButton = new QPushButton("Details");
  m_runningLayout->addStretch();
  m_runningLayout->addWidget(m_runningButton);
  connect(m_runningButton,SIGNAL(clicked()),m_mantidUI,SLOT(showAlgMonitor()));
  //
  QVBoxLayout * layout = new QVBoxLayout();
  f->setLayout(layout);
  layout->addLayout(buttonLayout);
  layout->addWidget(m_tree);
  layout->addLayout(m_runningLayout);
  //

  m_treeChanged = false;
  m_findAlgChanged = false;

  setWidget(f);

}

//Use an anonymous namespace to keep these at file scope
namespace {

  bool Algorithm_descriptor_less(const Algorithm_descriptor& d1,const Algorithm_descriptor& d2)
  {
    if (d1.category < d2.category) return true;
    else if (d1.category == d2.category && d1.name < d2.name) return true;
    else if (d1.name == d2.name && d1.version > d2.version) return true;

    return false;
  }

  bool Algorithm_descriptor_name_less(const Algorithm_descriptor& d1,const Algorithm_descriptor& d2)
  {
    return d1.name < d2.name;
  }

}

void AlgorithmDockWidget::update()
{
  m_tree->clear();

  typedef std::vector<Algorithm_descriptor> AlgNamesType;
  AlgNamesType names = AlgorithmFactory::Instance().getDescriptors();

  // sort by algorithm names only to fill m_findAlg combobox
  sort(names.begin(),names.end(),Algorithm_descriptor_name_less);

  m_findAlg->clear();
  std::string prevName = "";
  for(AlgNamesType::const_iterator i=names.begin();i!=names.end();i++)
  {
    if (i->name != prevName)
      m_findAlg->addItem(QString::fromStdString(i->name));
    prevName = i->name;
  }
  m_findAlg->setCurrentIndex(-1);

  // sort by category/name/version to fill QTreeWidget m_tree
  sort(names.begin(),names.end(),Algorithm_descriptor_less);

  QMap<QString,QTreeWidgetItem*> categories;// keeps track of categories added to the tree
  QMap<QString,QTreeWidgetItem*> algorithms;// keeps track of algorithms added to the tree (needed in case there are different versions of an algorithm)

  for(AlgNamesType::const_iterator i=names.begin();i!=names.end();i++)
  {
    QString algName = QString::fromStdString(i->name);
    QString catName = QString::fromStdString(i->category);
    QStringList subCats = catName.split('\\');
    if (!categories.contains(catName))
    {
      if (subCats.size() == 1)
      {
        QTreeWidgetItem *catItem = new QTreeWidgetItem(QStringList(catName));
        categories.insert(catName,catItem);
        m_tree->addTopLevelItem(catItem);
      }
      else
      {
        QString cn = subCats[0];
        QTreeWidgetItem *catItem = 0;
        int n = subCats.size();
        for(int j=0;j<n;j++)
        {
          if (categories.contains(cn))
          {
            catItem = categories[cn];
          }
          else
          {
            QTreeWidgetItem *newCatItem = new QTreeWidgetItem(QStringList(subCats[j]));
            categories.insert(cn,newCatItem);
            if (!catItem)
            {
              m_tree->addTopLevelItem(newCatItem);
            }
            else
            {
              catItem->addChild(newCatItem);
            }
            catItem = newCatItem;
          }
          if (j != n-1) cn += "\\" + subCats[j+1];
        }
      }
    }

    QTreeWidgetItem *algItem = new QTreeWidgetItem(QStringList(algName+" v."+QString::number(i->version)));
    QString cat_algName = catName+algName;
    if (!algorithms.contains(cat_algName))
    {
      algorithms.insert(cat_algName,algItem);
      categories[catName]->addChild(algItem);
    }
    else
      algorithms[cat_algName]->addChild(algItem);

  }
}

void AlgorithmDockWidget::findAlgTextChanged(const QString& text)
{
  int i = m_findAlg->findText(text,Qt::MatchFixedString);
  if (i >= 0) m_findAlg->setCurrentIndex(i);
  if (!m_treeChanged)
  {
    m_findAlgChanged = true;
    selectionChanged(text);
  }
}

void AlgorithmDockWidget::treeSelectionChanged()
{
  QString algName;
  int version;
  m_mantidUI->getSelectedAlgorithm(algName,version);
  if (!m_findAlgChanged)
  {
    m_treeChanged = true;
    selectionChanged(algName);
  }
}

void AlgorithmDockWidget::selectionChanged(const QString& algName)
{
  if (m_treeChanged) m_findAlg->setCurrentIndex(m_findAlg->findText(algName,Qt::MatchFixedString));
  if (m_findAlgChanged) m_tree->setCurrentIndex(QModelIndex());
  m_treeChanged = false;
  m_findAlgChanged = false;
}

void AlgorithmDockWidget::updateProgress(void* alg, const int p, const QString& msg)
{
  if (m_algID.empty()) return;
  if (alg == m_algID.first() && p >= 0 && p <= 100 && m_progressBar)
  {
    m_progressBar->setValue(p);
    m_progressBar->setFormat(msg + " %p%");
  }
}

void AlgorithmDockWidget::algorithmStarted(void* alg)
{
  m_algID.push_front(alg);
  hideProgressBar();
  showProgressBar();
}

void AlgorithmDockWidget::algorithmFinished(void* alg)
{
  if (m_algID.empty()) return;
  if (alg == m_algID.first())
  {
    m_algID.pop_front();
    hideProgressBar();
  }
}

void AlgorithmDockWidget::showProgressBar()
{
  if (m_progressBar == NULL)
  {
    // insert progress bar
    m_progressBar = new QProgressBar();
    m_progressBar->setAlignment(Qt::AlignHCenter);
    m_runningLayout->insertWidget(1,m_progressBar);
    // remove the stretch item
    m_runningLayout->removeItem(m_runningLayout->takeAt(0));
  }
}

void AlgorithmDockWidget::hideProgressBar()
{

  if (m_progressBar && m_algID.empty())
  {
    m_runningLayout->insertStretch(0);
    m_runningLayout->removeWidget(m_progressBar);
    m_progressBar->close();
    delete m_progressBar;
    m_progressBar = NULL;
  }
}


//-------------------- AlgorithmTreeWidget ----------------------//

void AlgorithmTreeWidget::mousePressEvent (QMouseEvent *e)
{
  if (e->button() == Qt::LeftButton)
  {
    if( !itemAt(e->pos()) ) selectionModel()->clear();
    m_dragStartPosition = e->pos();
  }

  QTreeWidget::mousePressEvent(e);
}

void AlgorithmTreeWidget::mouseMoveEvent(QMouseEvent *e)
{
  if (!(e->buttons() & Qt::LeftButton))
    return;
  if ((e->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance())
    return;

  // Start dragging
  QDrag *drag = new QDrag(this);
  QMimeData *mimeData = new QMimeData;

  mimeData->setText("Algorithm");
  drag->setMimeData(mimeData);

  Qt::DropAction dropAction = drag->exec(Qt::CopyAction | Qt::MoveAction);
  (void) dropAction;
}

void AlgorithmTreeWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
  QString algName;
  int version;
  m_mantidUI->getSelectedAlgorithm(algName,version);
  if ( ! algName.isEmpty() )
  {
    m_mantidUI->executeAlgorithm(algName, version);
    return;
  }

  QTreeWidget::mouseDoubleClickEvent(e);
}

