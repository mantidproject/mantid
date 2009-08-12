#include "MantidUI.h"
#include "MantidMatrix.h"
#include "MantidDock.h"
#include <algorithm>
#include "ImportWorkspaceDlg.h"

#include "LoadDAEDlg.h"

#include "AlgMonitor.h"
#include "../Spectrogram.h"
#include "../pixmaps.h"
#include "MantidSampleLogDialog.h"
#include "../ScriptWindow.h"

#include "MantidKernel/Property.h"
#include "MantidKernel/LogFilter.h"
#include "MantidPlotReleaseDate.h"
#include "InstrumentWidget/InstrumentWindow.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include "MantidQtAPI/InterfaceManager.h"
#include "MantidQtAPI/AlgorithmDialog.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidKernel/EnvironmentHistory.h"
#include "AlgorithmHistoryWindow.h"

//#include "MemoryImage.h"

#include <QMessageBox>
#include <QTextEdit>
#include <QListWidget>
#include <QMdiArea>
#include <QMenuBar>
#include <QApplication>
#include <QToolBar>
#include <QMenu>
#include <QInputDialog>

#include <qwt_plot_curve.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include <fstream>
#include <iostream>
#include <sstream>
using namespace std;

using namespace Mantid::API;

Mantid::Kernel::Logger& MantidUI::logObject=Mantid::Kernel::Logger::get("MantidUI");

MantidUI::MantidUI(ApplicationWindow *aw):m_appWindow(aw),
m_finishedLoadDAEObserver(*this, &MantidUI::handleLoadDAEFinishedNotification),
m_addObserver(*this,&MantidUI::handleAddWorkspace),
m_replaceObserver(*this,&MantidUI::handleReplaceWorkspace),
m_deleteObserver(*this,&MantidUI::handleDeleteWorkspace),
m_progressDialog(0)
{
    m_exploreMantid = new MantidDockWidget(this,aw);
    m_exploreAlgorithms = new AlgorithmDockWidget(this,aw);

  	actionCopyRowToTable = new QAction(tr("Copy spectra to table"), this);
	actionCopyRowToTable->setIcon(QIcon(QPixmap(table_xpm)));
	connect(actionCopyRowToTable, SIGNAL(activated()), this, SLOT(copyRowToTable()));

  	actionCopyRowToGraph = new QAction(tr("Plot spectra (values only)"), this);
	actionCopyRowToGraph->setIcon(QIcon(QPixmap(graph_xpm)));
	connect(actionCopyRowToGraph, SIGNAL(activated()), this, SLOT(copyRowToGraph()));

  	actionCopyRowToGraphErr = new QAction(tr("Plot spectra (values + errors)"), this);
	actionCopyRowToGraphErr->setIcon(QIcon(QPixmap(graph_xpm)));
	connect(actionCopyRowToGraphErr, SIGNAL(activated()), this, SLOT(copyRowToGraphErr()));

    actionCopyDetectorsToTable = new QAction(tr("Copy detectors to table"), this);
	actionCopyDetectorsToTable->setIcon(QIcon(QPixmap(table_xpm)));
	connect(actionCopyDetectorsToTable, SIGNAL(activated()), this, SLOT(copyDetectorsToTable()));

  	actionCopyValues = new QAction(tr("Copy"), this);
	actionCopyValues->setIcon(QIcon(QPixmap(copy_xpm)));
	connect(actionCopyValues, SIGNAL(activated()), this, SLOT(copyValues()));

  	actionCopyColumnToTable = new QAction(tr("Copy bin to table"), this);
	actionCopyColumnToTable->setIcon(QIcon(QPixmap(table_xpm)));
	connect(actionCopyColumnToTable, SIGNAL(activated()), this, SLOT(copyColumnToTable()));

  	actionCopyColumnToGraph = new QAction(tr("Plot bin (values only)"), this);
	actionCopyColumnToGraph->setIcon(QIcon(QPixmap(graph_xpm)));
	connect(actionCopyColumnToGraph, SIGNAL(activated()), this, SLOT(copyColumnToGraph()));

  	actionCopyColumnToGraphErr = new QAction(tr("Plot bin (values + errors)"), this);
	actionCopyColumnToGraphErr->setIcon(QIcon(QPixmap(graph_xpm)));
	connect(actionCopyColumnToGraphErr, SIGNAL(activated()), this, SLOT(copyColumnToGraphErr()));

    connect(this,SIGNAL(needToCreateLoadDAEMantidMatrix(const Mantid::API::IAlgorithm*)),this,SLOT(createLoadDAEMantidMatrix(const Mantid::API::IAlgorithm*)));
    connect(this,SIGNAL(needToShowCritical(const QString&)),this,SLOT(showCritical(const QString&)));

    m_algMonitor = new AlgorithmMonitor(this);
    connect(m_algMonitor,SIGNAL(countChanged(int)),m_exploreAlgorithms,SLOT(countChanged(int)), Qt::QueuedConnection);
    m_algMonitor->start();

    Mantid::API::AnalysisDataService::Instance().notificationCenter.addObserver(m_addObserver);
    Mantid::API::AnalysisDataService::Instance().notificationCenter.addObserver(m_replaceObserver);
    Mantid::API::AnalysisDataService::Instance().notificationCenter.addObserver(m_deleteObserver);

    mantidMenu = new QMenu(m_appWindow);
    mantidMenu->setObjectName("mantidMenu");
	// for activating the keyboard shortcut for Clear All Memory even if no clciking on Mantid Menu
	// Ticket #672
    //connect(mantidMenu, SIGNAL(aboutToShow()), this, SLOT(mantidMenuAboutToShow()));
	mantidMenuAboutToShow();

    menuMantidMatrix = new QMenu(m_appWindow);
	connect(menuMantidMatrix, SIGNAL(aboutToShow()), this, SLOT(menuMantidMatrixAboutToShow()));

    // To be able to use them in signals they need to be registered
    static bool Workspace_sptr_qRegistered = false;
    if (!Workspace_sptr_qRegistered)
    {
        Workspace_sptr_qRegistered = true;
        qRegisterMetaType<Mantid::API::Workspace_sptr>();
        qRegisterMetaType<Mantid::API::MatrixWorkspace_sptr>();
    }

   /* QAction* tstAction = new QAction("Test",this);
    tstAction->setShortcut(QKeySequence::fromString("Ctrl+A"));
    connect(tstAction,SIGNAL(triggered()), this, SLOT(test()));
    mantidMenu->addAction(tstAction);*/

}

// Should it be moved to the constructor?
void MantidUI::init()
{
    FrameworkManager::Instance();
    MantidLog::connect(this);

    actionToggleMantid = m_exploreMantid->toggleViewAction();
    actionToggleMantid->setIcon(QPixmap(mantid_matrix_xpm));
    actionToggleMantid->setShortcut( tr("Ctrl+Shift+M") );
    appWindow()->view->addAction(actionToggleMantid);

    actionToggleAlgorithms = m_exploreAlgorithms->toggleViewAction();
    actionToggleAlgorithms->setShortcut( tr("Ctrl+Shift+A") );
    appWindow()->view->addAction(actionToggleAlgorithms);

    // Now that the framework is initialized we need to populate the algorithm tree
    m_exploreAlgorithms->update();

}

MantidUI::~MantidUI()
{
  if( m_algMonitor ) delete m_algMonitor;
}

void MantidUI::saveSettings() const
{
  // Save algorithm dialog input
  MantidQt::API::AlgorithmInputHistory::Instance().save();
}

