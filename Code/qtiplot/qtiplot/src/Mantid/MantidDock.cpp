#include "MantidDock.h"
#include "MantidUI.h"
#include "../ApplicationWindow.h"
#include "../pixmaps.h"
#include <MantidAPI/AlgorithmFactory.h>
#include <MantidAPI/MemoryManager.h>



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

MantidDockWidget::MantidDockWidget(MantidUI *mui, ApplicationWindow *w):
QDockWidget(w)
{
    m_mantidUI = mui;
    setWindowTitle(tr("Mantid Workspaces"));
    setObjectName("exploreMantid"); // this is needed for QMainWindow::restoreState()
    setMinimumHeight(150);
    setMinimumWidth(200);
    w->addDockWidget( Qt::RightDockWidgetArea, this );//*/

    QFrame *f = new QFrame(this);
    setWidget(f);

    m_tree = new MantidTreeWidget(f,m_mantidUI);
    m_tree->setHeaderLabel("Workspaces");

    QHBoxLayout * buttonLayout = new QHBoxLayout();
    m_loadButton = new QPushButton("Load");
    m_deleteButton = new QPushButton("Delete");
    buttonLayout->addWidget(m_loadButton);
    buttonLayout->addWidget(m_deleteButton);
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

    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tree, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(popupMenu(const QPoint &)));

    connect(m_mantidUI, SIGNAL(workspace_added(const QString &, Mantid::API::Workspace_sptr)), 
	    this, SLOT(updateWorkspaceEntry(const QString &, Mantid::API::Workspace_sptr)));
    connect(m_mantidUI, SIGNAL(workspace_replaced(const QString &, Mantid::API::Workspace_sptr)), 
	    this, SLOT(updateWorkspaceEntry(const QString &, Mantid::API::Workspace_sptr)));
    connect(m_mantidUI, SIGNAL(workspace_removed(const QString &)), 
	    this, SLOT(removeWorkspaceEntry(const QString &)));
}

void MantidDockWidget::clearWorkspaceTree()
{
  m_tree->clear();
}


