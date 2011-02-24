#include "MantidUI.h"
#include "MantidMatrix.h"
#include "MantidDock.h"
#include "ImportWorkspaceDlg.h"
#include "AlgMonitor.h"
#include "MantidSampleLogDialog.h"
#include "AlgorithmHistoryWindow.h"
#include "MantidCurve.h"
#include "FitPropertyBrowser.h"

#include "../Spectrogram.h"
#include "../pixmaps.h"
#include "../ScriptingWindow.h"
#include "../ColorBox.h"

#include "MantidKernel/Property.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/LogFilter.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidPlotReleaseDate.h"
#include "InstrumentWidget/InstrumentWindow.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include "MantidQtAPI/InterfaceManager.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"

#include "MantidKernel/EnvironmentHistory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"

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
#include <algorithm>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include <fstream>
#include <iostream>
#include <sstream>
using namespace std;

using namespace Mantid::API;
using Mantid::Kernel::DateAndTime;
namespace MantidException = Mantid::Kernel::Exception;


MantidUI::MantidUI(ApplicationWindow *aw):
    m_finishedLoadDAEObserver(*this, &MantidUI::handleLoadDAEFinishedNotification),
    m_addObserver(*this,&MantidUI::handleAddWorkspace),
    m_replaceObserver(*this,&MantidUI::handleReplaceWorkspace),
    m_deleteObserver(*this,&MantidUI::handleDeleteWorkspace),
    m_clearADSObserver(*this,&MantidUI::handleClearADS),
    m_algUpdatesObserver(*this, &MantidUI::handleAlgorithmFactoryUpdates),
    m_renameObserver(*this,&MantidUI::handleRenameWorkspace),
    m_groupworkspacesObserver(*this,&MantidUI::handleGroupWorkspaces),
    m_ungroupworkspaceObserver(*this,&MantidUI::handleUnGroupWorkspace),
    m_appWindow(aw)
{
  // To be able to use them in queued signals they need to be registered
  static bool registered_addtional_types = false;
  if( !registered_addtional_types )
  {
    registered_addtional_types = true;
    qRegisterMetaType<Mantid::API::Workspace_sptr>();
    qRegisterMetaType<Mantid::API::MatrixWorkspace_sptr>();
    //Register std::string as well as we use it alot
    qRegisterMetaType<std::string>();
  }

  m_exploreMantid = new MantidDockWidget(this,aw);
  m_exploreAlgorithms = new AlgorithmDockWidget(this,aw);

  actionCopyRowToTable = new QAction(tr("Copy spectra to table"), this);
  actionCopyRowToTable->setIcon(QIcon(getQPixmap("table_xpm")));
  connect(actionCopyRowToTable, SIGNAL(activated()), this, SLOT(copyRowToTable()));

  actionCopyRowToGraph = new QAction(tr("Plot spectra (values only)"), this);
  actionCopyRowToGraph->setIcon(QIcon(getQPixmap("graph_xpm")));
  connect(actionCopyRowToGraph, SIGNAL(activated()), this, SLOT(copyRowToGraph()));

  actionCopyRowToGraphErr = new QAction(tr("Plot spectra (values + errors)"), this);
  actionCopyRowToGraphErr->setIcon(QIcon(getQPixmap("graph_xpm")));
  connect(actionCopyRowToGraphErr, SIGNAL(activated()), this, SLOT(copyRowToGraphErr()));

  actionCopyDetectorsToTable = new QAction(tr("View detectors table"), this);
  actionCopyDetectorsToTable->setIcon(QIcon(getQPixmap("table_xpm")));
  connect(actionCopyDetectorsToTable, SIGNAL(activated()), this, SLOT(copyDetectorsToTable()));

  actionCopyValues = new QAction(tr("Copy"), this);
  actionCopyValues->setIcon(QIcon(getQPixmap("copy_xpm")));
  connect(actionCopyValues, SIGNAL(activated()), this, SLOT(copyValues()));

  actionCopyColumnToTable = new QAction(tr("Copy bin to table"), this);
  actionCopyColumnToTable->setIcon(QIcon(getQPixmap("table_xpm")));
  connect(actionCopyColumnToTable, SIGNAL(activated()), this, SLOT(copyColumnToTable()));

  actionCopyColumnToGraph = new QAction(tr("Plot bin (values only)"), this);
  actionCopyColumnToGraph->setIcon(QIcon(getQPixmap("graph_xpm")));
  connect(actionCopyColumnToGraph, SIGNAL(activated()), this, SLOT(copyColumnToGraph()));

  actionCopyColumnToGraphErr = new QAction(tr("Plot bin (values + errors)"), this);
  actionCopyColumnToGraphErr->setIcon(QIcon(getQPixmap("graph_xpm")));
  connect(actionCopyColumnToGraphErr, SIGNAL(activated()), this, SLOT(copyColumnToGraphErr()));

  connect(this,SIGNAL(needToCreateLoadDAEMantidMatrix(const QString&)),this,SLOT(createLoadDAEMantidMatrix(const QString&)));
  connect(this,SIGNAL(needToShowCritical(const QString&)),this,SLOT(showCritical(const QString&)));

  m_algMonitor = new AlgorithmMonitor(this);
  connect(m_algMonitor,SIGNAL(algorithmStarted(void*)),m_exploreAlgorithms,SLOT(algorithmStarted(void*)), Qt::QueuedConnection);
  connect(m_algMonitor,SIGNAL(algorithmFinished(void*)),m_exploreAlgorithms,SLOT(algorithmFinished(void*)), Qt::QueuedConnection);
  connect(m_algMonitor,SIGNAL(needUpdateProgress(void*,int, const QString&)),
      m_exploreAlgorithms,SLOT(updateProgress(void*,int, const QString&)), Qt::QueuedConnection);
  m_algMonitor->start();

  mantidMenu = new QMenu(m_appWindow);
  mantidMenu->setObjectName("mantidMenu");
  // for activating the keyboard shortcut for Clear All Memory even if no clciking on Mantid Menu
  // Ticket #672
  //connect(mantidMenu, SIGNAL(aboutToShow()), this, SLOT(mantidMenuAboutToShow()));
  mantidMenuAboutToShow();

  menuMantidMatrix = new QMenu(m_appWindow);
  connect(menuMantidMatrix, SIGNAL(aboutToShow()), this, SLOT(menuMantidMatrixAboutToShow()));
}

// Should it be moved to the constructor?
void MantidUI::init()
{
  Mantid::Kernel::ConfigService::Instance();
  // Echo Mantid's welcome message to the logger
  m_appWindow->writeToLogWindow(QString::fromStdString(Mantid::welcomeMessage()));
  MantidLog::connect(this);
  FrameworkManager::Instance();

  AnalysisDataService::Instance().notificationCenter.addObserver(m_addObserver);
  AnalysisDataService::Instance().notificationCenter.addObserver(m_replaceObserver);
  AnalysisDataService::Instance().notificationCenter.addObserver(m_deleteObserver);
  AnalysisDataService::Instance().notificationCenter.addObserver(m_clearADSObserver);
  AnalysisDataService::Instance().notificationCenter.addObserver(m_renameObserver);
  AnalysisDataService::Instance().notificationCenter.addObserver(m_groupworkspacesObserver);
  AnalysisDataService::Instance().notificationCenter.addObserver(m_ungroupworkspaceObserver);

  // Now that the framework is initialized we need to populate the algorithm tree
  m_exploreAlgorithms->update();
  try
  {
    m_fitFunction = new FitPropertyBrowser(m_appWindow);
    m_fitFunction->init();
    m_fitFunction->hide();
  }
  catch(...)
  {
    m_fitFunction = NULL;
    showCritical("The curve fitting plugin is missing");
  }

  //connect the signal from the algorithm factory to monitor updates
  connect(this, SIGNAL(algorithms_updated()), m_exploreAlgorithms, SLOT(update()));
  Mantid::API::AlgorithmFactory::Instance().notificationCenter.addObserver(m_algUpdatesObserver);

}