QString MantidUI::releaseDate()
{
    return MANTIDPLOT_RELEASE_DATE;
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

/**
    CreateAlgorithm

    @param algName Algorithm's name
    @return Pointer to the created algorithm
*/
IAlgorithm_sptr MantidUI::CreateAlgorithm(const QString& algName)
{
	IAlgorithm_sptr alg = AlgorithmManager::Instance().create(algName.toStdString());

	return alg;
}
/**
     loadWorkspace.

     Loads a workspace from a raw file. The LoadRaw's properties are set in loadRawDlg dialog.
*/
void MantidUI::loadWorkspace()
{
  //Just use the generic executeAlgorithm method which now uses specialised dialogs if they are
  //available
  executeAlgorithm("LoadRaw", -1);

 }
/**
    Ticket #678
*/
void MantidUI::loadNexusWorkspace()
{
	executeAlgorithm("LoadNexus", -1);

}
/**

    Ticket #678
*/
void MantidUI::saveNexusWorkspace()
{
	executeSaveNexus("SaveNexus",-1);
}

/**
    loadDAEWorkspace

    Loads a workspace from DAE by executing LoadDAE algorithm asynchronously.
    Algorithm's proprties are set in loadDAEDlg dialog.
*/
void MantidUI::loadDAEWorkspace()
{

	loadDAEDlg* dlg = new loadDAEDlg(m_appWindow);
	dlg->setModal(true);
	dlg->exec();
	if (!dlg->getHostName().isEmpty())
	{
		//Check workspace does not exist
		QString workspaceName = dlg->getWorkspaceName();
		if (AnalysisDataService::Instance().doesExist(workspaceName.toStdString()))
		{
			if ( QMessageBox::question(appWindow(),"MantidPlot - Confirm","Workspace "+workspaceName+" already exists. Do you want to replace it?",
				QMessageBox::Yes, QMessageBox::No) != QMessageBox::Yes) return;
		}

		Mantid::API::IAlgorithm_sptr alg =CreateAlgorithm("LoadDAE");

		alg->setPropertyValue("DAEname", dlg->getHostName().toStdString());
		alg->setPropertyValue("OutputWorkspace", dlg->getWorkspaceName().toStdString());
		if ( !dlg->getSpectrumMin().isEmpty() && !dlg->getSpectrumMax().isEmpty() )
		{
			alg->setPropertyValue("SpectrumMin", dlg->getSpectrumMin().toStdString());
			alg->setPropertyValue("SpectrumMax", dlg->getSpectrumMax().toStdString());
		}
		if ( !dlg->getSpectrumList().isEmpty() )
		{
			alg->setPropertyValue("SpectrumList", dlg->getSpectrumList().toStdString());
		}

		m_DAE_map[dlg->getWorkspaceName().toStdString()] = dlg->updateInterval();

		executeAlgorithmAsync(alg);
		//fix for  Ticket #699 moved this line down
		alg->addObserver(m_finishedLoadDAEObserver);
	}
}

/**
     deleteWorkspace

     @param workspaceName Name of the workspace to delete
*/
bool MantidUI::deleteWorkspace(const QString& workspaceName)
{
  return FrameworkManager::Instance().deleteWorkspace(workspaceName.toStdString());
}

/**
       getSelectedWorkspaceName


*/
QString MantidUI::getSelectedWorkspaceName()
{
    QList<QTreeWidgetItem*> items = m_exploreMantid->m_tree->selectedItems();
    QString str("");
    if( !items.empty() )
    {
      QTreeWidgetItem *item = items[0];
	  //commented below if loop as it was pointing to the parent item in the tree
	  //this will fail incase of  workspace groups as right click on child item always points to parent item
     /* if( item->parent() )
      {
		item = item->parent();
      }*/
	  if(item) str = item->text(0);
    }
    if( !str.isEmpty() ) return str;

    //Check if a mantid matrix is selected
    MantidMatrix *m = qobject_cast<MantidMatrix*>(appWindow()->activeWindow());
    if( !m ) return "";

    return m->workspaceName();
}
/*void MantidUI::removeFromWSGroupNames(const QString& wsName)
{
	//std::vector<std::string> wsGrpNames=getWorkspaceGroupNames();
	std::vector<std::string>::iterator it;
	if(!m_wsGroupNames.empty())
	{
		for (it=m_wsGroupNames.begin();it!=m_wsGroupNames.end();it++)
		{
			//logObject.error()<<"comparison string "<<pchild->text(0).toStdString()<<endl;;
			if((*it)==(wsName.toStdString()))
			{
				//logObject.error()<<"name erased from vector=  "<<(*it)<<endl;
				m_wsGroupNames.erase(it);
				return;
			}
		}
	}

}*/

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

/**   Extension to ApplicationWindow::menuAboutToShow() to deal with Mantid.
  */
bool MantidUI::menuAboutToShow(QMdiSubWindow *w)
{

    if (w && w->isA("MantidMatrix"))
    {
        appWindow()->menuBar()->insertItem(tr("3D &Plot"), appWindow()->plot3DMenu);
        appWindow()->actionCopySelection->setEnabled(true);
		appWindow()->actionPasteSelection->setEnabled(false);
		appWindow()->actionClearSelection->setEnabled(false);
        appWindow()->plotMatrixBar->setEnabled (true);

        appWindow()->menuBar()->insertItem(tr("Mantid &Matrix"),menuMantidMatrix);
        return true;
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

/**  Import a MatrixWorkspace into a MantidMatrix.
     @param wsName Workspace name
     @param lower An optional lower boundary
     @param upper An optional upper boundary
     @param showDlg If true show a dialog box to set some import parameters
     @param makeVisible If true show the created MantidMatrix, hide otherwise.
     @return A pointer to the new MantidMatrix.
 */
MantidMatrix* MantidUI::importMatrixWorkspace(const QString& wsName, int lower, int upper, bool showDlg, bool makeVisible)
{
    MatrixWorkspace_sptr ws;
  	if (AnalysisDataService::Instance().doesExist(wsName.toStdString()))
	{
		ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName.toStdString()));
	}

    if (!ws.get()) return 0;

    MantidMatrix* w = 0;
    if (showDlg)
    {
	    ImportWorkspaceDlg* dlg = new ImportWorkspaceDlg(appWindow(), ws->getNumberHistograms());
	    dlg->setModal(true);
	    if (dlg->exec() == QDialog::Accepted)
	    {
		    int start = dlg->getLowerLimit();
		    int end = dlg->getUpperLimit();

	        w = new MantidMatrix(ws, appWindow(), "Mantid",wsName, start, end );
            if (dlg->isFiltered())
                w->setRange(0,dlg->getMaxValue());
	    }
    }
    else
    {
        w = new MantidMatrix(ws, appWindow(), "Mantid",wsName, lower, upper);
    }
    if ( !w ) return 0;

    connect(w, SIGNAL(closedWindow(MdiSubWindow*)), appWindow(), SLOT(closeWindow(MdiSubWindow*)));
    connect(w,SIGNAL(hiddenWindow(MdiSubWindow*)),appWindow(), SLOT(hideWindow(MdiSubWindow*)));
    connect (w,SIGNAL(showContextMenu()),appWindow(),SLOT(showWindowContextMenu()));

    appWindow()->d_workspace->addSubWindow(w);
    if( makeVisible ) w->showNormal();
    else w->showMinimized();
    return w;
}

/**  Import a Workspace into MantidPlot.
     @param wsName Workspace name
     @param showDlg If true show a dialog box to set some import parameters
     @param makeVisible If true show the created widget, hide otherwise.
 */
void MantidUI::importWorkspace(const QString& wsName, bool showDlg, bool makeVisible)
{
  MantidMatrix* mm = importMatrixWorkspace(wsName,-1, -1, showDlg,makeVisible);
    if (!mm) importTableWorkspace(wsName,showDlg,makeVisible);
}

/**  Import the selected workspace, if any. Displays the import dialog.
 */
void MantidUI::importWorkspace()
{
    QString wsName = getSelectedWorkspaceName();
    importWorkspace(wsName,true,true);
}

/** #539: For adding Workspace History display to MantidPlot
	Show Algorithm History Details in a window .
 */
void MantidUI::showAlgorithmHistory()
{
	QString wsName=getSelectedWorkspaceName();
	Mantid::API::Workspace_sptr wsptr=getWorkspace(wsName);
	if(wsptr)
	{
		WorkspaceHistory wsHistory= wsptr->getHistory();
		std::vector<AlgorithmHistory>algHistory=wsHistory.getAlgorithmHistories();
		Mantid::Kernel::EnvironmentHistory envHistory=wsHistory.getEnvironmentHistory();
		//create a  window to display Algorithmic History
		if(!algHistory.empty())
		{
			AlgorithmHistoryWindow *palgHist= new AlgorithmHistoryWindow(m_appWindow,algHistory,envHistory);
			if(palgHist)palgHist->show();
		}
	}
	else
	{
		QMessageBox::information(appWindow(),"Mantid","Invalid WorkSpace");
		return;
	}
 }

/**  Create a new Table and fill it with the data from a Tableworkspace
     @param wsName Workspace name
     @param showDlg If true show a dialog box to set some import parameters
     @param makeVisible If true show the created Table, hide otherwise.
     @return A pointer to the new Table.
 */
Table* MantidUI::importTableWorkspace(const QString& wsName, bool, bool makeVisible)
{
    ITableWorkspace_sptr ws;
  	if (AnalysisDataService::Instance().doesExist(wsName.toStdString()))
	{
		ws = boost::dynamic_pointer_cast<ITableWorkspace>(AnalysisDataService::Instance().retrieve(wsName.toStdString()));
	}

    if (!ws.get()) return 0;

    if (ws->rowCount() == 0 || ws->columnCount() == 0)
    {
        showCritical("Cannot create an empty table");
        return 0;
    }

    // Create new Table
    Table* t = new Table(appWindow()->scriptEnv, ws->rowCount(), ws->columnCount(), "", appWindow(), 0);
    appWindow()->initTable(t, appWindow()->generateUniqueName(wsName+"-"));
    t->askOnCloseEvent(false);
    if (makeVisible) t->showNormal();
    else t->showMinimized();

    for(int i=0;i<ws->columnCount();i++)
    {
        Column_sptr c = ws->getColumn(i);
        t->setColName(i,QString::fromStdString(c->name()));
        t->setReadOnlyColumn(i);
        for(int j=0;j<ws->rowCount();j++)
        {
            std::ostringstream ostr;
            c->print(ostr,j);
            t->setText(j,i,QString::fromStdString(ostr.str()));
        }
    }
    return t;
}

void MantidUI::plotFirstSpectrum()
{
    QString wsName = getSelectedWorkspaceName();
    plotSpectrum(wsName,0);
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
        bool areSpectraSelected = static_cast<MantidMatrix*>(w)->setSelectedRows();
        bool areColumnsSelected = static_cast<MantidMatrix*>(w)->setSelectedColumns();
        cm.addAction(actionCopyValues);
        if (areSpectraSelected) cm.addAction(actionCopyRowToTable);
        if (areColumnsSelected) cm.addAction(actionCopyColumnToTable);
        cm.addAction(actionCopyDetectorsToTable);
        cm.addSeparator();

        if (areSpectraSelected)
        {
            cm.addAction(actionCopyRowToGraph);
            cm.addAction(actionCopyRowToGraphErr);
        }
        if (areColumnsSelected)
        {
            cm.addAction(actionCopyColumnToGraph);
            cm.addAction(actionCopyColumnToGraphErr);
        }
    }
}

void MantidUI::copyRowToTable()
{
    MantidMatrix* m = (MantidMatrix*)appWindow()->activeWindow();
    if (!m || !m->isA("MantidMatrix")) return;
    Table* t = createTableFromSelectedRows(m,true,true);
    t->showNormal();
}

void MantidUI::copyColumnToTable()
{
    MantidMatrix* m = (MantidMatrix*)appWindow()->activeWindow();
    if (!m || !m->isA("MantidMatrix")) return;
    Table* t = createTableFromSelectedColumns(m,true);
    t->showNormal();
}

void MantidUI::copyRowToGraph()
{
    MantidMatrix* m = (MantidMatrix*)appWindow()->activeWindow();
    if (!m || !m->isA("MantidMatrix")) return;
    createGraphFromSelectedRows(m,false,false);

}

void MantidUI::copyColumnToGraph()
{
    MantidMatrix* m = (MantidMatrix*)appWindow()->activeWindow();
    if (!m || !m->isA("MantidMatrix")) return;
    createGraphFromSelectedColumns(m,false);

}

void MantidUI::copyColumnToGraphErr()
{
    MantidMatrix* m = (MantidMatrix*)appWindow()->activeWindow();
    if (!m || !m->isA("MantidMatrix")) return;
    createGraphFromSelectedColumns(m,true);

}

void MantidUI::copyRowToGraphErr()
{
    MantidMatrix* m = (MantidMatrix*)appWindow()->activeWindow();
    if (!m || !m->isA("MantidMatrix")) return;
    createGraphFromSelectedRows(m,true,true);

}

void MantidUI::copyDetectorsToTable()
{
    MantidMatrix* m = (MantidMatrix*)appWindow()->activeWindow();
    if (!m || !m->isA("MantidMatrix")) return;
    createTableDetectors(m);
}

void MantidUI::copyValues()
{
    MantidMatrix* m = (MantidMatrix*)appWindow()->activeWindow();
    if (!m || !m->isA("MantidMatrix")) return;
    m->copySelection();
}


Table* MantidUI::createTableDetectors(MantidMatrix *m)
{
	 Table* t = new Table(appWindow()->scriptEnv, m->numRows(), 6, "", appWindow(), 0);
	 appWindow()->initTable(t, appWindow()->generateUniqueName(m->name()+"-Detectors-"));
   t->showNormal();
   t->askOnCloseEvent(false);

   Mantid::API::MatrixWorkspace_sptr ws = m->workspace();
   Mantid::API::Axis *spectraAxis = ws->getAxis(1);
   Mantid::Geometry::IObjComponent_const_sptr sample = ws->getInstrument()->getSample();
   for(int i=0;i<m->numRows();i++)
   {

         int ws_index = m->workspaceIndex(i);
         int currentSpec = spectraAxis->spectraNo(ws_index);
         int detID = 0;
         double R = 0.;
         double Theta = 0.;
         double Phi = 0.;
         try
         {
             boost::shared_ptr<Mantid::Geometry::IDetector> det = ws->getDetector(ws_index);
             detID = det->getID();
             // We want the position of the detector relative to the sample
             Mantid::Geometry::V3D pos = det->getPos() - sample->getPos();
             pos.getSpherical(R,Theta,Phi);
             // Need to get R & Theta through these methods to be correct for grouped detectors
             R = det->getDistance(*sample);
             Theta = ws->detectorTwoTheta(det)*180.0/M_PI;
         }
         catch(...)
         {
             detID = 0;
         }
         t->setCell(i,0,ws_index);
         if (i == 0) t->setColName(0,"Index");

         t->setCell(i,1,currentSpec);
         if (i == 0) t->setColName(1,"Spectra");

         t->setCell(i,2,detID);
         if (i == 0) t->setColName(2,"Detectors");

         t->setCell(i,3,R);
         if (i == 0) t->setColName(3,"R");

         t->setCell(i,4,Theta);
         if (i == 0) t->setColName(4,"Theta");

         t->setCell(i,5,Phi);
         if (i == 0) t->setColName(5,"Phi");
   }
   return t;
 }

