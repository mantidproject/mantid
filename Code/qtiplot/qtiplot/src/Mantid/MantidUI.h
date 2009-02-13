#ifndef MANTIDUI_H
#define MANTIDUI_H

//----------------------------------
// Includes
//----------------------------------
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
#include <QMutex>

//----------------------------------
// Forward declarations
//----------------------------------
class Graph3D;
class ScriptingEnv;
class MantidMatrix;
class MantidDockWidget;
class AlgorithmDockWidget;
class AlgorithmMonitor;

using namespace Mantid::API;

/** 
    MantidUI is the extension of QtiPlot's ApplicationWindow which deals with Mantid framework.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 02/07/2008

    Copyright &copy; 2008 STFC Rutherford Appleton Laboratories

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>    
*/

class MantidUI:public QObject
{

    Q_OBJECT

public:
    // Constructor
    MantidUI(ApplicationWindow *aw);

    // Destructor
    ~MantidUI();
    
    // Initialization
    void init();
    
    // Pointer to QtiPLot main window
    ApplicationWindow *appWindow(){return m_appWindow;}
    
    // Release date string formed during scons building
    QString releaseDate();

    // Returns a list of open workspaces
    QStringList getWorkspaceNames();
    
    // Returns a list of registered algorithms
    QStringList getAlgorithmNames();
    
    // Load a workspace from a raw file.
    void LoadIsisRawFile(const QString& fileName, const QString& workspaceName,const QString& spectrum_min,const QString& spectrum_max
                               ,const QString& spectrum_list,const QString& cache);
    // Create an algorithm using Mantid FrameworkManager
    Mantid::API::IAlgorithm* CreateAlgorithm(const QString& algName);
    
    // Gets a pointer to workspace workspaceName
    Mantid::API::Workspace_sptr getWorkspace(const QString& workspaceName);

    // Deletes workspace from QtiPlot
    bool deleteWorkspace(const QString& workspaceName);
    
    // Returns the name of selected workspace in exploreMantid window
    QString getSelectedWorkspaceName();
    
    // Returns the pointer of workspace selected in exploreMantid window
    Mantid::API::Workspace_sptr getSelectedWorkspace();
    
    // Returns the name and version of the algorithm selected in algorithm dock window
    void getSelectedAlgorithm(QString& algName, int& version);
    
    // Returns the number of bins in a workspace (do we need it?)
    int getBinNumber(const QString& workspaceName);
    
    // Returns the number of histograms in a workspace (do we need it?)
    int getHistogramNumber(const QString& workspaceName);
    
    // Adjusts QtiPlot's main menu if a MantidMatrix becomes active (receives focus)
    bool menuAboutToShow(QMdiSubWindow *w);
    
    // Creates a 3D plot in QtiPlot if the active window is a MantidMatrix
    Graph3D *plot3DMatrix(int style);
    
    // Creates a 2D plot in QtiPlot if the active window is a MantidMatrix
    MultiLayer *plotSpectrogram(Graph::CurveType type);
    
    // Removes references to MantidMatrix w in QtiPlot (called when matrix closes)
    void removeWindowFromLists(MdiSubWindow* w);
    
    // Prepares the contex menu for MantidMatrix
    void showContextMenu(QMenu& cm, MdiSubWindow* w);
    
    // Copies selected rows from m to Y and errY columns of a new Table. If vis == true the table is visible,
    // if errs == true errors are copied
    Table* createTableFromSelectedRows(MantidMatrix *m, bool vis = true, bool errs = true, bool forPlotting = false);

    // Shows 1D graphs of the spectra (rows) selected in a MantidMatrix
    void createGraphFromSelectedRows(MantidMatrix *m, bool vis = true, bool errs = true);

    // Creates and shows a Table with detector ids for the workspace in the MantidMatrix
    Table* createTableDetectors(MantidMatrix *m);

    // Copies selected columns (time bins) in a MantidMatrix to a Table
    Table* createTableFromSelectedColumns(MantidMatrix *m, bool visible, bool errs);

    // Shows 1D graphs of selected time bins (columns) in a MantidMatrix
    void createGraphFromSelectedColumns(MantidMatrix *m, bool vis = true, bool errs = true);

    // Copy to a Table Y-values (and Err-values if errs==true) of spectra with indeces from i0 to i1 (inclusive) from a workspace
    Table* createTableFromSelectedRows(const QString& wsName, Mantid::API::MatrixWorkspace_sptr workspace, int i0, int i1, bool errs=true, bool forPlotting=false);

    // Copy to a Table Y-values (and Err-values if errs==true) of spectra with indeces in a std::list from a workspace
	Table* createTableFromSelectedRowsList(const QString& wsName, Mantid::API::MatrixWorkspace_sptr workspace, std::vector<int> index, bool errs=true, bool forPlotting=false);

    // Copy to a Table Y-values (and Err-values if errs==true) of bins with indeces from i0 to i1 (inclusive) from a workspace
    Table* createTableFromSelectedColumns(const QString& wsName, Mantid::API::MatrixWorkspace_sptr workspace, int c0, int c1, bool errs=true, bool forPlotting=false);


