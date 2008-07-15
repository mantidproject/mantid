#include "MantidUI.h"
#include "MantidLog.h"
#include "MantidMatrix.h"
#include "MantidDock.h"
#include "WorkspaceMatrix.h"
#include "LoadRawDlg.h"
#include "ImportWorkspaceDlg.h"
#include "ExecuteAlgorithm.h"
#include "../Spectrogram.h"
#include "../pixmaps.h"

#include <QMessageBox>
#include <QTextEdit>
#include <QListWidget>
#include <QMdiArea>
#include <QMenuBar>
#include <QApplication>
#include <QToolBar>
#include <QMenu>

#include <iostream>
using namespace std;

using namespace Mantid::API;

void ApplicationWindow::initMantid()
{

    mantidUI = new MantidUI(this);
    mantidUI->d_workspace = d_workspace;
    mantidUI->aw_menuBar = menuBar();
    mantidUI->aw_plot2DMenu = plot2DMenu;
    mantidUI->aw_plot3DMenu = plot3DMenu;
    mantidUI->aw_plotMatrixBar = plotMatrixBar;
    mantidUI->aw_scriptEnv = scriptEnv;
    mantidUI->aw_view = view;
    mantidUI->aw_actionShowUndoStack = actionShowUndoStack;

    mantidUI->init();
}


MantidUI::MantidUI(ApplicationWindow *aw):m_appWindow(aw)
{
    m_exploreMantid = new MantidDockWidget(this,aw);
    m_exploreAlgorithms = new AlgorithmDockWidget(this,aw);

  	actionCopyRowToTable = new QAction(tr("Copy to Table"), this);
	actionCopyRowToTable->setIcon(QIcon(QPixmap(table_xpm)));
	connect(actionCopyRowToTable, SIGNAL(activated()), this, SLOT(copyRowToTable()));

  	actionCopyRowToGraph = new QAction(tr("Plot spectra (values only)"), this);
	actionCopyRowToGraph->setIcon(QIcon(QPixmap(graph_xpm)));
	connect(actionCopyRowToGraph, SIGNAL(activated()), this, SLOT(copyRowToGraph()));

  	actionCopyRowToGraphErr = new QAction(tr("Plot spectra (values + errors)"), this);
	actionCopyRowToGraphErr->setIcon(QIcon(QPixmap(graph_xpm)));
	connect(actionCopyRowToGraphErr, SIGNAL(activated()), this, SLOT(copyRowToGraphErr()));
}

