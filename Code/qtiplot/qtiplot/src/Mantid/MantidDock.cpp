#include "MantidDock.h"
#include "MantidUI.h"
#include "../ApplicationWindow.h"
#include "../pixmaps.h"
#include <MantidAPI/AlgorithmFactory.h>
#include <MantidAPI/MemoryManager.h>

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

#include <map>
#include <iostream>

using namespace std;
using namespace Mantid::API;

Mantid::Kernel::Logger& MantidDockWidget::logObject=Mantid::Kernel::Logger::get("mantidDockWidget");
Mantid::Kernel::Logger& MantidTreeWidget::logObject=Mantid::Kernel::Logger::get("MantidTreeWidget");

MantidDockWidget::MantidDockWidget(MantidUI *mui, ApplicationWindow *parent) :
    QDockWidget(tr("Mantid Workspaces"),parent), m_mantidUI(mui)
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

  QMenu *loadMenu = new QMenu(this);
  QAction *loadRawAction = new QAction("Load RAW file",this);
  connect(loadRawAction,SIGNAL(activated()),m_mantidUI,SLOT(loadWorkspace()));
  QAction *loadDAEAction = new QAction("Load from DAE",this);
  connect(loadDAEAction,SIGNAL(activated()),m_mantidUI,SLOT(loadDAEWorkspace()));

  QAction *loadNexusAction = new QAction("Load Nexus",this);
  connect(loadNexusAction,SIGNAL(activated()),m_mantidUI,SLOT(loadNexusWorkspace()));

  loadMenu->addAction(loadRawAction);
  loadMenu->addAction(loadDAEAction);
  loadMenu->addAction(loadNexusAction);
  m_loadButton->setMenu(loadMenu);

  connect(m_deleteButton,SIGNAL(clicked()),this,SLOT(deleteWorkspaces()));
  connect(m_tree,SIGNAL(itemClicked(QTreeWidgetItem*, int)),this,SLOT(clickedWorkspace(QTreeWidgetItem*, int)));
  connect(m_tree,SIGNAL(itemSelectionChanged()),this,SLOT(workspaceSelected()));
  connect(m_groupButton,SIGNAL(clicked()),this,SLOT(groupOrungroupWorkspaces()));

  m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_tree, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(popupMenu(const QPoint &)));

  connect(m_mantidUI, SIGNAL(workspace_added(const QString &, Mantid::API::Workspace_sptr)),
          this, SLOT(updateWorkspaceEntry(const QString &, Mantid::API::Workspace_sptr)));
  connect(m_mantidUI, SIGNAL(workspace_replaced(const QString &, Mantid::API::Workspace_sptr)),
          this, SLOT(updateWorkspaceEntry(const QString &, Mantid::API::Workspace_sptr)));
  connect(m_mantidUI, SIGNAL(workspace_removed(const QString &)),
          this, SLOT(removeWorkspaceEntry(const QString &)));
  connect(m_mantidUI, SIGNAL(workspaces_cleared()), m_tree, SLOT(clear()));

  connect(m_tree,SIGNAL(itemSelectionChanged()),this,SLOT(treeSelectionChanged()));
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

void MantidDockWidget::updateWorkspaceEntry(const QString & ws_name, Mantid::API::Workspace_sptr workspace)
{
  bool bGroupParent=false;
  bGroupParent=isItWorkspaceGroupParentItem(workspace);
  populateWorkspaceTree(ws_name,workspace,bGroupParent);
}

