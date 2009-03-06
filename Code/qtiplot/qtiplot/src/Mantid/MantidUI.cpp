#include "MantidUI.h"
#include "MantidMatrix.h"
#include "MantidDock.h"

#include "ImportWorkspaceDlg.h"

// Replaced with separate library
//#include "ExecuteAlgorithm.h"
//#include "LoadRawDlg.h"

//#include "InputHistory.h" 

#include "LoadDAEDlg.h"

#include "AlgMonitor.h"
#include "../Spectrogram.h"
#include "../pixmaps.h"
#include "MantidSampleLogDialog.h"
#include "../ScriptWindow.h"

#include "MantidKernel/Property.h"
#include "MantidPlotReleaseDate.h"
#include "InstrumentWidget/InstrumentWindow.h"


#include "MantidQtAPI/DialogManager.h"
#include "MantidQtAPI/AlgorithmDialog.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"

#include <QMessageBox>
#include <QTextEdit>
#include <QListWidget>
#include <QMdiArea>
#include <QMenuBar>
#include <QApplication>
#include <QToolBar>
#include <QMenu>
#include <QInputDialog>

#ifdef _WIN32
#include <windows.h>
#endif

#include <fstream>
#include <iostream>
using namespace std;
 
MantidUI::MantidUI(ApplicationWindow *aw):m_appWindow(aw),
m_finishedObserver(*this, &MantidUI::handleAlgorithmFinishedNotification),
m_finishedLoadDAEObserver(*this, &MantidUI::handleLoadDAEFinishedNotification),
m_progressObserver(*this, &MantidUI::handleAlgorithmProgressNotification),
m_errorObserver(*this, &MantidUI::handleAlgorithmErrorNotification),
m_addObserver(*this,&MantidUI::handleAddWorkspace),
m_replaceObserver(*this,&MantidUI::handleReplaceWorkspace),
m_deleteObserver(*this,&MantidUI::handleDeleteWorkspace)
{ 
    m_progressDialog = 0;
    m_algAsync = 0;
    m_exploreMantid = new MantidDockWidget(this,aw);
    m_exploreAlgorithms = new AlgorithmDockWidget(this,aw);

  	actionCopyRowToTable = new QAction(tr("Copy spectra to Table"), this);
	actionCopyRowToTable->setIcon(QIcon(QPixmap(table_xpm)));
	connect(actionCopyRowToTable, SIGNAL(activated()), this, SLOT(copyRowToTable()));

  	actionCopyRowToGraph = new QAction(tr("Plot spectra (values only)"), this);
	actionCopyRowToGraph->setIcon(QIcon(QPixmap(graph_xpm)));
	connect(actionCopyRowToGraph, SIGNAL(activated()), this, SLOT(copyRowToGraph()));

  	actionCopyRowToGraphErr = new QAction(tr("Plot spectra (values + errors)"), this);
	actionCopyRowToGraphErr->setIcon(QIcon(QPixmap(graph_xpm)));
	connect(actionCopyRowToGraphErr, SIGNAL(activated()), this, SLOT(copyRowToGraphErr()));

    actionCopyDetectorsToTable = new QAction(tr("Copy Detectors to Table"), this);
	actionCopyDetectorsToTable->setIcon(QIcon(QPixmap(table_xpm)));
	connect(actionCopyDetectorsToTable, SIGNAL(activated()), this, SLOT(copyDetectorsToTable()));

  	actionCopyValues = new QAction(tr("Copy"), this);
	actionCopyValues->setIcon(QIcon(QPixmap(copy_xpm)));
	connect(actionCopyValues, SIGNAL(activated()), this, SLOT(copyValues()));

  	actionCopyColumnToTable = new QAction(tr("Copy time bin to Table"), this);
	actionCopyColumnToTable->setIcon(QIcon(QPixmap(table_xpm)));
	connect(actionCopyColumnToTable, SIGNAL(activated()), this, SLOT(copyColumnToTable()));

  	actionCopyColumnToGraph = new QAction(tr("Plot time bin (values only)"), this);
	actionCopyColumnToGraph->setIcon(QIcon(QPixmap(graph_xpm)));
	connect(actionCopyColumnToGraph, SIGNAL(activated()), this, SLOT(copyColumnToGraph()));

  	actionCopyColumnToGraphErr = new QAction(tr("Plot time bin (values + errors)"), this);
	actionCopyColumnToGraphErr->setIcon(QIcon(QPixmap(graph_xpm)));
	connect(actionCopyColumnToGraphErr, SIGNAL(activated()), this, SLOT(copyColumnToGraphErr()));

    connect(this,SIGNAL(needsUpdating()),this,SLOT(update()));
    connect(this,SIGNAL(needToCloseProgressDialog()),this,SLOT(closeProgressDialog()));
    connect(this,SIGNAL(needToUpdateProgressDialog(int,const QString&)),this,SLOT(updateProgressDialog(int,const QString&)));
    connect(this,SIGNAL(needToCreateLoadDAEMantidMatrix(const Mantid::API::Algorithm*)),this,SLOT(createLoadDAEMantidMatrix(const Mantid::API::Algorithm*)));
    connect(this,SIGNAL(needToShowCritical(const QString&)),this,SLOT(showCritical(const QString&)));

    m_algMonitor = new AlgorithmMonitor(this);
    connect(m_algMonitor,SIGNAL(countChanged(int)),m_exploreAlgorithms,SLOT(countChanged(int)), Qt::QueuedConnection);
    m_algMonitor->start();

    Mantid::API::AnalysisDataService::Instance().notificationCenter.addObserver(m_addObserver);
    Mantid::API::AnalysisDataService::Instance().notificationCenter.addObserver(m_replaceObserver);
    Mantid::API::AnalysisDataService::Instance().notificationCenter.addObserver(m_deleteObserver);

	mantidMenu = new QMenu(m_appWindow);
	mantidMenu->setObjectName("mantidMenu");
	connect(mantidMenu, SIGNAL(aboutToShow()), this, SLOT(mantidMenuAboutToShow()));

    menuMantidMatrix = new QMenu(m_appWindow);
	connect(menuMantidMatrix, SIGNAL(aboutToShow()), this, SLOT(menuMantidMatrixAboutToShow()));

}