void MantidUI::init()
{
    FrameworkManager::Instance();
    MantidLog::connect(appWindow());

	actionToggleMantid = m_exploreMantid->toggleViewAction();
	actionToggleMantid->setIcon(QPixmap(mantid_matrix_xpm));
	actionToggleMantid->setShortcut( tr("Ctrl+Shift+M") );
    aw_view->addAction(actionToggleMantid);

	actionToggleAlgorithms = m_exploreAlgorithms->toggleViewAction();
	//actionToggleAlgorithms->setIcon(QPixmap(mantid_matrix_xpm));
	actionToggleAlgorithms->setShortcut( tr("Ctrl+Shift+A") );
    aw_view->addAction(actionToggleAlgorithms);

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
    m_exploreAlgorithms->update();
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

Mantid::API::Workspace_sptr MantidUI::getWorkspace(const QString& workspaceName)
{
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
	return output->blocksize();
}

bool MantidUI::menuAboutToShow(QMdiSubWindow *w)
{
    if (w->isA("MantidMatrix"))
    {
        aw_menuBar->insertItem(tr("3D &Plot"), aw_plot3DMenu);

        aw_plotMatrixBar->setEnabled (true);
    }

    return false;
}

Graph3D *MantidUI::plot3DMatrix(int style)
{
    QMdiSubWindow *w = appWindow()->activeWindow();
    if (w->isA("MantidMatrix"))
    {
        return static_cast<MantidMatrix*>(w)->plotGraph3D(style);
    }

    return 0;
}

MultiLayer* MantidUI::plotSpectrogram(Graph::CurveType type)
{
    MantidMatrix *m = dynamic_cast<MantidMatrix*>(appWindow()->activeWindow());
    if (m) return m->plotGraph2D(type);
    return 0;

}

void MantidUI::tst()
{
    cerr<<"\n\n\ntst\n\n\n";
}

void MantidUI::importWorkspace()
{
    Workspace_sptr ws = getSelectedWorkspace();
    if (!ws.get()) return;

	ImportWorkspaceDlg* dlg = new ImportWorkspaceDlg(appWindow(), ws->getNumberHistograms());
	dlg->setModal(true);	
	if (dlg->exec() == QDialog::Accepted)
	{
		int start = dlg->getLowerLimit();
		int end = dlg->getUpperLimit();
		
	    MantidMatrix* w = new MantidMatrix(ws, appWindow(), "Mantid",getSelectedWorkspaceName(), start, end,dlg->isFiltered(),dlg->getMaxValue() );
   	    connect(w, SIGNAL(closedWindow(MdiSubWindow*)), appWindow(), SLOT(closeWindow(MdiSubWindow*)));
	    connect(w,SIGNAL(hiddenWindow(MdiSubWindow*)),appWindow(), SLOT(hideWindow(MdiSubWindow*)));
	    connect (w,SIGNAL(showContextMenu()),appWindow(),SLOT(showWindowContextMenu()));

        d_workspace->addSubWindow(w);
	    w->showNormal(); 
	}

}

void MantidUI::removeWindowFromLists(MdiSubWindow* m)
{
	if (!m)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    if (m->isA("MantidMatrix")) static_cast<MantidMatrix*>(m)->removeWindow();

	QApplication::restoreOverrideCursor();
}

void MantidUI::showContextMenu(QMenu& cm, MdiSubWindow* w)
{
    if (w->isA("MantidMatrix")) 
    {
        //cm.insertItem(QPixmap(copy_xpm),tr("&Copy"), t, SLOT(copySelection()));
        cm.addAction(actionCopyRowToTable);
        cm.addAction(actionCopyRowToGraph);
        cm.addAction(actionCopyRowToGraphErr);
    }
}

void MantidUI::copyRowToTable()
{
    MantidMatrix* m = (MantidMatrix*)appWindow()->activeWindow();
    if (!m || !m->isA("MantidMatrix")) return;
    createTableFromSelectedRows(m,true,true);
}

void MantidUI::copyRowToGraph()
{
    MantidMatrix* m = (MantidMatrix*)appWindow()->activeWindow();
    if (!m || !m->isA("MantidMatrix")) return;
    createGraphFromSelectedRows(m,false,false);
  
}

void MantidUI::copyRowToGraphErr()
{
    MantidMatrix* m = (MantidMatrix*)appWindow()->activeWindow();
    if (!m || !m->isA("MantidMatrix")) return;
    createGraphFromSelectedRows(m,false);
  
}

Table* MantidUI::createTableFromSelectedRows(MantidMatrix *m, bool visible, bool errs)
{
     int i0,i1; 
     m->getSelectedRows(i0,i1);
     if (i0 < 0 || i1 < 0) return 0;

     int c = errs?2:1;

	 Table* t = new Table(aw_scriptEnv, m->numCols(), c*(i1 - i0 + 1) + 1, "", appWindow(), 0);
	 appWindow()->initTable(t, appWindow()->generateUniqueName(m->name()+"-"));
     if (visible) t->showNormal();
    
     int kY,kErr;
     for(int i=i0;i<=i1;i++)
     {
         kY = c*(i-i0)+1;
         t->setColName(kY,"Y"+QString::number(i));
         if (errs)
         {
             kErr = 2*(i - i0) + 2;
             t->setColPlotDesignation(kErr,Table::yErr);
             t->setColName(kErr,"Err"+QString::number(i));
         }
         for(int j=0;j<m->numCols();j++)
         {
             if (i == i0) t->setCell(j,0,m->dataX(i,j));
             t->setCell(j,kY,m->cell(i,j)); 
             if (errs) t->setCell(j,kErr,m->dataE(i,j));
         }
     }
     return t;
 }

void MantidUI::createGraphFromSelectedRows(MantidMatrix *m, bool visible, bool errs)
{
    Table *t = createTableFromSelectedRows(m,visible,errs);
    if (!t) return;

    QStringList cn;
    cn<<t->colName(1);
    if (errs) cn<<t->colName(2);
    Graph *g = appWindow()->multilayerPlot(t,t->colNames(),Graph::Line)->activeGraph();
    appWindow()->polishGraph(g,Graph::Line);
    m->setGraph1D(g);
}

bool MantidUI::drop(QDropEvent* e)
{
    if (e->source() == m_exploreMantid->m_tree) 
    {
        importWorkspace();
        return true;
    }

    return false;
}

void MantidUI::getSelectedAlgorithm(QString& algName, int& version)
{
    QList<QTreeWidgetItem*> items = m_exploreAlgorithms->m_tree->selectedItems();
    if (items.size() == 0 || items[0]->parent() == 0) 
    {
        algName = "";
        version = 0;
    }
    else
    {
        QString str = items[0]->text(0);
        QStringList lst = str.split(" v.");
        algName = lst[0];
        version = lst[1].toInt();
    }
}

void MantidUI::executeAlgorithm()
{
    QString algName;
    int version;
    getSelectedAlgorithm(algName,version);
    if (algName.isEmpty())
    {
        QMessageBox::warning(appWindow(),"Mantid","Please select an algorithm");
        return;
    }
    Mantid::API::Algorithm* alg = dynamic_cast<Mantid::API::Algorithm*>
          (Mantid::API::FrameworkManager::Instance().createAlgorithm(algName.toStdString(),version));

	std::vector<Mantid::Kernel::Property*> propList = alg->getProperties();

	if (propList.size() > 0)
	{
      	QStringList wkspaces = getWorkspaceNames();

		ExecuteAlgorithm* dlg = new ExecuteAlgorithm(appWindow());
		dlg->CreateLayout(wkspaces, propList);
		dlg->setModal(true);
	
		if (dlg->exec()== QDialog::Accepted)
		{			
			std::map<std::string, std::string>::iterator resItr = dlg->results.begin();
			
			for (; resItr != dlg->results.end(); ++resItr)
			{				
				try
				{
					alg->setPropertyValue(resItr->first, resItr->second);
				}
				catch (std::invalid_argument err)
				{
					int ret = QMessageBox::warning(appWindow(), tr("Mantid Algorithm"),
					tr(QString::fromStdString(resItr->first) + " was invalid."),
					QMessageBox::Ok);
				
					return;
				}
			}
			
			//Check properties valid
			if (!alg->validateProperties())
			{
				//Properties not valid
				int ret = QMessageBox::warning(appWindow(), tr("Mantid Algorithm"),
					tr("One or more of the property values entered was invalid. "
					"Please see the Mantid log for details."),
					QMessageBox::Ok);
				
				return;
			}
			
			if (!alg->execute() == true)
			{
				//Algorithm did not execute properly
				int ret = QMessageBox::warning(appWindow(), tr("Mantid Algorithm"),
					tr("The algorithm failed to execute correctly. "
					"Please see the Mantid log for details."),
					QMessageBox::Ok);
			}
			
			update();
		}
	}
}
