#include "MantidDock.h"
#include "MantidUI.h"
#include "MantidMatrix.h"
#include "../ApplicationWindow.h"
#include "../pixmaps.h"
#include "MantidWSIndexDialog.h"
#include "FlowLayout.h"

#include <MantidAPI/AlgorithmFactory.h>
#include <MantidAPI/MemoryManager.h>
#include <MantidAPI/IEventWorkspace.h>
#include <MantidAPI/IMaskWorkspace.h>
#include <MantidAPI/IMDEventWorkspace.h>
#include <MantidAPI/IMDWorkspace.h>
#include "MantidAPI/IMDHistoWorkspace.h"
#include <MantidAPI/FileProperty.h>
#include "MantidAPI/ExperimentInfo.h"
#include <MantidGeometry/MDGeometry/IMDDimension.h>
#include <MantidGeometry/Crystal/OrientedLattice.h>
#include <MantidQtAPI/InterfaceManager.h>
#include <MantidQtAPI/Message.h>

#include <Poco/Path.h>
#include <boost/algorithm/string.hpp>

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
#include <QtGui>

#include <map>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdio>
#include <algorithm>
#include <limits>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

Mantid::Kernel::Logger& MantidDockWidget::logObject=Mantid::Kernel::Logger::get("mantidDockWidget");
Mantid::Kernel::Logger& MantidTreeWidget::logObject=Mantid::Kernel::Logger::get("MantidTreeWidget");

MantidDockWidget::MantidDockWidget(MantidUI *mui, ApplicationWindow *parent) :
QDockWidget(tr("Workspaces"),parent), m_mantidUI(mui), m_known_groups(), m_rerunStackSize( 0 )
{
  setObjectName("exploreMantid"); // this is needed for QMainWindow::restoreState()
  setMinimumHeight(150);
  setMinimumWidth(200);
  parent->addDockWidget( Qt::RightDockWidgetArea, this );

  QFrame *f = new QFrame(this);
  setWidget(f);

  m_tree = new MantidTreeWidget(f,m_mantidUI);
  m_tree->setHeaderLabel("Workspaces");

  FlowLayout * buttonLayout = new FlowLayout();
  m_loadButton = new QPushButton("Load");
  m_deleteButton = new QPushButton("Delete");
  m_groupButton= new QPushButton("Group");
  m_sortButton= new QPushButton("Sort");
  if(m_groupButton)
    m_groupButton->setEnabled(false);
  buttonLayout->addWidget(m_loadButton);
  buttonLayout->addWidget(m_deleteButton);
  buttonLayout->addWidget(m_groupButton);
  buttonLayout->addWidget(m_sortButton);
  //
  QVBoxLayout * layout = new QVBoxLayout();
  f->setLayout(layout);
  layout->addLayout(buttonLayout);
  layout->addWidget(m_tree);
  //

  m_loadMenu = new QMenu(this);
  
  QAction* loadFileAction = new QAction("File",this);
  QAction *liveDataAction = new QAction("Live Data",this);
  m_loadMapper = new QSignalMapper(this);
  m_loadMapper->setMapping(liveDataAction,"StartLiveData");
  m_loadMapper->setMapping(loadFileAction,"Load");
  connect(liveDataAction,SIGNAL(activated()), m_loadMapper, SLOT(map()));
  connect(loadFileAction,SIGNAL(activated()),m_loadMapper,SLOT(map()));
  connect(m_loadMapper, SIGNAL(mapped(const QString &)), m_mantidUI, SLOT(executeAlgorithm(const QString&)));
  m_loadMenu->addAction(loadFileAction);
  m_loadMenu->addAction(liveDataAction);
  m_loadButton->setMenu(m_loadMenu);

  // SET UP SORT
  chooseByName();
  m_sortMenu = new QMenu(this);
  m_choiceMenu = new QMenu(m_sortMenu);

  m_choiceMenu->setTitle(tr("Sort by"));

  QAction* m_ascendingSortAction = new QAction("Ascending", this);
  QAction* m_descendingSortAction = new QAction("Descending", this);
  QAction* m_byNameChoice = new QAction("Name", this);
  QAction* m_byLastModifiedChoice = new QAction("Last Modified", this);
  
  m_byNameChoice->setCheckable(true);
  m_byNameChoice->setEnabled(true);
  m_byNameChoice->setToggleAction(true);
  
  m_byLastModifiedChoice->setCheckable(true);
  m_byLastModifiedChoice->setEnabled(true);
  m_byLastModifiedChoice->setToggleAction(true);

  m_sortChoiceGroup = new QActionGroup(m_sortMenu);
  m_sortChoiceGroup->addAction(m_byNameChoice);
  m_sortChoiceGroup->addAction(m_byLastModifiedChoice);
  m_sortChoiceGroup->setExclusive(true);
  m_byNameChoice->setChecked(true);
  
  connect(m_ascendingSortAction, SIGNAL(activated()), this, SLOT(sortAscending()));
  connect(m_descendingSortAction, SIGNAL(activated()), this, SLOT(sortDescending()));
  connect(m_byNameChoice, SIGNAL(activated()), this, SLOT(chooseByName()));
  connect(m_byLastModifiedChoice, SIGNAL(activated()), this, SLOT(chooseByLastModified()));
  m_sortMenu->addAction(m_ascendingSortAction);
  m_sortMenu->addAction(m_descendingSortAction);
  m_sortMenu->addSeparator();
  m_sortMenu->addMenu(m_choiceMenu);
  m_choiceMenu->addActions(m_sortChoiceGroup->actions());
  m_sortButton->setMenu(m_sortMenu);
  createWorkspaceMenuActions();

  connect(m_deleteButton,SIGNAL(clicked()),this,SLOT(deleteWorkspaces()));
  connect(m_tree,SIGNAL(itemClicked(QTreeWidgetItem*, int)),this,SLOT(clickedWorkspace(QTreeWidgetItem*, int)));
  connect(m_tree,SIGNAL(itemSelectionChanged()),this,SLOT(workspaceSelected()));
  connect(m_groupButton,SIGNAL(clicked()),this,SLOT(groupingButtonClick()));

  m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_tree, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(popupMenu(const QPoint &)));

  connect(m_mantidUI, SIGNAL(workspace_added(const QString &, Mantid::API::Workspace_sptr)),
    this, SLOT(addWorkspace(const QString &, Mantid::API::Workspace_sptr)), Qt::QueuedConnection);
  connect(m_mantidUI, SIGNAL(workspace_replaced(const QString &, Mantid::API::Workspace_sptr)),
    this, SLOT(replaceTreeEntry(const QString &, Mantid::API::Workspace_sptr)),Qt::QueuedConnection);

  connect(m_mantidUI, SIGNAL(workspace_ungrouped(const QString &, Mantid::API::Workspace_sptr)),
    this, SLOT(unrollWorkspaceGroup(const QString &,Mantid::API::Workspace_sptr)),Qt::QueuedConnection);

  connect(m_mantidUI, SIGNAL(workspace_removed(const QString &)),
    this, SLOT(removeWorkspaceEntry(const QString &)),Qt::QueuedConnection);

  connect(m_mantidUI, SIGNAL(workspace_renamed(const QString &,const QString &)),
    this, SLOT(renameWorkspaceEntry(const QString &,const QString &)),Qt::QueuedConnection);

  connect(m_mantidUI, SIGNAL(workspace_group_updated(const QString&)),
    this, SLOT(updateWorkspaceGroup(const QString&)),Qt::QueuedConnection);

  connect(m_mantidUI, SIGNAL(workspaces_cleared()), m_tree, SLOT(clear()),Qt::QueuedConnection);
  connect(m_tree,SIGNAL(itemSelectionChanged()),this,SLOT(treeSelectionChanged()));

  connect(m_tree, SIGNAL(itemExpanded(QTreeWidgetItem*)), this, SLOT(populateChildData(QTreeWidgetItem*)));

  connect(this,SIGNAL(rerunFindAbandonedWorkspaces()),this,SLOT(findAbandonedWorkspaces()),Qt::QueuedConnection);
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
* Add a workspace to the tree list
* @param ws_name The name of the workspace
* @param workspace :: A pointer to the workspace
*/
void MantidDockWidget::addWorkspace(const QString & ws_name, Mantid::API::Workspace_sptr workspace)
{
  addTreeEntry( ws_name, workspace );
  scheduleFindAbandonedWorkspaces();
}

