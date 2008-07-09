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

class MantidUI:public QObject
{

    Q_OBJECT

public:
    MantidUI(ApplicationWindow *aw);
    void init();
    QStringList getWorkspaceNames();
    QStringList getAlgorithmNames();
    Mantid::API::Workspace_sptr LoadIsisRawFile(const QString& fileName, const QString& workspaceName);
    Mantid::API::IAlgorithm* CreateAlgorithm(const QString& algName);
    bool deleteWorkspace(const QString& workspaceName);
    ApplicationWindow *appWindow(){return m_appWindow;}
    QString getSelectedWorkspaceName();
    Mantid::API::Workspace_sptr getSelectedWorkspace();
    Mantid::API::Workspace_sptr getWorkspace(const QString& workspaceName);
    int getBinNumber(const QString& workspaceName);
    int getHistogramNumber(const QString& workspaceName);
    bool menuAboutToShow(QMdiSubWindow *w);
    Graph3D *plot3DMatrix(int style);
    MultiLayer *plotSpectrogram(Graph::CurveType type);
    void removeWindowFromLists(MdiSubWindow* w);
    void showContextMenu(QMenu& cm, MdiSubWindow* w);
    Table* createTableFromSelectedRows(MantidMatrix *m, bool vis = true, bool errs = true);
    void createGraphFromSelectedRows(MantidMatrix *m, bool vis = true, bool errs = true);
    bool drop(QDropEvent* e);

    void update();

public slots:

    void tst();
    void loadWorkspace();
    void deleteWorkspace();
    void importWorkspace();
    void copyRowToTable();
    void copyRowToGraph();
    void copyRowToGraphErr();

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
    QAction *actionCopyRowToTable;
    QAction *actionCopyRowToGraph;
    QAction *actionCopyRowToGraphErr;
    QAction *actionToggleMantid;
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