bool MantidUI::drop(QDropEvent* e)
{
    if (e->source() == m_exploreMantid->m_tree)
    {
      QString wsName = getSelectedWorkspaceName();
      importWorkspace(wsName,false);
      return true;
    }

    return false;
}

/**  Sets the dependence between sindows: if the first one closes the second must close too.
     @param first The master window
     @param second The dependent window
 */
void MantidUI::setDependency(MdiSubWindow* first,MdiSubWindow* second)
{
    m_mdiDependency.insert( pair<MdiSubWindow*,MdiSubWindow*>(first,second) );
    connect(first, SIGNAL(closedWindow(MdiSubWindow*)), this, SLOT(closeDependents(MdiSubWindow*)));
}

// Called in response to closedWindow(...) signal from a window with dependecies
void MantidUI::closeDependents(MdiSubWindow*)
{
  //    std::cerr<<"Hi\n";
}

void MantidUI::getSelectedAlgorithm(QString& algName, int& version)
{
    QList<QTreeWidgetItem*> items = m_exploreAlgorithms->m_tree->selectedItems();
    if ( items.size() == 0 )
    {
        int i = m_exploreAlgorithms->m_findAlg->currentIndex();
        QString itemText = m_exploreAlgorithms->m_findAlg->itemText(i);
        if (i < 0 || itemText != m_exploreAlgorithms->m_findAlg->currentText())
        {
            algName = "";
            version = 0;
        }
        else
        {
            algName = itemText;
            version = -1;
        }
    }
    else if ( items[0]->childCount() != 0 && !items[0]->text(0).contains(" v."))
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
    executeAlgorithm(algName, version);
}
/**
    executes Save Nexus
	saveNexus Input Dialog is a generic dialog.Below code is added to remove
	the workspaces except the selected workspace from the InputWorkspace combo

*/
void MantidUI::executeSaveNexus(QString algName,int version)
{
	QString selctedWsName = getSelectedWorkspaceName();
	Mantid::API::IAlgorithm_sptr alg;
	try
	{
		alg=Mantid::API::AlgorithmManager::Instance().create(algName.toStdString(),version);

	}
	catch(...)
	{
		QMessageBox::critical(appWindow(),"MantidPlot - Algorithm error","Cannot create algorithm "+algName+" version "+QString::number(version));
		return;
	}
	if (alg)
	{
		MantidQt::API::AlgorithmDialog *dlg =
		  MantidQt::API::InterfaceManager::Instance().createDialog(alg.get(), m_appWindow);
		if( !dlg ) return;
		//getting the combo box which has input workspaces and removing the workspaces except the selected one
		QComboBox *combo = dlg->findChild<QComboBox*>();
		if(combo)
		{
			int count=combo->count();
			int index=count-1;
			while(count>1)
			{
				int selectedIndex=combo->findText(selctedWsName,Qt::MatchExactly );
				if(selectedIndex!=index)
				{
					combo->removeItem(index);
					count=combo->count();
				}
				index=index-1;

			}

		}//end of if loop for combo
		if ( dlg->exec() == QDialog::Accepted)
		{
			delete dlg;
			executeAlgorithmAsync(alg);
		}
		else
		{
			delete dlg;
		}
	}



}
void MantidUI::executeAlgorithm(QString algName, int version)
{

    Mantid::API::IAlgorithm_sptr alg;
    try
    {
        alg = Mantid::API::AlgorithmManager::Instance().create(algName.toStdString(),version);

    }
    catch(...)
    {
        QMessageBox::critical(appWindow(),"MantidPlot - Algorithm error","Cannot create algorithm "+algName+" version "+QString::number(version));
        return;
    }
    if (alg)
    {
      MantidQt::API::AlgorithmDialog *dlg = MantidQt::API::InterfaceManager::Instance().createDialog(alg.get(), m_appWindow);
      if( !dlg ) return;
      if ( dlg->exec() == QDialog::Accepted)
      {
	delete dlg;
	executeAlgorithmAsync(alg);
      }
      else
      {
	delete dlg;
      }
    }
}
void  MantidUI::copyWorkspacestoVector(const QList<QTreeWidgetItem*> &selectedItems,std::vector<std::string> &inputWSVec)
{
	//iterate through each of the selected workspaces
	QList<QTreeWidgetItem*>::const_iterator itr;
		for(itr=selectedItems.begin();itr!=selectedItems.end();itr++)
		{
			std::string inputWSName=(*itr)->text(0).toStdString();
			Workspace_sptr wsSptr=Mantid::API::AnalysisDataService::Instance().retrieve(inputWSName);
			WorkspaceGroup_sptr group=boost::dynamic_pointer_cast<WorkspaceGroup>(wsSptr);
			if(group)
			{
				//it's a group worksopace
				std::vector<std::string> inputGrpWS=group->getNames();
				if(!inputGrpWS.empty())
				{
					std::vector<std::string>::const_iterator it=inputGrpWS.begin();
					//copy  group workspace members to a vector except the 1st as it is of type workspacegroup
					for (it++;it!=inputGrpWS.end();it++)
					{	inputWSVec.push_back((*it));
					}

				}
			}
			else{

				inputWSVec.push_back(inputWSName);
			}


		}//end of for loop for input workspaces
}
void MantidUI::PopulateData(Workspace_sptr workspace,QTreeWidgetItem*  ws_item)
{
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
void MantidUI::renameWorkspace()
{ //get selected workspace
	QList<QTreeWidgetItem*>selectedItems=m_exploreMantid->m_tree->selectedItems();
	QString selctedWsName ("");
	if(!selectedItems.empty())
	{
		selctedWsName=selectedItems[0]->text(0);

	}

	std::string algName("RenameWorkspace");
	int version=-1;
	Mantid::API::IAlgorithm_sptr alg;
    try
    {
        alg = Mantid::API::AlgorithmManager::Instance().create(algName,version);

    }
    catch(...)
    {
		QMessageBox::critical(appWindow(),"MantidPlot - Algorithm error","Cannot create algorithm "+QString::fromStdString(algName)+" version "+QString::number(version));
        return;
    }
    if (alg)
    {
      MantidQt::API::AlgorithmDialog *dlg = MantidQt::API::InterfaceManager::Instance().createDialog(alg.get(), m_appWindow);
      if( !dlg ) return;
	  //getting the combo box which has input workspaces and removing the workspaces except the selected one
	  QComboBox *combo = dlg->findChild<QComboBox*>();
	  if(combo)
	  {
		  int count=combo->count();
		  int index=count-1;
		  while(count>1)
		  {
			  int selectedIndex=combo->findText(selctedWsName,Qt::MatchExactly );
			  if(selectedIndex!=index)
			  {
				  combo->removeItem(index);
				  count=combo->count();
			  }
			  index=index-1;

		  }

	  }//end of if loop for combo
	  if ( dlg->exec() == QDialog::Accepted)
	  {
		  delete dlg;
		  alg->execute();
	  }
      else
      {
	delete dlg;
      }
	  //getting the renamed ws name
	  std::string WSName=alg->getPropertyValue("OutputWorkspace");
	  //at this point selected group workspace is getting deleted from ADS by the algorithm RenameWorkspace ,
	  //so its children too getting deleted from tree
	  //below code adds the children back to the tree
	  moveSelctedWSChildrentoRenamedWS(WSName,selectedItems);

    }
}
void MantidUI::moveSelctedWSChildrentoRenamedWS(const std::string & renamedWSName,QList<QTreeWidgetItem*>& selectedItems)
{
	if(Mantid::API::AnalysisDataService::Instance().doesExist(renamedWSName))
	  {
		  Workspace_sptr wsSptr=Mantid::API::AnalysisDataService::Instance().retrieve(renamedWSName);
		  WorkspaceGroup_sptr wsGrpSptr=boost::dynamic_pointer_cast<WorkspaceGroup>(wsSptr);
		  if(wsGrpSptr)
		  {
			  QList<QTreeWidgetItem*>::const_iterator selectedItr;
			  for (selectedItr=selectedItems.begin();selectedItr!=selectedItems.end();selectedItr++)
			  {
				  //child count
				  int count=(*selectedItr)->childCount();
				  //for loop for moving the selected ws's children to  newly created (renamed) ws's children
				  for (int i=1;i<count;i++)
				  {
					  QTreeWidgetItem *pchild=NULL;
					  pchild=(*selectedItr)->child(i);
					  if(pchild)
					  {
						  //m_exploreMantid->m_tree->addTopLevelItem(pchild);
						  QTreeWidgetItem* ws_item = new QTreeWidgetItem(QStringList(pchild->text(0)));
						  if(ws_item)
						  {
							  std::string wsName=pchild->text(0).toStdString();
							  //get the workspace pointer
							  Workspace_sptr ws_ptr=Mantid::API::AnalysisDataService::Instance().retrieve(wsName);
							  if(ws_ptr)
							  {	//call function to add code for each workspace populating with histograms,bins etc
								  PopulateData(ws_ptr,ws_item);
							  }
							  ws_item->setIcon(0,QIcon(QPixmap(mantid_matrix_xpm)));
							  QList<QTreeWidgetItem *> name_matches = m_exploreMantid->m_tree->findItems(QString::fromStdString(renamedWSName),Qt::MatchExactly);
							  QTreeWidgetItem*  renamedWS=NULL;
							  if(!name_matches.isEmpty())
								  renamedWS=name_matches[0];
							  if(renamedWS)renamedWS->addChild(ws_item);
						  }

					  }
				  }//end of for loop child count
			  }//end of for loop of selected workspaces
		  }

	  }//end of liip for ADS exists check

}
void MantidUI::groupWorkspaces()
{
	try
	{
		std::string sgrpName("NewGroup");
		QString   qwsGrpName=QString::fromStdString(sgrpName);
		std::vector<std::string> inputWSVec;
		//get selected workspaces
		QList<QTreeWidgetItem*>selectedItems=m_exploreMantid->m_tree->selectedItems();
		if(selectedItems.size()<2)
		{	throw std::runtime_error("Select atleast two workspaces to group ");
		}
		if(Mantid::API::AnalysisDataService::Instance().doesExist(sgrpName))
		{
			if ( QMessageBox::question(appWindow(),"","Workspace "+qwsGrpName+" already exists. Do you want to replace it?",
				QMessageBox::Yes, QMessageBox::No) != QMessageBox::Yes) return;
		}
		//
		copyWorkspacestoVector(selectedItems,inputWSVec);
		std::string algName("GroupWorkspaces");
		Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create(algName,1);
		alg->initialize();
		alg->setProperty("InputWorkspaces",inputWSVec);
		alg->setPropertyValue("OutputWorkspace",sgrpName);
		//execute the algorithm
		bool bStatus =alg->execute();

		// move the selcted workspaces to the new group created on mantid tree
       if(bStatus)
	   {
		std::vector<std::string> grpVec;
		Workspace_sptr ws;

		if( !Mantid::API::AnalysisDataService::Instance().doesExist(sgrpName) ) return;

		//remove the selected workspaces from tree
		QList<QTreeWidgetItem*>::const_iterator selectedItr;
		for(selectedItr=selectedItems.begin();selectedItr!=selectedItems.end();selectedItr++)
		{
            //get the child count
			int count=(*selectedItr)->childCount();
			//for loop for removing the children
			for (int i=0;i<count;i++)
			{
				//QTreeWidgetItem *pchild=(*selectedItr)->child(0);
				(*selectedItr)->takeChild(0);
			}
			QTreeWidgetItem* parent=NULL;
			parent=(*selectedItr)->parent();
			if(parent==NULL)
			{
				int index=m_exploreMantid->m_tree->indexOfTopLevelItem((*selectedItr));
				m_exploreMantid->m_tree->takeTopLevelItem(index);
			}
			else
			{
				parent->removeChild((*selectedItr));
			}


		}//end of for loop for removing the  selected workspaces from the tree

		//add selected workspaces to the new group created

		//search for newly created workspace ("NewGroup")on  mantid workspace tree
		QList<QTreeWidgetItem *> name_matches = m_exploreMantid->m_tree->findItems(qwsGrpName,Qt::MatchExactly);
		QTreeWidgetItem*  newGroup=NULL;
		if(!name_matches.isEmpty())
			newGroup=name_matches[0];
		else
		{
			if(Mantid::API::AnalysisDataService::Instance().doesExist(sgrpName))
			{
				//if one of the selcted workspaces to group is "NewGroup" (i.e "NewGroup"  already exists in tree)
				//as i'm deleting the selected workspaces above "NewGroup" also getting deleted
				//so adding it to tree again
				QTreeWidgetItem*  ws_item=new QTreeWidgetItem(QStringList(QString::fromStdString(sgrpName)));
				ws_item->setIcon(0,QIcon(QPixmap(mantid_wsgroup_xpm)));
				QTreeWidgetItem* wsid_item=new QTreeWidgetItem(QStringList("WorkspaceGroup"));
				ws_item->addChild(wsid_item);
				m_exploreMantid->m_tree->addTopLevelItem(ws_item);
				newGroup=ws_item;
			}

		}
		if(newGroup)
		{
			//retrieve the output group workspace
			ws=Mantid::API::AnalysisDataService::Instance().retrieve(sgrpName);
			WorkspaceGroup_sptr grpWS=boost::dynamic_pointer_cast<WorkspaceGroup>(ws);
			//get the workspace group members
			if(grpWS)grpVec=grpWS->getNames();
			std::vector<std::string>::iterator itr=grpVec.begin();
			for(itr++;itr!=grpVec.end();itr++)
			{
				//add each output workspaces to the newly created group
				QTreeWidgetItem*  wsid_item=new QTreeWidgetItem(QStringList(QString::fromStdString((*itr))));
				if(wsid_item)
				{
					wsid_item->setIcon(0,QIcon(QPixmap(mantid_matrix_xpm)));

					//get the workspace pointer
					Workspace_sptr ws_ptr=Mantid::API::AnalysisDataService::Instance().retrieve((*itr));
					//Mantid::API::MatrixWorkspace_sptr ws_ptr = boost::dynamic_pointer_cast<MatrixWorkspace>(ws);
					if(ws_ptr)
					{
						//call function to add code for each workspace populating with histograms,bins etc
						PopulateData(ws_ptr,wsid_item);
					}

					newGroup->addChild(wsid_item);
				}
			}//end of for loop for adding the members in group workspace vector to mantid tree

		}//end of if loop for newGroup

	   }//end of if loop for bStatus

	}
	catch(std::invalid_argument &)
	{
		//logObject.error()<<"Error:"<<ex.what()<<std::endl;
	}
	catch(Mantid::Kernel::Exception::NotFoundError&)//if not a valid object in analysis data service
	{
		//logObject.error()<<"Error: "<< e.what()<<std::endl;
	}
	catch(std::runtime_error& )
	{
		//logObject.error()<<"Error:"<<ex.what()<<std::endl;
	}
	catch(std::exception& )
	{
		//logObject.error()<<"Error:"<<ex.what()<<std::endl;
	}

}
void MantidUI::ungroupWorkspaces()
{
	try
	{
		QList<QTreeWidgetItem*>selectedItems=m_exploreMantid->m_tree->selectedItems();
		if(selectedItems.isEmpty())
			throw std::runtime_error("Select a group workspace to Ungroup. ");

		std::vector<std::string > inputWSVec;
		QList<QTreeWidgetItem*>::const_iterator itr;
		for(itr=selectedItems.begin();itr!=selectedItems.end();itr++)
		{
			std::string name=(*itr)->text(0).toStdString();
			inputWSVec.push_back(name);
			Workspace_sptr wsSptr= Mantid::API::AnalysisDataService::Instance().retrieve(name);
			if(wsSptr)
			{
			WorkspaceGroup_sptr wsGrpSptr=boost::dynamic_pointer_cast<WorkspaceGroup>(wsSptr);
			if(!wsGrpSptr)
			throw std::invalid_argument("Selected Workspace is not a Group to Ungroup.\n"
			"Check the selected workspace type and ensure that the workspace is a group workspace to Ungroup");
			}
		}

		std::string algName("UnGroupWorkspace");
		Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create(algName,1);
		alg->initialize();
		alg->setProperty("InputWorkspaces",inputWSVec);

		//execute the algorithm
		bool bStatus=alg->execute();
		if(bStatus)
		{
			//at this point selected group workspace is getting deleted from ADS ,
			//so its children too getting deleted from tree
			//below code adds the children back to the tree as toplevel item
			QList<QTreeWidgetItem*>::const_iterator selectedItr;
			for (selectedItr=selectedItems.begin();selectedItr!=selectedItems.end();selectedItr++)
			{
				//child count
				int count=(*selectedItr)->childCount();
				//for loop for moving the children to tree as toplevel item
				for (int i=1;i<count;i++)
				{
					QTreeWidgetItem *pchild=NULL;
					pchild=(*selectedItr)->child(i);
					if(pchild)
					{
						//m_exploreMantid->m_tree->addTopLevelItem(pchild);
						QTreeWidgetItem* ws_item = new QTreeWidgetItem(QStringList(pchild->text(0)));
						if(ws_item)
						{
							std::string wsName=pchild->text(0).toStdString();
							//get the workspace pointer
							Workspace_sptr ws_ptr=Mantid::API::AnalysisDataService::Instance().retrieve(wsName);
							//Mantid::API::MatrixWorkspace_sptr ws_ptr = boost::dynamic_pointer_cast<MatrixWorkspace>(ws);
							if(ws_ptr)
							{	//call function to add code for each workspace populating with histograms,bins etc
								PopulateData(ws_ptr,ws_item);
							}
							ws_item->setIcon(0,QIcon(QPixmap(mantid_matrix_xpm)));
							m_exploreMantid->m_tree->addTopLevelItem(ws_item);
						}

					}//end of if loop for pchild
				}//end of for loop for child count iteration
			}//end of for loop selected items
		}

	}
	catch(std::invalid_argument &)
	{
	//	logObject.error()<<"Error:"<<ex.what()<<std::endl;
	}
	catch(std::runtime_error & )
	{
	//	logObject.error()<<"Error:"<<e.what()<<std::endl;
	}
	catch(std::exception & )
	{
		//logObject.error()<<"Error:"<<e.what()<<std::endl;
	}

}
void MantidUI::executeAlgorithmAsync(Mantid::API::IAlgorithm_sptr alg, bool showDialog)
{
    if (showDialog)
    {
        m_progressDialog = new ProgressDlg(alg,appWindow());
    }

    m_algMonitor->add(alg);
    try
    {
	  Poco::ActiveResult<bool> res = alg->executeAsync();
      if ( !res.tryWait(100) && showDialog)
      {
          //Use show rather than exec so that control is returned to the caller immediately
		   m_progressDialog->exec();
      }

    }
    catch(...)
    {
        QMessageBox::critical(appWindow(),"MantidPlot - Algorithm error","Exception is caught");
    }
}

void MantidUI::handleLoadDAEFinishedNotification(const Poco::AutoPtr<Mantid::API::Algorithm::FinishedNotification>& pNf)
{
    emit needToCreateLoadDAEMantidMatrix(pNf->algorithm());
}

void MantidUI::createLoadDAEMantidMatrix(const Mantid::API::IAlgorithm* alg)
{
    std::string wsName = alg->getProperty("OutputWorkspace");
    Workspace_sptr ws = AnalysisDataService::Instance().retrieve(wsName);

    if (ws.use_count() == 0)
    {
	    QMessageBox::warning(m_appWindow, tr("Mantid"),
           		    tr("A workspace with this name already exists.\n")
            		    , QMessageBox::Ok, QMessageBox::Ok);
	    return;
    }

    importMatrixWorkspace(QString::fromStdString(wsName), -1, -1, false, true);

    int updateInterval = m_DAE_map[wsName];
    if (updateInterval > 0)
    {
        IAlgorithm_sptr updater = CreateAlgorithm("UpdateDAE");
        updater->setPropertyValue("Workspace",wsName);
        updater->setPropertyValue("update_rate",QString::number(updateInterval).toStdString());
        executeAlgorithmAsync(updater,false);
    }

}

void MantidUI::showCritical(const QString& text)
{
    QMessageBox::critical(appWindow(),"Mantid - Error",text);
}

void MantidUI::showAlgMonitor()
{
    m_algMonitor->showDialog();
}

void MantidUI::handleAddWorkspace(WorkspaceAddNotification_ptr pNf)
{
  emit workspace_replaced(QString::fromStdString(pNf->object_name()), pNf->object());
}

void MantidUI::handleReplaceWorkspace(WorkspaceAfterReplaceNotification_ptr pNf)
{
  emit workspace_replaced(QString::fromStdString(pNf->object_name()), pNf->object());
}

void MantidUI::handleDeleteWorkspace(WorkspaceDeleteNotification_ptr pNf)
{
  emit workspace_removed(QString::fromStdString(pNf->object_name()));
}

void MantidUI::logMessage(const Poco::Message& msg)
{
    if (!appWindow()->results) return;
    QString str = msg.getText().c_str();
    //if (s_logEdit->document()->blockCount() > 1000) s_logEdit->document()->clear();
	//Ticket #671
	//to display the logwindow if there is ann error or higher log message
	if (msg.getPriority() <= Poco::Message::PRIO_ERROR)
	{
		appWindow()->logWindow->show();
	}
    if (msg.getPriority() < Poco::Message::PRIO_WARNING)
        appWindow()->results->setTextColor(Qt::red);
    else
        appWindow()->results->setTextColor(Qt::black);
    appWindow()->results->insertPlainText(str+"\n");
    //cerr<<":"<<appWindow()->results->document()->blockCount()<<'\n';
    QTextCursor cur = appWindow()->results->textCursor();
    cur.movePosition(QTextCursor::End);
    appWindow()->results->setTextCursor(cur);
}

void MantidUI::manageMantidWorkspaces()
{
#ifdef _WIN32
    memoryImage();
#else
	QMessageBox::warning(appWindow(),tr("Mantid Workspace"),tr("Clicked on Managed Workspace"),tr("Ok"),tr("Cancel"),QString(),0,1);
#endif
}

/**
 * Create an instrument window from a named workspace or simply return the window if
 * it already exists
 */
InstrumentWindow* MantidUI::getInstrumentView(const QString & wsName)
{

  if( !Mantid::API::AnalysisDataService::Instance().doesExist(wsName.toStdString()) ) return NULL;

  //See if a window for this instrument already exists
  //QMdiSubWindow *subWin(NULL);
  //foreach( subWin, appWindow()->d_workspace->subWindowList(QMdiArea::StackingOrder) )
  //{
  //  if( subWin->name() == QString("InstrumentWindow:") + wsName ) break;
  //}
  //if( subWin )
  //{
  //  return static_cast<InstrumentWindow*>(subWin);
  //}

  //Need a new window
  InstrumentWindow *insWin = new InstrumentWindow(QString("Instrument"),appWindow());
  insWin->setName(QString("InstrumentWindow:") + wsName);
  insWin->setWindowTitle(QString("Instrument - ") + wsName);
  appWindow()->d_workspace->addSubWindow(insWin);

  insWin->setWorkspaceName(wsName.toStdString());
  connect(insWin, SIGNAL(closedWindow(MdiSubWindow*)), appWindow(), SLOT(closeWindow(MdiSubWindow*)));
  connect(insWin,SIGNAL(hiddenWindow(MdiSubWindow*)), appWindow(), SLOT(hideWindow(MdiSubWindow*)));
  connect (insWin,SIGNAL(showContextMenu()), appWindow(),SLOT(showWindowContextMenu()));
  connect(insWin,SIGNAL(plotSpectra(const QString&,int)),this,SLOT(plotInstrumentSpectrum(const QString&,int)));
  connect(insWin,SIGNAL(plotSpectraList(const QString&,std::vector<int>)),this,SLOT(plotInstrumentSpectrumList(const QString&,std::vector<int>)));

  //  insWin->resize(400,400);

  return insWin;
}


void MantidUI::showMantidInstrument(const QString& wsName)
{
  InstrumentWindow *insWin = getInstrumentView(wsName);
  insWin->showWindow();
}

void MantidUI::showMantidInstrument()
{
	QStringList wsNames=getWorkspaceNames();
	if(!wsNames.isEmpty())
	{
		bool ok;
		QString selectedName = QInputDialog::getItem(appWindow(),tr("Select Workspace"), tr("Please select your workspace"), wsNames, 0, false,&ok);
		if(ok) showMantidInstrument(selectedName);
	}
}

void MantidUI::showMantidInstrumentSelected()
{
    QString wsName = getSelectedWorkspaceName();
    if (!wsName.isEmpty()) showMantidInstrument(wsName);
}

void MantidUI::mantidMenuAboutToShow()
{
	mantidMenu->clear();
	// Ticket #672 Mantid Menu Improvements

	/*mantidMenu->insertItem(tr("&Manage Workspaces"), this, SLOT(manageMantidWorkspaces() ) );
	mantidMenu->insertItem(tr("&Instrument Window"), this, SLOT(showMantidInstrument() ) );
	mantidMenu->insertItem(tr("&Plot Memory Usage"), this, SLOT(manageMantidWorkspaces() ));
	*/

	QAction* tstAction = new QAction("&Instrument Window",this);
	connect(tstAction,SIGNAL(triggered()), this, SLOT(showMantidInstrument()));
	mantidMenu->addAction(tstAction);

	tstAction = new QAction("&Plot Memory Usage",this);
	connect(tstAction,SIGNAL(triggered()), this, SLOT(manageMantidWorkspaces() ));
	mantidMenu->addAction(tstAction);

	tstAction = new QAction("&Clear All Memory",this);
	tstAction->setShortcut(QKeySequence::fromString("Ctrl+Shift+L"));
	connect(tstAction,SIGNAL(triggered()), this, SLOT(clearAllMemory() ));
	mantidMenu->addAction(tstAction);


}

void MantidUI::insertMenu()
{
	appWindow()->menuBar()->insertItem(tr("Man&tid"), mantidMenu);
}

void MantidUI::clearAllMemory()
{
  QMessageBox::StandardButton pressed =
    QMessageBox::question(appWindow(), "MantidPlot", "All workspaces and windows will be removed. Are you sure?", QMessageBox::Ok|QMessageBox::Cancel, QMessageBox::Ok);

  if( pressed != QMessageBox::Ok ) return;


  foreach( MdiSubWindow* sub_win, m_appWindow->windowsList() )
  {
    if( qobject_cast<MantidMatrix*>(sub_win) || qobject_cast<InstrumentWindow*>(sub_win))
    {
      sub_win->close();
    }
  }

  // Note that this call does not emit delete notifications
  Mantid::API::FrameworkManager::Instance().clear();
  m_exploreMantid->clearWorkspaceTree();
}

/**
  *  Prepares the Mantid Menu depending on the state of the active MantidMatrix.
  */
void MantidUI::menuMantidMatrixAboutToShow()
{
    menuMantidMatrix->clear();
    MantidMatrix *w = dynamic_cast<MantidMatrix*>(appWindow()->activeWindow());
    menuMantidMatrix->addAction(actionCopyValues);
    menuMantidMatrix->addAction(actionCopyDetectorsToTable);
    menuMantidMatrix->addSeparator();
    menuMantidMatrix->insertItem(tr("Set &Properties..."), w, SLOT(setMatrixProperties() ) );

    /*bool areSpectraSelected;
    int i0,i1;
    w->getSelectedRows(i0,i1);
    areSpectraSelected = i0 >= 0 && i1 >= 0;

    bool areColumnsSelected = w->setSelectedColumns();
    if (areSpectraSelected)
    {
        menuMantidMatrix->addAction(actionCopyRowToGraph);
        menuMantidMatrix->addAction(actionCopyRowToGraphErr);
    }
    if (areColumnsSelected)
    {
        menuMantidMatrix->addAction(actionCopyColumnToGraph);
        menuMantidMatrix->addAction(actionCopyColumnToGraphErr);
    }*/
}

/// Catches the signal from InstrumentWindow to plot a spectrum.
MultiLayer* MantidUI::plotInstrumentSpectrum(const QString& wsName, int spec)
{
  QMessageBox::information(appWindow(),"OK",wsName+" "+QString::number(spec));
  return plotSpectraRange(wsName, spec, spec, false, false, false);
}

/// Catches the signal from InstrumentWindow to plot a spectrum.
MultiLayer* MantidUI::plotInstrumentSpectrumList(const QString& wsName, std::vector<int> spec)
{
//    QMessageBox::information(appWindow(),"OK",wsName+" "+QString::number(spec));
  return plotSpectraList(wsName, spec, false, false, false);
}

MultiLayer* MantidUI::plotTimeBin(const QString& wsName, int bin, bool showMatrix)
{
  MantidMatrix* m = getMantidMatrix(wsName);
  if( !m )
  {
    m = importMatrixWorkspace(wsName, -1, -1, false, showMatrix);
  }
  MatrixWorkspace_sptr ws;
  if (AnalysisDataService::Instance().doesExist(wsName.toStdString()))
  {
    ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName.toStdString()));
  }
  if( !ws.get() ) return NULL;

  Table *t = createTableFromBins(wsName, ws, bin, bin, false, false);
  t->askOnCloseEvent(false);
  t->setAttribute(Qt::WA_QuitOnClose);
  MultiLayer* ml(NULL);
  if( !t ) return ml;

  ml = appWindow()->multilayerPlot(t,t->colNames(),Graph::Line);
  Graph *g = ml->activeGraph();
  appWindow()->polishGraph(g,Graph::Line);
  m->setBinGraph(ml,t);
  ml->askOnCloseEvent(false);
  return ml;
}