void MantidDockWidget::populateWorkspaceTree(const QString & ws_name, Mantid::API::Workspace_sptr workspace,bool bGroupParent)
{
  // This check is here because the signals don't get delivered immediately when the add/replace notification in MantidUI
  // is recieved. The signal cannot be removed in favour of a direct call because the call is from a separate thread.
  if( !Mantid::API::AnalysisDataService::Instance().doesExist(ws_name.toStdString()) ) return;
  QTreeWidgetItem *ws_item = NULL;
  //This will only ever be of size zero or one
  QList<QTreeWidgetItem *> name_matches = m_tree->findItems(ws_name, Qt::MatchFixedString);
  if( name_matches.isEmpty() )
  {
    ws_item = new QTreeWidgetItem(QStringList(ws_name));
  }
  else
  {	ws_item = name_matches[0];
    ws_item->takeChildren();
  }
  QTreeWidgetItem*  wsid_item=new QTreeWidgetItem(QStringList(QString::fromStdString(workspace->id())));
  wsid_item->setFlags(Qt::NoItemFlags);
  ws_item->addChild(wsid_item);

  if(bGroupParent)
  {	ws_item->setIcon(0,QIcon(QPixmap(mantid_wsgroup_xpm)));
    m_tree->addTopLevelItem(ws_item);
  }
  else
  {
    try
    {
      Mantid::API::Workspace_sptr parentWS;
      QString parentName;
      Mantid::API::WorkspaceGroup_sptr grpWSsptr;
      //getting the group parent workspace name from the group member workspace name
      int index=ws_name.lastIndexOf ("_",-1,Qt::CaseSensitive);
      if(index!=-1)
      {
        parentName=ws_name.left(index);
        if(Mantid::API::AnalysisDataService::Instance().doesExist(parentName.toStdString()))
        {
          parentWS=Mantid::API::AnalysisDataService::Instance().retrieve(parentName.toStdString());
          grpWSsptr=boost::dynamic_pointer_cast<WorkspaceGroup>(parentWS);
        }
      }
      if(isItWorkspaceGroupItem(grpWSsptr,ws_name))
      {
        // at this point ws_name is workspace group member
        //search for the group parent workspace in workspace tree
        QList<QTreeWidgetItem*> matchedNames=m_tree->findItems(parentName,Qt::MatchExactly);
        if(!matchedNames.isEmpty())
        {
          //check the group member already exists in the tree
          //this check & deletion is done bcoz sometiomes when script executes group workspaces members are misplaced.
          QList<QTreeWidgetItem*> findNames=m_tree->findItems(ws_name,Qt::MatchExactly);
          if(!findNames.isEmpty())
          {	//if the group member exists in the tree then delete it
            int index=m_tree->indexOfTopLevelItem(ws_item);
            if(index!=-1)
              m_tree->takeTopLevelItem(index);
          }
          //add the workspace group member to group parent
          matchedNames[0]->addChild(ws_item);
        }
      }
      else //non group workspace
      {m_tree->addTopLevelItem(ws_item);
      }
    }//end of try
    catch(Mantid::Kernel::Exception::NotFoundError &e)//if not a valid object in analysis data service
    {
      logObject.error()<<"Error:"<<e.what()<<std::endl;
    }
    catch (std::runtime_error &ex)
    {
      logObject.error()<<"Error: "<< ex.what()<<std::endl;
    }

  }//end of else loop

  populateWorkspaceData(workspace,ws_item);
}