/**
* Add an item to the tree list
* @param ws_name The name of the workspace
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
  MantidTreeWidgetItem *ws_item = createEntry(ws_name, workspace);
  setItemIcon(ws_item, workspace);
  m_tree->addTopLevelItem(ws_item);
}

/**
* Replace an item in the tree list
* @param ws_name The name of the workspace
* @param workspace :: A pointer to the workspace
*/
void MantidDockWidget::replaceTreeEntry(const QString & ws_name, Mantid::API::Workspace_sptr workspace)
{
  QString group_name = findParentName(ws_name, workspace);

  QList<QTreeWidgetItem *> matches = m_tree->findItems(ws_name, Qt::MatchFixedString, 0);
  if( matches.empty() )
  {
    addTreeEntry(ws_name,workspace);
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

/**
* Create the action items associated with the dock
*/
void MantidDockWidget::createWorkspaceMenuActions()
{
  m_showData = new QAction(tr("Show Data"),this);
  connect(m_showData,SIGNAL(triggered()),m_mantidUI,SLOT(importWorkspace()));

  m_showInst = new QAction(tr("Show Instrument"),this);
  connect(m_showInst,SIGNAL(triggered()),m_mantidUI,SLOT(showMantidInstrumentSelected()));

  m_plotSpec = new QAction(tr("Plot Spectrum..."),this);
  connect(m_plotSpec,SIGNAL(triggered()),this,SLOT(plotSpectra()));

  m_plotSpecErr = new QAction(tr("Plot Spectrum with Errors..."),this);
  connect(m_plotSpecErr,SIGNAL(triggered()),this,SLOT(plotSpectraErr()));

  m_plotSpecDistr = new QAction(tr("Plot spectrum as distribution..."),this);
  connect(m_plotSpecDistr,SIGNAL(triggered()),this,SLOT(plotSpectraDistribution()));

  m_colorFill = new QAction(tr("Color Fill Plot"), this);
  connect(m_colorFill, SIGNAL(triggered()), this, SLOT(drawColorFillPlot()));

  m_showDetectors = new QAction(tr("Show Detectors"),this);
  connect(m_showDetectors,SIGNAL(activated()),this,SLOT(showDetectorTable()));

  m_showBoxData = new QAction(tr("Show Box Data Table"),this);
  connect(m_showBoxData,SIGNAL(activated()),m_mantidUI,SLOT(importBoxDataTable()));

  m_showVatesGui = new QAction(tr("Show Vates Simple Interface"), this);
  { QIcon icon; icon.addFile(QString::fromUtf8(":/VatesSimpleGuiViewWidgets/icons/pvIcon.png"), QSize(), QIcon::Normal, QIcon::Off);
  m_showVatesGui->setIcon(icon); }
  connect(m_showVatesGui, SIGNAL(activated()), m_mantidUI, SLOT(showVatesSimpleInterface()));

  m_showMDPlot = new QAction(tr("Plot MD"), this);
  connect(m_showMDPlot, SIGNAL(activated()), m_mantidUI, SLOT(showMDPlot()));

  m_showListData = new QAction(tr("List Data"), this);
  connect(m_showListData, SIGNAL(activated()), m_mantidUI, SLOT(showListData())); 

  m_showImageViewer = new QAction(tr("Show Image Viewer"), this);
  connect(m_showImageViewer, SIGNAL(activated()), m_mantidUI, SLOT(showImageViewer()));

  m_showSliceViewer = new QAction(tr("Show Slice Viewer"), this);
  { QIcon icon; icon.addFile(QString::fromUtf8(":/SliceViewer/icons/SliceViewerWindow_icon.png"), QSize(), QIcon::Normal, QIcon::Off);
  m_showSliceViewer->setIcon(icon); }
  connect(m_showSliceViewer, SIGNAL(activated()), m_mantidUI, SLOT(showSliceViewer()));

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

  m_showTransposed = new QAction(tr("Show Transposed"),this);
  connect(m_showTransposed,SIGNAL(triggered()),m_mantidUI,SLOT(importTransposed()));

  m_convertToMatrixWorkspace = new QAction(tr("Convert to MatrixWorkpace"),this);
  m_convertToMatrixWorkspace->setIcon(QIcon(getQPixmap("mantid_matrix_xpm")));
  connect(m_convertToMatrixWorkspace,SIGNAL(triggered()),this,SLOT(convertToMatrixWorkspace()));

  m_convertMDHistoToMatrixWorkspace = new QAction(tr("Convert to MatrixWorkpace"),this);
  m_convertMDHistoToMatrixWorkspace->setIcon(QIcon(getQPixmap("mantid_matrix_xpm")));
  connect(m_convertMDHistoToMatrixWorkspace,SIGNAL(triggered()),this,SLOT(convertMDHistoToMatrixWorkspace()));
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
  MantidTreeWidgetItem *wsid_item = new MantidTreeWidgetItem(QStringList(QString::fromStdString(workspace->id())), m_tree);
  wsid_item->setFlags(Qt::NoItemFlags);
  excludeItemFromSort(wsid_item);
  item->addChild(wsid_item);

  bool showMemory = true;

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
    showMemory = false;
    populateWorkspaceGroupData(ws_group, item);
  }
  else if(Mantid::API::ITableWorkspace_sptr table_ws = boost::dynamic_pointer_cast<ITableWorkspace>(workspace) )
  {
    populateTableWorkspaceData(table_ws, item);
  }
  else return;

  // Experiment info data
  ExperimentInfo_sptr experimentInfo_ws = boost::dynamic_pointer_cast<ExperimentInfo>(workspace);
  bool specialWorkspace = (workspace->id() == "SpecialWorkspace2D" || workspace->id() == "MaskWorkspace"
                      || workspace->id() == "OffsetsWorkspace" || workspace->id() == "GroupingWorkspace");
  if (experimentInfo_ws && (!specialWorkspace))
    populateExperimentInfoData(experimentInfo_ws, item);

  if (showMemory)
  {
    // For all workspaces, show the memory
    MantidTreeWidgetItem* data_item;
    double kb = double(workspace->getMemorySize()/1024);
    if (kb > 1000000)
    {
      // Show in MB if over 1 GB
      data_item = new MantidTreeWidgetItem(QStringList("Memory used: "
                      + QLocale(QLocale::English).toString(kb/1024, 'd', 0)
                      + " MB"), m_tree);
    }
    else
    {
      // Show in MB
      data_item = new MantidTreeWidgetItem(QStringList("Memory used: "
                      + QLocale(QLocale::English).toString(kb, 'd', 0)
                      + " KB"), m_tree);
    }
    data_item->setFlags(Qt::NoItemFlags);
    excludeItemFromSort(data_item);
    item->addChild(data_item);
  }
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
  else if( boost::dynamic_pointer_cast<Mantid::API::IMDWorkspace>(workspace) )
  {
    ws_item->setIcon(0,QIcon(getQPixmap("mantid_mdws_xpm")));
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
* @param ws_name :: The workspace name
* @param workspace :: A pointer to the workspace
*/
MantidTreeWidgetItem * MantidDockWidget::createEntry(const QString & ws_name, Mantid::API::Workspace_sptr workspace)
{
  MantidTreeWidgetItem *ws_item = new MantidTreeWidgetItem(QStringList(ws_name), m_tree);

  // Need to add a child so that it becomes expandable. Using the correct ID is needed when plotting from non-expanded groups.
  std::string workspace_type = workspace->id();
  MantidTreeWidgetItem *wsid_item = new MantidTreeWidgetItem(QStringList(workspace_type.c_str()), m_tree);
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
    for( std::vector<std::string>::const_iterator sitr = group_names.begin(); sitr != group_names.end(); ++sitr )
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

      QString wsname = QString::fromStdString(name);
      QList<QTreeWidgetItem*> exists = m_tree->findItems(wsname, Qt::MatchFixedString);
      if ( exists.empty() )
      {
        MantidTreeWidgetItem *item = createEntry(wsname, member_ws);
        setItemIcon(item, member_ws);
        m_tree->addTopLevelItem(item);
      }
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
  MantidTreeWidgetItem* data_item = new MantidTreeWidgetItem(QStringList("Title: "+QString::fromStdString(workspace->getTitle())), m_tree);
  data_item->setFlags(Qt::NoItemFlags);
  excludeItemFromSort(data_item);
  ws_item->addChild(data_item);

  // Now add each dimension
  for (size_t i=0; i < workspace->getNumDims(); i++)
  {
    std::ostringstream mess;
    IMDDimension_const_sptr dim = workspace->getDimension(i);
    mess << "Dim " << i << ": (" << dim->getName() << ") " << dim->getMinimum() << " to " << dim->getMaximum() << " in " << dim->getNBins() << " bins";
    // Also show the dimension ID string, if different than name
    if (dim->getDimensionId() != dim->getName())
      mess << ". Id=" << dim->getDimensionId();

    std::string s = mess.str();
    MantidTreeWidgetItem* sub_data_item = new MantidTreeWidgetItem(QStringList(QString::fromStdString(s)), m_tree);
    sub_data_item->setFlags(Qt::NoItemFlags);
    excludeItemFromSort(data_item);

    ws_item->addChild(sub_data_item);
  }

  // A line describing that this workspace is binned from another
  if (workspace->hasOriginalWorkspace())
  {
    std::string text = "Binned from '" + workspace->getOriginalWorkspace()->getName() + "'";
    MantidTreeWidgetItem* sub_data_item = new MantidTreeWidgetItem(QStringList(QString::fromStdString( text )), m_tree);
    sub_data_item->setFlags(Qt::NoItemFlags);
    excludeItemFromSort(sub_data_item);
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
  this->populateMDWorkspaceData(workspace, ws_item);

  MantidTreeWidgetItem* data_item;

  // Now box controller details
  std::vector<std::string> stats = workspace->getBoxControllerStats();
  for (size_t i=0; i < stats.size(); i++)
  {
    MantidTreeWidgetItem* sub_data_item = new MantidTreeWidgetItem(QStringList(QString::fromStdString( stats[i] )), m_tree);
    sub_data_item->setFlags(Qt::NoItemFlags);
    excludeItemFromSort(sub_data_item);
    ws_item->addChild(sub_data_item);
  }

  data_item = new MantidTreeWidgetItem(QStringList("Events: "+ QLocale(QLocale::English).toString(double(workspace->getNPoints()), 'd', 0)), m_tree);
  data_item->setFlags(Qt::NoItemFlags);
  excludeItemFromSort(data_item);
  ws_item->addChild(data_item);
}



/** Populate the children of this item with data relevant to a workspace with ExperimentInfo (instrument, sample, logs)
 *
 * @param workspace :: ExperimentInfo *
 * @param ws_item :: The tree item that has been expanded
*/
void MantidDockWidget::populateExperimentInfoData(Mantid::API::ExperimentInfo_sptr workspace, QTreeWidgetItem* ws_item)
{
  MantidTreeWidgetItem* data_item;
  std::string s;
  std::ostringstream out;

  Instrument_const_sptr inst = workspace->getInstrument();
  out << "Instrument: " << inst->getName() << " ("
      << inst->getValidFromDate().toFormattedString("%Y-%b-%d")
      << " to " << inst->getValidToDate().toFormattedString("%Y-%b-%d") << ")";
  s = out.str();

  data_item = new MantidTreeWidgetItem(QStringList(QString::fromStdString(s)), m_tree);
  data_item->setFlags(Qt::NoItemFlags);
  excludeItemFromSort(data_item);
  ws_item->addChild(data_item);

  if (workspace->sample().hasOrientedLattice())
  {
    const OrientedLattice & latt = workspace->sample().getOrientedLattice();
    out.str("");
    out << "Sample: a " << std::fixed << std::setprecision(1) << latt.a() <<", b " << latt.b() << ", c " << latt.c();
    out << "; alpha " << std::fixed << std::setprecision(0) << latt.alpha() <<", beta " << latt.beta() << ", gamma " << latt.gamma();
    s = out.str();
    data_item = new MantidTreeWidgetItem(QStringList(QString::fromStdString(s)), m_tree);
    data_item->setFlags(Qt::NoItemFlags);
    excludeItemFromSort(data_item);
    ws_item->addChild(data_item);
  }
}


/**
* Populate the children of this item with data relevant to the MatrixWorkspace object
* @param workspace :: A pointer to the MatrixWorkspace object to inspect
* @param ws_item :: The tree item that has been expanded
*/
void MantidDockWidget::populateMatrixWorkspaceData(Mantid::API::MatrixWorkspace_sptr workspace, QTreeWidgetItem* ws_item)
{
  // Are we showing one of the special workspaces, OffsetsWorkspace or GroupingWorkspace
  bool specialWorkspace = (workspace->id() == "MaskWorkspace" || workspace->id() == "SpecialWorkspace2D" || workspace->id() == "OffsetsWorkspace" || workspace->id() == "GroupingWorkspace");

  MantidTreeWidgetItem* data_item = new MantidTreeWidgetItem(QStringList("Title: "+QString::fromStdString(workspace->getTitle())), m_tree);
  data_item->setFlags(Qt::NoItemFlags);
  excludeItemFromSort(data_item);
  ws_item->addChild(data_item);

  data_item = new MantidTreeWidgetItem(QStringList("Histograms: "+ QLocale(QLocale::English).toString(double(workspace->getNumberHistograms()), 'd', 0)), m_tree);
  data_item->setFlags(Qt::NoItemFlags);
  excludeItemFromSort(data_item);
  ws_item->addChild(data_item);

  if (!specialWorkspace)
  {
    data_item = new MantidTreeWidgetItem(QStringList("Bins: "+QString::number(workspace->blocksize())), m_tree);
    data_item->setFlags(Qt::NoItemFlags);
    excludeItemFromSort(data_item);
    ws_item->addChild(data_item);

    data_item = new MantidTreeWidgetItem(QStringList(workspace->isHistogramData() ? "Histogram" : "Data points"), m_tree);
    data_item->setFlags(Qt::NoItemFlags);
    excludeItemFromSort(data_item);
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
    data_item = new MantidTreeWidgetItem(QStringList(QString::fromStdString(s)), m_tree);
    data_item->setFlags(Qt::NoItemFlags);
    excludeItemFromSort(data_item);
    ws_item->addChild(data_item);

    s = "Y axis: " + workspace->YUnitLabel();
    data_item = new MantidTreeWidgetItem(QStringList(QString::fromStdString(s)), m_tree);
    data_item->setFlags(Qt::NoItemFlags);
    excludeItemFromSort(data_item);
    ws_item->addChild(data_item);
  }
  else
  {
    if (workspace->id() == "MaskWorkspace")
    {
      IMaskWorkspace_sptr maskWS = boost::dynamic_pointer_cast<IMaskWorkspace>(workspace);
      data_item = new MantidTreeWidgetItem(QStringList("Masked: " + QLocale(QLocale::English).toString(double(maskWS->getNumberMasked()), 'd', 0)), m_tree);
      data_item->setFlags(Qt::NoItemFlags);
      excludeItemFromSort(data_item);
      ws_item->addChild(data_item);
    }
  }


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
    data_item = new MantidTreeWidgetItem(QStringList("Number of events: "
        + QLocale(QLocale::English).toString(double(eventWS->getNumberEvents()), 'd', 0)
        + extra.c_str() ), m_tree);
    data_item->setFlags(Qt::NoItemFlags);
    excludeItemFromSort(data_item);
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
  size_t unnamedCount = 0;
  std::vector<std::string>::const_iterator sitr = group_names.begin();
  std::vector<std::string>::const_iterator send = group_names.end();
  for( ; sitr != send; ++sitr )
  {
    const std::string& name = *sitr;
    Workspace_sptr member_ws;
    if ( !name.empty() )
    {
      try
      {
        member_ws = AnalysisDataService::Instance().retrieve(name);
      }
      catch(Exception::NotFoundError&)
      {
        continue;
      }
      MantidTreeWidgetItem *item = createEntry(QString::fromStdString(name), member_ws);
      setItemIcon(item, member_ws);
      ws_item->addChild(item);
    }
    else
    {
      QString itemName = QString("Unnamed_%1").arg(unnamedCount);
      MantidTreeWidgetItem *item = new MantidTreeWidgetItem(QStringList(itemName), m_tree);
      setItemIcon(item, member_ws);
      ws_item->addChild(item);
      ++unnamedCount;
    }
  }
}

/**
* Populate the children of this item with data relevant to the TableWorkspace object
* @param workspace :: A pointer to the TableWorkspace object to inspect
* @param ws_item :: The tree item that has been expanded
*/
void MantidDockWidget::populateTableWorkspaceData(Mantid::API::ITableWorkspace_sptr workspace, QTreeWidgetItem* ws_item)
{
  MantidTreeWidgetItem* data_item = new MantidTreeWidgetItem(QStringList("Columns: "+QString::number(workspace->columnCount())), m_tree);
  excludeItemFromSort(data_item);
  data_item->setFlags(Qt::NoItemFlags);
  ws_item->addChild(data_item);

  data_item = new MantidTreeWidgetItem(QStringList("Rows: "+QString::number(workspace->rowCount())), m_tree);
  excludeItemFromSort(data_item);
  data_item->setFlags(Qt::NoItemFlags);
  ws_item->addChild(data_item);
}



void MantidDockWidget::removeWorkspaceEntry(const QString & ws_name)
{
  //This will only ever be of size zero or one
  QList<QTreeWidgetItem *> name_matches = m_tree->findItems(ws_name,Qt::MatchFixedString);
  if( name_matches.isEmpty() )
  {
    QTreeWidgetItem *parent_item(NULL);
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
 * Rename a workspace entry. Slot to connect to a workspace_renamed signal.
 * @param ws_name :: The old workspace name.
 * @param new_name :: The new workspace name.
 */
void MantidDockWidget::renameWorkspaceEntry(const QString & ws_name, const QString& new_name)
{
  (void)ws_name;
  (void)new_name;
  scheduleFindAbandonedWorkspaces();
}

/**
 * Update the widget following an update of a workspace group.
 * @param group_name :: The name of the updated workspace group.
 */
void MantidDockWidget::updateWorkspaceGroup(const QString & group_name)
{
  (void)group_name;
  scheduleFindAbandonedWorkspaces();
}

/**
 * Finds mismatches between the ADS and the contents of the widget and fixes them.
 */
void MantidDockWidget::findAbandonedWorkspaces()
{
  if ( m_rerunStackSize > 1 )
  {
    --m_rerunStackSize;
    return;
  }
  else
  {
    m_rerunStackSize = 0;
  }
  // find all top-level item names in the widget
  QStringList topItems;
  int n = m_tree->topLevelItemCount();
  for( int i = 0; i < n; ++i )
  {
    topItems.append( m_tree->topLevelItem(i)->text(0) );
  }
  // list of all workspaces in the ADS
  auto wsSet = Mantid::API::AnalysisDataService::Instance().getObjectNames();
  std::vector<std::string> workspaces( wsSet.begin(), wsSet.end() );
  QList<QTreeWidgetItem*> groupItemsToExpand;
  // find all groups, remove their members from workspaces
  for( auto wsName = workspaces.begin(); wsName != workspaces.end(); ++wsName )
  {
    if ( wsName->empty() ) continue;
    try
    {
      auto ws = Mantid::API::AnalysisDataService::Instance().retrieve( *wsName );
      if ( auto group = boost::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>( ws ) )
      {
        auto groupItems = m_tree->findItems( QString::fromStdString( *wsName ), Qt::MatchExactly );
        if ( !groupItems.isEmpty() && groupItems[0]->isExpanded() )
        {
          groupItemsToExpand.append( groupItems[0] );
        }
        auto groupMembers = group->getNames();
        for(auto name = groupMembers.begin(); name != groupMembers.end(); ++name)
        {
          if ( name->empty() ) continue; // unnamed workspace
          auto it = std::find( workspaces.begin(), workspaces.end(), *name );
          if ( it != workspaces.end() )
          {
            it->clear();
          }
        }
      }
    }
    catch(...) 
    {
      scheduleFindAbandonedWorkspaces();
      return;
    }
  }
  // now workspaces contains only top-level items
  for( auto wsName = workspaces.begin(); wsName != workspaces.end(); ++wsName )
  {
    if ( wsName->empty() ) continue;
    QString qName = QString::fromStdString( *wsName );
    auto item = m_tree->findItems( qName, Qt::MatchFixedString );
    if ( item.isEmpty() )
    {
      try
      {
        auto ws = Mantid::API::AnalysisDataService::Instance().retrieve( *wsName );
        addTreeEntry( qName, ws );
      }
      catch(...)
      {
        scheduleFindAbandonedWorkspaces();
        return;
      }
    }
    else
    {
      int i = topItems.indexOf( qName );
      if ( i >= 0 )
      {
        topItems.removeAt( i );
      }
    }
  }
  // if there are some names left in topItems - remove them
  foreach( QString qName, topItems )
  {
    removeWorkspaceEntry( qName );
  }
  foreach(QTreeWidgetItem* item, groupItemsToExpand)
  {
    m_tree->collapseItem( item );
    m_tree->expandItem( item );
  }
}

/**
 * Schedule a re-run of findAbandonedWorkspaces().
 */
void MantidDockWidget::scheduleFindAbandonedWorkspaces()
{
  if ( m_rerunStackSize == 0 )
  {
    ++m_rerunStackSize;
    emit rerunFindAbandonedWorkspaces();
  }
}

/**
 * Add the actions that are appropriate for a MatrixWorkspace
 * @param menu :: The menu to store the items
 * @param matrixWS :: The workspace related to the menu
 */
void MantidDockWidget::addMatrixWorkspaceMenuItems(QMenu *menu, Mantid::API::MatrixWorkspace_const_sptr matrixWS) const
{
  // Add all options except plot of we only have 1 value
  menu->addAction(m_showData);
  menu->addAction(m_showInst);
  // Disable the 'show instrument' option if a workspace doesn't have an instrument attached
  m_showInst->setEnabled( matrixWS->getInstrument() && !matrixWS->getInstrument()->getName().empty() );
  menu->addSeparator();
  menu->addAction(m_plotSpec);
  menu->addAction(m_plotSpecErr);
  //menu->addAction(m_plotSpecDistr);
  // Don't plot a spectrum if only one X value
  m_plotSpec->setEnabled ( matrixWS->blocksize() > 1 );
  m_plotSpecErr->setEnabled ( matrixWS->blocksize() > 1 );
/*
  if( boost::dynamic_pointer_cast<const IEventWorkspace>(matrixWS) )
  {
*/
    menu->addAction(m_showImageViewer); // The 2D image viewer
//  }
  menu->addAction(m_colorFill);
  // Show the color fill plot if you have more than one histogram
  m_colorFill->setEnabled( ( matrixWS->axes() > 1 && matrixWS->getNumberHistograms() > 1) );
  menu->addAction(m_showSliceViewer); // The 2D slice viewer
  menu->addSeparator();
  menu->addAction(m_showDetectors);
  menu->addAction(m_showLogs);
  menu->addAction(m_showHist);
  menu->addAction(m_saveNexus);
}

/**
 * Add the actions that are appropriate for a MDEventWorkspace
 * @param menu :: The menu to store the items
 * @param WS :: The workspace related to the menu
 */
void MantidDockWidget::addMDEventWorkspaceMenuItems(QMenu *menu, Mantid::API::IMDEventWorkspace_const_sptr WS) const
{
  (void) WS;

  //menu->addAction(m_showBoxData); // Show MD Box data (for debugging only)
  menu->addAction(m_showVatesGui); // Show the Vates simple interface
  if (!MantidQt::API::InterfaceManager::hasVatesLibraries())
  {
    m_showVatesGui->setEnabled(false);
  }
  else
  {
    std::size_t nDim = WS->getNonIntegratedDimensions().size();
    m_showVatesGui->setEnabled(nDim >= 3 && nDim < 5);
  }
  menu->addAction(m_showSliceViewer); // The 2D slice viewer
  menu->addAction(m_showHist);  // Algorithm history
  menu->addAction(m_showListData); // Show data in table
  menu->addAction(m_showLogs);
}

void MantidDockWidget::addMDHistoWorkspaceMenuItems(QMenu *menu, Mantid::API::IMDWorkspace_const_sptr WS) const
{
  (void) WS;
  menu->addAction(m_showHist); // Algorithm history
  menu->addAction(m_showVatesGui); // Show the Vates simple interface
  if (!MantidQt::API::InterfaceManager::hasVatesLibraries())
  {
    m_showVatesGui->setEnabled(false);
  }
  else
  {
    std::size_t nDim = WS->getNonIntegratedDimensions().size();
    m_showVatesGui->setEnabled(nDim >= 3 && nDim < 5);
  }
  menu->addAction(m_showSliceViewer); // The 2D slice viewer
  menu->addAction(m_showMDPlot); // A plot of intensity vs bins
  menu->addAction(m_showListData); // Show data in table
  menu->addAction(m_convertMDHistoToMatrixWorkspace);
  menu->addAction(m_showLogs);
}


/** Add the actions that are appropriate for a PeaksWorkspace
 * @param menu :: The menu to store the items
 * @param WS :: The workspace related to the menu
 */
void MantidDockWidget::addPeaksWorkspaceMenuItems(QMenu *menu, Mantid::API::IPeaksWorkspace_const_sptr WS) const
{
  (void) WS;
  menu->addAction(m_showData);
  menu->addAction(m_showVatesGui); // Show the Vates simple interface
  if (!MantidQt::API::InterfaceManager::hasVatesLibraries())
  {
    m_showVatesGui->setEnabled(false);
  }
  menu->addSeparator();
  menu->addAction(m_showDetectors);
  menu->addAction(m_showHist);
}

/**
 * Add the actions that are appropriate for a MatrixWorkspace
 * @param menu :: The menu to store the items
 */
void MantidDockWidget::addWorkspaceGroupMenuItems(QMenu *menu) const
{
  m_plotSpec->setEnabled(true);
  menu->addAction(m_plotSpec);
  m_plotSpecErr->setEnabled(true);
  menu->addAction(m_plotSpecErr);
  menu->addAction(m_colorFill);
  m_colorFill->setEnabled(true);
  menu->addSeparator();
  menu->addAction(m_saveNexus);
}

/**
 * Add the actions that are appropriate for a MatrixWorkspace
 * @param menu :: The menu to store the items
 */
void MantidDockWidget::addTableWorkspaceMenuItems(QMenu * menu) const
{
  menu->addAction(m_showData);
  menu->addAction(m_showTransposed);
  menu->addAction(m_showHist);
  menu->addAction(m_saveNexus);
  menu->addAction(m_convertToMatrixWorkspace);
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
    MantidMatrix* m = dynamic_cast<MantidMatrix*>(m_mantidUI->appWindow()->activeWindow());
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

void MantidDockWidget::sortAscending()
{
  m_tree->setSortOrder(Qt::Ascending);
  m_tree->sortItems(m_tree->sortColumn(), Qt::Ascending);
}

void MantidDockWidget::sortDescending()
{
   m_tree->setSortOrder(Qt::Descending);
   m_tree->sortItems(m_tree->sortColumn(), Qt::Descending);
}

void MantidDockWidget::chooseByName()
{
  m_tree->setSortScheme(ByName);
}

void MantidDockWidget::chooseByLastModified()
{
  m_tree->setSortScheme(ByLastModified);
}

void MantidDockWidget::excludeItemFromSort(MantidTreeWidgetItem *item)
{
  static int counter = 0;

  item->setData(0,Qt::UserRole, counter);

  counter++;
}

/**
* Saves a workspace based on the program the user chooses to save to.
* @param name :: A string containing the name of the program
*/

void MantidDockWidget::saveToProgram(const QString & name)
{
  //Create a map for the keys and details to go into
  std::map<std::string,std::string> programKeysAndDetails;
  programKeysAndDetails["name"] = name.toStdString();

  //Get a list of the program detail keys (mandatory - target, saveusing) (optional - arguments, save parameters, workspace type)
  std::vector<std::string> programKeys = (Mantid::Kernel::ConfigService::Instance().getKeys(("workspace.sendto." + programKeysAndDetails.find("name")->second)));

  for (size_t i=0; i<programKeys.size(); i++)
  {
    //Assign a key to its value using the map
    programKeysAndDetails[programKeys[i]] = (Mantid::Kernel::ConfigService::Instance().getString(("workspace.sendto." + programKeysAndDetails.find("name")->second + "." + programKeys[i])));
  }
     
  //Check to see if mandatory information is included
  if ((programKeysAndDetails.count("name") != 0) && (programKeysAndDetails.count("target") != 0) && (programKeysAndDetails.count("saveusing") != 0))    
  {
    std::string expTarget = Poco::Path::expand(programKeysAndDetails.find("target")->second);

    QFileInfo target = QString::fromStdString(expTarget);
    if(target.exists())
    {
      try
      {
        //Setup a shared pointer for the algorithm using the appropriate save type
        Mantid::API::IAlgorithm_sptr alg;

        //Convert to QString and create Algorithm
        QString saveUsing = QString::fromStdString(programKeysAndDetails.find("saveusing")->second);

        //Create a new save based on what files the new program can open
        alg = m_mantidUI->createAlgorithm(saveUsing);

        //Get the file extention based on the workspace
        Property* prop = alg->getProperty("Filename");
        FileProperty *fileProp = dynamic_cast<FileProperty*>(prop);
        std::string ext;
        if(fileProp)
        {
          ext = fileProp->getDefaultExt();
        }

        //Save as.. default save + the file type i.e .nxs
        alg->setPropertyValue("fileName", "auto_save_" + selectedWsName.toStdString() + ext);

        //Save the workspace
        alg->setPropertyValue("InputWorkspace", selectedWsName.toStdString());

        //If there are any save parameters
        if (programKeysAndDetails.count("saveparameters") != 0)
        {
          QString saveParametersGrouped = QString::fromStdString(programKeysAndDetails.find("saveparameters")->second);
          QStringList saveParameters = saveParametersGrouped.split(',');

          //For each one found split it up and assign the parameter
          for (int i = 0; i<saveParameters.size(); i++)
          {
            QStringList sPNameAndDetail = saveParameters[i].split('=');
            std::string saveParameterName = sPNameAndDetail[0].trimmed().toStdString();
            std::string saveParameterDetail = sPNameAndDetail[1].trimmed().toStdString();
            if(saveParameterDetail == "True")
              alg->setProperty(saveParameterName, true);
            else if(saveParameterDetail == "False")
              alg->setProperty(saveParameterName, false);
            else  //if not true or false then must be a value
            {
              alg->setPropertyValue(saveParameterName, saveParameterDetail);
            }
          }
        }

        //Execute the save
        m_mantidUI->executeAlgorithmAsync(alg, true);
        //alg->execute();

        //Get the save location of the file (should be default Mantid folder)
        //std::string savedFile = alg->getProperty("Filename");
        QString savedFile = QString::fromStdString(alg->getProperty("Filename"));
        QStringList arguments;

        //Arguments for the program to take. Default will be the file anyway.
        if (programKeysAndDetails.count("arguments") != 0)
        {
          QString temp = QString::fromStdString(programKeysAndDetails.find("arguments")->second);
          temp.replace(QString("[file]"), savedFile);
          //temp.replace(QString("[user]"), user;
          arguments = temp.split(",");
        }
        else
          arguments.insert(0, savedFile);
  
        //convert the list into a standard vector for compatibility with Poco
        std::vector<std::string> argumentsV;

        for (int i = 0; i<arguments.size(); i++)
        {
          argumentsV.assign(1, (arguments[i].toStdString()));
        }
    
        //Execute the program
        try
        {
          Mantid::Kernel::ConfigService::Instance().launchProcess(expTarget, argumentsV);
        }
        catch(std::runtime_error&)
        {
          QMessageBox::information(this, "Error", "User tried to open program from: " + QString::fromStdString(expTarget) + " There was an error opening the program. Please check the target and arguments list to ensure that these are correct");
        }
      }
      catch(std::exception&)
      {
        QMessageBox::information(this, "Mantid - Send to Program", "A file property wasn't found. Please check that the correct"
          + QString("save algorithm was used.\n(View -> Preferences -> Mantid -> SendTo -> Edit -> SaveUsing)") );
      }
    }
    else
      QMessageBox::information(this, "Target Path Error", "User tried to open program from: " + QString::fromStdString(expTarget) + " The target file path for the program can't be found. Please check that the full path is correct");
  }
}

	
void MantidDockWidget::renameWorkspace()
{
  //get selected workspace
  QList<QTreeWidgetItem*>selectedItems=m_tree->selectedItems();
  QStringList selctedwsNames;
  if(!selectedItems.empty())
  {
    for(int i=0; i < selectedItems.size(); ++i)
    {
       selctedwsNames.append(selectedItems[i]->text(0));
    }
  }
  m_mantidUI->renameWorkspace(selctedwsNames);
}

void MantidDockWidget::showDetectorTable()
{
  //get selected workspace
  QList<QTreeWidgetItem*>selectedItems=m_tree->selectedItems();
  QString selctedwsName;
  if(!selectedItems.empty())
  {
    selctedwsName=selectedItems[0]->text(0);
  }
  m_mantidUI->createDetectorTable(selctedwsName,std::vector<int>());
}

void MantidDockWidget::popupMenu(const QPoint & pos)
{
  QTreeWidgetItem* treeItem = m_tree->itemAt(pos);
  selectedWsName = "";
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
    menu->setObjectName("WorkspaceContextMenu");
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
      addMatrixWorkspaceMenuItems(menu, matrixWS);
    }
    else if( IMDEventWorkspace_const_sptr mdeventWS = boost::dynamic_pointer_cast<const IMDEventWorkspace>(ws) )
    {
      addMDEventWorkspaceMenuItems(menu, mdeventWS);
    }
    else if( IMDWorkspace_const_sptr mdWS = boost::dynamic_pointer_cast<const IMDWorkspace>(ws) )
    {
      addMDHistoWorkspaceMenuItems(menu, mdWS);
    }
    else if( IPeaksWorkspace_const_sptr peaksWS = boost::dynamic_pointer_cast<const IPeaksWorkspace>(ws) )
    {
      addPeaksWorkspaceMenuItems(menu, peaksWS);
    }
    else if( boost::dynamic_pointer_cast<const WorkspaceGroup>(ws) ) 
    {
      addWorkspaceGroupMenuItems(menu);
    }
    else if( boost::dynamic_pointer_cast<const Mantid::API::ITableWorkspace>(ws) )
    {
      addTableWorkspaceMenuItems(menu);
    }
    
    //Get the names of the programs for the send to option
    std::vector<std::string> programNames = (Mantid::Kernel::ConfigService::Instance().getKeys("workspace.sendto.name"));
    bool firstPass(true);
    //Check to see if any options aren't visible
    for (size_t i = 0; i<programNames.size(); i++)
    {
      std::string visible = Mantid::Kernel::ConfigService::Instance().getString("workspace.sendto." + programNames[i] + ".visible");
      std::string target = Mantid::Kernel::ConfigService::Instance().getString("workspace.sendto." + programNames[i] + ".target");
      if (Mantid::Kernel::ConfigService::Instance().isExecutable(target) && visible == "Yes")
      {
        bool compatible(true);
        std::string saveUsing(Mantid::Kernel::ConfigService::Instance().getString("workspace.sendto." + programNames[i] + ".saveusing") );
        try
        {
          Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create(saveUsing);
          alg->setPropertyValue("InputWorkspace", selectedWsName.toStdString() );
        }
        catch(std::exception&)
        {
          compatible = false;
        }
        if (compatible != false)
        {
          if (firstPass == true)
          {
            m_saveToProgram = new QMenu(tr("Send to"),this);
	          menu->addMenu(m_saveToProgram);

	          //Sub-menu for program list
	          m_programMapper = new QSignalMapper(this);
          }
          QString name = QString::fromStdString(programNames[i]);
          //Setup new menu option for the program
          m_program = new QAction(tr(name),this);
          connect(m_program,SIGNAL(activated()),m_programMapper,SLOT(map()));
          //Send name of program when clicked
          m_programMapper->setMapping(m_program, name);
          m_saveToProgram->addAction(m_program);		
            
          // Set first pass to false so that it doesn't set up another menu entry for all programs.
          firstPass = false;
        }
      } 
    }
    
    //Tell the button what to listen for and what to do once clicked (if there is anything to connect it will be set to false)
    if (firstPass == false)    
      connect(m_programMapper, SIGNAL(mapped(const QString &)), this, SLOT(saveToProgram(const QString &)));

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
  const QMultiMap<QString,std::set<int> > toPlot = m_tree->chooseSpectrumFromSelected();
  // An empty map will be returned if the user clicks cancel in the spectrum selection
  if (toPlot.empty()) return;

  m_mantidUI->plotSpectraList(toPlot, false);
}

/// Plots a single spectrum from each selected workspace
void MantidDockWidget::plotSpectraDistribution()
{
  const QMultiMap<QString,std::set<int> > toPlot = m_tree->chooseSpectrumFromSelected();
  // An empty map will be returned if the user clicks cancel in the spectrum selection
  if (toPlot.empty()) return;
  
  m_mantidUI->plotSpectraList(toPlot, false, true );
}

/// Plots a single spectrum from each selected workspace with errors
void MantidDockWidget::plotSpectraErr()
{
  const QMultiMap<QString,std::set<int> > toPlot = m_tree->chooseSpectrumFromSelected();
  // An empty map will be returned if the user clicks cancel in the spectrum selection
  if (toPlot.empty()) return;
  
  m_mantidUI->plotSpectraList(toPlot, true);
}

/// Plots a single spectrum from each selected workspace with erros
void MantidDockWidget::plotSpectraDistributionErr()
{
  const QMultiMap<QString,std::set<int> > toPlot = m_tree->chooseSpectrumFromSelected();
  // An empty map will be returned if the user clicks cancel in the spectrum selection
  if (toPlot.empty()) return;
  
  m_mantidUI->plotSpectraList(toPlot, true, true );
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

/**
 * Convert selected TableWorkspace to a MatrixWorkspace.
 */
void MantidDockWidget::convertToMatrixWorkspace()
{
  m_mantidUI->executeAlgorithm("ConvertTableToMatrixWorkspace",-1);
}

/**
 * Convert selected MDHistoWorkspace to a MatrixWorkspace.
 */
void MantidDockWidget::convertMDHistoToMatrixWorkspace()
{
  m_mantidUI->executeAlgorithm("ConvertMDHistoToMatrixWorkspace",-1);
}

//------------ MantidTreeWidget -----------------------//

MantidTreeWidget::MantidTreeWidget(QWidget *w, MantidUI *mui):QTreeWidget(w),m_mantidUI(mui),m_sortScheme()
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

  QStringList wsnames = getSelectedWorkspaceNames();
  if (wsnames.size() == 0) return;
  mimeData->setText("Workspace::"+getSelectedWorkspaceNames()[0]);
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
    /// This relies on the item descriptions being up-to-date
    /// so ensure that they are or if something was
    /// replaced then it might not be correct.
    static_cast<MantidDockWidget*>(parentWidget())->populateChildData(*it);

    // Look for children (workspace groups)
    QTreeWidgetItem *child = (*it)->child(0);
    if ( child && child->text(0) == "WorkspaceGroup" )
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
QMultiMap<QString,std::set<int> > MantidTreeWidget::chooseSpectrumFromSelected() const
{
  // Get hold of the names of all the selected workspaces
  QList<QString> allWsNames = this->getSelectedWorkspaceNames();
  QList<QString> wsNames;

  for (int i=0; i<allWsNames.size(); i++)
  {
    if (AnalysisDataService::Instance().retrieve(allWsNames[i].toStdString())->id() != "TableWorkspace")
      wsNames.append(allWsNames[i]);
  }

  // cppcheck-suppress redundantAssignment
  QList<QString>::const_iterator it = wsNames.constBegin();

  // Check to see if all workspaces have a *single* histogram ...
  QList<size_t> wsSizes;
  it = wsNames.constBegin();
  size_t maxHists = 0;
  for ( ; it != wsNames.constEnd(); ++it )
  {
    MatrixWorkspace_const_sptr ws = boost::dynamic_pointer_cast<const MatrixWorkspace>(AnalysisDataService::Instance().retrieve((*it).toStdString()));
    if ( !ws ) continue;
    const size_t currentHists = ws->getNumberHistograms();
    wsSizes.append(currentHists);
    if ( currentHists > maxHists ) maxHists = currentHists;
  }

  QMultiMap<QString,std::set<int> > toPlot;

  // ... if so, no need to ask user which one to plot - just go!
  if(maxHists == 1)
  {
    it = wsNames.constBegin();
    for ( ; it != wsNames.constEnd(); ++it )
    {
      std::set<int> zero;
      zero.insert(0);
      toPlot.insert((*it),zero);
    }

    return toPlot;
  }

  MantidWSIndexDialog *dio = new MantidWSIndexDialog(m_mantidUI, 0, wsNames);
  dio->exec();
  return dio->getPlots();
}

void MantidTreeWidget::setSortScheme(MantidItemSortScheme sortScheme)
{
  m_sortScheme = sortScheme;
}

void MantidTreeWidget::setSortOrder(Qt::SortOrder sortOrder)
{
  m_sortOrder = sortOrder;
}

Qt::SortOrder MantidTreeWidget::getSortOrder() const
{
  return m_sortOrder;
}

MantidItemSortScheme MantidTreeWidget::getSortScheme() const
{
  return m_sortScheme;
}


//-------------------- MantidTreeWidgetItem ----------------------//
/**Constructor.
 * Must be passed its parent MantidTreeWidget, to facilitate correct sorting.
 */
MantidTreeWidgetItem::MantidTreeWidgetItem(MantidTreeWidget* parent):QTreeWidgetItem(parent)
{
  m_parent = parent;
}

/**Constructor.
 * Must be passed its parent MantidTreeWidget, to facilitate correct sorting.
 */
MantidTreeWidgetItem::MantidTreeWidgetItem(QStringList list, MantidTreeWidget* parent):QTreeWidgetItem(list)
{
  m_parent = parent;
}

/**Overidden operator.
 * Must be passed its parent MantidTreeWidget, to facilitate correct sorting.
 */
bool MantidTreeWidgetItem::operator<(const QTreeWidgetItem &other)const 
{
  // If this and/or other has been set to have a Qt::UserRole, then
  // it has an accompanying sort order that we must maintain, no matter
  // what the user has seletected in terms of order or scheme.
  
  bool thisShouldBeSorted = data(0,Qt::UserRole).isNull();
  bool otherShouldBeSorted = other.data(0,Qt::UserRole).isNull();

  // just in case m_parent is NULL. I think I saw this once but cannot reproduce.
  if ( !m_parent ) return false;

  if(!thisShouldBeSorted && !otherShouldBeSorted)
  {
    if(m_parent->getSortOrder() == Qt::Ascending)
      return data(0,Qt::UserRole).toInt() < other.data(0,Qt::UserRole).toInt();
    else
      return data(0,Qt::UserRole).toInt() >= other.data(0,Qt::UserRole).toInt();
  }
  else if(thisShouldBeSorted && !otherShouldBeSorted)
  {
    if(m_parent->getSortOrder() == Qt::Ascending)
      return false;
    else
      return true;
  }
  else if(!thisShouldBeSorted && otherShouldBeSorted)
  {
    if(m_parent->getSortOrder() == Qt::Ascending)
      return true;
    else
      return false;
  }

  // If both should be sorted, and the scheme is set to ByName ...
  if(m_parent->getSortScheme() == ByName)
  {
    if(QString::compare(text(0), other.text(0), Qt::CaseInsensitive) < 0)
      return true;
    return false;
  }
  // ... else both should be sorted and the scheme is set to ByLastModified.
  else
  {
    try
    {
      if(childCount() > 0 && other.childCount() > 0)
      {
        const QTreeWidgetItem * other_ptr = &other;

        try
        {
          return getLastModified(this) < getLastModified(other_ptr);
        }
        catch(std::out_of_range &e)
        {
          QMessageBox::warning(m_parent, "Error", e.what());
          return false;
        }
      }
    }
    catch (Mantid::Kernel::Exception::NotFoundError&)
    {
      ;
    }
    return false;
  }
}

/**Finds the date and time of the last modification made to the workspace who's details
 * are found in the given QTreeWidgetItem.
 */
DateAndTime MantidTreeWidgetItem::getLastModified(const QTreeWidgetItem* workspaceWidget)
{
  const QString wsName = workspaceWidget->text(0);
  Mantid::API::Workspace_sptr ws = Mantid::API::AnalysisDataService::Instance().retrieve(wsName.toStdString());
  
  const Mantid::API::WorkspaceHistory wsHist = ws->getHistory();

  if(wsHist.empty())
  {
    throw std::out_of_range("The workspace \"" + wsName.toStdString() +
      "\" has no history and so cannot be sorted by date last modified.");
  }

  const size_t indexOfLast = wsHist.size() - 1;
  AlgorithmHistory lastAlgHist = wsHist.getAlgorithmHistory(indexOfLast);
  DateAndTime output = lastAlgHist.executionDate();

  return output;
}

//-------------------- AlgorithmDockWidget ----------------------//
/** Create a QDockWidget containing:
 * The AlgorithmSelectorWidget
 * The progress bar and Details button
 */
AlgorithmDockWidget::AlgorithmDockWidget(MantidUI *mui, ApplicationWindow *w):
QDockWidget(w),m_progressBar(NULL),m_algID(),m_mantidUI(mui)
{
  setWindowTitle(tr("Algorithms"));
  setObjectName("exploreAlgorithms"); // this is needed for QMainWindow::restoreState()
  setMinimumHeight(150);
  setMinimumWidth(200);
  w->addDockWidget( Qt::RightDockWidgetArea, this );//*/

  //Add the AlgorithmSelectorWidget
  m_selector = new MantidQt::MantidWidgets::AlgorithmSelectorWidget(this);
  connect(m_selector,SIGNAL(executeAlgorithm(const QString &, const int)),
        m_mantidUI,SLOT(executeAlgorithm(const QString &, const int)));

  m_runningLayout = new QHBoxLayout();
  m_runningLayout->setName("testA");

  m_runningButton = new QPushButton("Details");
  m_runningLayout->addStretch();
  m_runningLayout->addWidget(m_runningButton);
  connect(m_runningButton,SIGNAL(clicked()),m_mantidUI,SLOT(showAlgMonitor()));


  QFrame *f = new QFrame(this);
  QVBoxLayout * layout = new QVBoxLayout(f, 4 /*border*/, 4 /*spacing*/);
  f->setLayout(layout);
  layout->addWidget(m_selector);
  layout->addLayout(m_runningLayout);

  setWidget(f);

}

/** Update the list of algorithms in the dock */
void AlgorithmDockWidget::update()
{
  m_selector->update();
}


void AlgorithmDockWidget::updateProgress(void* alg, const double p, const QString& msg, double estimatedTime, int progressPrecision)
{
  if (m_algID.empty()) return;
  if (alg == m_algID.first() && p >= 0 && p <= 100 && m_progressBar)
  {
    m_progressBar->setValue( static_cast<int>(p) );
    // Make the progress string
    std::ostringstream mess;
    mess << msg.toStdString();
    mess.precision(progressPrecision);
    mess << " " << std::fixed << p << "%";
    if (estimatedTime > 0.5)
    {
      mess.precision(0);
      mess << " (~";
      if (estimatedTime < 60)
        mess << static_cast<int>(estimatedTime) << "s";
      else if (estimatedTime < 60*60)
      {
        int min = static_cast<int>(estimatedTime/60);
        int sec = static_cast<int>(estimatedTime - min*60);
        mess << min << "m"
            << std::setfill('0') << std::setw(2) << sec << "s";
      }
      else
      {
        int hours = static_cast<int>(estimatedTime/3600);
        int min = static_cast<int>( (estimatedTime-hours*3600)/60);
        mess << hours << "h"
            << std::setfill('0') << std::setw(2) << min << "h";
      }
      mess << ")";
    }
    QString formatStr = QString::fromStdString(mess.str());
    m_progressBar->setFormat(formatStr);
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
  m_algID.removeAll(alg);
  hideProgressBar();
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


//--------------------  ----------------------//