// Should it be moved to the constructor?
void MantidUI::init()
{
    FrameworkManager::Instance();
    MantidLog::connect(this);
 
//    InputHistory::Instance();

	actionToggleMantid = m_exploreMantid->toggleViewAction();
	actionToggleMantid->setIcon(QPixmap(mantid_matrix_xpm));
	actionToggleMantid->setShortcut( tr("Ctrl+Shift+M") );
    appWindow()->view->addAction(actionToggleMantid);

	actionToggleAlgorithms = m_exploreAlgorithms->toggleViewAction();
	//actionToggleAlgorithms->setIcon(QPixmap(mantid_matrix_xpm));
	actionToggleAlgorithms->setShortcut( tr("Ctrl+Shift+A") );
    appWindow()->view->addAction(actionToggleAlgorithms);

    //LoadIsisRawFile("C:/Mantid/Test/Data/MAR11060.RAW","MAR11060","0","10");
    update();

}

MantidUI::~MantidUI()
{
//  InputHistory::Instance().save();
 MantidQt::API::AlgorithmInputHistory::Instance().save();
  if( m_algMonitor ) delete m_algMonitor;

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
IAlgorithm* MantidUI::CreateAlgorithm(const QString& algName)
{
	IAlgorithm* alg = FrameworkManager::Instance().createAlgorithm(algName.toStdString());

	return alg;
}
/**
     LoadIsisRawFile. Loads a raw file by executing LoadRaw algorithm asynchronously.

     @param fileName The name of the RAW file
     @param workspaceName The name of the workspace
     @param spectrum_min Lowest spectrum to load
     @param spectrum_max Highest spectrum to load
     @param spectrum_list Comma-separated list of spectra to load
     @param cache Three-valued parameter defining when to cache the raw file if it is to be loaded into a ManagedRawFileWorkspace2D.
                  Possible values are "If slow", "Always", "Never". The default is "If slow".
*/
void MantidUI::LoadIsisRawFile(const QString& fileName,const QString& workspaceName,const QString& spectrum_min,const QString& spectrum_max
                               ,const QString& spectrum_list,const QString& cache)
{
	//Check workspace does not exist
	if (AnalysisDataService::Instance().doesExist(workspaceName.toStdString()))
	{
        if ( QMessageBox::question(appWindow(),"MantidPlot - Confirm","Workspace "+workspaceName+" already exists. Do you want to replace it?",
            QMessageBox::Yes, QMessageBox::No) != QMessageBox::Yes) return;
    }
    QStringList ALGS = getAlgorithmNames();
    QString loader = ALGS.contains("LoadRaw2|1")?"LoadRaw2":"LoadRaw";
	Algorithm* alg = static_cast<Algorithm*>(CreateAlgorithm(loader));
	alg->setPropertyValue("Filename", fileName.toStdString());
	alg->setPropertyValue("OutputWorkspace", workspaceName.toStdString());
    if ( !spectrum_min.isEmpty() && !spectrum_max.isEmpty() )
    {
	    alg->setPropertyValue("spectrum_min", spectrum_min.toStdString());
	    alg->setPropertyValue("spectrum_max", spectrum_max.toStdString());
    }
    if ( !spectrum_list.isEmpty() )
	    alg->setPropertyValue("spectrum_list", spectrum_list.toStdString());
    if ( !cache.isEmpty() )
	    alg->setPropertyValue("Cache", cache.toStdString());

    executeAlgorithmAsync(alg);
}

/**
     loadWorkspace.

     Loads a workspace from a raw file. The LoadRaw's properties are set in loadRawDlg dialog.
*/
void MantidUI::loadWorkspace()
{
    
	// loadRawDlg* dlg = new loadRawDlg(m_appWindow);
	// dlg->setModal(true);	
	// dlg->exec();
	
	// if (!dlg->getFilename().isEmpty())
	// {	
		
		// LoadIsisRawFile(dlg->getFilename(), 
                        // dlg->getWorkspaceName(),
                        // dlg->getSpectrumMin(),
                        // dlg->getSpectrumMax(),
                        // dlg->getSpectrumList(),
                        // dlg->getCacheOption());
	// }

  //Just use the generic executeAlgorithm method which now uses specialised dialogs if they are
  //available
  executeAlgorithm("LoadRaw", -1);
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
        Mantid::API::Algorithm* alg = dynamic_cast<Mantid::API::Algorithm*>(CreateAlgorithm("LoadDAE"));
	    alg->setPropertyValue("DAEname", dlg->getHostName().toStdString());
	    alg->setPropertyValue("OutputWorkspace", dlg->getWorkspaceName().toStdString());
        if ( !dlg->getSpectrumMin().isEmpty() && !dlg->getSpectrumMax().isEmpty() )
        {
	        alg->setPropertyValue("spectrum_min", dlg->getSpectrumMin().toStdString());
	        alg->setPropertyValue("spectrum_max", dlg->getSpectrumMax().toStdString());
        }
        if ( !dlg->getSpectrumList().isEmpty() )
        {
	        alg->setPropertyValue("spectrum_list", dlg->getSpectrumList().toStdString());
     }

        DAEstruct dae;
        dae.m_WorkspaceName = dlg->getWorkspaceName();
        dae.m_HostName = dlg->getHostName();
        dae.m_SpectrumMin = dlg->getSpectrumMin();
        dae.m_SpectrumMax = dlg->getSpectrumMax();
        dae.m_SpectrumList = dlg->getSpectrumList();
        dae.m_UpdateInterval = dlg->updateInterval();
        m_DAE_map[alg] = dae;
	    
        alg->notificationCenter.addObserver(m_finishedLoadDAEObserver);
        executeAlgorithmAsync(alg);
  }
    // Mantid::API::Algorithm *alg = dynamic_cast<Mantid::API::Algorithm*>
          // (Mantid::API::FrameworkManager::Instance().createAlgorithm("LoadDAE",-1));
    // if( !alg ) return;
    
    // alg->notificationCenter.addObserver(m_finishedLoadDAEObserver);
    // MantidQt::API::AlgorithmDialog *dlg = MantidQt::API::DialogManager::Instance().createDialog(alg, (QWidget*)parent());
    // if( !dlg ) return;
		// if ( dlg->exec() == QDialog::Accepted) executeAlgorithmAsync(alg);
}