  // Handles workspace drop operation to QtiPlot (imports the workspace to MantidMatrix)
    bool drop(QDropEvent* e);
#ifdef _WIN32
    // Shows 2D plot of current memory usage.
    void memoryImage();
#endif
  /** ---------------------------------
   * Commands purely for python interaction
   */
  MultiLayer* plotSpectrum(const QString& wsName, int spec, bool showMatrix = false);
  MantidMatrix* getMantidMatrix(const QString& wsName);
  MantidMatrix* newMantidMatrix(const QString& name, int start=-1, int end=-1);
  MultiLayer* plotTimeBin(const QString& wsName, int bin, bool showMatrix = false);
  bool runAlgorithmAsynchronously(const QString & algName);
  bool createPropertyInputDialog(const QString & algName, const QString & message);

public slots:
  void cancelAllRunningAlgorithms();

signals:
  //A signal to indicate that we want a script to produce a dialog
  void showPropertyInputDialog(const QString & algName);
private:
  Mantid::API::Algorithm* findAlgorithmPointer(const QString & algName);

  //-----------------------------------

public:

signals:

    // These signals are to be fired from methods run in threads other than the main one
    // (e.g. handlers of algorithm notifications)

    // Signals that the UI needs to be updated.
    void needsUpdating();

    // Close the small progress dialog
    void needToCloseProgressDialog();

    // Update the small progress dialog
    void needToUpdateProgressDialog(int ip,const QString& msg);

    void needToCreateLoadDAEMantidMatrix(const Mantid::API::Algorithm*);

    // Display a critical error dialog box
    void needToShowCritical(const QString&);
					   
public slots:

    // Display a message in QtiPlot's results window. Used by MantidLog class to display Mantid log information.
    void logMessage(const Poco::Message& msg);
    void tst();
    void tst(MdiSubWindow* w);

    // Updates Mantid the user interface
    void update();

    // Load a workspace from a raw file by running a LoadRaw algorithm with properties supplied through a dialog box.
    void loadWorkspace();

    // Load a workspace from a DAE by running a LoadDAE algorithm with properties supplied through a dialog box.
    void loadDAEWorkspace();

    // Delete workspace selected in the Workspace dock window.
    void deleteWorkspace();

    // Create a MantidMatrix from workspace wsName
    MantidMatrix *importWorkspace(const QString& wsName, bool showDlg = true, bool makeVisible = true);

    // Import the workspace selected in the Workspace dock window
    void importWorkspace();

    // Slots responding to MantidMatrix context menu commands
    void copyRowToTable();
    void copyColumnToTable();
    void copyRowToGraph();
    void copyColumnToGraph();
    void copyRowToGraphErr();
    void copyColumnToGraphErr();
    void copyDetectorsToTable();
    void copyValues();

    // Execute selected algorithm
    void executeAlgorithm();

    // Execute algorithm given name and version
    void executeAlgorithm(QString algName, int version);

    // Close the small progress dialog
    void closeProgressDialog();

    // Update the small progress dialog
    void updateProgressDialog(int ip,const QString& msg);

    // Cancel running algorithm when pressing 'Cancel' button in the small progress dialog
    void cancelAsyncAlgorithm();

    // Close the small progress dialog without stopping the algorithm
    void backgroundAsyncAlgorithm();

    void createLoadDAEMantidMatrix(const Mantid::API::Algorithm*);

    // Show Qt critical error message box
    void showCritical(const QString&);

    // Show the dialog monitoring currently running algorithms
    void showAlgMonitor();

    // Called from ApplicationWindow to customize the main menu 
	void mantidMenuAboutToShow();

	void manageMantidWorkspaces();


	void showMantidInstrument();

    // Show instrument for the selected workspace
	void showMantidInstrumentSelected();

    // Show instrument. Workspace name is passed as the argument
	void showMantidInstrument(const QString&);

    // Show log files for selected workspace
    void showLogFileWindow();

    void insertMenu();

    // Customize MantidMatrix menu.
    void menuMantidMatrixAboutToShow();

    // Plot a spectrum in response from a InstrumentWindow signal
    MultiLayer* plotInstrumentSpectrum(const QString&,int);
	MultiLayer* plotInstrumentSpectrumList(const QString&,std::vector<int>);

	void importSampleLog(const QString & filename, const QString & data, bool numeric);

private:

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

    void handleReplaceWorkspace(WorkspaceAfterReplaceNotification_ptr pNf);
    Poco::NObserver<MantidUI, WorkspaceAfterReplaceNotification> m_replaceObserver;

    void handleDeleteWorkspace(WorkspaceDeleteNotification_ptr pNf);
    Poco::NObserver<MantidUI, WorkspaceDeleteNotification> m_deleteObserver;

    ApplicationWindow *m_appWindow;             // QtiPlot main ApplicationWindow
    MantidDockWidget *m_exploreMantid;          // Dock window for manipulating workspaces
    AlgorithmDockWidget *m_exploreAlgorithms;   // Dock window for using algorithms
    QAction *actionCopyRowToTable;
    QAction *actionCopyRowToGraph;
    QAction *actionCopyRowToGraphErr;
    QAction *actionCopyColumnToTable;
    QAction *actionCopyColumnToGraph;
    QAction *actionCopyColumnToGraphErr;
    QAction *actionToggleMantid;
    QAction *actionToggleAlgorithms;
    QAction *actionCopyDetectorsToTable;
    QAction *actionCopyValues;

	QMenu *mantidMenu;
    QMenu *menuMantidMatrix;             //  ManteidMatrix specific menu
    AlgorithmMonitor *m_algMonitor;      //  Class for monitoring running algorithms

    ProgressDlg *m_progressDialog;       //< Progress of algorithm running asynchronously
    Mantid::API::Algorithm* m_algAsync;  //< asynchronously running algorithm. Zero if there are no running algorithms

    // Structure to hold DAE parameters while LoadDAE is running asynchronously
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