MultiLayer* MantidUI::plotBin(const QString& wsName, int bin, bool showMatrix)
{
  return plotTimeBin(wsName, bin, showMatrix);
}

//-------------------------------------------------
// The following commands are purely for the Python
// interface
//-------------------------------------------------

// In Python scripts we don't click and import a matrix so that we can plot
// spectra from it so this command does an import of a workspace and then plots
// the requested spectrum
MultiLayer* MantidUI::plotSpectrum(const QString& wsName, int spec, bool errorbars, bool showPlot, bool showMatrix)
{
  MantidMatrix* m = getMantidMatrix(wsName);
  if( !m )
  {
    m = importMatrixWorkspace(wsName, -1, -1, false, showMatrix);
  }
  MatrixWorkspace_sptr ws;
  if (AnalysisDataService::Instance().doesExist(wsName.toStdString()))
  {
    ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName.toStdString()));
  }
  if (!ws.get()) return NULL;

  Table *t = createTableFromSpectraRange(wsName, ws, spec, spec, errorbars, false);
  int type = ws->isHistogramData() ? 3 : 1;  // Type 3 is steps, 1 is a line
  MultiLayer* ml = createGraphFromTable(t,type);
  if (!ml) return NULL;

  m->setSpectrumGraph(ml,t);
  if( !showPlot ) ml->setVisible(false);
  return ml;
}