/**
     deleteWorkspace

     @param workspaceName Name of the workspace to delete
*/
bool MantidUI::deleteWorkspace(const QString& workspaceName)
{
    bool ret = FrameworkManager::Instance().deleteWorkspace(workspaceName.toStdString());
    if (ret) update();
    return ret;
}

/**
     update

     Widgets showing workspaces and algorithms are updated.
*/
void MantidUI::update()
{
    m_exploreMantid->update();
    m_exploreAlgorithms->update();
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
      if( item->parent() )
      {
	item = item->parent();
      }
      str = item->text(0);
    }
    if( !str.isEmpty() ) return str;

    //Check if a mantid matrix is selected
    MantidMatrix *m = qobject_cast<MantidMatrix*>(appWindow()->activeWindow());
    if( !m ) return "";
    
    return m->workspaceName();
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
    MatrixWorkspace_sptr output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(workspaceName.toStdString()));
	return output->getNumberHistograms();
}

int MantidUI::getBinNumber(const QString& workspaceName)
{
	MatrixWorkspace_sptr output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(workspaceName.toStdString()));
	return output->blocksize();
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

MantidMatrix* MantidUI::importWorkspace(const QString& wsName, bool showDlg, bool makeVisible)
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
        w = new MantidMatrix(ws, appWindow(), "Mantid",wsName, -1, -1 );
    }
    if (!w) return 0;

    connect(w, SIGNAL(closedWindow(MdiSubWindow*)), appWindow(), SLOT(closeWindow(MdiSubWindow*)));
    connect(w,SIGNAL(hiddenWindow(MdiSubWindow*)),appWindow(), SLOT(hideWindow(MdiSubWindow*)));
    connect (w,SIGNAL(showContextMenu()),appWindow(),SLOT(showWindowContextMenu()));

    appWindow()->d_workspace->addSubWindow(w);
    if( makeVisible ) w->showNormal(); 
    else w->showMinimized();
    return w;
}

void MantidUI::importWorkspace()
{
    QString wsName = getSelectedWorkspaceName();
    importWorkspace(wsName);
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
    createTableFromSelectedRows(m,true,true);
}

void MantidUI::copyColumnToTable()
{
    MantidMatrix* m = (MantidMatrix*)appWindow()->activeWindow();
    if (!m || !m->isA("MantidMatrix")) return;
    createTableFromSelectedColumns(m,true,true);
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
    createGraphFromSelectedColumns(m,false,false);
  
}

void MantidUI::copyColumnToGraphErr()
{
    MantidMatrix* m = (MantidMatrix*)appWindow()->activeWindow();
    if (!m || !m->isA("MantidMatrix")) return;
    createGraphFromSelectedColumns(m,false);
  
}

