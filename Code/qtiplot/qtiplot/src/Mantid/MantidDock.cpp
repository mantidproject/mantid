#include "MantidDock.h"
#include "MantidUI.h"
#include "../ApplicationWindow.h"
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

#include <map>
#include <iostream>
using namespace std;

using namespace Mantid::API;

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
    loadMenu->addAction(loadRawAction);
    loadMenu->addAction(loadDAEAction);
    m_loadButton->setMenu(loadMenu);

    connect(m_deleteButton,SIGNAL(clicked()),this,SLOT(deleteWorkspaces()));
    connect(m_tree,SIGNAL(itemClicked(QTreeWidgetItem*, int)),this,SLOT(clickedWorkspace(QTreeWidgetItem*, int)));

    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tree, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(popupMenu(const QPoint &)));
}

void MantidDockWidget::update()
{
    // Populate MantifTreeWidget with workspaces
    QStringList sl = m_mantidUI->getWorkspaceNames();
    m_tree->clear();
    for(int i=0;i<sl.size();i++)
    {
        QTreeWidgetItem *wsItem = new QTreeWidgetItem(QStringList(sl[i]));
        Mantid::API::Workspace_sptr ws0 = m_mantidUI->getWorkspace(sl[i]);
        if (boost::dynamic_pointer_cast<MatrixWorkspace>(ws0))
        {
            MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(ws0);
            wsItem->setIcon(0,QIcon(QPixmap(mantid_matrix_xpm)));
            wsItem->addChild(new QTreeWidgetItem(QStringList("Histograms: "+QString::number(ws->getNumberHistograms()))));
            wsItem->addChild(new QTreeWidgetItem(QStringList("Bins: "+QString::number(ws->blocksize()))));
            bool isHistogram = ws->blocksize() && ws->isHistogramData();
            wsItem->addChild(new QTreeWidgetItem(QStringList(    isHistogram?"Histogram":"Data points"   )));
            std::string s = "X axis: ";
            if (ws->axes() > 0 )
	    {
	      Mantid::API::Axis *ax = ws->getAxis(0);
	      if( ax && ax->unit().get() ) s += ax->unit()->caption() + " / " + ax->unit()->label();
	    }
            else
	    {
                s += "Unknown";
	    }
            wsItem->addChild(new QTreeWidgetItem(QStringList(QString::fromStdString(s)))); 
            s = "Y axis: " + ws->YUnit();
            wsItem->addChild(new QTreeWidgetItem(QStringList(QString::fromStdString(s))));
            QString WsType = QString::fromStdString(ws->id());
            wsItem->addChild(new QTreeWidgetItem(QStringList(WsType)));
            wsItem->addChild(new QTreeWidgetItem(QStringList("Memory used: "+QString::number(ws->getMemorySize())+" KB")));
        }
        m_tree->addTopLevelItem(wsItem);
    }
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
  QList<QTreeWidgetItem*>::const_iterator it;
  QList<QString> wsNames;
  // Need two loops because deleting a workspace moves QTreeWidgetItems
  // First loop over getting workspace names
  for (it = items.constBegin(); it != items.constEnd(); ++it)
  {
    wsNames.push_back((*it)->text(0));
  }
  // Now loop over, calling delete for each workspace
  QList<QString>::const_iterator it2;
  for (it2 = wsNames.constBegin(); it2 != wsNames.constEnd(); ++it2)
  {
    if (!(*it2).isEmpty()) m_mantidUI->deleteWorkspace(*it2);
  }
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
  }
  //else show instrument, sample logs and delete
  else
  {
    QAction *action = new QAction("Show data",this);
    connect(action,SIGNAL(triggered()),m_mantidUI,SLOT(importWorkspace()));
    menu->addAction(action);

    action = new QAction("Show instrument",this);
    connect(action,SIGNAL(triggered()),m_mantidUI,SLOT(showMantidInstrumentSelected()));
    menu->addAction(action);

    action = new QAction("Plot spectrum 0",this);
    connect(action,SIGNAL(triggered()),m_mantidUI,SLOT(plotFirstSpectrum()));
    menu->addAction(action);

    action = new QAction("Sample Logs...", this);
    connect(action,SIGNAL(triggered()),m_mantidUI,SLOT(showLogFileWindow()));
    menu->addAction(action);
  
    //separate delete
    menu->addSeparator();

    action = new QAction("Delete workspace",this);
    connect(action,SIGNAL(triggered()),this,SLOT(deleteWorkspaces()));
    menu->addAction(action);
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
    QString wsName = m_mantidUI->getSelectedWorkspaceName();
    if ( ! wsName.isEmpty() )
    {
        m_mantidUI->importWorkspace(wsName,false);
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