/**
 * Merge the curves from the two given MultiLayer objects
 */
MultiLayer* MantidUI::mergePlots(MultiLayer* mlayer_1, MultiLayer* mlayer_2)
{
  if( !mlayer_1 ) return NULL;
  if( !mlayer_2 ) return mlayer_1;

  int ncurves_on_two = mlayer_2->activeGraph()->visibleCurves();
  for( int c = 0; c < ncurves_on_two; ++c )
  {
    mlayer_1->insertCurve(mlayer_2, c);
  }

  // Hide the second graph for now as closing it
  // deletes the curves that were associated with it
  mlayer_2->hide();

  return mlayer_1;
};

MantidMatrix* MantidUI::getMantidMatrix(const QString& wsName)
{
  QList<MdiSubWindow*> windows = appWindow()->windowsList();
  QListIterator<MdiSubWindow*> itr(windows);
  MantidMatrix* m(0);
  while( itr.hasNext() )
  {
    MdiSubWindow *w = itr.next();
    if( w->isA("MantidMatrix") && w->name() == wsName )
    {
      m = qobject_cast<MantidMatrix*>(w);
    }
  }
  return m;
}

MantidMatrix* MantidUI::newMantidMatrix(const QString& wsName, int start, int end)
{
  return importMatrixWorkspace(wsName, false, false, start, end);
}