void MantidUI::copyRowToGraphErr()
{
    MantidMatrix* m = (MantidMatrix*)appWindow()->activeWindow();
    if (!m || !m->isA("MantidMatrix")) return;
    createGraphFromSelectedRows(m,false);
  
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

/*  Creates a Qtiplot Table from selected spectra of MantidMatrix m. 
    The columns are: 1st column is x-values from the first selected spectrum,
    2nd column is y-values of the first spectrum. Depending on value of errs
    the 3rd column contains either first spectrum errors (errs == true) or 
    y-values of the second spectrum (errs == false). Consecutive columns have
    y-values and errors (if errs is true) of the following spectra. If visible == true
    the table is made visible in Qtiplot.

    The name of a Y column is "Y"+QString::number(i), where i is the row in the MantidMatrix,
    not the spectrum index in the workspace.

*/
Table* MantidUI::createTableFromSelectedRows(MantidMatrix *m, bool visible, bool errs, bool forPlotting)
{
     int i0,i1; 
     m->getSelectedRows(i0,i1);
     if (i0 < 0 || i1 < 0) return 0;

     int c = errs?2:1;
     int numRows = m->numCols();
     if (m->isHistogram() && !forPlotting) numRows++;

	 Table* t = new Table(appWindow()->scriptEnv, numRows, c*(i1 - i0 + 1) + 1, "", appWindow(), 0);
	 appWindow()->initTable(t, appWindow()->generateUniqueName(m->name()+"-"));
     if (visible) 
     {
         t->showNormal();
         t->askOnCloseEvent(false);
     }
    
     int kY,kErr;
     for(int i=i0;i<=i1;i++)
     {
         kY = c*(i-i0)+1;
         t->setColName(kY,"YS"+QString::number(i));
         if (errs)
         {
             kErr = 2*(i - i0) + 2;
             t->setColPlotDesignation(kErr,Table::yErr);
             t->setColName(kErr,"ES"+QString::number(i));
         }
         for(int j=0;j<m->numCols();j++)
         {
             if (i == i0) 
             {
                 // in histograms the point is to be drawn in the centre of the bin.
                 if (m->isHistogram() && forPlotting) t->setCell(j,0,( m->dataX(i,j) + m->dataX(i,j+1) )/2);
                 else
                     t->setCell(j,0,m->dataX(i,j));
             }
             t->setCell(j,kY,m->dataY(i,j)); 
             if (errs) t->setCell(j,kErr,m->dataE(i,j));
         }
         if (m->isHistogram() && !forPlotting)
         {
             int iRow = numRows - 1;
             if (i == i0) t->setCell(iRow,0,m->dataX(i,iRow));
             t->setCell(iRow,kY,0); 
             if (errs) t->setCell(iRow,kErr,0);
         }
     }
     return t;
 }

Table* MantidUI::createTableFromSelectedColumns(MantidMatrix *m, bool visible, bool errs)
{
     int i0,i1; 
     m->getSelectedColumns(i0,i1);
     if (i0 < 0 || i1 < 0) return 0;

     int c = errs?2:1;
     int numRows = m->numRows();
     Table* t = new Table(appWindow()->scriptEnv, numRows, c*(i1 - i0 + 1) + 1, "", appWindow(), 0);
     appWindow()->initTable(t, appWindow()->generateUniqueName(m->name()+"-"));
     if (visible) 
     {
         t->showNormal();
         t->askOnCloseEvent(false);
     }
    
     int kY,kErr;
     for(int i=i0;i<=i1;i++)
     {
         kY = c*(i-i0)+1;
         t->setColName(kY,"YB"+QString::number(i));
         if (errs)
         {
             kErr = 2*(i - i0) + 2;
             t->setColPlotDesignation(kErr,Table::yErr);
             t->setColName(kErr,"EB"+QString::number(i));
         }
         for(int j=0;j<numRows;j++)
         {
             if (i == i0) 
             {
                 //t->setCell(j,0,m->dataX(j,i));//?
                 t->setCell(j,0,j);
             }
             t->setCell(j,kY,m->dataY(j,i)); 
             if (errs) t->setCell(j,kErr,m->dataE(j,i));
         }
     }
     return t;
 }

void MantidUI::createGraphFromSelectedRows(MantidMatrix *m, bool visible, bool errs)
{
    Table *t = createTableFromSelectedRows(m,visible,errs,true);
    if (!t) return;
    t->askOnCloseEvent(false);
    t->showNormal();
    MultiLayer* ml = appWindow()->multilayerPlot(t,t->colNames(),Graph::Line);
    Graph *g = ml->activeGraph();
    appWindow()->polishGraph(g,Graph::Line);
    m->setGraph1D(ml,t);
    ml->askOnCloseEvent(false);
}

void MantidUI::createGraphFromSelectedColumns(MantidMatrix *m, bool visible, bool errs)
{
    Table *t = createTableFromSelectedColumns(m,visible,errs);
    t->askOnCloseEvent(false);
    if (!t) return;
    t->showNormal();
    MultiLayer* ml = appWindow()->multilayerPlot(t,t->colNames(),Graph::Line);
    Graph *g = ml->activeGraph();
    appWindow()->polishGraph(g,Graph::Line);
    m->setGraph1D(ml,t);
    ml->askOnCloseEvent(false);
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

void MantidUI::executeAlgorithm(QString algName, int version)
{

    Mantid::API::Algorithm* alg;
    try
    {
        alg = dynamic_cast<Mantid::API::Algorithm*>
          (Mantid::API::FrameworkManager::Instance().createAlgorithm(algName.toStdString(),version));
    }
    catch(...)
    {
        QMessageBox::critical(appWindow(),"MantidPlot - Algorithm error","Cannot create algorithm "+algName+" version "+QString::number(version));
        return;
    }
	if (alg)
	{		
		// ExecuteAlgorithm* dlg = new ExecuteAlgorithm(appWindow());
		// dlg->CreateLayout(alg);
		// dlg->setModal(true);
    
    MantidQt::API::AlgorithmDialog *dlg = MantidQt::API::DialogManager::Instance().createDialog(alg, (QWidget*)parent());
    if( !dlg ) return;
		if ( dlg->exec() == QDialog::Accepted) executeAlgorithmAsync(alg);
	}
}

void MantidUI::executeAlgorithmAsync(Mantid::API::Algorithm* alg, bool showDialog)
{
    if (showDialog)
    {
        m_progressDialog = new ProgressDlg(appWindow());
        //m_progressDialog->setWindowModality(Qt::WindowModal);
        connect(m_progressDialog,SIGNAL(canceled()),this,SLOT(cancelAsyncAlgorithm()));
        connect(m_progressDialog,SIGNAL(toBackground()),this,SLOT(backgroundAsyncAlgorithm()));

	//M.Gigg - Not sure why this is necessary. It seems to have the effect (on Linux anway)
	//of trying to close the same dialog twice
	//        connect(m_progressDialog,SIGNAL(rejected()),this,SLOT(backgroundAsyncAlgorithm()));

        alg->notificationCenter.addObserver(m_progressObserver);
    }
    alg->notificationCenter.addObserver(m_finishedObserver);
    alg->notificationCenter.addObserver(m_errorObserver);
    m_algAsync = alg;
    m_algMonitor->add(alg);
    try
    {
      Poco::ActiveResult<bool> res = alg->executeAsync();
      if ( !res.tryWait(100) && showDialog)
      {
	//Use show rather than exec so that control is returned to the caller immediately
	m_progressDialog->exec();
      }

//        InputHistory::Instance().updateAlgorithm(alg);
    }
    catch(...)
    {
        QMessageBox::critical(appWindow(),"MantidPlot - Algorithm error","Exception is caught");
    }
}

void MantidUI::cancelAsyncAlgorithm()
{
    if (m_algAsync)
    {
        m_algAsync->cancel();
    }
    m_algAsync = 0;
    closeProgressDialog();
}

void MantidUI::backgroundAsyncAlgorithm()
{
    closeProgressDialog();
    m_algAsync = 0;
}

void MantidUI::tst()
{
    QWidget* m = (QWidget*)appWindow()->activeWindow();
    if (!m) return;
    if (m->isA("MantidMatrix"))
        ((MantidMatrix*)m)->tst(); 

}

void MantidUI::tst(MdiSubWindow* w)
{
    cerr<<"OK closed\n";
}

void MantidUI::updateProgressDialog(int ip,const QString& msg)
{
    if (m_progressDialog) m_progressDialog->setValue( ip , msg );
}

void MantidUI::closeProgressDialog()
{
    if (m_progressDialog) 
    {
        m_progressDialog->close();
        m_progressDialog = 0;
    }
}

void MantidUI::handleAlgorithmFinishedNotification(const Poco::AutoPtr<Mantid::API::Algorithm::FinishedNotification>& pNf)
{
    if (pNf->algorithm() == m_algAsync) m_algAsync = 0;
    emit needToUpdateProgressDialog(100,"Algorithm finished.");
    emit needToCloseProgressDialog();
    emit needsUpdating();
}

void MantidUI::handleAlgorithmErrorNotification(const Poco::AutoPtr<Mantid::API::Algorithm::ErrorNotification>& pNf)
{

  if (pNf->algorithm() == m_algAsync) m_algAsync = 0;
  if (pNf->what != "Algorithm terminated")
    emit needToShowCritical("Error in algorithm: "+QString::fromStdString(pNf->what));

  //emit needToCloseProgressDialog();
  emit needsUpdating();
}

void MantidUI::handleLoadDAEFinishedNotification(const Poco::AutoPtr<Mantid::API::Algorithm::FinishedNotification>& pNf)
{
    emit needsUpdating();
    emit needToCreateLoadDAEMantidMatrix(pNf->algorithm());
}

void MantidUI::createLoadDAEMantidMatrix(const Mantid::API::Algorithm* alg)
{
    QMap<const Mantid::API::Algorithm*,DAEstruct>::iterator dae = m_DAE_map.find(alg);
    if (dae == m_DAE_map.end()) return;

    Workspace_sptr ws = AnalysisDataService::Instance().retrieve(dae->m_WorkspaceName.toStdString());

    if (ws.use_count() == 0)
    {
	    QMessageBox::warning(m_appWindow, tr("Mantid"),
           		    tr("A workspace with this name already exists.\n")
            		    , QMessageBox::Ok, QMessageBox::Ok);
	    return;
    }

    MantidMatrix *m = importWorkspace(dae->m_WorkspaceName,false, true);

    if (dae->m_UpdateInterval > 0)
    {
        Algorithm* updater = dynamic_cast<Algorithm*>(CreateAlgorithm("UpdateDAE"));
        updater->setPropertyValue("Workspace",dae->m_WorkspaceName.toStdString());
        updater->setPropertyValue("update_rate",QString::number(dae->m_UpdateInterval).toStdString());
        executeAlgorithmAsync(updater,false);
    }

    m_DAE_map.erase(dae);
}

void MantidUI::handleAlgorithmProgressNotification(const Poco::AutoPtr<Mantid::API::Algorithm::ProgressNotification>& pNf)
{
    if (m_algAsync == pNf->algorithm())
    {
        emit needToUpdateProgressDialog( int(pNf->progress*100) ,QString::fromStdString(pNf->message));
    }
}

void MantidUI::showCritical(const QString& text)
{
    QMessageBox::critical(appWindow(),"Mantid - Error",text);
    closeProgressDialog();
}

void MantidUI::showAlgMonitor()
{
    m_algMonitor->showDialog();
}

void MantidUI::handleAddWorkspace(WorkspaceAddNotification_ptr pNf)
{
    emit needsUpdating();
}

void MantidUI::handleReplaceWorkspace(WorkspaceAfterReplaceNotification_ptr pNf)
{
    emit needsUpdating();
}

void MantidUI::handleDeleteWorkspace(WorkspaceDeleteNotification_ptr pNf)
{
    emit needsUpdating();
}

void MantidUI::logMessage(const Poco::Message& msg)
{
    if (!appWindow()->results) return;
    QString str = msg.getText().c_str();
    //if (s_logEdit->document()->blockCount() > 1000) s_logEdit->document()->clear();
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
	bool ok;
	QString selectedName = QInputDialog::getItem(appWindow(),tr("Select Workspace"), tr("Please select your workspace"), wsNames, 0, false,&ok);
	if(ok) showMantidInstrument(selectedName);
}

void MantidUI::showMantidInstrumentSelected()
{
    QString wsName = getSelectedWorkspaceName();
    if (!wsName.isEmpty()) showMantidInstrument(wsName);
}

void MantidUI::mantidMenuAboutToShow()
{
	mantidMenu->clear();
	
	mantidMenu->insertItem(tr("&Manage Workspaces"), this, SLOT(manageMantidWorkspaces() ) );
	mantidMenu->insertItem(tr("&Instrument Window"), this, SLOT(showMantidInstrument() ) );
}

void MantidUI::insertMenu()
{
	appWindow()->menuBar()->insertItem(tr("Man&tid"), mantidMenu);}

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
    Mantid::API::MatrixWorkspace_sptr workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName.toStdString()));
    Table *t = createTableFromSelectedRows(wsName,workspace,spec,spec,false,false);
    t->askOnCloseEvent(false);
    MultiLayer* ml(NULL);
    if (!t) return ml;

    ml = appWindow()->multilayerPlot(t,t->colNames(),Graph::Line);
    ml->askOnCloseEvent(false);
    ml->setAttribute(Qt::WA_QuitOnClose);
    Graph* g = ml->activeGraph();
    appWindow()->polishGraph(g,Graph::Line);
    g->setTitle(tr("Workspace ")+name());
    Mantid::API::Axis* ax;
    ax = workspace->getAxis(0);
    std::string s;
    if (ax->unit().get()) s = ax->unit()->caption() + " / " + ax->unit()->label();
    else
        s = "X axis";
    g->setXAxisTitle(tr(s.c_str()));
    g->setYAxisTitle(tr(workspace->YUnit().c_str()));
    return ml;
}