void MantidDockWidget::updateWorkspaceEntry(const QString & ws_name, Mantid::API::Workspace_sptr workspace)
{	
	bool bParent=false;
	bParent=isItWorkspaceGroupParentItem(workspace);
	populateWorkspaceTree(ws_name,workspace,bParent);
}
void MantidDockWidget::populateWorkspaceTree(const QString & ws_name, Mantid::API::Workspace_sptr workspace,bool bParent)
{ 	
     // This check is here because the signals don't get delivered immediately when the add/replace notification in MantidUI
	// is recieved. The signal cannot be removed in favour of a direct call because the call is from a separate thread.
	std::vector<std::string> wsNames;
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
	ws_item->addChild(wsid_item);
	
	wsNames=Mantid::API::AnalysisDataService::Instance().getObjectNames();
	if(!wsNames.empty())
	{
		if(bParent)
		{
			ws_item->setIcon(0,QIcon(QPixmap(mantid_wsgroup_xpm)));
			m_tree->addTopLevelItem(ws_item);
		}
		else 
		{
			std::vector<std::string> wsGroupNames;
			Mantid::API::Workspace_sptr parentWS;
			QString parentName;
			int index=ws_name.lastIndexOf ("_",-1,Qt::CaseSensitive);
			if(index!=-1)
			{
				parentName=ws_name.left(index);
				try
				{//retrieve the  workspace pointer for parent
				  parentWS=Mantid::API::AnalysisDataService::Instance().retrieve(parentName.toStdString());
				}
				catch(Mantid::Kernel::Exception::NotFoundError &e)//if not a valid object in analysis data service
				{
					logObject.error()<<parentName.toStdString()<<" Object not found in ADS"<<std::endl;

				}
				Mantid::API::WorkspaceGroup_sptr grpWSsptr=boost::dynamic_pointer_cast< WorkspaceGroup>(parentWS);
				if(grpWSsptr)wsGroupNames=grpWSsptr->getNames();
			}
			if(isItWorkspaceGroupItem(wsGroupNames,ws_name))
			{
				//search for the parent name in workspace tree
				QList<QTreeWidgetItem*> matchedNames=m_tree->findItems(parentName,Qt::MatchStartsWith);
				if(!matchedNames.isEmpty())	
				{
					matchedNames[0]->addChild(ws_item);
				}
			}
			else //non group workspace 
			{
				m_tree->addTopLevelItem(ws_item);
			}
		}


	}
	
	Mantid::API::MatrixWorkspace_sptr ws_ptr = boost::dynamic_pointer_cast<MatrixWorkspace>(workspace);
	if( ws_ptr )
	{
		ws_item->setIcon(0,QIcon(QPixmap(mantid_matrix_xpm)));
		ws_item->addChild(new QTreeWidgetItem(QStringList("Histograms: "+QString::number(ws_ptr->getNumberHistograms()))));
		ws_item->addChild(new QTreeWidgetItem(QStringList("Bins: "+QString::number(ws_ptr->blocksize()))));
		bool isHistogram = ws_ptr->blocksize() && ws_ptr->isHistogramData();
		ws_item->addChild(new QTreeWidgetItem(QStringList(isHistogram?"Histogram":"Data points")));
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
		ws_item->addChild(new QTreeWidgetItem(QStringList(QString::fromStdString(s)))); 
		s = "Y axis: " + ws_ptr->YUnit();
		ws_item->addChild(new QTreeWidgetItem(QStringList(QString::fromStdString(s))));
		ws_item->addChild(new QTreeWidgetItem(QStringList("Memory used: "+QString::number(ws_ptr->getMemorySize())+" KB")));
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
bool MantidDockWidget::isItWorkspaceGroupItem(const std::vector<std::string> & wsGroupNames,const QString& ws_name)
{
	std::vector<std::string>::const_iterator it;
	
	if(wsGroupNames.empty())return false;
   //if the name is there in m_wsGroupNames vector it's workspace group member,return then
	for(it=wsGroupNames.begin();it!=wsGroupNames.end();it++)
	{
		if(ws_name.toStdString()==(*it))
		{	
			return true;
		}
	}
	return false;

}

bool MantidDockWidget::isItWorkspaceGroupParentItem(Mantid::API::Workspace_sptr workspace)
{	
	Mantid::API::WorkspaceGroup_sptr grpWSsptr=boost::dynamic_pointer_cast< WorkspaceGroup>(workspace);
	if(grpWSsptr)
	{return true;
	}
	else return false;
}


void MantidDockWidget::removeWorkspaceEntry(const QString & ws_name)
{
  //This will only ever be of size zero or one
  QList<QTreeWidgetItem *> name_matches = m_tree->findItems(ws_name,Qt::MatchFixedString);
  if( name_matches.isEmpty() )return;
  m_tree->takeTopLevelItem(m_tree->indexOfTopLevelItem(name_matches[0]));
 
}

void MantidDockWidget::clickedWorkspace(QTreeWidgetItem*, int)
{
}

/**
     deleteWorkspaces
*/
void MantidDockWidget::deleteWorkspaces()
{
  QList<QTreeWidgetItem*> items = m_tree->selectedItems();
  QList<QTreeWidgetItem*>::const_iterator itr=items.constBegin();
  std::vector<std::string>::iterator it;
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
		  //removeFromWSGroupNames(pchild->text(0));
		 
		  (*itr)->takeChild(0);
	  }	  
	  //now remove the parent 
	  if(Mantid::API::AnalysisDataService::Instance().doesExist((*itr)->text(0).toStdString()))
		  m_mantidUI->deleteWorkspace((*itr)->text(0));
	  QTreeWidgetItem* topItem=m_tree->topLevelItem(0);
			if(topItem)topItem->removeChild((*itr));
		//removeFromWSGroupNames((*itr)->text(0));
	
  }//end of for loop for selected items

 /* else
  { QList<QTreeWidgetItem*>::const_iterator it;
  QList<QString> wsNames;

	  // Need two loops because deleting a workspace moves QTreeWidgetItems
	  // First loop over getting workspace names
	  for (it = items.constBegin(); it != items.constEnd(); ++it)
	  {
		  // Only want the top-level items (the workspace name), not the child info
		  if((*it)->childCount()>0) wsNames.push_back((*it)->text(0));
	  }
	  // Now loop over, calling delete for each workspace
	  QList<QString>::const_iterator it2;
	  for (it2 = wsNames.constBegin(); it2 != wsNames.constEnd(); ++it2)
	  {
		  if (!(*it2).isEmpty()) m_mantidUI->deleteWorkspace(*it2);
	  }
  }
*/
}
/*void MantidDockWidget::removeFromWSGroupNames(const QString& wsName)
{
	m_mantidUI->removeFromWSGroupNames(wsName);
	
}*/
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
	  Mantid::API::Workspace_sptr grpWSPstr;
	  bool bDisable=false;
	  try
	  {
		  grpWSPstr=boost::dynamic_pointer_cast< WorkspaceGroup>
			  (Mantid::API::AnalysisDataService::Instance().retrieve(selectedWsName.toStdString()));
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

	  action = new QAction("Plot first spectrum",this);
	  connect(action,SIGNAL(triggered()),m_mantidUI,SLOT(plotFirstSpectrum()));
	  menu->addAction(action);
	  if(grpWSPstr||bDisable)
	  {action->setEnabled(false);
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
	 if(grpWSPstr||bDisable)
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

//------------ MantidTreeWidget -----------------------//

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
    update();

}

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

void AlgorithmDockWidget::update()
{
    m_tree->clear();

    typedef std::vector<Algorithm_descriptor> AlgNamesType;
    AlgNamesType names = AlgorithmFactory::Instance().getDescriptors();

    /*Algorithm_descriptor desc = {"LoadLog","DataHandling\\Logs",3};
    Algorithm_descriptor desc1 = {"LoadLog","DataHandling\\Logs",7};
    names.push_back(desc);
    names.push_back(desc1);//*/

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

