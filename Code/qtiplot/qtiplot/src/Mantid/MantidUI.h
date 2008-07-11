#ifndef MANTIDUI_H
#define MANTIDUI_H

#include "../ApplicationWindow.h"
#include "../Graph.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataHandling/LoadRaw.h"

#include <QDockWidget>
#include <QTreeWidget>

class Graph3D;
class ScriptingEnv;
class MantidMatrix;
class MantidDockWidget;
class AlgorithmDockWidget;

class MantidUI:public QObject
{

    Q_OBJECT

public:
    MantidUI(ApplicationWindow *aw);
    void init();
    // Pointer to QtiPLot main window
    ApplicationWindow *appWindow(){return m_appWindow;}

    // Returns a list of open workspaces
    QStringList getWorkspaceNames();
    // Returns a list of registered algorithms
    QStringList getAlgorithmNames();
    Mantid::API::Workspace_sptr LoadIsisRawFile(const QString& fileName, const QString& workspaceName);
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
    Table* createTableFromSelectedRows(MantidMatrix *m, bool vis = true, bool errs = true);
    void createGraphFromSelectedRows(MantidMatrix *m, bool vis = true, bool errs = true);

    // Handles workspace drop operation to QtiPlot (imports the workspace to MantidMatrix)
    bool drop(QDropEvent* e);
    // Updates Mantid user interfase
    void update();

public slots:

    void tst();
    void loadWorkspace();
    void deleteWorkspace();
    void importWorkspace();
    void copyRowToTable();
    void copyRowToGraph();
    void copyRowToGraphErr();
    void executeAlgorithm();
public:
    ApplicationWindow *m_appWindow;
    QMdiArea *d_workspace;// ApplicationWindow's private member
    QMenuBar *aw_menuBar;
    QMenu *aw_plot2DMenu, *aw_plot3DMenu;
    QToolBar *aw_plotMatrixBar;
    ScriptingEnv *aw_scriptEnv;
    QMenu *aw_view;
    QAction *aw_actionShowUndoStack;

    MantidDockWidget *m_exploreMantid;
    AlgorithmDockWidget *m_exploreAlgorithms;
    QAction *actionCopyRowToTable;
    QAction *actionCopyRowToGraph;
    QAction *actionCopyRowToGraphErr;
    QAction *actionToggleMantid;
    QAction *actionToggleAlgorithms;
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
