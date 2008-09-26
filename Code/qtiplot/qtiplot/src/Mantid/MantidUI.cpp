#include "MantidUI.h"
#include "MantidMatrix.h"
#include "MantidDock.h"
#include "LoadRawDlg.h"
#include "LoadDAEDlg.h"
#include "ImportWorkspaceDlg.h"
#include "ExecuteAlgorithm.h"
#include "AlgMonitor.h"
#include "../Spectrogram.h"
#include "../pixmaps.h"

#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidPlotReleaseDate.h"

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
    mantidUI->aw_results = results;

    mantidUI->init();
}


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

  	actionCopyRowToTable = new QAction(tr("Copy to Table"), this);
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

    connect(this,SIGNAL(needsUpdating()),this,SLOT(update()));
    connect(this,SIGNAL(needToCloseProgressDialog()),this,SLOT(closeProgressDialog()));
    connect(this,SIGNAL(needToUpdateProgressDialog(int)),this,SLOT(updateProgressDialog(int)));
    connect(this,SIGNAL(needToCreateLoadDAEMantidMatrix(const Mantid::API::Algorithm*)),this,SLOT(createLoadDAEMantidMatrix(const Mantid::API::Algorithm*)));
    connect(this,SIGNAL(needToShowCritical(const QString&)),this,SLOT(showCritical(const QString&)));

    m_algMonitor = new AlgorithmMonitor(this);
    connect(m_algMonitor,SIGNAL(countChanged(int)),m_exploreAlgorithms,SLOT(countChanged(int)));

    Mantid::API::AnalysisDataService::Instance().notificationCenter.addObserver(m_addObserver);
    Mantid::API::AnalysisDataService::Instance().notificationCenter.addObserver(m_replaceObserver);
    Mantid::API::AnalysisDataService::Instance().notificationCenter.addObserver(m_deleteObserver);
}

void MantidUI::init()
{
    FrameworkManager::Instance();
    MantidLog::connect(this);

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

MantidUI::~MantidUI()
{
    //delete m_algMonitor;
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



IAlgorithm* MantidUI::CreateAlgorithm(const QString& algName)
{
	IAlgorithm* alg = FrameworkManager::Instance().createAlgorithm(algName.toStdString());

	return alg;
}

void MantidUI::LoadIsisRawFile(const QString& fileName,const QString& workspaceName,const QString& spectrum_min,const QString& spectrum_max)
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

    executeAlgorithmAsync(alg);
}

void MantidUI::loadWorkspace()
{
    
	loadRawDlg* dlg = new loadRawDlg(m_appWindow);
	dlg->setModal(true);	
	dlg->exec();
	
	if (!dlg->getFilename().isEmpty())
	{	
		
		LoadIsisRawFile(dlg->getFilename(), 
                        dlg->getWorkspaceName(),
                        dlg->getSpectrumMin(),
                        dlg->getSpectrumMax());
	}
}

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

MantidMatrix* MantidUI::importWorkspace(const QString& wsName, bool showDlg)
{
    Workspace_sptr ws;
  	if (AnalysisDataService::Instance().doesExist(wsName.toStdString()))
	{
		ws = AnalysisDataService::Instance().retrieve(wsName.toStdString());
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
    		
	        w = new MantidMatrix(ws, appWindow(), "Mantid",wsName, start, end,dlg->isFiltered(),dlg->getMaxValue() );
	    }
    }
    else
    {
        w = new MantidMatrix(ws, appWindow(), "Mantid",wsName, -1, -1,false,0 );
    }
    if (!w) return 0;

    connect(w, SIGNAL(closedWindow(MdiSubWindow*)), appWindow(), SLOT(closeWindow(MdiSubWindow*)));
    connect(w,SIGNAL(hiddenWindow(MdiSubWindow*)),appWindow(), SLOT(hideWindow(MdiSubWindow*)));
    connect (w,SIGNAL(showContextMenu()),appWindow(),SLOT(showWindowContextMenu()));

    d_workspace->addSubWindow(w);
    w->showNormal(); 
    return w;
}