void MantidDockWidget::populateWorkspaceData(Mantid::API::Workspace_sptr workspace, QTreeWidgetItem* ws_item)
{
  Mantid::API::MatrixWorkspace_sptr ws_ptr = boost::dynamic_pointer_cast<MatrixWorkspace>(workspace);
  if( ws_ptr )
  {
    ws_item->setIcon(0,QIcon(QPixmap(mantid_matrix_xpm)));
    QTreeWidgetItem* numHists = new QTreeWidgetItem(QStringList("Histograms: "+QString::number(ws_ptr->getNumberHistograms())));
    numHists->setFlags(Qt::NoItemFlags);
    ws_item->addChild(numHists);
    QTreeWidgetItem* numBins = new QTreeWidgetItem(QStringList("Bins: "+QString::number(ws_ptr->blocksize())));
    numBins->setFlags(Qt::NoItemFlags);
    ws_item->addChild(numBins);
    QTreeWidgetItem* isHistogram = new QTreeWidgetItem(QStringList(ws_ptr->isHistogramData() ? "Histogram" : "Data points"));
    isHistogram->setFlags(Qt::NoItemFlags);
    ws_item->addChild(isHistogram);
    std::string s = "X axis: ";
    if (ws_ptr->axes() > 0 )
    {
      Mantid::API::Axis *ax = ws_ptr->getAxis(0);
      if ( ax && ax->unit() ) s += ax->unit()->caption() + " / " + ax->unit()->label();
      else s += "Not set";
    }
    else
    {
      s += "N/A";
    }
    QTreeWidgetItem* xUnit = new QTreeWidgetItem(QStringList(QString::fromStdString(s)));
    xUnit->setFlags(Qt::NoItemFlags);
    ws_item->addChild(xUnit);
    s = "Y axis: " + ws_ptr->YUnit();
    QTreeWidgetItem* yUnit = new QTreeWidgetItem(QStringList(QString::fromStdString(s)));
    yUnit->setFlags(Qt::NoItemFlags);
    ws_item->addChild(yUnit);
    QTreeWidgetItem* footprint = new QTreeWidgetItem(QStringList("Memory used: "+QString::number(ws_ptr->getMemorySize())+" KB"));
    footprint->setFlags(Qt::NoItemFlags);
    ws_item->addChild(footprint);
  }
  else
  {
    Mantid::API::ITableWorkspace_sptr ws_ptr = boost::dynamic_pointer_cast<ITableWorkspace>(workspace);
    if( ws_ptr )
    {
      ws_item->setIcon(0,QIcon(QPixmap(worksheet_xpm)));
    }
  }
}

bool MantidDockWidget::isItWorkspaceGroupItem(Mantid::API::WorkspaceGroup_sptr grpSPtr,const QString& ws_name) //std::vector<std::string>wsGroupNames,const QString& ws_name)
{
  if(!grpSPtr)
  {	return false;
  }
  const std::vector<std::string> & wsGroupNames=grpSPtr->getNames();
  if(wsGroupNames.empty())return false;
  std::vector<std::string>::const_iterator it;
  //if the name is there in m_wsGroupNames vector it's workspace group member,return then
  for(it=wsGroupNames.begin();it!=wsGroupNames.end();++it)
  {
    if(ws_name.toStdString()==(*it))
    {return true;
    }
  }
  return false;
}

bool MantidDockWidget::isItWorkspaceGroupParentItem(Mantid::API::Workspace_sptr workspace)
{
  try
  {
    Mantid::API::WorkspaceGroup_sptr grpWSsptr=boost::dynamic_pointer_cast< WorkspaceGroup>(workspace);
    if(grpWSsptr)
    {  return true;
    }
    else return false;
  }
  catch (std::runtime_error &ex)
  {
    logObject.error()<<"Error:"<< ex.what()<<std::endl;
    return false;

  }
}


void MantidDockWidget::removeWorkspaceEntry(const QString & ws_name)
{
  //This will only ever be of size zero or one
  QList<QTreeWidgetItem *> name_matches = m_tree->findItems(ws_name,Qt::MatchFixedString);
  if( name_matches.isEmpty() )return;
  m_tree->takeTopLevelItem(m_tree->indexOfTopLevelItem(name_matches[0]));

}

void MantidDockWidget::clickedWorkspace(QTreeWidgetItem* item, int)
{


}
void MantidDockWidget::workspaceSelected()
{ QList<QTreeWidgetItem*> selectedItems=m_tree->selectedItems();
  if(selectedItems.isEmpty()) return;
  QString wsName=selectedItems[0]->text(0);
  if(Mantid::API::AnalysisDataService::Instance().doesExist(wsName.toStdString()))
    m_mantidUI->enableSaveNexus(wsName);
}

/**
     deleteWorkspaces
*/
void MantidDockWidget::deleteWorkspaces()
{
  QList<QTreeWidgetItem*> items = m_tree->selectedItems();
  QList<QTreeWidgetItem*>::const_iterator itr=items.constBegin();
  for (itr = items.constBegin(); itr != items.constEnd(); ++itr)
  {
    int count=(*itr)->childCount();
    //for loop for removing the children
    for (int i=0;i<count;i++)
    {
      QTreeWidgetItem *pchild=(*itr)->child(0);
      if(Mantid::API::AnalysisDataService::Instance().doesExist(pchild->text(0).toStdString()))
      {
        m_mantidUI->deleteWorkspace(pchild->text(0));
      }

      (*itr)->takeChild(0);
    }
    //now remove the parent
    if(Mantid::API::AnalysisDataService::Instance().doesExist((*itr)->text(0).toStdString()))
      m_mantidUI->deleteWorkspace((*itr)->text(0));
    QTreeWidgetItem* topItem=m_tree->topLevelItem(0);
    if(topItem)topItem->removeChild((*itr));

  }//end of for loop for selected items
}

