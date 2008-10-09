#ifndef MANTIDUI_H
#define MANTIDUI_H

#include "../ApplicationWindow.h"
#include "../Graph.h"
#include "ProgressDlg.h"
#include "MantidLog.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Workspace.h"

#include <Poco/NObserver.h>

#include <QDockWidget>
#include <QTreeWidget>
#include <QProgressDialog>
#include <QMap>

class Graph3D;
class ScriptingEnv;
class MantidMatrix;
class MantidDockWidget;
class AlgorithmDockWidget;
class AlgorithmMonitor;

using namespace Mantid::API;

class MantidUI:public QObject
{

    Q_OBJECT

public:
    MantidUI(ApplicationWindow *aw);
    ~MantidUI();
    void init();
    // Pointer to QtiPLot main window
    ApplicationWindow *appWindow(){return m_appWindow;}
    QString releaseDate();

    // Returns a list of open workspaces
    QStringList getWorkspaceNames();
    // Returns a list of registered algorithms
    QStringList getAlgorithmNames();
    void LoadIsisRawFile(const QString& fileName, const QString& workspaceName,const QString& spectrum_min,const QString& spectrum_max);
    Mantid::API::IAlgorithm* CreateAlgorithm(const QString& algName);
    // Gets a pointer to workspace workspaceName
    Mantid::API::Workspace_sptr getWorkspace(const QString& workspaceName);

    // Deletes workspace from QtiPlot
    bool deleteWorkspace(const QString& workspaceName);
    // Returns the name of selected workspace in exploreMantid window
    QString getSelectedWorkspaceName();
    // Returns the pointer of workspace selected in exploreMantid window
    Mantid::API::Workspace_sptr getSelectedWorkspace();

    void getSelectedAlgorithm(QString& algName, int& version);
    int getBinNumber(const QString& workspaceName);
    int getHistogramNumber(const QString& workspaceName);
    // Adjusts QtiPlot's main menu
    bool menuAboutToShow(QMdiSubWindow *w);
    // Creates a 3D plot in QtiPlot if the active window is a MantidMatrix
    Graph3D *plot3DMatrix(int style);
    // Creates a 2D plot in QtiPlot if the active window is a MantidMatrix
    MultiLayer *plotSpectrogram(Graph::CurveType type);
    // Removes referances to MantidMatrix w in QtiPlot
    void removeWindowFromLists(MdiSubWindow* w);
    // Prepares the contex menu for MantidMatrix
    void showContextMenu(QMenu& cm, MdiSubWindow* w);
    // Copies selected rows from m to Y and errY columns of a new Table. If vis == true the table is visible,
    // if errs == true errors are copied
    Table* createTableFromSelectedRows(MantidMatrix *m, bool vis = true, bool errs = true, bool forPlotting = false);
    void createGraphFromSelectedRows(MantidMatrix *m, bool vis = true, bool errs = true);
    Table* createTableDetectors(MantidMatrix *m);

    Table* createTableFromSelectedRows(const QString& wsName, Mantid::API::Workspace_sptr workspace, int i0, int i1, bool errs=true, bool forPlotting=false);

    // Handles workspace drop operation to QtiPlot (imports the workspace to MantidMatrix)
    bool drop(QDropEvent* e);

signals:

    void needsUpdating();
    void needToCloseProgressDialog();
    void needToUpdateProgressDialog(int ip);
    void needToCreateLoadDAEMantidMatrix(const Mantid::API::Algorithm*);
    void needToShowCritical(const QString&);

public slots:

    void logMessage(const Poco::Message& msg);
    void tst();
    void tst(MdiSubWindow* w);
    // Updates Mantid user interfase
    void update();
    void loadWorkspace();
    void loadDAEWorkspace();
    void deleteWorkspace();
    MantidMatrix *importWorkspace(const QString& wsName, bool showDlg = true);
    void importWorkspace();
    void copyRowToTable();
    void copyRowToGraph();
    void copyRowToGraphErr();
    void executeAlgorithm();
    void executeAlgorithm(QString algName, int version);
    void copyDetectorsToTable();
    void copyValues();
    void closeProgressDialog();
    void updateProgressDialog(int ip);
    void cancelAsyncAlgorithm();
    void backgroundAsyncAlgorithm();
    void createLoadDAEMantidMatrix(const Mantid::API::Algorithm*);
    void showCritical(const QString&);
    void showAlgMonitor();