void MantidUI::addMenuItems(QMenu *menu)
{
  actionToggleMantid = m_exploreMantid->toggleViewAction();
  actionToggleMantid->setIcon(getQPixmap("mantid_matrix_xpm"));
  actionToggleMantid->setShortcut( tr("Ctrl+Shift+M") );
  menu->addAction(actionToggleMantid);

  actionToggleAlgorithms = m_exploreAlgorithms->toggleViewAction();
  actionToggleAlgorithms->setShortcut( tr("Ctrl+Shift+A") );
  menu->addAction(actionToggleAlgorithms);

  if (m_fitFunction)
  {
    actionToggleFitFunction = m_fitFunction->toggleViewAction();
    menu->addAction(actionToggleFitFunction);
  }

}

// Show / hide the FitPropertyBrowser
void MantidUI::showFitPropertyBrowser(bool on)
{
  if (!m_fitFunction) return;
  if (on)
  {
    m_fitFunction->show();
  }
  else
  {
    m_fitFunction->hide();
  }
}

/**
 * Be careful where this is called, if it is a called too late in the Qt shutdown the application
 * crashes
 */
void MantidUI::shutdown()
{
  Mantid::API::FrameworkManager::Instance().clear();
}

MantidUI::~MantidUI()
{
  if( m_algMonitor ) delete m_algMonitor;
  Mantid::API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_groupworkspacesObserver);
  Mantid::API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_ungroupworkspaceObserver);
  Mantid::API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_addObserver);
  Mantid::API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_replaceObserver);
  Mantid::API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_deleteObserver);
  Mantid::API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_clearADSObserver);
  Mantid::API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_algUpdatesObserver);
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
  std::set<std::string> sv = Mantid::API::AnalysisDataService::Instance().getObjectNames();
  for (std::set<std::string>::const_iterator it = sv.begin(); it != sv.end(); ++it)
    sl<<QString::fromStdString(*it);
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

    @param algName :: Algorithm's name
    @return Pointer to the created algorithm
 */
IAlgorithm_sptr MantidUI::CreateAlgorithm(const QString& algName)
{
  IAlgorithm_sptr alg = AlgorithmManager::Instance().create(algName.toStdString());

  return alg;
}

/**
 * Ticket #678
 */
void MantidUI::saveNexusWorkspace()
{
  executeSaveNexus("SaveNexus",-1);
}

/**
 * DeleteWorkspace
 @param workspaceName :: Name of the workspace to delete
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
  QString str = m_exploreMantid->getSelectedWorkspaceName();
  if ( str.isEmpty() )
  {
    //Check if a mantid matrix is selected
    MantidMatrix *m = qobject_cast<MantidMatrix*>(appWindow()->activeWindow());
    if( !m ) return "";

    str = m->workspaceName();
  }
  return str;
}

Mantid::API::Workspace_sptr MantidUI::getSelectedWorkspace()
{
  return m_exploreMantid->getSelectedWorkspace();
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

    appWindow()->menuBar()->insertItem(tr("&Workspace"),menuMantidMatrix);
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
     @param wsName :: Workspace name
     @param lower :: An optional lower boundary
     @param upper :: An optional upper boundary
     @param showDlg :: If true show a dialog box to set some import parameters
     @param makeVisible :: If true show the created MantidMatrix, hide otherwise.
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
     @param wsName :: Workspace name
     @param showDlg :: If true show a dialog box to set some import parameters
     @param makeVisible :: If true show the created widget, hide otherwise.
 */
void MantidUI::importWorkspace(const QString& wsName, bool showDlg, bool makeVisible)
{
  MantidMatrix* mm = importMatrixWorkspace(wsName,-1, -1, showDlg,makeVisible);
  appWindow()->addListViewItem(mm);
  if (!mm)
  {Table * t=importTableWorkspace(wsName,showDlg,makeVisible);
  appWindow()->addListViewItem(t);
  }
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
     @param wsName :: Workspace name
     @param showDlg :: If true show a dialog box to set some import parameters
     @param makeVisible :: If true show the created Table, hide otherwise.
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
  Table* t = new Table(appWindow()->scriptingEnv(), ws->rowCount(), ws->columnCount(), "", appWindow(), 0);
  appWindow()->initTable(t, appWindow()->generateUniqueName(wsName+"-"));
  //  t->askOnCloseEvent(false);
  if (makeVisible) t->showNormal();
  else t->showMinimized();

  for(int i=0;i<ws->columnCount();i++)
  {
    Column_sptr c = ws->getColumn(i);
    QString colName = QString::fromStdString(c->name());
    t->setColName(i,colName);
    t->setReadOnlyColumn(i);
    if (colName.endsWith("_err",Qt::CaseInsensitive) ||
        colName.endsWith("_error",Qt::CaseInsensitive))
    {
      t->setColPlotDesignation(i,Table::yErr);
    }
    for(int j=0;j<ws->rowCount();j++)
    {
      std::ostringstream ostr;
      c->print(ostr,j);
      t->setText(j,i,QString::fromStdString(ostr.str()));
    }
  }
  return t;
}

void MantidUI::removeWindowFromLists(MdiSubWindow* m)
{
  if (!m)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  if (m->isA("MantidMatrix"))
  {static_cast<MantidMatrix*>(m)->removeWindow();
  }

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
    cm.addSeparator();
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
  plotSelectedRows(m,false);

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
  plotSelectedRows(m,true);

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
  std::vector<int> indices(m->numRows(), 0);
  for(int i = 0; i < m->numRows();i++)
  {
    indices[i] = m->workspaceIndex(i);
  }
  return createDetectorTable(m->workspaceName(), indices);
}

Table* MantidUI::createDetectorTable(const QString & wsName, const std::vector<int>& indices, bool include_data)
{
  const int nrows = indices.size();
  int ncols = 6;
  QStringList col_names;
  col_names << "Index" << "Spectra" << "Detector ID";
  if( include_data )
  {
    ncols += 2;
    col_names << "Data Value" << "Data Error";  
  }
  col_names << "R" << "Theta" << "Phi";

  Table* t = new Table(appWindow()->scriptingEnv(), nrows, ncols, "", appWindow(), 0);
  appWindow()->initTable(t, appWindow()->generateUniqueName(wsName + "-Detectors-"));
  //Set the column names
  for( int col = 0; col < ncols; ++col )
  {
    t->setColName(col, col_names[col]);
  }

  MatrixWorkspace_sptr ws;
  if( AnalysisDataService::Instance().doesExist(wsName.toStdString()) )
  {
    ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName.toStdString()));
  }

  if( !ws ) 
  {
    delete t;
    return NULL;
  }

  Mantid::API::Axis *spectraAxis = ws->getAxis(1);
  Mantid::Geometry::IObjComponent_const_sptr sample = ws->getInstrument()->getSample();
  QList<double> col_values;
  for( int row = 0; row < nrows; ++row )
  {
    int ws_index = indices[row];

    int currentSpec;
    try
    {
      currentSpec = spectraAxis->spectraNo(ws_index);
    }
    catch(std::domain_error)
    {//if there is no spectra number information in the workspace display the spectra numbers as -1
      currentSpec = -1;
    }

    int detID = 0;
    double R(0.0), Theta(0.0), Phi(0.0);
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
    if (!col_values.isEmpty()) col_values.clear();
    col_values << static_cast<double>(ws_index) <<  static_cast<double>(currentSpec) << static_cast<double>(detID);
    if( include_data )
    {
      col_values << ws->readY(ws_index)[0] << ws->readE(ws_index)[0];
    }
    col_values << R << Theta << Phi;
    for(int col = 0; col < ncols; ++col)
    {
      t->setCell(row, col, col_values[col]);
    }

  }
  t->showNormal();
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
  Mantid::API::IAlgorithm_sptr alg = this->createAlgorithm(algName, version);
  if( !alg ) return;
  MantidQt::API::AlgorithmDialog* dlg=createLoadAlgorithmDialog(alg);
  executeLoadAlgorithm(dlg,alg);

}

/** This method is to execute loadraw from ICat Interface
 *@param fileName :: name of the file
 *@param wsName :: name of the workspace
 */