/// Catches the signal from InstrumentWindow to plot a spectrum.
MultiLayer* MantidUI::plotInstrumentSpectrumList(const QString& wsName, std::vector<int> spec)
{
//    QMessageBox::information(appWindow(),"OK",wsName+" "+QString::number(spec));
    Mantid::API::MatrixWorkspace_sptr workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName.toStdString()));
    Table *t = createTableFromSelectedRowsList(wsName,workspace,spec,false,false);
    t->askOnCloseEvent(false);
    t->setAttribute(Qt::WA_QuitOnClose);
    MultiLayer* ml=NULL;
    if (!t) return ml;

    ml = appWindow()->multilayerPlot(t,t->colNames(),Graph::Line);
    ml->askOnCloseEvent(false);
    ml->setAttribute(Qt::WA_QuitOnClose);
    Graph* g = ml->activeGraph();
    appWindow()->polishGraph(g,Graph::Line);
    g->setTitle(tr("Workspace ")+name());
    Mantid::API::Axis* ax;
    ax = workspace->getAxis(0);
    std::string s;
    if (ax->unit().get()) s = ax->unit()->caption() + " / " + ax->unit()->label();
    else
        s = "X axis";
    g->setXAxisTitle(tr(s.c_str()));
    g->setYAxisTitle(tr(workspace->YUnit().c_str()));
    return ml;
}