	void mantidMenuAboutToShow();
	void manageMantidWorkspaces();
	void showMantidInstrument();
    void insertMenu();

    void plotInstrumentSpectrum(const QString&,int);

public:
    void executeAlgorithmAsync(Mantid::API::Algorithm* alg, bool showDialog = true);

    void handleAlgorithmFinishedNotification(const Poco::AutoPtr<Mantid::API::Algorithm::FinishedNotification>& pNf);
    Poco::NObserver<MantidUI, Mantid::API::Algorithm::FinishedNotification> m_finishedObserver;

    void handleLoadDAEFinishedNotification(const Poco::AutoPtr<Mantid::API::Algorithm::FinishedNotification>& pNf);
    Poco::NObserver<MantidUI, Mantid::API::Algorithm::FinishedNotification> m_finishedLoadDAEObserver;

    void handleAlgorithmProgressNotification(const Poco::AutoPtr<Mantid::API::Algorithm::ProgressNotification>& pNf);
    Poco::NObserver<MantidUI, Mantid::API::Algorithm::ProgressNotification> m_progressObserver;

    void handleAddWorkspace(WorkspaceAddNotification_ptr pNf);
    Poco::NObserver<MantidUI, WorkspaceAddNotification> m_addObserver;

    void handleAlgorithmErrorNotification(const Poco::AutoPtr<Mantid::API::Algorithm::ErrorNotification>& pNf);
    Poco::NObserver<MantidUI, Mantid::API::Algorithm::ErrorNotification> m_errorObserver;

    void handleReplaceWorkspace(WorkspaceReplaceNotification_ptr pNf);
    Poco::NObserver<MantidUI, WorkspaceReplaceNotification> m_replaceObserver;

    void handleDeleteWorkspace(WorkspaceDeleteNotification_ptr pNf);
    Poco::NObserver<MantidUI, WorkspaceDeleteNotification> m_deleteObserver;

    ApplicationWindow *m_appWindow;
    MantidDockWidget *m_exploreMantid;
    AlgorithmDockWidget *m_exploreAlgorithms;
    QAction *actionCopyRowToTable;
    QAction *actionCopyRowToGraph;
    QAction *actionCopyRowToGraphErr;
    QAction *actionToggleMantid;
    QAction *actionToggleAlgorithms;
    QAction *actionCopyDetectorsToTable;
    QAction *actionCopyValues;

	//Mantid
	QMenu *mantidMenu;
    AlgorithmMonitor *m_algMonitor;

    ProgressDlg *m_progressDialog;   //< Progress of algorithm running asynchronously
    Mantid::API::Algorithm* m_algAsync;  //< asynchronously running algorithm. Zero if there are no running algorithms

    // Variables to hold DAE parameters while LoadDAE is running asynchronously
    struct DAEstruct
    {
        QString m_WorkspaceName;
        QString m_HostName;
        QString m_SpectrumMin;
        QString m_SpectrumMax;
        QString m_SpectrumList;
        int m_UpdateInterval;
    };
    QMap<const Mantid::API::Algorithm*,DAEstruct> m_DAE_map;


};

static const char * mantid_matrix_xpm[] = { 
"13 12 20 1",
" 	c None",
".	c #000821",
"+	c #000720",
"@	c #000927",
"#	c #000C39",
"$	c #000C37",
"%	c #000929",
"&	c #00041B",
"*	c #000519",
"=	c #597384",
"-	c #5A7387",
";	c #00061E",
">	c #4C7799",
",	c #517595",
"'	c #000212",
")	c #000209",
"!	c #000207",
"~	c #00030C",
"{	c #010101",
"]	c #8080FF",
".++@#$%++%#$&",
"*=-;>>;==;>,'",
")!))~~~!))~){",
"{]]{]]{]]{]]{",
"{]]{]]{]]{]]{",
"{{{{{{{{{{{{{",
"{]]{]]{]]{]]{",
"{]]{]]{]]{]]{",
"{{{{{{{{{{{{{",
"{]]{]]{]]{]]{",
"{]]{]]{]]{]]{",
"{{{{{{{{{{{{{"};

static const char * mantid_xpm[] = { 
"13 12 3 1",
" 	c None",
".	c #000821",
"+	c #000720",
"             ",
"             ",
"             ",
"             ",
"             ",
"             ",
"             ",
"             ",
"             ",
"             ",
"             ",
"             "};

#endif