void MantidUI::loadrawfromICatInterface(const QString& fileName ,const QString& wsName)
{

  Mantid::API::IAlgorithm_sptr alg = this->createAlgorithm("LoadRaw", -1);
  if( !alg ) return;

  MantidQt::API::AlgorithmDialog* dlg=createLoadAlgorithmDialog(alg);
  QList<QLineEdit*> list = dlg->findChildren <QLineEdit*>();
  if(!list.empty())
  {
    //set the file name(including location) obtained from ICat API
    list[0]->setText(fileName);
    //set the workspace name
    if(list[1])
    {
      list[1]->setText(wsName);
    }
  }
  executeLoadAlgorithm(dlg,alg);


}
void MantidUI::loadnexusfromICatInterface(const QString& fileName,const QString& wsName)
{
  Mantid::API::IAlgorithm_sptr alg = this->createAlgorithm("LoadNexus", -1);
  if( !alg ) return;

  MantidQt::API::AlgorithmDialog* dlg=createLoadAlgorithmDialog(alg);
  if(!dlg) return;
  QList<QLineEdit*> list = dlg->findChildren <QLineEdit*>();
  if(!list.empty())
  {
    list[0]->setText(fileName);
    if(list[1])
    {
      list[1]->setText(wsName);
    }
  }
  executeLoadAlgorithm(dlg,alg);
}

void MantidUI ::executeloadAlgorithm(const QString& algName, const QString& fileName, const QString& wsName)
{

  Mantid::API::IAlgorithm_sptr alg = this->createAlgorithm(algName, -1);
  if( !alg ) return;
  try
  {
    alg->setProperty("Filename",fileName.toStdString());
    alg->setPropertyValue("OutputWorkspace",wsName.toStdString());
  }
  catch(std::invalid_argument& e)
  {
    //emit error(e.what());
    m_appWindow->writeToLogWindow(QString::fromStdString(e.what()), true);
    return ;
  }
  catch (Mantid::Kernel::Exception::NotFoundError& e)
  {
    //emit error(e.what());
    m_appWindow->writeToLogWindow(QString::fromStdString(e.what()), true);
    return ;
  }

  executeAlgorithmAsync(alg);
}

/** This creates an algorithm dialog (the default property entry thingie).
 * It is NOT exclusively for "Load" algorithms!
 */
MantidQt::API::AlgorithmDialog*  MantidUI::createLoadAlgorithmDialog(Mantid::API::IAlgorithm_sptr alg)
{
  QString presets(""), enabled("");
  //If a workspace is selected in the dock then set this as a preset for the dialog
  QString selected = getSelectedWorkspaceName();
  if( !selected.isEmpty() )
  {
    QString property_name = findInputWorkspaceProperty(alg);
    presets = "|" + property_name + "=" + selected + "|";
    enabled = property_name + ",";
  }
  //Check if a workspace is selected in the dock and set this as a preference for the input workspace

  //This is an optional message displayed at the top of the GUI.
  QString optional_msg(alg->getOptionalMessage().c_str());

  MantidQt::API::AlgorithmDialog *dlg =
      MantidQt::API::InterfaceManager::Instance().createDialog(alg.get(), m_appWindow, false, presets, optional_msg , enabled);
  return dlg;
}


void MantidUI::executeLoadAlgorithm(MantidQt::API::AlgorithmDialog* dlg,Mantid::API::IAlgorithm_sptr alg)
{
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

void MantidUI::executeAlgorithm(const QString & algName, const QString & paramList)
{
  //Get latest version of the algorithm
  Mantid::API::IAlgorithm_sptr alg = this->createAlgorithm(algName, -1);
  if( !alg ) return;

  alg->setProperties(paramList.toStdString());
  executeAlgorithmAsync(alg);
}

/**
 * Find the first input workspace for an algorithm
 * @param algorithm :: A pointer to the algorithm instance
 */
QString MantidUI::findInputWorkspaceProperty(Mantid::API::IAlgorithm_sptr algorithm) const
{
  //Iterate through the properties and find the first input one
  std::vector<Mantid::Kernel::Property*> props = algorithm->getProperties();
  std::vector<Mantid::Kernel::Property*>::const_iterator pend = props.end();
  for(std::vector<Mantid::Kernel::Property*>::const_iterator pitr = props.begin(); pitr != props.end(); ++pitr)
  {
    Mantid::Kernel::Property *base_prop = *pitr;
    const Mantid::API::IWorkspaceProperty *ws_prop = dynamic_cast<Mantid::API::IWorkspaceProperty*>(base_prop);
    if(ws_prop)
    {
      unsigned int direction = base_prop->direction();
      if (direction == Mantid::Kernel::Direction::Input || direction == Mantid::Kernel::Direction::InOut)
      {
        return QString::fromStdString(base_prop->name());
      }
    }
  }
  return QString();
}

void  MantidUI::copyWorkspacestoVector(const QList<QTreeWidgetItem*> &selectedItems,std::vector<std::string> &inputWSVec)
{
  //iterate through each of the selected workspaces
  QList<QTreeWidgetItem*>::const_iterator itr;
  for(itr=selectedItems.begin();itr!=selectedItems.end();itr++)
  {
    std::string inputWSName=(*itr)->text(0).toStdString();
    inputWSVec.push_back(inputWSName);
  }//end of for loop for input workspaces
}

/**
 * Renames selected workspace
 * @param wsName :: selected workspace name
 */
void MantidUI::renameWorkspace(QString wsName)
{ 
  // If the wsname is blank look for an active window and assume this workspace is
  // the one to rename
  if( wsName.isEmpty() )
  {
    MantidMatrix *matrix = dynamic_cast<MantidMatrix*>(appWindow()->activeWindow());
    if( matrix )
    {
      wsName = matrix->workspaceName();
    }
    else
    {
      return;
    }
  }

  //execute the algorithm
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
  if (!alg)
  {
    return;
  }
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
      int selectedIndex=combo->findText(wsName,Qt::MatchExactly );
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
    if(!bStatus)
    {
      QMessageBox::critical(appWindow(),"MantidPlot - Algorithm error"," Error in GroupWorkspaces algorithm");
    }

  }
  catch(std::invalid_argument &)
  {
    QMessageBox::critical(appWindow(),"MantidPlot - Algorithm error"," Error in GroupWorkspaces algorithm");
  }
  catch(Mantid::Kernel::Exception::NotFoundError&)//if not a valid object in analysis data service
  {
    QMessageBox::critical(appWindow(),"MantidPlot - Algorithm error"," Error in GroupWorkspaces algorithm");
  }
  catch(std::runtime_error& )
  {
    QMessageBox::critical(appWindow(),"MantidPlot - Algorithm error"," Error in GroupWorkspaces algorithm");
  }
  catch(std::exception& )
  {
    QMessageBox::critical(appWindow(),"MantidPlot - Algorithm error"," Error in GroupWorkspaces algorithm");
  }

}
void MantidUI::ungroupWorkspaces()
{
  try
  {
    QList<QTreeWidgetItem*>selectedItems=m_exploreMantid->m_tree->selectedItems();
    if(selectedItems.isEmpty())
    {
      throw std::runtime_error("Select a group workspace to Ungroup.");
    }

    //workspace name
    std::string wsname=selectedItems[0]->text(0).toStdString();

    std::string algName("UnGroupWorkspace");
    Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create(algName,1);
    alg->initialize();
    alg->setProperty("InputWorkspace",wsname);

    //execute the algorithm
    bool bStatus=alg->execute();
    if(!bStatus)
    {
      QMessageBox::critical(appWindow(),"MantidPlot - Algorithm error"," Error in UnGroupWorkspace algorithm");
    }

  }
  catch(std::invalid_argument &)
  {
    QMessageBox::critical(appWindow(),"MantidPlot - Algorithm error"," Error in UnGroupWorkspace algorithm");
  }
  catch(std::runtime_error & )
  {
    QMessageBox::critical(appWindow(),"MantidPlot - Algorithm error"," Error in UnGroupWorkspace algorithm");
  }
  catch(std::exception & )
  {
    QMessageBox::critical(appWindow(),"MantidPlot - Algorithm error"," Error in UnGroupWorkspace algorithm");
  }

}

Mantid::API::IAlgorithm_sptr MantidUI::createAlgorithm(const QString& algName, int version)
{
  emit algorithmAboutToBeCreated(); 
  Mantid::API::IAlgorithm_sptr alg;
  try
  {
    alg = Mantid::API::AlgorithmManager::Instance().create(algName.toStdString(),version);
  }
  catch(...)
  {
    QMessageBox::critical(appWindow(),"MantidPlot",
        "Cannot create algorithm \""+ algName + "\" version "+QString::number(version));
    alg = Mantid::API::IAlgorithm_sptr();
  }
  return alg;
}