/**
 * Run the named algorithm asynchronously
 */
bool MantidUI::runAlgorithmAsynchronously(const QString & algName)
{
  Mantid::API::IAlgorithm_sptr alg = findAlgorithmPointer(algName);
  if( !alg ) return false;
  if( m_algMonitor ) m_algMonitor->add(alg);
  Poco::ActiveResult<bool> result = alg->executeAsync();
  while( !result.available() )
  {
    QCoreApplication::processEvents();
  }
  return result.data();
}

void MantidUI::cancelAllRunningAlgorithms()
{
  if( m_algMonitor ) m_algMonitor->cancelAll();
}

bool MantidUI::createPropertyInputDialog(const QString & alg_name, const QString & preset_values,
					 const QString & optional_msg,  const QString & enabled_names)
{
  Mantid::API::IAlgorithm_sptr alg = findAlgorithmPointer(alg_name);
  if( !alg )
  {
    return false;
  }

  MantidQt::API::AlgorithmDialog *dlg =
    MantidQt::API::InterfaceManager::Instance().createDialog(alg.get(), m_appWindow->getScriptWindowHandle(),
							     true, preset_values, optional_msg, enabled_names);
  return (dlg->exec() == QDialog::Accepted);
}

Mantid::API::IAlgorithm_sptr MantidUI::findAlgorithmPointer(const QString & algName)
{
  const deque<Mantid::API::IAlgorithm_sptr> & algorithms = Mantid::API::AlgorithmManager::Instance().algorithms();
  Mantid::API::IAlgorithm_sptr alg;
  deque<Mantid::API::IAlgorithm_sptr>::const_reverse_iterator aEnd = algorithms.rend();
  for(  deque<Mantid::API::IAlgorithm_sptr>::const_reverse_iterator aIter = algorithms.rbegin() ;
	aIter != aEnd; ++aIter )
  {
    if( !(*aIter)->isExecuted() && (*aIter)->name() == algName.toStdString()  )
    {
      alg = (*aIter);
      break;
    }
  }
  return alg;
}

void MantidUI::importSampleLog(const QString & filename, const QString & data, bool numeric)
{
  QStringList loglines = data.split("\n", QString::SkipEmptyParts);

  int rowcount(loglines.count());
  Table* t = new Table(appWindow()->scriptEnv, rowcount, 2, "", appWindow(), 0);
  if( !t ) return;
  t->askOnCloseEvent(false);
  //Have to replace "_" since the legend widget uses them to separate things
  QString label = filename;
  label.replace("_","-");

  appWindow()->initTable(t, appWindow()->generateUniqueName(label.section("-",0, 0) + "-"));
  t->setColName(0, "Time");
  t->setColumnType(0, Table::Time);
  t->setTimeFormat("HH:mm:ss", 0, false);
  t->setColName(1, label.section("-",1));

  QStringList::const_iterator sEnd = loglines.end();
  int row(0);
  for( QStringList::const_iterator sItr = loglines.begin(); sItr != sEnd; ++sItr, ++row )
  {
    QStringList ts = (*sItr).split(QRegExp("\\s+"));
    t->setText(row, 0, ts[1]);
    if( numeric ) t->setCell(row, 1, ts[2].toDouble());
    else
    {
        QStringList ds(ts);
        ds.removeFirst();// remove date
        ds.removeFirst();// and time
        t->setText(row, 1, ds.join(" "));
    }
  }

  //Show table
  t->resize(2*t->table()->horizontalHeader()->sectionSize(0) + 55,
	    (QMIN(10,rowcount)+1)*t->table()->verticalHeader()->sectionSize(0)+100);
  t->askOnCloseEvent(false);
  t->setAttribute(Qt::WA_DeleteOnClose);
  t->showNormal();

  // For string data we are done
  if( !numeric ) return;

  MultiLayer *ml = appWindow()->multilayerPlot(t,t->colNames(),Graph::Line);
  ml->askOnCloseEvent(false);
  ml->setAttribute(Qt::WA_DeleteOnClose);

  Graph* g = ml->activeGraph();
  g->setCurveStyle(0,3);
  g->setXAxisTitle(t->colLabel(0));
  g->setYAxisTitle(t->colLabel(1).section(".",0,0));
  g->setTitle(label.section("-",0, 0));

  ml->showNormal();
}

/**  Import a numeric log data. It will be shown in a graph and copied into a table
     @param wsName The workspace name which log data will be imported
     @param logname The name of the log property to import
     @param filter Filter flag telling how to filter the log data.
                - 0 means no filtering
                - 1 filter by running status
                - 2 filter by period
                - 3 filter by status & period
 */
void MantidUI::importNumSampleLog(const QString &wsName, const QString & logname, int filter)
{
//#define DBG
    Mantid::API::MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(getWorkspace(wsName));
    if (!ws) return;

    Mantid::Kernel::Property * logData = ws->getSample()->getLogData(logname.toStdString());
    if (!logData) return;

    Mantid::Kernel::LogFilter flt(logData);

    int rowcount = flt.data()->size();
    Table* t = new Table(appWindow()->scriptEnv, rowcount, 2, "", appWindow(), 0);
    if( !t ) return;
    t->askOnCloseEvent(false);
    //Have to replace "_" since the legend widget uses them to separate things
    QString label = logname;
    label.replace("_","-");

    appWindow()->initTable(t, appWindow()->generateUniqueName(label.section("-",0, 0) + "-"));
    t->setColName(0, "Time");
#ifndef DBG
    t->setColumnType(0, Table::Date);
    t->setDateFormat("yyyy-MMM-dd HH:mm:ss", 0, false);
#endif
    t->setColName(1, label.section("-",1));

    int iValueCurve = 0;
    int iFilterCurve = 1;

    // Applying filters
    if (filter > 0)
    {
        Mantid::Kernel::TimeSeriesProperty<bool>* f = 0;
        if (filter == 1 || filter ==3)
        {
            try
            {
                f = dynamic_cast<Mantid::Kernel::TimeSeriesProperty<bool> *>(ws->getSample()->getLogData("running"));
                if (f) flt.addFilter(f);
                else
                {
                    importNumSampleLog(wsName,logname,0);
                    return;
                }
            }
            catch(...)
            {
                importNumSampleLog(wsName,logname,0);
                return;
            }
        }

        if (filter == 2 || filter ==3)
        {
            std::vector<Mantid::Kernel::Property*> ps = ws->getSample()->getLogData();
            for(std::vector<Mantid::Kernel::Property*>::const_iterator it=ps.begin();it!=ps.end();it++)
                if ((*it)->name().find("period ") == 0)
                {
                    try
                    {
                        f = dynamic_cast<Mantid::Kernel::TimeSeriesProperty<bool> *>(*it);
                        if (f) flt.addFilter(f);
                        else
                        {
                            importNumSampleLog(wsName,logname,0);
                            return;
                        }
                    }
                    catch(...)
                    {
                        importNumSampleLog(wsName,logname,0);
                        return;
                    }

                    break;
                }
        }

        //std::cerr<<flt.filter()->value()<<'\n';

        t->addColumns(2);
        t->setColName(2, "FTime");
#ifndef DBG
        t->setColumnType(2, Table::Date);
        t->setDateFormat("yyyy-MMM-dd HH:mm:ss", 2, false);
#endif
        t->setColPlotDesignation(2,Table::X);
        t->setColName(3, "Filter");

        if (flt.filter()->size() > rowcount)
        {
            t->addRows(flt.filter()->size() - rowcount);
        }

        for(int i=0;i<flt.filter()->size();i++)
        {
#ifdef DBG
            t->setCell(i,2,int(flt.filter()->nthInterval(i).begin())-1223815000);
#else
            t->setText(i,2,QString::fromStdString(flt.filter()->nthInterval(i).begin_str()));
#endif
            t->setCell(i,3,!flt.filter()->nthValue(i));
        }

        iValueCurve = 1;
        iFilterCurve = 0;
    }

    // Show unfiltered data
    //flt.clear();

    Mantid::Kernel::dateAndTime lastTime;
    double lastValue;
    for(int i=0;i<flt.data()->size();i++)
    {
        lastTime = flt.data()->nthInterval(i).begin();
        lastValue = flt.data()->nthValue(i);
#ifdef DBG
        t->setCell(i,0,int(flt.data()->nthInterval(i).begin())-1223815000);
#else
        t->setText(i,0,QString::fromStdString(flt.data()->nthInterval(i).begin_str()));
#endif
        t->setCell(i,1,lastValue);
    }

    if (filter && lastTime < flt.filter()->lastTime())
    {
        rowcount = flt.data()->size();
        if (rowcount == t->numRows()) t->addRows(1);
        t->setText(rowcount,0,QDateTime::fromTime_t(flt.filter()->lastTime()).toString("yyyy-MMM-dd HH:mm:ss"));
        //t->setText(rowcount,0,QString::fromStdString(flt.filter()->nthInterval(flt.filter()->size()-1).end_str()));
        t->setCell(rowcount,1,lastValue);
    }

  //Show table

  t->resize(2*t->table()->horizontalHeader()->sectionSize(0) + 55,
	    (QMIN(10,t->numRows())+1)*t->table()->verticalHeader()->sectionSize(0)+100);
  t->askOnCloseEvent(false);
  t->setAttribute(Qt::WA_DeleteOnClose);
  t->showNormal();

  QStringList colNames;
  colNames << t->colName(3)<<t->colName(1);
  MultiLayer *ml = appWindow()->multilayerPlot(t,colNames,Graph::Line);
  ml->askOnCloseEvent(false);
  ml->setAttribute(Qt::WA_DeleteOnClose);

  Graph* g = ml->activeGraph();

#ifndef DBG
  // Set x-axis label format
  QDateTime dt = QDateTime::fromTime_t(flt.data()->nthInterval(0).begin());
  QString format = dt.toString(Qt::ISODate) + ";HH:mm:ss";
  g->setLabelsDateTimeFormat(2,ScaleDraw::Date,format);
#endif

  // Set style #3 (HorizontalSteps) for curve iValueCurve
  g->setCurveStyle(iValueCurve,3);
  QPen pn = QPen(Qt::black);
  g->setCurvePen(iValueCurve, pn);

  if (filter)
  {
      QwtPlotCurve *c = g->curve(iFilterCurve);
      // Set the right axis as Y axis for the filter curve.
      c->setAxis(2,1);
      // Set style #3 (HorizontalSteps) for curve 1
      g->setCurveStyle(iFilterCurve,3);
      // Set scale of right Y-axis (#3) from 0 to 1
      g->setScale(3,0,1);
      // Fill area under the curve with a pattern
      QBrush br = QBrush(Qt::gray, Qt::Dense5Pattern);
      g->setCurveBrush(iFilterCurve, br);
      // Set line colour
      QPen pn = QPen(Qt::gray);
      g->setCurvePen(iFilterCurve, pn);
  }
  g->setXAxisTitle(t->colLabel(0));
  g->setYAxisTitle(t->colLabel(1).section(".",0,0));
  g->setTitle(label.section("-",0, 0));
  g->setAutoScale();

  ml->showNormal();

}

