#include "MantidUI.h"
#include "MantidMatrix.h"
#include "LoadRawDlg.h"

#include <QMessageBox>
#include <QTextEdit>
#include <QListWidget>
#include <QMdiArea>

using namespace Mantid::API;

void ApplicationWindow::initMantid()
{

    mantidUI = new MantidUI(this);
    mantidUI->init();
    mantidUI->d_workspace = d_workspace;

}


MantidUI::MantidUI(ApplicationWindow *aw):m_appWindow(aw)
{
    m_exploreMantid = new MantidDockWidget(this,aw);
}

void MantidUI::init()
{

    //LoadIsisRawFile("C:/Mantid/Test/Data/MAR11060.RAW","MAR11060");
    update();
}

QStringList MantidUI::getWorkspaceNames()
{
    QStringList sl;
    std::vector<std::string> sv;
    sv = Mantid::API::AnalysisDataService::Instance().getObjectNames();
    for(size_t i=0;i<sv.size();i++)
        sl<<QString::fromStdString(sv[i]);
    return sl;
}

QStringList MantidUI::getAlgorithmNames()
{
    QStringList sl;
    std::vector<std::string> sv;
    sv = Mantid::API::AlgorithmFactory::Instance().getKeys();
    for(size_t i=0;i<sv.size();i++)
        sl<<QString::fromStdString(sv[i]);
    return sl;
}



IAlgorithm* MantidUI::CreateAlgorithm(const QString& algName)
{
	IAlgorithm* alg = FrameworkManager::Instance().createAlgorithm(algName.toStdString());

	return alg;
}

Workspace_sptr MantidUI::LoadIsisRawFile(const QString& fileName,const QString& workspaceName)
{
	//Check workspace does not exist
	if (!AnalysisDataService::Instance().doesExist(workspaceName.toStdString()))
	{
		IAlgorithm* alg = CreateAlgorithm("LoadRaw");
		alg->setPropertyValue("Filename", fileName.toStdString());
		alg->setPropertyValue("OutputWorkspace", workspaceName.toStdString());

		alg->execute();

		Workspace_sptr output = AnalysisDataService::Instance().retrieve(workspaceName.toStdString());

		return output;
	}
	
	Workspace_sptr empty;
	
	return empty;
}

void MantidUI::loadWorkspace()
{
    
	loadRawDlg* dlg = new loadRawDlg(m_appWindow);
	dlg->setModal(true);	
	dlg->exec();
	
	if (!dlg->getFilename().isEmpty())
	{	
		
		Mantid::API::Workspace_sptr ws = LoadIsisRawFile(dlg->getFilename(), dlg->getWorkspaceName());
		if (ws.use_count() == 0)
		{
			QMessageBox::warning(m_appWindow, tr("Mantid"),
                   		tr("A workspace with this name already exists.\n")
                    		, QMessageBox::Ok, QMessageBox::Ok);
			return;
		}
		
		update();

	}
}

bool MantidUI::deleteWorkspace(const QString& workspaceName)
{
    bool ret = FrameworkManager::Instance().deleteWorkspace(workspaceName.toStdString());
    if (ret) update();
    return ret;
}

void MantidUI::deleteWorkspace()
{
    QString wsName = getSelectedWorkspaceName();
    if (!wsName.size()) return;
	deleteWorkspace(wsName);
}

void MantidUI::update()
{
    m_exploreMantid->update();
}

QString MantidUI::getSelectedWorkspaceName()
{
    QList<QTreeWidgetItem*> items = m_exploreMantid->m_tree->selectedItems();
    QString str;
    if (!items.size()) str = "";
    else
    {
        QTreeWidgetItem *item = items[0]->parent()?items[0]->parent():items[0];
        str = item->text(0);
    }
    return str;
}

Mantid::API::Workspace_sptr MantidUI::getSelectedWorkspace()
{
    QString workspaceName = getSelectedWorkspaceName();
	if (AnalysisDataService::Instance().doesExist(workspaceName.toStdString()))
	{
		return AnalysisDataService::Instance().retrieve(workspaceName.toStdString());
	}
	
	Workspace_sptr empty;
	
	return empty;//??
}

int MantidUI::getHistogramNumber(const QString& workspaceName)
{
	Workspace_sptr output = AnalysisDataService::Instance().retrieve(workspaceName.toStdString());
	return output->getNumberHistograms();
}

int MantidUI::getBinNumber(const QString& workspaceName)
{
	Workspace_sptr output = AnalysisDataService::Instance().retrieve(workspaceName.toStdString());
	return output->dataX(0).size();
}

void MantidUI::tst()
{
    Workspace_sptr ws = getSelectedWorkspace();
    if (!ws.get()) return;
	MantidMatrix* w = new MantidMatrix(ws, appWindow(), "Mantid",getSelectedWorkspaceName() );
	/*initMatrix(w, caption);
	if (w->objectName() != caption)//the matrix was renamed
		renamedTables << caption << w->objectName();*/

    d_workspace->addSubWindow(w);
	w->showNormal(); 
    QString str = QString::number(w->numRows(),'g',6);
    w->goTo(100,20);
    //QMessageBox::information(appWindow(),"MantidUI",str);
}

//------------------------------------------------------------------------------------------
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

    m_tree = new QTreeWidget();
    m_tree->setHeaderLabel("Workspaces");

    QHBoxLayout * buttonLayout = new QHBoxLayout();
    m_loadButton = new QPushButton("Load");
    m_deleteButton = new QPushButton("Delete");
    QPushButton *tstButton = new QPushButton("Test");
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
    connect(tstButton,SIGNAL(clicked()),m_mantidUI,SLOT(tst()));
}

void MantidDockWidget::update()
{
    QStringList sl = m_mantidUI->getWorkspaceNames();
    m_tree->clear();
    for(int i=0;i<sl.size();i++)
    {
        QTreeWidgetItem *wsItem = new QTreeWidgetItem(QStringList(sl[i]));
        wsItem->setIcon(0,QIcon(QPixmap(mantid_matrix_xpm)));
        wsItem->addChild(new QTreeWidgetItem(QStringList("Histograms: "+QString::number(m_mantidUI->getHistogramNumber(sl[i])))));
        wsItem->addChild(new QTreeWidgetItem(QStringList("Bins: "+QString::number(m_mantidUI->getBinNumber(sl[i])))));
        m_tree->addTopLevelItem(wsItem);
    }
}