void MantidUI::executeAlgorithmAsync(Mantid::API::IAlgorithm_sptr alg)
{
  m_algMonitor->add(alg);

  try
  {
    alg->executeAsync();
  }
  catch(...)
  {
    QMessageBox::critical(appWindow(),"MantidPlot - Algorithm error","Exception is caught");
  }
}
bool MantidUI::executeICatLogout(int version)
{
  Mantid::API::IAlgorithm_sptr alg = this->createAlgorithm("CatalogLogout", version);
  if( !alg ) return false;
  Poco::ActiveResult<bool> result(alg->executeAsync());
  return result.failed();

}
/// This method is to execute download data files algorithm from ICat
void MantidUI::executeDownloadDataFiles(const std::vector<std::string>& filenNames,const std::vector<long long>& fileIds)
{
  std::vector<std::string> fileLocs;
  Mantid::API::IAlgorithm_sptr alg;
  try
  {
    alg = Mantid::API::AlgorithmManager::Instance().create("CatalogDownloadDataFiles",-1);
  }
  catch(...)
  {
    m_appWindow->writeToLogWindow("Error when getting/downloading data file from isis server ", true);
    return;
  }
  try
  {
    alg->setProperty("FileNames",filenNames);
	alg->setProperty("FileIds",fileIds);
    
  }
  catch(std::invalid_argument& e)
  {
    m_appWindow->writeToLogWindow(QString::fromStdString(e.what()), true);
    return;

  }
  catch (Mantid::Kernel::Exception::NotFoundError& e)
  {
    m_appWindow->writeToLogWindow(QString::fromStdString(e.what()), true);
    return;
  }
  m_algMonitor->add(alg);

  try
  {
    Poco::ActiveResult<bool> result(alg->executeAsync());
    while( !result.available() )
    {
      QCoreApplication::processEvents();
    }
  }
  catch(...)
  {
    m_appWindow->writeToLogWindow("Error when getting/downloading data file from isis server", true);
    return;
  }
  try
  {
    fileLocs = alg->getProperty("FileLocations");
  }
  catch (Mantid::Kernel::Exception::NotFoundError&e)
  {
    m_appWindow->writeToLogWindow(QString::fromStdString(e.what()), true);
    return;
  }

  emit fileLocations(fileLocs);

}


void MantidUI::handleLoadDAEFinishedNotification(const Poco::AutoPtr<Mantid::API::Algorithm::FinishedNotification>& pNf)
{
  std::string wsNAme = pNf->algorithm()->getProperty("OutputWorkspace");
  emit needToCreateLoadDAEMantidMatrix(QString::fromStdString(wsNAme));
}

void MantidUI::createLoadDAEMantidMatrix(const QString& wsQName)
{
  std::string wsName = wsQName.toStdString();
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
    updater->setPropertyValue("UpdateRate",QString::number(updateInterval).toStdString());
    executeAlgorithmAsync(updater);
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
  emit workspace_added(QString::fromStdString(pNf->object_name()),(pNf->object()));
}

void MantidUI::handleReplaceWorkspace(WorkspaceAfterReplaceNotification_ptr pNf)
{
  emit workspace_replaced(QString::fromStdString(pNf->object_name()),(pNf->object()));
}

void MantidUI::handleDeleteWorkspace(WorkspaceDeleteNotification_ptr pNf)
{
  emit workspace_removed(QString::fromStdString(pNf->object_name()));
}

void MantidUI::handleClearADS(ClearADSNotification_ptr)
{
  emit workspaces_cleared();
}

void MantidUI::handleAlgorithmFactoryUpdates(Mantid::API::AlgorithmFactoryUpdateNotification_ptr)
{
  emit algorithms_updated();
}
void MantidUI::handleRenameWorkspace(WorkspaceRenameNotification_ptr pNf)
{
  emit workspace_renamed(QString::fromStdString(pNf->object_name()), QString::fromStdString(pNf->new_objectname()));
}
void MantidUI::handleGroupWorkspaces(Mantid::API::WorkspacesGroupedNotification_ptr pNf)
{
  const std::vector<std::string> wsvec=pNf->inputworkspacenames();
  QStringList wsList;
  std::vector<std::string>::const_iterator citr;
  for(citr=wsvec.begin();citr!=wsvec.end();++citr)
  {
    wsList.append(QString::fromStdString(*citr));
  }
  emit workspaces_grouped(wsList);
}
void MantidUI::handleUnGroupWorkspace(Mantid::API::WorkspaceUnGroupingNotification_ptr pNf)
{
  emit workspace_ungrouped(QString::fromStdString(pNf->object_name()), pNf->object());
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
  Mantid::API::MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(getWorkspace(wsName));
  if (!ws) return NULL;
  Mantid::Geometry::IInstrument_const_sptr instr = ws->getInstrument();
  if (!instr || instr->getName().empty())
  {
    return NULL;
  }

  //Need a new window
  InstrumentWindow *insWin = new InstrumentWindow(QString("Instrument"),appWindow());
  insWin->setName(QString("InstrumentWindow:") + wsName);
  insWin->setWindowTitle(QString("Instrument - ") + wsName);
  appWindow()->d_workspace->addSubWindow(insWin);
  appWindow()->addListViewItem(insWin);

  insWin->setWorkspaceName(wsName.toStdString());
  connect(insWin, SIGNAL(closedWindow(MdiSubWindow*)), appWindow(), SLOT(closeWindow(MdiSubWindow*)));
  connect(insWin,SIGNAL(hiddenWindow(MdiSubWindow*)), appWindow(), SLOT(hideWindow(MdiSubWindow*)));
  connect (insWin,SIGNAL(showContextMenu()), appWindow(),SLOT(showWindowContextMenu()));
  connect(insWin,SIGNAL(plotSpectra(const QString&,const std::set<int>&)),this,
      SLOT(plotSpectraList(const QString&,const std::set<int>&)));
  connect(insWin,SIGNAL(createDetectorTable(const QString&,const std::vector<int>&,bool)),this,
      SLOT(createDetectorTable(const QString&,const std::vector<int>&,bool)));
  connect(insWin, SIGNAL(execMantidAlgorithm(const QString&,const QString&)), this,
      SLOT(executeAlgorithm(const QString&, const QString&)));
  return insWin;
}


void MantidUI::showMantidInstrument(const QString& wsName)
{
  InstrumentWindow *insWin = getInstrumentView(wsName);
  if (!insWin)
  {
    QMessageBox::critical(appWindow(),"MantidPlot - Error","Instrument view cannot be opened");
    return;
  }
  insWin->show();
}