void MantidUI::showLogFileWindow()
{
  //Need a new window to display entries
  MantidSampleLogDialog *dlg = new MantidSampleLogDialog(getSelectedWorkspaceName(), this);
  dlg->setModal(false);
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->show();
  dlg->setFocus();
}

    //  *****      Plotting Methods     *****  //

/** Create a Table form specified spectra in a MatrixWorkspace
    @param tableName Table name
    @param workspace Shared pointer to the workspace
    @param indexList A list of spectra indices to go to the table
    @param errs If true include the errors into the table
    @param binCentres If true the X column will contain the bin centres, i.e. (x_i+1 + x_i)/2.
       If false the Y values will be in the same row with the left bin boundaries.
       If the workspace is not a histogram the parameter is ignored.
 */
Table* MantidUI::createTableFromSpectraList(const QString& tableName, Mantid::API::MatrixWorkspace_sptr workspace, std::vector<int> indexList, bool errs, bool binCentres)
{
	 int nspec = workspace->getNumberHistograms();
	 //Loop through the list of index and remove all the indexes that are out of range
	 for (std::vector<int>::iterator it=indexList.begin();it!=indexList.end();it++)
		std::cout << *it << "\n";
	 for(std::vector<int>::iterator it=indexList.begin();it!=indexList.end();it++)
	 {
		if ((*it) > nspec || (*it) < 0) indexList.erase(it);
	 }
	 if ( indexList.empty() ) return 0;

     int c = errs?2:1;
     int numRows = workspace->blocksize();
     bool isHistogram = workspace->isHistogramData();
     int no_cols = static_cast<int>(indexList.size());
     Table* t = new Table(appWindow()->scriptEnv, numRows, (1+c)*no_cols, "", appWindow(), 0);
     appWindow()->initTable(t, appWindow()->generateUniqueName(tableName+"-"));
     t->askOnCloseEvent(false);

     int kX(0),kY(0),kErr(0);
     for(int i=0;i < no_cols; i++)
     {
       const std::vector<double>& dataX = workspace->readX(indexList[i]);
       const std::vector<double>& dataY = workspace->readY(indexList[i]);
       const std::vector<double>& dataE = workspace->readE(indexList[i]);

         kY =(c+1)*i+1;
		 kX=(c+1)*i;
		 t->setColName(kY,"YS"+QString::number(indexList[i]));
		 t->setColName(kX,"XS"+QString::number(indexList[i]));
         t->setColPlotDesignation(kX,Table::X);
         if (errs)
         {
            // kErr = 2*i + 2;
			 kErr=(c+1)*i+2;
             t->setColPlotDesignation(kErr,Table::yErr);
             t->setColName(kErr,"ES"+QString::number(indexList[i]));
         }
         for(int j=0;j<numRows;j++)
         {

             //if (i == 0)
             //{
                 // in histograms the point is to be drawn in the centre of the bin.
                 if (isHistogram && binCentres)
				 {
					// logObject.error()<<"inside isHistogram true= "<<j<< std::endl;
					 t->setCell(j,kX,( dataX[j] + dataX[j+1] )/2);
				 }
                 else
				 {//logObject.error()<<"inside isHistogram false= "<< std::endl;
                     t->setCell(j,kX,dataX[j]);
				 }
    //         }
			 //else
			 //{
				// t->setCell(j,kX,dataX[j]);
			 //}
		     t->setCell(j,kY,dataY[j]);


             if (errs) t->setCell(j,kErr,dataE[j]);
         }
         if (isHistogram && (!binCentres))
         {
             int iRow = numRows;
             t->addRows(1);
	         if (i == 0) t->setCell(iRow,0,dataX[iRow]);
             t->setCell(iRow,kY,0);
             if (errs) t->setCell(iRow,kErr,0);
         }
     }
     t->askOnCloseEvent(false);
     return t;
 }


/** Creates a Qtiplot Table from selected spectra of MantidMatrix m.
    The columns are: 1st column is x-values from the first selected spectrum,
    2nd column is y-values of the first spectrum. Depending on value of errs
    the 3rd column contains either first spectrum errors (errs == true) or
    y-values of the second spectrum (errs == false). Consecutive columns have
    y-values and errors (if errs is true) of the following spectra. If visible == true
    the table is made visible in Qtiplot.

    The name of a Y column is "Y"+QString::number(i), where i is the row in the MantidMatrix,
    not the spectrum index in the workspace.

*/
Table* MantidUI::createTableFromSelectedRows(MantidMatrix *m, bool errs, bool binCentres)
{
     int i0,i1;
     m->getSelectedRows(i0,i1);
     if (i0 < 0 || i1 < 0) return 0;
     std::vector<int> indexList;
     for(int i=i0;i<=i1;i++)
         indexList.push_back(m->workspaceIndex(i));

     return createTableFromSpectraList(m->name(), m->workspace(), indexList, errs, binCentres);
 }

Table* MantidUI::createTableFromSpectraRange(const QString& wsName, Mantid::API::MatrixWorkspace_sptr workspace, int i0, int i1, bool errs, bool binCentres)
{
     if (i0 < 0 || i1 < 0) return 0;
     int nspec = workspace->getNumberHistograms();
     if (i0 > nspec || i1 > nspec) return 0;
     std::vector<int> indexList;
     for(int i=i0;i<=i1;i++)
         indexList.push_back(i);

     return createTableFromSpectraList(wsName, workspace, indexList, errs, binCentres);
 }

/**  Create a 1d graph from a Table.
     @param t Pointer to the Table.
     @param type Type of the curve. Possible values are:
         - Graph::Line
         - Graph::Scatter
         - Graph::LineSymbols
         - Graph::HorizontalSteps
 */
MultiLayer* MantidUI::createGraphFromTable(Table* t, int type)
{
    if (!t) return NULL;
	QStringList  lst=t->colNames();
	QStringList::const_iterator itr;
	for (itr=lst.begin();itr!=lst.end();itr++)
	{
		//remove the X names from the column list and pass the X removed list
		//to multilayerPlot
		QString str=(*itr);
		int index=0;
		if(str.contains("XS",Qt::CaseInsensitive))
		{
			index=lst.indexOf(str);
			lst.removeAt(index);
		}
	}
	//MultiLayer* ml = appWindow()->multilayerPlot(t,t->colNames(),Graph::Line);
	MultiLayer* ml = appWindow()->multilayerPlot(t,lst,Graph::Line);
	Graph *g = ml->activeGraph();
	appWindow()->polishGraph(g,type);
    for(int i=0;i<g->curves();i++)
        g->setCurveStyle(i,type);
	ml->askOnCloseEvent(false);

    return ml;
}

/** Set properties of a 1d graph which plots spectrum data from a workspace such as the title and axes captions.
    @param ml MultiLayer plot with the graph
    @param Name Name of the graph
    @param workspace The workspace
 */
void MantidUI::setUpSpectrumGraph(MultiLayer* ml, const QString& Name, Mantid::API::MatrixWorkspace_sptr workspace)
{
    Graph* g = ml->activeGraph();
    g->setTitle(tr("Workspace ")+Name);
    Mantid::API::Axis* ax;
    ax = workspace->getAxis(0);
    std::string s;
    if (ax->unit().get()) s = ax->unit()->caption() + " / " + ax->unit()->label();
    else if (!ax->title().empty())
        s = ax->title();
    else
        s = "X axis";
    g->setXAxisTitle(tr(s.c_str()));
    g->setYAxisTitle(tr(workspace->YUnit().c_str()));
    g->setAntialiasing(false);
}

/** Set properties of a 1d graph which plots bin data from a workspace.
    @param ml MultiLayer plot with the graph
    @param Name Name of the graph
    @param workspace The workspace
 */
void MantidUI::setUpBinGraph(MultiLayer* ml, const QString& Name, Mantid::API::MatrixWorkspace_sptr workspace)
{
    Graph* g = ml->activeGraph();
    g->setTitle(tr("Workspace ")+Name);
    std::string xtitle;
    if (workspace->axes() > 1)   // Protection against calling this on 1D/single value workspaces
    {
      const Axis* const axis = workspace->getAxis(1);
      if ( axis->isSpectra() ) xtitle = "Spectrum Number";
      else if ( axis->unit() ) xtitle = axis->unit()->caption() + " / " + axis->unit()->label();
    }
    g->setXAxisTitle(tr(xtitle.c_str()));
    g->setYAxisTitle(tr(workspace->YUnit().c_str()));
    g->setAntialiasing(false);
}

/** Create a 1d graph form specified spectra in a MatrixWorkspace
    @param graphName Graph name
    @param workspace Shared pointer to the workspace
    @param indexList A list of spectra indices to be shown in the graph
    @param errs If true include the errors to the graph
    @param binCentres  If the workspace is a histogram binCentres defines the way the plot is drawn.
         If true it is a line going through the bin centres. Otherwise it will be made of horizontal steps
    @param tableVisible Visibility flag for the Table with the plotted data.
 */
MultiLayer* MantidUI::createGraphFromSpectraList(const QString& graphName, Mantid::API::MatrixWorkspace_sptr workspace, std::vector<int>& indexList, bool errs, bool binCentres, bool tableVisible)
{
	/// Needs to make the spectra list of unique number. I don't know why we did not used set
	/// in the first place. Will need to address that.
	std::set<int> indexS;
	for (std::vector<int>::iterator it=indexList.begin();it!=indexList.end();it++)
		indexS.insert(*it);
	std::vector<int> newIndex(indexS.size());
	std::copy(indexS.begin(),indexS.end(),newIndex.begin());

	Table *t = createTableFromSpectraList(graphName, workspace, newIndex, errs, binCentres);

    if (tableVisible) t->showNormal();

    //Graph::CurveType type = binCentres? Graph::Line : Graph::HorizontalSteps;
    int type = binCentres || !workspace->isHistogramData()? 1 : 3;
    MultiLayer* ml = createGraphFromTable(t,type);
    setUpSpectrumGraph(ml,graphName,workspace);
    setDependency(ml,t);

    return ml;
}

/** Create a 1d graph form specified spectra in a MatrixWorkspace
    @param graphName Graph name
    @param workspace Shared pointer to the workspace
    @param i0 Starting index
    @param i1 Last index
    @param errs If true include the errors to the graph
    @param binCentres  If the workspace is a histogram binCentres defines the way the plot is drawn.
         If true it is a line going through the bin centres. Otherwise it will be made of horizontal steps
    @param tableVisible Visibility flag for the Table with the plotted data.
 */
MultiLayer* MantidUI::createGraphFromSpectraRange(const QString& graphName, Mantid::API::MatrixWorkspace_sptr workspace, int i0, int i1, bool errs, bool binCentres, bool tableVisible)
{
     if (i0 < 0 || i1 < 0) return 0;
     int nspec = workspace->getNumberHistograms();
     if (i0 > nspec || i1 > nspec) return 0;
     /** For instrument with one to many spectra-detector mapping,
      * different pixels with correspond to the same specta so
      * we need to remove doublons in this case.
      */
     std::set<int> indexList;
     for(int i=i0;i<=i1;i++)
         indexList.insert(i);
     // The index list vector
     std::vector<int> indexListV(indexList.size());
     std::copy(indexList.begin(),indexList.end(),indexListV.begin());

    return createGraphFromSpectraList(graphName,workspace,indexListV,errs,binCentres,tableVisible);
}