void MantidDockWidget::popupMenu(const QPoint & pos)
{
  QTreeWidgetItem* treeItem = m_tree->itemAt(pos);
  QString selectedWsName("");
  if( treeItem ) selectedWsName = treeItem->text(0);
  else m_tree->selectionModel()->clear();
  QMenu *menu = new QMenu(this);

  //If no workspace is here then have load raw and dae
  if( selectedWsName.isEmpty() )
  {
    QAction *action = new QAction("Load RAW file",this);
    connect(action,SIGNAL(triggered()),m_mantidUI,SLOT(loadWorkspace()));
    menu->addAction(action);

    action = new QAction("Load from DAE",this);
    connect(action,SIGNAL(triggered()),m_mantidUI,SLOT(loadDAEWorkspace()));
    menu->addAction(action);

    action = new QAction("Load Nexus",this);
    connect(action,SIGNAL(triggered()),m_mantidUI,SLOT(loadNexusWorkspace()));
    menu->addAction(action);

  }
  //else show instrument, sample logs and delete
  else
  {
    Mantid::API::Workspace_const_sptr grpWSPstr;
    bool singleValueWS = false;
    bool bDisable = false;
    try
    {
      Mantid::API::Workspace_const_sptr ws = Mantid::API::AnalysisDataService::Instance().retrieve(selectedWsName.toStdString());
      grpWSPstr = boost::dynamic_pointer_cast<const WorkspaceGroup>(ws);
      // Check for single-valued workspaces because they don't like to be plotted!
      if ( ws->id() == "WorkspaceSingleValue" ) singleValueWS = true;
    }
    catch(Mantid::Kernel::Exception::NotFoundError &e)//if not a valid object in analysis data service
    {
      bDisable=true;

    }
    QAction *action = new QAction("Show data",this);
    connect(action,SIGNAL(triggered()),m_mantidUI,SLOT(importWorkspace()));
    menu->addAction(action);
    if(grpWSPstr|| bDisable)
    {action->setEnabled(false);
    }

    action = new QAction("Show instrument",this);
    connect(action,SIGNAL(triggered()),m_mantidUI,SLOT(showMantidInstrumentSelected()));
    menu->addAction(action);
    if(grpWSPstr||bDisable)
    {action->setEnabled(false);
    }

    action = new QAction("Plot spectrum...",this);
    connect(action,SIGNAL(triggered()),this,SLOT(plotSpectra()));
    menu->addAction(action);
    if( bDisable || singleValueWS )
    {
      action->setEnabled(false);
    }

    action = new QAction("Sample Logs...", this);
    connect(action,SIGNAL(triggered()),m_mantidUI,SLOT(showLogFileWindow()));
    menu->addAction(action);
    if(grpWSPstr||bDisable)
    {action->setEnabled(false);
    }

    action = new QAction("Show History", this);
    connect(action,SIGNAL(triggered()),m_mantidUI,SLOT(showAlgorithmHistory()));
    menu->addAction(action);
    if(grpWSPstr||bDisable)
    {action->setEnabled(false);
    }

    action = new QAction("Save Nexus",this);
    connect(action,SIGNAL(activated()),m_mantidUI,SLOT(saveNexusWorkspace()));
    menu->addAction(action);
    if(bDisable)
    {action->setEnabled(false);
    }

    action = new QAction("Rename Workspace",this);
    connect(action,SIGNAL(activated()),m_mantidUI,SLOT(renameWorkspace()));
    menu->addAction(action);
    if(bDisable)
    {action->setEnabled(false);
    }


    //separate delete
    menu->addSeparator();

    action = new QAction("Delete workspace",this);
    connect(action,SIGNAL(triggered()),this,SLOT(deleteWorkspaces()));
    menu->addAction(action);
    if(bDisable)
    {action->setEnabled(false);
    }

  }

  menu->popup(QCursor::pos());
}