Table* MantidUI::createTableFromSelectedRowsList(const QString& wsName, Mantid::API::MatrixWorkspace_sptr workspace, std::vector<int> index, bool errs, bool forPlotting)
{
     int nspec = workspace->getNumberHistograms();
	 //Loop through the list of index and remove all the indexes that are out of range
	 for(std::vector<int>::iterator it=index.begin();it!=index.end();it++)
	 {
		if ((*it) > nspec || (*it) < 0) index.erase(it);
	 }
     if (index.size()==0) return 0;

     int c = errs?2:1;
     int numRows = workspace->blocksize() - 1;
     bool isHistogram = workspace->isHistogramData();
     if (isHistogram && !forPlotting) numRows++;

	 Table* t = new Table(appWindow()->scriptEnv, numRows, c*index.size() + 1, "", appWindow(), 0);
	 appWindow()->initTable(t, appWindow()->generateUniqueName(wsName+"-"));
	 t->askOnCloseEvent(false);

     int kY,kErr;
     for(int i=0;i<index.size();i++)
     {
         const std::vector<double>& dataX = workspace->readX(index[i]);
		 const std::vector<double>& dataY = workspace->readY(index[i]);
         const std::vector<double>& dataE = workspace->readE(index[i]);
    
         kY = c*i+1;
         t->setColName(kY,"YS"+QString::number(index[i]));
         if (errs)
         {
             kErr = 2*i + 2;
             t->setColPlotDesignation(kErr,Table::yErr);
             t->setColName(kErr,"ES"+QString::number(index[i]));
         }
         for(int j=0;j<numRows;j++)
         {
             if (i == 0) 
             {
                 // in histograms the point is to be drawn in the centre of the bin.
                 if (isHistogram && forPlotting) t->setCell(j,0,( dataX[j] + dataX[j+1] )/2);
                 else
                     t->setCell(j,0,dataX[j]);
             }
             t->setCell(j,kY,dataY[j]); 
             if (errs) t->setCell(j,kErr,dataE[j]);
         }
         if (isHistogram && !forPlotting)
         {
             int iRow = numRows - 1;
             if (i == 0) t->setCell(iRow,0,dataX[iRow]);
             t->setCell(iRow,kY,0); 
             if (errs) t->setCell(iRow,kErr,0);
         }
     }
     return t;
 }

