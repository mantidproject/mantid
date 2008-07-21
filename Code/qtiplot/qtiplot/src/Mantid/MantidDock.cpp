#include "MantidDock.h"
#include "MantidUI.h"
#include "../ApplicationWindow.h"
#include <MantidAPI/AlgorithmFactory.h>

#include <QMessageBox>
#include <QTextEdit>
#include <QListWidget>
#include <QApplication>
#include <QMap>

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

    m_tree = new MantidTreeWidget(f);
    m_tree->setHeaderLabel("Workspaces");

    QHBoxLayout * buttonLayout = new QHBoxLayout();
    m_loadButton = new QPushButton("Load");
    m_deleteButton = new QPushButton("Delete");
    QPushButton *tstButton = new QPushButton("Import");
    buttonLayout->addWidget(m_loadButton);
    buttonLayout->addWidget(m_deleteButton);
    buttonLayout->addWidget(tstButton);
    buttonLayout->addStretch();
    //
    QVBoxLayout * layout = new QVBoxLayout();
    f->setLayout(layout);
    layout->addLayout(buttonLayout);
    layout->addWidget(m_tree);
    //
    connect(m_loadButton,SIGNAL(clicked()),m_mantidUI,SLOT(loadWorkspace()));
    connect(m_deleteButton,SIGNAL(clicked()),m_mantidUI,SLOT(deleteWorkspace()));
    connect(tstButton,SIGNAL(clicked()),m_mantidUI,SLOT(importWorkspace()));
    connect(m_tree,SIGNAL(itemClicked(QTreeWidgetItem*, int)),this,SLOT(clickedWorkspace(QTreeWidgetItem*, int)));
}

void MantidDockWidget::update()
{
    // Populate MantifTreeWidget with workspaces
    QStringList sl = m_mantidUI->getWorkspaceNames();
    m_tree->clear();
    for(int i=0;i<sl.size();i++)
    {
        QTreeWidgetItem *wsItem = new QTreeWidgetItem(QStringList(sl[i]));
        Mantid::API::Workspace_sptr ws = m_mantidUI->getWorkspace(sl[i]);
        wsItem->setIcon(0,QIcon(QPixmap(mantid_matrix_xpm)));
        wsItem->addChild(new QTreeWidgetItem(QStringList("Histograms: "+QString::number(ws->getNumberHistograms()))));
        //wsItem->addChild(new QTreeWidgetItem(QStringList("Bins: "+QString::number(m_mantidUI->getBinNumber(sl[i])))));
        wsItem->addChild(new QTreeWidgetItem(QStringList("Bins: "+QString::number(ws->blocksize()))));
        Mantid::API::Axis* ax;
        ax = ws->getAxis(0);
        std::string s = "X axis: ";
        if (ax->unit().get()) s += ax->unit()->caption() + " / " + ax->unit()->label();
        else
            s += "Unknown";
        wsItem->addChild(new QTreeWidgetItem(QStringList(QString::fromStdString(s))));
        m_tree->addTopLevelItem(wsItem);
    }
}

void MantidDockWidget::clickedWorkspace(QTreeWidgetItem*, int)
{
}

//------------ MantidTreeWidget -----------------------//

void MantidTreeWidget::mousePressEvent (QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
         m_dragStartPosition = e->pos();

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

//-------------------- AlgorithmDockWidget ----------------------//

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

    QHBoxLayout * buttonLayout = new QHBoxLayout();
    QPushButton *execButton = new QPushButton("Execute");
    QPushButton *refreshButton = new QPushButton("Refresh");

    buttonLayout->addWidget(execButton);
    buttonLayout->addWidget(refreshButton);
    buttonLayout->addStretch();
    //
    QVBoxLayout * layout = new QVBoxLayout();
    f->setLayout(layout);
    layout->addLayout(buttonLayout);
    layout->addWidget(m_tree);
    //
    connect(execButton,SIGNAL(clicked()),m_mantidUI,SLOT(executeAlgorithm()));
    connect(refreshButton,SIGNAL(clicked()),this,SLOT(update()));

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

void AlgorithmDockWidget::update()
{
    m_tree->clear();

    typedef std::vector<Algorithm_descriptor> AlgNamesType;
    AlgNamesType names = AlgorithmFactory::Instance().getDescriptors();

    /*Algorithm_descriptor desc = {"Divide","General",3};
    Algorithm_descriptor desc1 = {"Divide","General",7};
    names.push_back(desc);
    names.push_back(desc1);*/

    sort(names.begin(),names.end(),Algorithm_descriptor_less);

    QMap<QString,QTreeWidgetItem*> categories;
    QMap<QString,QTreeWidgetItem*> algorithms;

    for(AlgNamesType::const_iterator i=names.begin();i!=names.end();i++)
    {
        QString algName = QString::fromStdString(i->name);
        QString catName = QString::fromStdString(i->category);
        if (!categories.contains(catName))
        {
            QTreeWidgetItem *catItem = new QTreeWidgetItem(QStringList(catName));
            categories.insert(catName,catItem);
            m_tree->addTopLevelItem(catItem);
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

//-------------------- AlgorithmTreeWidget ----------------------//

void AlgorithmTreeWidget::mousePressEvent (QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
         m_dragStartPosition = e->pos();

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
    }

    return QTreeWidget::mouseDoubleClickEvent(e);
}

