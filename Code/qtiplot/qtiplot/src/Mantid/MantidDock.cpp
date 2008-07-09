#include "MantidDock.h"
#include "MantidUI.h"
#include "../ApplicationWindow.h"

#include <QMessageBox>
#include <QTextEdit>
#include <QListWidget>
#include <QApplication>

#include <iostream>
using namespace std;

using namespace Mantid::API;

MantidDockWidget::MantidDockWidget(MantidUI *mui, ApplicationWindow *w):
QDockWidget(w)
{
    m_mantidUI = mui;
    setWindowTitle(tr("Mantid Explorer"));
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