void MantidDockWidget::groupOrungroupWorkspaces()
{
  if(m_groupButton)
  {
    QString qButtonName=m_groupButton->text();
    if(!qButtonName.compare("Group"))
      m_mantidUI->groupWorkspaces();
    else
    {
      if(!qButtonName.compare("UnGroup"))
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

void MantidDockWidget::treeSelectionChanged()
{
  //logObject.error()<<"Item changed"<<std::endl;
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
      //logObject.debug()<<"Name=  "<<wsName.toStdString()<<"normal workspace"<<"  " << std::endl;
      if ( ! wsName.isEmpty() )
      {
        m_mantidUI->importWorkspace(wsName,false);
        return;
      }
    }
  }
  catch(Mantid::Kernel::Exception::NotFoundError &e)
  {
    return;
  }
  QTreeWidget::mouseDoubleClickEvent(e);
}

/** Returns a list of all selected workspaces
 *  (including members of groups if appropriate)
 */
QList<QString> MantidTreeWidget::getSelectedWorkspaceNames() const
{
  QList<QString> names;
  const QList<QTreeWidgetItem*> items = this->selectedItems();
  QList<QTreeWidgetItem*>::const_iterator it;
  // Need to look for workspace groups and add all children if found
  for (it = items.constBegin(); it != items.constEnd(); ++it)
  {
    // Look for children (workspace groups)
    if ( (*it)->child(0)->text(0) == "WorkspaceGroup" )
    {
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
    QDockWidget(w)
{
  m_mantidUI = mui;
  setWindowTitle(tr("Mantid Algorithms"));
  setObjectName("exploreAlgorithms"); // this is needed for QMainWindow::restoreState()
  setMinimumHeight(150);
  setMinimumWidth(200);
  w->addDockWidget( Qt::RightDockWidgetArea, this );//*/

  QFrame *f = new QFrame(this);

  m_tree = new AlgorithmTreeWidget(f,mui);
  m_tree->setHeaderLabel("Algorithms");
  connect(m_tree,SIGNAL(itemSelectionChanged()),this,SLOT(treeSelectionChanged()));

  QHBoxLayout * buttonLayout = new QHBoxLayout();
  QPushButton *execButton = new QPushButton("Execute");
  m_findAlg = new FindAlgComboBox;
  m_findAlg->setEditable(true);
  connect(m_findAlg,SIGNAL(editTextChanged(const QString&)),this,SLOT(findAlgTextChanged(const QString&)));
  connect(m_findAlg,SIGNAL(enterPressed()),m_mantidUI,SLOT(executeAlgorithm()));
  connect(execButton,SIGNAL(clicked()),m_mantidUI,SLOT(executeAlgorithm()));

  buttonLayout->addWidget(execButton);
  buttonLayout->addWidget(m_findAlg);
  buttonLayout->addStretch();

  QHBoxLayout * runningLayout = new QHBoxLayout();
  m_runningAlgsLabel = new QLabel("Running 0");
  QPushButton *runningButton = new QPushButton("Details");
  runningLayout->addWidget(m_runningAlgsLabel);
  runningLayout->addStretch();
  runningLayout->addWidget(runningButton);
  connect(runningButton,SIGNAL(clicked()),m_mantidUI,SLOT(showAlgMonitor()));
  //
  QVBoxLayout * layout = new QVBoxLayout();
  f->setLayout(layout);
  layout->addLayout(buttonLayout);
  layout->addWidget(m_tree);
  layout->addLayout(runningLayout);
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

void AlgorithmDockWidget::countChanged(int n)
{
  m_runningAlgsLabel->setText("Running "+QString::number(n));
}

void AlgorithmDockWidget::tst()
{
  MemoryManager::Instance().getMemoryInfo();
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