void MantidUI::showMantidInstrument()
{
  MantidMatrix* m = (MantidMatrix*)appWindow()->activeWindow();
  if (!m || !m->isA("MantidMatrix")) return;
  if(!m->workspaceName().isEmpty())
  {showMantidInstrument(m->workspaceName());
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

  QAction* tstAction = new QAction("&Plot Memory Usage",this);
  connect(tstAction,SIGNAL(triggered()), this, SLOT(manageMantidWorkspaces() ));
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

  // Relevant notifications are connected to signals that will close all dependent windows
  Mantid::API::FrameworkManager::Instance().clear();
}


/** Release any free memory back to the system */
void MantidUI::releaseFreeMemory()
{
  // This only does something if TCMalloc is used
  Mantid::API::MemoryManager::Instance().releaseFreeMemory();
}

void MantidUI::saveProject(bool saved)
{
  if( !saved )
  {
    QString savemsg = tr("Save changes to project: <p><b> %1 </b> ?").arg("untitled");
    int result = QMessageBox::information(appWindow(), tr("MantidPlot"), savemsg, tr("Yes"), tr("No"), 0, 2);
    if( result == 0)
      appWindow()->saveProject();
  }
  //close all the windows opened
  foreach( MdiSubWindow* sub_win, appWindow()->windowsList() )
  {
    sub_win->setconfirmcloseFlag(false);
    sub_win->close();
  }
  Mantid::API::FrameworkManager::Instance().clear();
}
void MantidUI::enableSaveNexus(const QString& wsName)
{
  appWindow()->enablesaveNexus(wsName);
}

/** This method is sueful for saving the currently loaded workspaces to project file on save.
 *  saves the names of all the workspaces loaded into mantid workspace tree
 *  into a string and calls save nexus on each workspace to save the data to a nexus file.
 * @param workingDir :: -working directory of teh current project
 */
QString MantidUI::saveToString(const std::string& workingDir)
{
  QString wsNames;
  wsNames="<mantidworkspaces>\n";
  wsNames+="WorkspaceNames";
  QTreeWidget *tree=m_exploreMantid->m_tree;
  int count=tree->topLevelItemCount();
  for(int i=0;i<count;++i)
  { QTreeWidgetItem* item=tree->topLevelItem(i);
  QString wsName=item->text(0);
  wsNames+="\t";
  wsNames+=wsName;

  std::string fileName(workingDir+"//"+wsName.toStdString()+".nxs");
  //saving to  nexus file
  try
  {
    savedatainNexusFormat(fileName,wsName.toStdString());
  }
  catch(...)
  {
  }
  }
  wsNames+="\n</mantidworkspaces>\n";
  return wsNames;
}
/**
 *  Prepares the Mantid Menu depending on the state of the active MantidMatrix.
 */
void MantidUI::menuMantidMatrixAboutToShow()
{
  menuMantidMatrix->clear();
  MantidMatrix *w = dynamic_cast<MantidMatrix*>(appWindow()->activeWindow());
  //menuMantidMatrix->addAction(actionCopyValues);
  menuMantidMatrix->addAction(actionCopyDetectorsToTable);
  menuMantidMatrix->addSeparator();
  menuMantidMatrix->insertItem(tr("Set &Properties..."), w, SLOT(setMatrixProperties() ) );

  ///
  menuMantidMatrix->addSeparator();
  QAction * action = new QAction("Show instrument",this);
  connect(action,SIGNAL(triggered()),this,SLOT(showMantidInstrument()));
  menuMantidMatrix->addAction(action);

  action = new QAction("Plot spectrum...",this);
  connect(action,SIGNAL(triggered()),m_exploreMantid,SLOT(plotSpectra()));
  menuMantidMatrix->addAction(action);

  action = new QAction("Sample Logs...", this);
  connect(action,SIGNAL(triggered()),this,SLOT(showLogFileWindow()));
  menuMantidMatrix->addAction(action);

  action = new QAction("Show History", this);
  connect(action,SIGNAL(triggered()),this,SLOT(showAlgorithmHistory()));
  menuMantidMatrix->addAction(action);

  action = new QAction("Save Nexus",this);
  connect(action,SIGNAL(activated()),this,SLOT(saveNexusWorkspace()));
  menuMantidMatrix->addAction(action);

  action = new QAction("Rename",this);
  connect(action,SIGNAL(activated()),this,SLOT(renameWorkspace()));
  menuMantidMatrix->addAction(action);


  //separate delete
  menuMantidMatrix->addSeparator();

  action = new QAction("Delete",this);
  connect(action,SIGNAL(triggered()),m_exploreMantid,SLOT(deleteWorkspaces()));
  menuMantidMatrix->addAction(action);

}

/// Catches the signal from InstrumentWindow to plot a spectrum.
MultiLayer* MantidUI::plotInstrumentSpectrum(const QString& wsName, int spec)
{
  QMessageBox::information(appWindow(),"OK",wsName+" "+QString::number(spec));
  return plotSpectraRange(wsName, spec, spec, false);
}

/// Catches the signal from InstrumentWindow to plot a spectrum.
MultiLayer* MantidUI::plotInstrumentSpectrumList(const QString& wsName, std::set<int> spec)
{
  return plotSpectraList(wsName, spec, false);
}

MultiLayer* MantidUI::plotBin(const QString& wsName, int bin, bool errors)
{
  MantidMatrix* m = getMantidMatrix(wsName);
  if( !m )
  {
    m = importMatrixWorkspace(wsName, -1, -1, false, false);
  }
  MatrixWorkspace_sptr ws;
  if (AnalysisDataService::Instance().doesExist(wsName.toStdString()))
  {
    ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName.toStdString()));
  }
  if( !ws.get() ) return NULL;

  QList<int> binAsList;
  binAsList.append(bin);
  Table *t = createTableFromBins(wsName, ws, binAsList, errors);
  t->askOnCloseEvent(false);
  t->setAttribute(Qt::WA_QuitOnClose);
  MultiLayer* ml(NULL);
  if( !t ) return ml;

  ml = appWindow()->multilayerPlot(t,t->colNames(),Graph::Line);
  Graph *g = ml->activeGraph();
  appWindow()->polishGraph(g,Graph::Line);
  setUpBinGraph(ml,wsName, ws);
  ml->askOnCloseEvent(false);
  return ml;
}

//-------------------------------------------------
// The following commands are purely for the Python
// interface
//-------------------------------------------------
/**
 * This is for the Python API to be able to call the method that takes a map as SIP didn't like accepting a multimap as an 
 * argument
 */
MultiLayer* MantidUI::pyPlotSpectraList(const QList<QString>& ws_names, const QList<int>& spec_list, bool errs)
{
  // Convert the list into a map (with the same workspace as key in each case)
  QMultiMap<QString,int> pairs;
  QListIterator<QString> ws_itr(ws_names);
  ws_itr.toBack();
  QListIterator<int> spec_itr(spec_list);
  spec_itr.toBack();

  // Need to iterate through the set in reverse order to get the curves in the correct order on the plot
  while( ws_itr.hasPrevious() )
  {
    QString workspace_name = ws_itr.previous();
    while( spec_itr.hasPrevious() )
    {
      pairs.insert(workspace_name, spec_itr.previous());
    }
    //Reset spectrum index pointer
    spec_itr.toBack();
  }

  // Pass over to the overloaded method
  return plotSpectraList(pairs,errs);
}

/**
 * Sets the flag that tells the scripting environment that
 * a script is currently running
 */