MultiLayer* MantidUI::plotTimeBin(const QString& wsName, int bin, bool showMatrix)
{
  MantidMatrix* m = getMantidMatrix(wsName);
  if( !m )
  {
    m = importWorkspace(wsName, false, showMatrix);
  }
  MatrixWorkspace_sptr ws;
  if (AnalysisDataService::Instance().doesExist(wsName.toStdString()))
  {
    ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName.toStdString()));
  }
  if( !ws.get() ) return NULL;
  
  Table *t = createTableFromSelectedColumns(wsName, ws, bin, bin, false, false);
  t->askOnCloseEvent(false);
  t->setAttribute(Qt::WA_QuitOnClose);
  MultiLayer* ml(NULL);
  if( !t ) return ml;
  
  ml = appWindow()->multilayerPlot(t,t->colNames(),Graph::Line);
  Graph *g = ml->activeGraph();
  appWindow()->polishGraph(g,Graph::Line);
  m->setGraph1D(ml,t);
  ml->askOnCloseEvent(false);
  return ml;
}

Table* MantidUI::createTableFromSelectedRows(const QString& wsName, Mantid::API::MatrixWorkspace_sptr workspace, int i0, int i1, bool errs, bool forPlotting)
{
     if (i0 < 0 || i1 < 0) return 0;
     int nspec = workspace->getNumberHistograms();
     if (i0 > nspec || i1 > nspec) return 0;

     int c = errs?2:1;
     int numRows = workspace->blocksize() - 1;
     bool isHistogram = workspace->isHistogramData();
     if (isHistogram && !forPlotting) numRows++;

	 Table* t = new Table(appWindow()->scriptEnv, numRows, c*(i1 - i0 + 1) + 1, "", appWindow(), 0);
	 appWindow()->initTable(t, appWindow()->generateUniqueName(wsName+"-"));
	 t->askOnCloseEvent(false);

     int kY,kErr;
     for(int i=i0;i<=i1;i++)
     {
         const std::vector<double>& dataX = workspace->readX(i);//index(i)
         const std::vector<double>& dataY = workspace->readY(i);
         const std::vector<double>& dataE = workspace->readE(i);
    
         kY = c*(i-i0)+1;
         t->setColName(kY,"YS"+QString::number(i));//index(i)
         if (errs)
         {
             kErr = 2*(i - i0) + 2;
             t->setColPlotDesignation(kErr,Table::yErr);
             t->setColName(kErr,"ES"+QString::number(i));//index(i)
         }
         for(int j=0;j<numRows;j++)
         {
             if (i == i0) 
             {
                 // in histograms the point is to be drawn in the centre of the bin.
                 if (isHistogram && forPlotting) t->setCell(j,0,( dataX[j] + dataX[j+1] )/2);
                 else
                     t->setCell(j,0,dataX[j]);
             }
             t->setCell(j,kY,dataY[j]); 
             if (errs) t->setCell(j,kErr,dataE[j]);
         }
         if (isHistogram && !forPlotting)
         {
             int iRow = numRows - 1;
             if (i == i0) t->setCell(iRow,0,dataX[iRow]);
             t->setCell(iRow,kY,0); 
             if (errs) t->setCell(iRow,kErr,0);
         }
     }
     return t;
 }