void MantidUI::importWorkspace()
{
    QString wsName = getSelectedWorkspaceName();
    importWorkspace(wsName);
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
        cm.addAction(actionCopyDetectorsToTable);
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

void MantidUI::copyDetectorsToTable()
{
    MantidMatrix* m = (MantidMatrix*)appWindow()->activeWindow();
    if (!m || !m->isA("MantidMatrix")) return;
    createTableDetectors(m);
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

	 Table* t = new Table(aw_scriptEnv, numRows, c*(i1 - i0 + 1) + 1, "", appWindow(), 0);
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
             t->setColName(kErr,"E"+QString::number(i));
         }
         for(int j=0;j<m->numCols();j++)
         {
             if (i == i0) 
             {
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

void MantidUI::createGraphFromSelectedRows(MantidMatrix *m, bool visible, bool errs)
{
    Table *t = createTableFromSelectedRows(m,visible,errs,true);
    if (!t) return;

    QStringList cn;
    cn<<t->colName(1);
    if (errs) cn<<t->colName(2);
    MultiLayer* ml = appWindow()->multilayerPlot(t,t->colNames(),Graph::Line);
    Graph *g = ml->activeGraph();
    appWindow()->polishGraph(g,Graph::Line);
    m->setGraph1D(ml,t);
}

Table* MantidUI::createTableDetectors(MantidMatrix *m)
{
	 Table* t = new Table(aw_scriptEnv, m->numRows(), 6, "", appWindow(), 0);
	 appWindow()->initTable(t, appWindow()->generateUniqueName(m->name()+"-Detectors-"));
     t->showNormal();
    
     Mantid::API::Workspace_sptr ws = m->workspace();
     Mantid::API::Axis *spectraAxis = ws->getAxis(1);
     boost::shared_ptr<Mantid::API::SpectraDetectorMap> spectraMap = ws->getSpectraMap();
     for(int i=0;i<m->numRows();i++)
     {
         
         int ws_index = m->workspaceIndex(i);
         int currentSpec = spectraAxis->spectraNo(ws_index);
         Mantid::Geometry::V3D pos;
         int detID = 0;
         double R = 0.;
         double Theta = 0.;
         double Phi = 0.;
         try
         {
             boost::shared_ptr<Mantid::Geometry::IDetector> det = spectraMap->getDetector(currentSpec);
             detID = det->getID();
             det->getPos().getSpherical(R,Theta,Phi);
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
        importWorkspace();
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
		ExecuteAlgorithm* dlg = new ExecuteAlgorithm(appWindow());
		dlg->CreateLayout(alg);
		dlg->setModal(true);
	
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
        connect(m_progressDialog,SIGNAL(rejected()),this,SLOT(backgroundAsyncAlgorithm()));
        alg->notificationCenter.addObserver(m_progressObserver);
    }
    alg->notificationCenter.addObserver(m_finishedObserver);
    alg->notificationCenter.addObserver(m_errorObserver);
    m_algAsync = alg;
    m_algMonitor->add(alg);
    try
    {
        alg->executeAsync();
        if (showDialog)
            m_progressDialog->exec();
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

void MantidUI::updateProgressDialog(int ip)
{
    if (m_progressDialog) m_progressDialog->setValue( ip );
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
    emit needToUpdateProgressDialog(100);
    emit needToCloseProgressDialog();
    emit needsUpdating();
}

void MantidUI::handleAlgorithmErrorNotification(const Poco::AutoPtr<Mantid::API::Algorithm::ErrorNotification>& pNf)
{
    if (pNf->algorithm() == m_algAsync) m_algAsync = 0;
    emit needToCloseProgressDialog();
    emit needsUpdating();
    if (pNf->what != "Algorithm terminated")
        emit needToShowCritical("Error in algorithm: "+QString::fromStdString(pNf->what));
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

    MantidMatrix *m = importWorkspace(dae->m_WorkspaceName,false);

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
        emit needToUpdateProgressDialog( int(pNf->progress*100) );
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
    emit needsUpdating();
}

void MantidUI::handleReplaceWorkspace(WorkspaceReplaceNotification_ptr pNf)
{
    emit needsUpdating();
}

void MantidUI::handleDeleteWorkspace(WorkspaceDeleteNotification_ptr pNf)
{
    emit needsUpdating();
}

void MantidUI::logMessage(const Poco::Message& msg)
{
    if (!aw_results) return;
    QString str = msg.getText().c_str();
    //if (s_logEdit->document()->blockCount() > 1000) s_logEdit->document()->clear();
    if (msg.getPriority() < Poco::Message::PRIO_WARNING)
        aw_results->setTextColor(Qt::red);
    else
        aw_results->setTextColor(Qt::black);
    aw_results->insertPlainText(str+"\n");
    //cerr<<":"<<aw_results->document()->blockCount()<<'\n';
    QTextCursor cur = aw_results->textCursor();
    cur.movePosition(QTextCursor::End);
    aw_results->setTextCursor(cur);
}