void MantidUI::setIsRunning(bool running)
{
  appWindow()->scriptingEnv()->setIsRunning(running);
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
  mlayer_2->close();

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
bool MantidUI::runAlgorithmAsync_PyCallback(const QString & alg_name)
{
  Mantid::API::IAlgorithm_sptr alg = findAlgorithmPointer(alg_name);

  if( !alg )
  {
    return false;
  }
  if( m_algMonitor ) 
  {
    m_algMonitor->add(alg);
  }
  Poco::ActiveResult<bool> result(alg->executeAsync());
  while( !result.available() )
  {
    QCoreApplication::processEvents();
  }
  result.wait();

  try
  {
    return result.data();
  }
  catch( Poco::NullPointerException& )
  {
    return false;
  }
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


/** Displays a string in a Qtiplot table
 *  @param logName :: the title of the table is based on this
 *  @param data :: the string to display
 */
void MantidUI::importString(const QString &logName, const QString &data)
{
  Table* t = new Table(appWindow()->scriptingEnv(), 1, 1, "", appWindow(), 0);
  if( !t ) return;
  //Have to replace "_" since the legend widget uses them to separate things
  QString label = logName;
  label.replace("_","-");

  appWindow()->initTable(t, 
      appWindow()->generateUniqueName(label.section("-",0, 0) + "-"));
  t->setColName(0, "Log entry");

  t->setText(0, 0, data);

  //Show table
  t->resize(2*t->table()->horizontalHeader()->sectionSize(0) + 55,
      (QMIN(10,1)+1)*t->table()->verticalHeader()->sectionSize(0)+100);
  t->setAttribute(Qt::WA_DeleteOnClose);
  t->showNormal();
}
/** Displays a string in a Qtiplot table
 *  @param logName :: the title of the table is based on this
 *  @param data :: a formated string with the time series data to display
 */
void MantidUI::importStrSeriesLog(const QString &logName, const QString &data)
{
  QStringList loglines = data.split("\n", QString::SkipEmptyParts);

  int rowcount(loglines.count());
  Table* t = new Table(appWindow()->scriptingEnv(), rowcount, 2, "", appWindow(), 0);
  if( !t ) return;
  //t->askOnCloseEvent(false);
  //Have to replace "_" since the legend widget uses them to separate things
  QString label = logName;
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
    QStringList ds(ts);
    ds.removeFirst();// remove date
    ds.removeFirst();// and time
    t->setText(row, 1, ds.join(" "));
  }

  //Show table
  t->resize(2*t->table()->horizontalHeader()->sectionSize(0) + 55,
      (QMIN(10,rowcount)+1)*t->table()->verticalHeader()->sectionSize(0)+100);
  // t->askOnCloseEvent(false);
  t->setAttribute(Qt::WA_DeleteOnClose);
  t->showNormal();
}

//------------------------------------------------------------------------------------------------
/**  Import a numeric log data. It will be shown in a graph and copied into a table
     @param wsName :: The workspace name which log data will be imported
     @param logname :: The name of the log property to import
     @param filter :: Filter flag telling how to filter the log data.
                - 0 means no filtering
                - 1 filter by running status
                - 2 filter by period
                - 3 filter by status & period
 */
void MantidUI::importNumSeriesLog(const QString &wsName, const QString &logname, int filter)
{
  Mantid::API::MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(getWorkspace(wsName));
  if (!ws) return;

  Mantid::Kernel::Property * logData =ws->run().getLogData(logname.toStdString());
  if (!logData) return;

  Mantid::Kernel::LogFilter flt(logData);

  //Get a map of time/value. This greatly speeds up display.
  // NOTE: valueAsMap() skips repeated values.
  std::map<DateAndTime, double> time_value_map = flt.data()->valueAsMap();
  int rowcount = time_value_map.size();
  int colCount = 2;
  Table* t = new Table(appWindow()->scriptingEnv(), rowcount, colCount, "", appWindow(), 0);

  if( !t ) return;
  // t->askOnCloseEvent(false);
  //Have to replace "_" since the legend widget uses them to separate things
  QString label = logname;
  label.replace("_","-");

  //Get the starting time of the log.
  Mantid::Kernel::DateAndTime startTime;
  if (time_value_map.size() > 0)
  {
    startTime = time_value_map.begin()->first;
  }

  //Make a unique title, and put in the start time of the log
  QString title = label + QString::fromStdString( " (" + startTime.to_simple_string() + ")" );
  appWindow()->initTable(t, appWindow()->generateUniqueName(title));

  //Toggle to switch between using the real date or the change in seconds.
  bool useAbsoluteDate = false;

  if (useAbsoluteDate)
  {
    // --------- Date
    t->setColName(0, "Time");
    t->setColumnType(0, Table::Date);
    t->setDateFormat("yyyy-MMM-dd HH:mm:ss.ffffff", 0, false);
  }
  else
  {
    //Seconds offset
    t->setColName(0, "Time (sec)");
    t->setColumnType(0, Table::Numeric); //six digits after 0
    t->setNumericPrecision(6); //six digits after 0
  }

  //Make the column header with the units, if any
  QString column1 = label.section("-",1);
  if (logData->units() != "")
    column1 = column1 + QString::fromStdString(" (in " + logData->units() + ")");
  t->setColName(1, column1);

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
        f = dynamic_cast<Mantid::Kernel::TimeSeriesProperty<bool> *>(ws->run().getLogData("running"));
        if (f) flt.addFilter(f);
        else
        {
          importNumSeriesLog(wsName,logname,0);
          return;
        }
      }
      catch(...)
      {
        importNumSeriesLog(wsName,logname,0);
        return;
      }
    }

    if (filter == 2 || filter ==3)
    {
      std::vector<Mantid::Kernel::Property*> ps =ws->run().getLogData();
      for(std::vector<Mantid::Kernel::Property*>::const_iterator it=ps.begin();it!=ps.end();it++)
        if ((*it)->name().find("period ") == 0)
        {
          try
          {
            f = dynamic_cast<Mantid::Kernel::TimeSeriesProperty<bool> *>(*it);
            if (f) flt.addFilter(f);
            else
            {
              importNumSeriesLog(wsName,logname,0);
              return;
            }
          }
          catch(...)
          {
            importNumSeriesLog(wsName,logname,0);
            return;
          }

          break;
        }
    }

    if (flt.filter())
    {
      //Valid filter was found
      t->addColumns(2);
      t->setColName(2, "FTime");

      if (useAbsoluteDate)
      {
        t->setColumnType(2, Table::Date);
        t->setDateFormat("yyyy-MMM-dd HH:mm:ss", 2, false); //This is the format of the date column
      }
      else
      {
        t->setColumnType(2, Table::Numeric); //six digits after 0
        t->setNumericPrecision(6); //six digits after 0
      }

      t->setColPlotDesignation(2,Table::X);
      t->setColName(3, "Filter");

      if (flt.filter()->size() > rowcount)
      {
        t->addRows(flt.filter()->size() - rowcount);
      }

      if (flt.data()->size() > rowcount)
      {
        t->addRows(flt.data()->size() - rowcount);
      }

      for(int i=0;i<flt.filter()->size();i++)
      {
        t->setText(i,2,QString::fromStdString(flt.filter()->nthInterval(i).begin_str()));
        t->setCell(i,3,!flt.filter()->nthValue(i));
      }

      iValueCurve = 1;
      iFilterCurve = 0;

    } //end (valid filter exists)


  }

  Mantid::Kernel::DateAndTime lastTime;
  double lastValue;

  //Iterate through the time-value map.
  int i = 0;
  std::map<DateAndTime, double>::iterator it = time_value_map.begin();
  if (it!=time_value_map.end())
  {
    for (; it!=time_value_map.end(); it++)
    {
      lastTime = it->first;
      lastValue = it->second;

      std::string time_string;

      if (useAbsoluteDate)
      {
        //Convert time into string
        std::string time_string = lastTime.to_simple_string();
      }
      else
      {
        //How many seconds elapsed?
        Mantid::Kernel::time_duration elapsed = lastTime - startTime;
        double seconds = Mantid::Kernel::DateAndTime::seconds_from_duration(elapsed);

        //Output with 6 decimal points
        std::ostringstream oss;
        oss.precision(6);
        oss << std::fixed << seconds;
        time_string = oss.str();
      }

      t->setText(i,0,QString::fromStdString(time_string));
      t->setCell(i,1,lastValue);
      i++;
    }
  }

  try
  {
    //Set the filter strings
    if (filter && flt.filter() && lastTime < flt.filter()->lastTime())
    {
      rowcount = time_value_map.size();
      if (rowcount == t->numRows()) t->addRows(1);
      std::string time_string = flt.filter()->lastTime().to_simple_string();
      t->setText(rowcount,0,QString::fromStdString(time_string));
      t->setCell(rowcount,1,lastValue);
    }
  }
  catch(...)
  {
  }

  //Show table

  t->resize(2*t->table()->horizontalHeader()->sectionSize(0) + 55,
      (QMIN(10,t->numRows())+1)*t->table()->verticalHeader()->sectionSize(0)+100);
  //t->askOnCloseEvent(false);
  t->setAttribute(Qt::WA_DeleteOnClose);
  t->showNormal();

  // Do not create graph if there is only one value in the table
  if (t->numRows() < 2) return;

  QStringList colNames;
  if (filter && flt.filter())
  {
    colNames <<t->colName(3);
  }
  colNames << t->colName(1);
  MultiLayer *ml = appWindow()->multilayerPlot(t,colNames,Graph::Line);
  // ml->askOnCloseEvent(false);
  ml->setAttribute(Qt::WA_DeleteOnClose);

  Graph* g = ml->activeGraph();

  // Set x-axis label format
  if (useAbsoluteDate)
  {
    Mantid::Kernel::DateAndTime label_as_ptime = flt.data()->nthInterval(0).begin();
    QDateTime dt = QDateTime::fromTime_t( label_as_ptime.to_localtime_t() );
    QString format = dt.toString(Qt::ISODate) + ";HH:mm:ss";
    g->setLabelsDateTimeFormat(2,ScaleDraw::Date,format);
  }
  else
  {
    //Make the x-axis a numeric format, 0 decimals
    g->setLabelsNumericFormat(2,1,0, "");
  }

  // Set style #3 (HorizontalSteps) for curve iValueCurve
  g->setCurveStyle(iValueCurve,3);
  QPen pn = QPen(Qt::black);
  g->setCurvePen(iValueCurve, pn);

  if (filter && flt.filter())
  {
    QwtPlotCurve *c = g->curve(iFilterCurve);
    // Set the right axis as Y axis for the filter curve.
    c->setAxis(2,1);
    // Set style #3 (HorizontalSteps) for curve 1
    // Set scale of right Y-axis (#3) from 0 to 1
    g->setCurveStyle(iFilterCurve,3);
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
    @param tableName :: Table name
    @param workspace :: Shared pointer to the workspace
    @param indexList :: A list of spectra indices to go to the table
    @param errs :: If true include the errors into the table
    @param binCentres :: If true the X column will contain the bin centres, i.e. (x_i+1 + x_i)/2.
       If false the Y values will be in the same row with the left bin boundaries.
       If the workspace is not a histogram the parameter is ignored.
 */
Table* MantidUI::createTableFromSpectraList(const QString& tableName, Mantid::API::MatrixWorkspace_sptr workspace, QList<int> indexList, bool errs, bool binCentres)
{
  int nspec = workspace->getNumberHistograms();
  //Loop through the list of index and remove all the indexes that are out of range

  for(QList<int>::iterator it=indexList.begin();it!=indexList.end();it++)
  {
    if ((*it) > nspec || (*it) < 0) indexList.erase(it);
  }
  if ( indexList.empty() ) return 0;

  int c = errs?2:1;
  int numRows = workspace->blocksize();
  bool isHistogram = workspace->isHistogramData();
  int no_cols = static_cast<int>(indexList.size());
  Table* t = new Table(appWindow()->scriptingEnv(), numRows, (1+c)*no_cols, "", appWindow(), 0);
  appWindow()->initTable(t, appWindow()->generateUniqueName(tableName+"-"));
  // t->askOnCloseEvent(false);

  int kX(0),kY(0),kErr(0);
  for(int i=0;i < no_cols; i++)
  {
    const Mantid::MantidVec& dataX = workspace->readX(indexList[i]);
    const Mantid::MantidVec& dataY = workspace->readY(indexList[i]);
    const Mantid::MantidVec& dataE = workspace->readE(indexList[i]);

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
  // t->askOnCloseEvent(false);
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
  const QList<int>& indexList = m->getSelectedRows();
  if (indexList.empty()) return NULL;

  return createTableFromSpectraList(m->name(), m->workspace(), indexList, errs, binCentres);
}

/**  Create a 1d graph from a Table.
     @param t :: Pointer to the Table.
     @param type :: Type of the curve. Possible values are:
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
  //ml->askOnCloseEvent(false);

  return ml;
}

/** Set properties of a 1d graph which plots spectrum data from a workspace such as the title and axes captions.
    @param ml :: MultiLayer plot with the graph
    @param wsName :: Workspace Name
 */
void MantidUI::setUpSpectrumGraph(MultiLayer* ml, const QString& wsName)
{
  Mantid::API::MatrixWorkspace_sptr workspace =
      boost::dynamic_pointer_cast<MatrixWorkspace>(
          AnalysisDataService::Instance().retrieve(wsName.toStdString())
      );
  Graph* g = ml->activeGraph();
  g->setTitle(tr("Workspace ")+wsName);
  Mantid::API::Axis* ax;
  ax = workspace->getAxis(0);
  std::string s;
  if (ax->unit() && ax->unit()->unitID() != "Empty" )
  {
    s = ax->unit()->caption();
    if ( !ax->unit()->label().empty() )
    {
      s += " / " + ax->unit()->label();
    }
  }
  else if (!ax->title().empty())
  {
    s = ax->title();
  }
  else
  {
    s = "X axis";
  }
  g->setXAxisTitle(tr(s.c_str()));
  g->setYAxisTitle(tr(workspace->YUnitLabel().c_str()));
  g->setAntialiasing(false);
  g->setAutoScale();
}

/** Set properties of a 1d graph which plots bin data from a workspace.
    @param ml :: MultiLayer plot with the graph
    @param Name :: Name of the graph
    @param workspace :: The workspace
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
  g->setYAxisTitle(tr(workspace->YUnitLabel().c_str()));
  g->setAntialiasing(false);
}

/** Create a 1d graph from the specified spectra in a MatrixWorkspace
    @param wsName :: Workspace name
    @param indexList :: A list of spectra indices to be shown in the graph
    @param errs :: If true include the errors on the graph
 */
MultiLayer* MantidUI::plotSpectraList(const QString& wsName, const std::set<int>& indexList, bool errs)
{
  // Convert the list into a map (with the same workspace as key in each case)
  QMultiMap<QString,int> pairs;
  // Need to iterate through the set in reverse order
  std::set<int>::const_reverse_iterator it;
  for (it = indexList.rbegin(); it != indexList.rend(); ++it)
  {
    pairs.insert(wsName,*it);
  }

  // Pass over to the overloaded method
  return plotSpectraList(pairs,errs);
}

/** Create a 1d graph form a set of workspace-spectrum pairs
    @param toPlot :: A list of spectra indices to be shown in the graph
    @param errs :: If true include the errors to the graph
 */
MultiLayer* MantidUI::plotSpectraList(const QMultiMap<QString,int>& toPlot, bool errs)
{
  const QString& firstWorkspace = toPlot.constBegin().key();
  MultiLayer* ml = appWindow()->multilayerPlot(appWindow()->generateUniqueName(firstWorkspace+"-"));
  //ml->askOnCloseEvent(false);
  ml->setCloseOnEmpty(true);
  Graph *g = ml->activeGraph();
  if (!g)
    return 0;

  connect(g,SIGNAL(curveRemoved()),ml,SLOT(maybeNeedToClose()));
  appWindow()->setPreferences(g);
  g->newLegend("");
  for(QMultiMap<QString,int>::const_iterator it=toPlot.begin();it!=toPlot.end();it++)
  {
    try {
      new MantidCurve(it.key(),g,"spectra",it.value(),errs);
    } 
    catch (Mantid::Kernel::Exception::NotFoundError&) 
    {
      // Get here if workspace name is invalid
      const std::string message("Workspace "+it.key().toStdString()+" not found");
      logMessage(Poco::Message("MantidPlot",message,Poco::Message::PRIO_WARNING));
    } catch (std::invalid_argument& ex) {
      // Get here if invalid spectrum number given
      logMessage(Poco::Message("MantidPlot",ex.what(),Poco::Message::PRIO_WARNING));
    }
  }

  // If no spectra have been plotted, close the window (unfortunately it will flash up briefly)
  if ( g->curves() == 0 )
  {
    ml->close();
    return NULL;
  }

  setUpSpectrumGraph(ml,firstWorkspace);
  return ml;

}

/**
 * Draw a color fill plot for each of the listed workspaces. Unfortunately the plotting is 
 * initimately linked to MantidMatrix so that one of these needs to be created first
 * @param wsNames :: For each workspace listed create a 2D colorfill plot 
 * @param curveType :: The curve type for each of the plots
 */
void MantidUI::drawColorFillPlots(const QStringList & wsNames, Graph::CurveType curveType)
{
  for( QStringList::const_iterator cit = wsNames.begin(); cit != wsNames.end(); ++cit )
  {
    this->drawSingleColorFillPlot(*cit, curveType);
  }
}

/**
 * Draw a single ColorFill plot for the named workspace
 * @param wsName :: The name of the workspace which provides data for the plot
 * @param curveType :: The type of curve
 * @returns A pointer to the created plot
 */
MultiLayer* MantidUI::drawSingleColorFillPlot(const QString & wsName, Graph::CurveType curveType)
{
  MantidMatrix *matrix =  importMatrixWorkspace(wsName, -1, -1, false,false);
  if(matrix)
  {
    return matrix->plotGraph2D(curveType);
  }
  return NULL;
}

/** Create a 1d graph form specified spectra in a MatrixWorkspace
    @param wsName :: Workspace name
    @param workspace :: Shared pointer to the workspace
    @param i0 :: Starting index
    @param i1 :: Last index
    @param errs :: If true include the errors to the graph
    @param binCentres ::  If the workspace is a histogram binCentres defines the way the plot is drawn.
         If true it is a line going through the bin centres. Otherwise it will be made of horizontal steps
    @param tableVisible :: Visibility flag for the Table with the plotted data.
 */
MultiLayer* MantidUI::plotSpectraRange(const QString& wsName, int i0, int i1, bool errs)
{
  if (i0 < 0 || i1 < 0) return 0;
  /** For instrument with one to many spectra-detector mapping,
   * different pixels with correspond to the same specta so
   * we need to remove doublons in this case.
   */
  std::set<int> indexList;
  for(int i=i0;i<=i1;i++)
    indexList.insert(i);

  return plotSpectraList(wsName,indexList,errs);
}

/**  Create a graph and plot the selected rows of a MantidMatrix
     @param m :: Mantid matrix
     @param errs :: True if the errors to be plotted
 */
MultiLayer* MantidUI::plotSelectedRows(MantidMatrix *m, bool errs)
{
  const QList<int>& rows = m->getSelectedRows();
  std::set<int> rowSet(rows.constBegin(),rows.constEnd());

  return plotSpectraList(m->workspaceName(),rowSet,errs);
}

Table* MantidUI::createTableFromBins(const QString& wsName, Mantid::API::MatrixWorkspace_sptr workspace, const QList<int>& bins, bool errs, int fromRow, int toRow)
{
  if (bins.empty()) return NULL;

  int c = errs?2:1;
  int numRows = workspace->getNumberHistograms();

  int j0 = fromRow >= 0? fromRow : 0;
  int j1 = toRow   >= 0? toRow   : numRows - 1;

  if (j0 >= numRows || j1 >= numRows) return NULL;

  Table* t = new Table(appWindow()->scriptingEnv(), numRows, c*bins.size() + 1, "", appWindow(), 0);
  appWindow()->initTable(t, appWindow()->generateUniqueName(wsName + "-"));
  // t->askOnCloseEvent(false);
  int kY,kErr;
  for(int i = 0; i < bins.size(); i++)
  {
    kY = c*i+1;
    t->setColName(kY,"YB"+QString::number(bins[i]));
    if (errs)
    {
      kErr = 2*i + 2;
      t->setColPlotDesignation(kErr,Table::yErr);
      t->setColName(kErr,"EB"+QString::number(bins[i]));
    }
    for(int j = j0; j <= j1; j++)
    {
      const Mantid::MantidVec& dataY = workspace->readY(j);
      const Mantid::MantidVec& dataE = workspace->readE(j);

      if (i == 0)
      {
        // Get the X axis values from the vertical axis of the workspace
        if (workspace->axes() > 1) t->setCell(j,0,(*workspace->getAxis(1))(j));
        else t->setCell(j,0,j);
      }
      t->setCell(j,kY,dataY[bins[i]]);
      if (errs) t->setCell(j,kErr,dataE[bins[i]]);
    }
  }
  return t;
}

Table* MantidUI::createTableFromSelectedColumns(MantidMatrix *m, bool errs)
{
  const QList<int>& cols = m->getSelectedColumns();
  if (cols.empty()) return 0;

  int j0 = m->workspaceIndex(0);
  int j1 = m->workspaceIndex(m->numRows()-1);

  return createTableFromBins(m->name(), m->workspace(), cols, errs, j0, j1);

}

MultiLayer* MantidUI::createGraphFromSelectedColumns(MantidMatrix *m, bool errs, bool tableVisible)
{
  Table *t = createTableFromSelectedColumns(m,errs);
  //t->askOnCloseEvent(false);
  if (!t) return NULL;
  if (tableVisible) t->showNormal();

  MultiLayer* ml = appWindow()->multilayerPlot(t,t->colNames(),Graph::Line);
  Graph *g = ml->activeGraph();
  appWindow()->polishGraph(g,Graph::Line);
  m->setBinGraph(ml,t);
  // ml->askOnCloseEvent(false);

  return ml;
}
/** Saves data to  nexus file
 * @param wsName :: Name of the workspace to be saved
 * @param fileName :: name of the nexus file to created
 */
void MantidUI::savedatainNexusFormat(const std::string& fileName,const std::string & wsName)
{ 
  try
  {
    Mantid::API::IAlgorithm_sptr alg =CreateAlgorithm("SaveNexusProcessed");
    alg->setPropertyValue("Filename",fileName);
    alg->setPropertyValue("InputWorkspace",wsName);
    alg->execute();
  }
  catch(...)
  {
  }
}
/** Loads data from nexus file
 * @param wsName :: Name of the workspace to be created
 * @param fileName :: name of the nexus file
 */
void MantidUI::loaddataFromNexusFile(const std::string& wsName,const std::string& fileName,bool project)
{
  if(fileName.empty()) return ;
  try
  {
    Mantid::API::IAlgorithm_sptr alg =CreateAlgorithm("LoadNexus");
    alg->setPropertyValue("Filename",fileName);
    alg->setPropertyValue("OutputWorkspace",wsName);
    if(project)alg->execute();
    else executeAlgorithmAsync(alg);
  }
  catch(...)
  {
  }
}
/** Loads data from raw file
 * @param wsName :: Name of the workspace to be created
 * @param fileName :: name of the raw file
 */
void MantidUI::loadadataFromRawFile(const std::string& wsName,const std::string& fileName,bool project)
{
  if(fileName.empty()) return ;
  try
  {
    Mantid::API::IAlgorithm_sptr alg =CreateAlgorithm("LoadRaw");
    alg->setPropertyValue("Filename",fileName);
    alg->setPropertyValue("OutputWorkspace",wsName);
    if(project)alg->execute();
    else executeAlgorithmAsync(alg);
  }
  catch(...)
  {
  }
}
MantidMatrix* MantidUI::openMatrixWorkspace(ApplicationWindow* parent,const QString& wsName,int lower,int upper)
{
  (void) parent; //Avoid compiler warning

  MatrixWorkspace_sptr ws;
  if (AnalysisDataService::Instance().doesExist(wsName.toStdString()))
  {
    ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wsName.toStdString()));
  }

  if (!ws.get())return 0 ;

  MantidMatrix* w = 0;
  w = new MantidMatrix(ws, appWindow(), "Mantid",wsName, lower, upper);
  if ( !w ) return 0;

  connect(w, SIGNAL(closedWindow(MdiSubWindow*)), appWindow(), SLOT(closeWindow(MdiSubWindow*)));
  connect(w,SIGNAL(hiddenWindow(MdiSubWindow*)),appWindow(), SLOT(hideWindow(MdiSubWindow*)));
  connect (w,SIGNAL(showContextMenu()),appWindow(),SLOT(showWindowContextMenu()));

  appWindow()->d_workspace->addSubWindow(w);
  w->showNormal();
  return w;
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


  std::cerr<<"count FREE = "<<std::dec<<double(free)/1024/1024<<'\n';
  std::cerr<<"count RESERVED = "<<double(reserved)/1024/1024<<'\n';
  std::cerr<<"count COMMITTED = "<<double(committed)/1024/1024<<'\n';

  std::cerr<<"max FREE = "<<std::dec<<double(free_max)/1024/1024<<'\n';
  std::cerr<<"max RESERVED = "<<double(reserved_max)/1024/1024<<'\n';
  std::cerr<<"max COMMITTED = "<<double(committed_max)/1024/1024<<'\n';
  std::cerr<<'\n';
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
  QImage image(colNum,rowNum,QImage::Format_Mono);
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
  appWindow()->plotSpectrogram(m, Graph::ColorMap);
}

void MantidUI::memoryImage2()
{
  //ofstream ofil("memory.txt");
  vector<mem_block> mem;
  int total;
  countVirtual(mem,total);
  int colNum = 1024;
  int rowNum = total/1024/colNum;
  int row = 0;
  int col = 0;
  QImage image(colNum,rowNum,QImage::Format_Mono);
  for(vector<mem_block>::iterator b=mem.begin();b!=mem.end();b++)
  {
    int n = b->size/1024;
    for(int i=0;i<n;i++)
    {
      if (row < rowNum && col < colNum)
      {
        image.setPixel(col,row,b->state > 600);
      }
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
  image.save("memory_image.jpg");
}

#endif
//=======================================================================
// End of Windows specfic stuff
//=======================================================================

#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidGeometry/Instrument/CompAssembly.h"

void MantidUI::test()
{
  std::cerr<<"\nTest\n\n";

  Mantid::API::MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(getSelectedWorkspace());
  if (ws)
  {
    boost::shared_ptr<Mantid::Geometry::Instrument> instr = ws->getBaseInstrument();
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