Table* MantidUI::createTableFromSelectedColumns(const QString& wsName, Mantid::API::MatrixWorkspace_sptr workspace, int c0, int c1, bool errs, bool forPlotting)
{
  if (c0 < 0 || c1 < 0) return NULL;

  int c = errs?2:1;
  int numRows = workspace->getNumberHistograms();

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
    for(int j = 0; j < numRows; j++)
      {
	const std::vector<double>& dataY = workspace->readY(j);
	const std::vector<double>& dataE = workspace->readE(j);

	if (i == c0) 
	{
	    t->setCell(j,0,j);
	}
	t->setCell(j,kY,dataY[i]); 
	if (errs) t->setCell(j,kErr,dataE[i]);
      }
  }
  return t;
}

//-------------------------------------------------
// The following commands are purely for the Python
// interface
//-------------------------------------------------

// In Python scripts we don't click and import a matrix so that we can plot
// spectra from it so this command does an import of a workspace and then plots
// the requested spectrum
MultiLayer* MantidUI::plotSpectrum(const QString& wsName, int spec, bool showMatrix)
{
  MantidMatrix* m = getMantidMatrix(wsName);
  if( !m )
  {
    m = importWorkspace(wsName, false, showMatrix);
  }
  MatrixWorkspace_sptr ws;
  if (AnalysisDataService::Instance().doesExist(wsName.toStdString()))
  {
    ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName.toStdString()));
  }
  if (!ws.get()) return NULL;
  
  Table *t = createTableFromSelectedRows(wsName, ws, spec, spec, false, false);
  MultiLayer* ml(NULL);
  if (!t) return ml;

  ml = appWindow()->multilayerPlot(t,t->colNames(),Graph::Line);
  Graph *g = ml->activeGraph();
  appWindow()->polishGraph(g,Graph::Line);
  m->setGraph1D(ml,t);
  ml->askOnCloseEvent(false);
  t->showMinimized();
  return ml;
}

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
  MatrixWorkspace_sptr ws;
  if (AnalysisDataService::Instance().doesExist(wsName.toStdString()))
    {
        ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName.toStdString()));
    }
  
  if (!ws.get()) return 0;

  MantidMatrix* w = new MantidMatrix(ws, appWindow(), "Mantid",wsName, start, end );
  if (!w) return 0;
  
  connect(w, SIGNAL(closedWindow(MdiSubWindow*)), appWindow(), SLOT(closeWindow(MdiSubWindow*)));
  connect(w,SIGNAL(hiddenWindow(MdiSubWindow*)),appWindow(), SLOT(hideWindow(MdiSubWindow*)));
  connect (w,SIGNAL(showContextMenu()),appWindow(),SLOT(showWindowContextMenu()));
  
  appWindow()->d_workspace->addSubWindow(w);
  w->showNormal(); 
  return w;
}

/**
 * This is for the scripting side of things.
 */
bool MantidUI::runAlgorithmAsynchronously(const QString & algName)
{
  Mantid::API::Algorithm *alg = findAlgorithmPointer(algName);
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

bool MantidUI::createPropertyInputDialog(const QString & algName, const QString & message)
{
  Mantid::API::Algorithm *alg = findAlgorithmPointer(algName);
  if( !alg ) 
  {
    return false;
  }

  MantidQt::API::AlgorithmDialog *dlg = MantidQt::API::DialogManager::Instance().createDialog(alg, 0, true, message);
  return (dlg->exec() == QDialog::Accepted);
}

Mantid::API::Algorithm* MantidUI::findAlgorithmPointer(const QString & algName)
{
  const vector<Mantid::API::Algorithm_sptr> & algorithms = Mantid::API::AlgorithmManager::Instance().algorithms();
  Mantid::API::Algorithm* alg(NULL);
  vector<Mantid::API::Algorithm_sptr>::const_reverse_iterator aEnd = algorithms.rend();
  for(  vector<Mantid::API::Algorithm_sptr>::const_reverse_iterator aIter = algorithms.rbegin() ; 
	aIter != aEnd; ++aIter )
  {
    if( !(*aIter)->isExecuted() && (*aIter)->name() == algName.toStdString()  )
    {
      alg = (*aIter).get();
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
    else t->setText(row, 1, ts[2]);
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
  g->setXAxisTitle(t->colLabel(0));
  g->setYAxisTitle(t->colLabel(1).section(".",0,0));
  g->setTitle(label.section("-",0, 0));
 
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


//----------------------------------------------------------------------------------//
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