/**  Create a graph and plot the selected rows of a MantidMatrix
     @param m Mantid matrix
     @param errs True if the errors to be plotted
     @param binCentres  If the workspace is a histogram binCentres defines the way the plot is drawn.
         If true it is a line going through the bin centres. Otherwise it will be made of horizontal steps
     @param tableVisible Defines if the created table should be visible
 */
MultiLayer* MantidUI::createGraphFromSelectedRows(MantidMatrix *m, bool errs, bool binCentres, bool tableVisible)
{
    Table *t = createTableFromSelectedRows(m,errs,binCentres);

    if (tableVisible) t->showNormal();

    //Graph::CurveType type = binCentres? Graph::Line : Graph::HorizontalSteps;
    int type = binCentres || !m->workspace()->isHistogramData()? 1 : 3;
    MultiLayer* ml = createGraphFromTable(t,type);
    m->setSpectrumGraph(ml,t);

    return ml;
}

/** Create a 1d graph form specified spectra in a MatrixWorkspace. The workspace must be in the AnalisysDataService
    under name wsName. The range of the spectra to plot is from i0 to i1 inclusive.
    @param graphName Graph name
    @param workspace Shared pointer to the workspace
    @param indexList
    @param errs If true include the errors to the graph
    @param binCentres  If the workspace is a histogram binCentres defines the way the plot is drawn.
         If true it is a line going through the bin centres. Otherwise it will be made of horizontal steps
    @param tableVisible Visibility flag for the Table with the plotted data.
 */
MultiLayer* MantidUI::plotSpectraList(const QString& wsName, std::vector<int> indexList, bool errs, bool binCentres, bool tableVisible)
{
    MatrixWorkspace_sptr ws;
    if (AnalysisDataService::Instance().doesExist(wsName.toStdString()))
    {
        ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName.toStdString()));
    }
    if (!ws.get()) return NULL;

    return createGraphFromSpectraList(wsName, ws, indexList, errs, binCentres, tableVisible);
}

/** Create a 1d graph form specified spectra in a MatrixWorkspace. The workspace must be in the AnalisysDataService
    under name wsName. The range of the spectra to plot is from i0 to i1 inclusive.
    @param graphName Graph name
    @param workspace Shared pointer to the workspace
    @param i0 First index
    @param i1 Last index
    @param errs If true include the errors to the graph
    @param binCentres  If the workspace is a histogram binCentres defines the way the plot is drawn.
         If true it is a line going through the bin centres. Otherwise it will be made of horizontal steps
    @param tableVisible Visibility flag for the Table with the plotted data.
 */
MultiLayer* MantidUI::plotSpectraRange(const QString& wsName, int i0, int i1, bool errs, bool binCentres, bool tableVisible )
{
    MatrixWorkspace_sptr ws;
    if (AnalysisDataService::Instance().doesExist(wsName.toStdString()))
    {
        ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName.toStdString()));
    }
    if (!ws.get()) return NULL;

    return createGraphFromSpectraRange(wsName,ws,i0,i1,errs,binCentres,tableVisible);
}

Table* MantidUI::createTableFromBins(const QString& wsName, Mantid::API::MatrixWorkspace_sptr workspace, int c0, int c1, bool errs, int fromRow, int toRow)
{
  if (c0 < 0 || c1 < 0) return NULL;

  int c = errs?2:1;
  int numRows = workspace->getNumberHistograms();

  int j0 = fromRow >= 0? fromRow : 0;
  int j1 = toRow   >= 0? toRow   : numRows - 1;

  if (j0 >= numRows || j1 >= numRows) return NULL;

  Table* t = new Table(appWindow()->scriptEnv, numRows, c*(c1 - c0 + 1) + 1, "", appWindow(), 0);
  appWindow()->initTable(t, appWindow()->generateUniqueName(wsName + "-"));
  t->askOnCloseEvent(false);
  int kY,kErr;
  for(int i = c0; i <= c1; i++)
  {
    kY = c*(i-c0)+1;
    t->setColName(kY,"YB"+QString::number(i));
    if (errs)
    {
        kErr = 2*(i - c0) + 2;
        t->setColPlotDesignation(kErr,Table::yErr);
        t->setColName(kErr,"EB"+QString::number(i));
    }
    for(int j = j0; j <= j1; j++)
    {
      const std::vector<double>& dataY = workspace->readY(j);
      const std::vector<double>& dataE = workspace->readE(j);

	    if (i == c0)
	    {
	      // Get the X axis values from the vertical axis of the workspace
	      if (workspace->axes() > 1) t->setCell(j,0,(*workspace->getAxis(1))(j));
	      else t->setCell(j,0,j);
	    }
	    t->setCell(j,kY,dataY[i]);
	    if (errs) t->setCell(j,kErr,dataE[i]);
    }
  }
  return t;
}

Table* MantidUI::createTableFromSelectedColumns(MantidMatrix *m, bool errs)
{
     int i0,i1;
     m->getSelectedColumns(i0,i1);
     if (i0 < 0 || i1 < 0) return 0;

     int j0 = m->workspaceIndex(0);
     int j1 = m->workspaceIndex(m->numRows()-1);

     return createTableFromBins(m->name(), m->workspace(), i0, i1, errs, j0, j1);

 }

MultiLayer* MantidUI::createGraphFromSelectedColumns(MantidMatrix *m, bool errs, bool tableVisible)
{
    Table *t = createTableFromSelectedColumns(m,errs);
    t->askOnCloseEvent(false);
    if (!t) return NULL;
    if (tableVisible) t->showNormal();

    MultiLayer* ml = appWindow()->multilayerPlot(t,t->colNames(),Graph::Line);
    Graph *g = ml->activeGraph();
    appWindow()->polishGraph(g,Graph::Line);
    m->setBinGraph(ml,t);
    ml->askOnCloseEvent(false);

    return ml;
}

//=========================================================================
//
// This section defines some stuff that is only used on Windows
//
//=========================================================================
#ifdef _WIN32

struct mem_block
{
    int size;
    int state;
};

///  Assess the virtual memeory of the current process.
void countVirtual(vector<mem_block>& mem, int& total)
{

    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx( &memStatus );

    MEMORY_BASIC_INFORMATION info;

    char *addr = 0;
    size_t free = 0;      // total free space
    size_t reserved = 0;  // total reserved space
    size_t committed = 0; // total commited (used) space
    size_t size = 0;
    size_t free_max = 0;     // maximum contiguous block of free memory
    size_t reserved_max = 0; // maximum contiguous block of reserved memory
    size_t committed_max = 0;// maximum contiguous block of committed memory

    size_t GB2 = memStatus.ullTotalVirtual;// Maximum memeory available to the process
    total = GB2;

    // Loop over all virtual memory to find out the status of every block.
    do
    {
        VirtualQuery(addr,&info,sizeof(MEMORY_BASIC_INFORMATION));

        int state;
        if (info.State == MEM_FREE)
        {
            free += info.RegionSize;
            if (info.RegionSize > free_max) free_max = info.RegionSize;
            state = 0;
        }
        if (info.State == MEM_RESERVE)
        {
            reserved += info.RegionSize;
            if (info.RegionSize > reserved_max) reserved_max = info.RegionSize;
            state = 500;
        }
        if (info.State == MEM_COMMIT)
        {
            committed += info.RegionSize;
            if (info.RegionSize > committed_max) committed_max = info.RegionSize;
            state = 1000;
        }

        addr += info.RegionSize;
        size += info.RegionSize;

        mem_block b = {info.RegionSize, state};
        mem.push_back(b);

    /*cerr<<"BaseAddress = "<< info.BaseAddress<<'\n';
    cerr<<"AllocationBase = "<< info.AllocationBase<<'\n';
    cerr<<"AllocationProtect = "<< info.AllocationProtect<<'\n';
    cerr<<"RegionSize = "<< hex << info.RegionSize<<'\n';
    cerr<<"State = "<< state_str(info.State)<<'\n';
    cerr<<"Protect = "<< hex << info.Protect <<' '<< protect_str(info.Protect)<<'\n';
    cerr<<"Type = "<< hex << info.Type<<'\n';*/

    }
    while(size < GB2);


    cerr<<"count FREE = "<<dec<<free/1024<<'\n';
    cerr<<"count RESERVED = "<<reserved/1024<<'\n';
    cerr<<"count COMMITTED = "<<committed/1024<<'\n';

    cerr<<"max FREE = "<<dec<<free_max<<'\n';
    cerr<<"max RESERVED = "<<reserved_max<<'\n';
    cerr<<"max COMMITTED = "<<committed_max<<'\n';
    cerr<<'\n';
}

/// Shows 2D plot of current memory usage.
/// One point is 1K of memory. One row is 1M.
/// Red - used memory block, blue - free, green - reserved.
void MantidUI::memoryImage()
{
    //ofstream ofil("memory.txt");
    vector<mem_block> mem;
    int total;
    countVirtual(mem,total);
    int colNum = 1024;
    int rowNum = total/1024/colNum;
    Matrix *m = appWindow()->newMatrix(rowNum,colNum);
    m->setCoordinates(0,colNum,0,rowNum);
    int row = 0;
    int col = 0;
    for(vector<mem_block>::iterator b=mem.begin();b!=mem.end();b++)
    {
        int n = b->size/1024;
        for(int i=0;i<n;i++)
        {
            m->setCell(row,col,b->state);
            //ofil<<b->state<<'\t';
            col++;
            if (col >= colNum)
            {
                col = 0;
                row++;
                //ofil<<'\n';
            }
        }
    }
    MultiLayer* g = appWindow()->plotSpectrogram(m, Graph::ColorMap);
}

#endif
//=======================================================================
// End of Windows specfic stuff
//=======================================================================

#include "MantidAPI/Instrument.h"
#include "MantidGeometry/CompAssembly.h"

void MantidUI::test()
{
    std::cerr<<"\nTest\n\n";

    Mantid::API::MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(getSelectedWorkspace());
    if (ws)
    {
        boost::shared_ptr<Mantid::API::Instrument> instr = ws->getBaseInstrument();
        boost::shared_ptr<Mantid::Geometry::CompAssembly> both = boost::dynamic_pointer_cast<Mantid::Geometry::CompAssembly>((*instr)[3]);
        if (both)
        {
            boost::shared_ptr<Mantid::Geometry::CompAssembly> first = boost::dynamic_pointer_cast<Mantid::Geometry::CompAssembly>((*both)[0]);
            if (first)
            {
                static int i = 0;
                Mantid::Geometry::V3D u = i++ ? Mantid::Geometry::V3D(1,0,0) : Mantid::Geometry::V3D(0,1,0);
                Mantid::Geometry::Quat q(30,u);
                first->rotate(q);
                return;
            }
        }
    }
    std::cerr<<"Failed...\n";
}
