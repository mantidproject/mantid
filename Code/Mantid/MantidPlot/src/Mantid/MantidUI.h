#ifndef MANTIDUI_H
#define MANTIDUI_H

//----------------------------------
// Includes
//----------------------------------
#include "../ApplicationWindow.h"
#include "../Graph.h"
#include "MantidLog.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidQtAPI/AlgorithmDialog.h"

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
class InstrumentWindow;
class FitPropertyBrowser;

/**
    MantidUI is the extension of QtiPlot's ApplicationWindow which deals with Mantid framework.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 02/07/2008

    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

/// Required by Qt to use Mantid::API::Workspace_sptr as a parameter type in signals
Q_DECLARE_METATYPE(Mantid::API::Workspace_sptr)
Q_DECLARE_METATYPE(Mantid::API::MatrixWorkspace_sptr)
Q_DECLARE_METATYPE(std::string)

class MantidUI:public QObject
{

    Q_OBJECT

public:
    // Constructor
    MantidUI(ApplicationWindow *aw);

    // Destructor
    ~MantidUI();

    //Clear the framework
    void shutdown();

    // Save settings to a persistent store
    void saveSettings() const;

    // Initialization
    void init();

    // Insert relevant items into a menu
    void addMenuItems(QMenu *menu);

    // Pointer to QtiPLot main window
    ApplicationWindow *appWindow(){return m_appWindow;}

    // Release date string formed during scons building
    QString releaseDate();

    // Returns a list of open workspaces
    QStringList getWorkspaceNames();

    // Returns a list of registered algorithms
    QStringList getAlgorithmNames();

    // Create an algorithm using Mantid FrameworkManager
    Mantid::API::IAlgorithm_sptr CreateAlgorithm(const QString& algName);

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

    // Adjusts QtiPlot's main menu if a MantidMatrix becomes active (receives focus)
    bool menuAboutToShow(QMdiSubWindow *w);

    // Removes references to MantidMatrix w in QtiPlot (called when matrix closes)
    void removeWindowFromLists(MdiSubWindow* w);

    // Prepares the contex menu for MantidMatrix
    void showContextMenu(QMenu& cm, MdiSubWindow* w);

    // Handles workspace drop operation to QtiPlot (imports the workspace to MantidMatrix)
    bool drop(QDropEvent* e);

    //  *****      Plotting Methods     *****  //

    // Creates a 3D plot in QtiPlot if the active window is a MantidMatrix
    Graph3D *plot3DMatrix(int style);

    // Creates a 2D plot in QtiPlot if the active window is a MantidMatrix
    MultiLayer *plotSpectrogram(Graph::CurveType type);

    /// Create a Table form specified spectra in a MatrixWorkspace
    Table* createTableFromSpectraList(const QString& tableName, Mantid::API::MatrixWorkspace_sptr workspace, QList<int> indexList, bool errs=true, bool binCentres=false);

    // Copies selected rows from MantidMatrix to Y and errY columns of a new Table.
    Table* createTableFromSelectedRows(MantidMatrix *m, bool errs = true, bool binCentres=false);

    /// Create a 1d graph form a Table
    MultiLayer* createGraphFromTable(Table* t, int type = 0);

    // Shows 1D graphs of the spectra (rows) selected in a MantidMatrix
    MultiLayer* plotSelectedRows(MantidMatrix *m, bool errs = true);

	/// This method executes loadraw algorithm from  ICatInterface
	void loadrawfromICatInterface(const QString& fileName,const QString& wsName);

	/// This method executes loadnexus algorithm from ICatInterface
	void loadnexusfromICatInterface(const QString& fileName,const QString& wsName);
	
	/// This method is to execute download data files algorithm from ICat
	void executeDownloadDataFiles(const std::vector<std::string>& filenNames,const std::vector<long long>& fileIds);

  AlgorithmMonitor* getAlgMinitor(){return m_algMonitor;}

public slots:
    // Create a 1d graph form specified spectra in a MatrixWorkspace
    MultiLayer* plotSpectraList(const QString& wsName, const std::set<int>& indexList, bool errs=false);

public:
    MultiLayer* plotSpectraList(const QMultiMap<QString,int>& toPlot, bool errs=false);
    /// Draw a color fill plot for each of the listed workspaces
    void drawColorFillPlots(const QStringList & wsNames, Graph::CurveType curveType = Graph::ColorMap);
    /// Draw a color fill plot for the named workspace
    MultiLayer* drawSingleColorFillPlot(const QString & wsName, Graph::CurveType curveType = Graph::ColorMap);

    // Create a 1d graph form specified spectra in a MatrixWorkspace
    MultiLayer* plotSpectraRange(const QString& wsName, int i0, int i1, bool errs=true);

    // Set properties of a 1d graph which plots data from a workspace
    static void setUpSpectrumGraph(MultiLayer* ml, const QString& wsName);

    // Set properties of a 1d graph which plots data from a workspace
    static void setUpBinGraph(MultiLayer* ml, const QString& wsName, Mantid::API::MatrixWorkspace_sptr workspace);

    // Copy to a Table Y-values (and Err-values if errs==true) of bins with indeces from i0 to i1 (inclusive) from a workspace
    Table* createTableFromBins(const QString& wsName, Mantid::API::MatrixWorkspace_sptr workspace, const QList<int>& bins, bool errs=true,int fromRow = -1, int toRow = -1);

    // Copies selected columns (time bins) in a MantidMatrix to a Table
    Table* createTableFromSelectedColumns(MantidMatrix *m, bool errs);


    // Shows 1D graphs of selected time bins (columns) in a MantidMatrix
    MultiLayer* createGraphFromSelectedColumns(MantidMatrix *m, bool errs = true, bool tableVisible = false);

    // Creates and shows a Table with detector ids for the workspace in the MantidMatrix
    Table* createTableDetectors(MantidMatrix *m);

	//
	bool executeICatLogout(int version);
	
public slots:
  /// Create a table showing detector information for the given workspace and indices and optionally the data for that detector
  Table* createDetectorTable(const QString & wsName, const std::vector<int>& indices, bool include_data = false);
  //  *****                            *****  //
  void renameWorkspace(QString = "");
public:

  // Return pointer to the fit function property browser
  FitPropertyBrowser* fitFunctionBrowser(){return m_fitFunction;}

  /** ---------------------------------
   * Commands purely for python interaction
   */
    // The Python API wouldn't accept a multimap as a type so had to resort to this which is still more efficient than
    // the old merge plots method
  MultiLayer* pyPlotSpectraList(const QList<QString>& wsnames, const QList<int>& spec_list, bool errs=true); 
  MultiLayer* mergePlots(MultiLayer* g1, MultiLayer* g2);
  MantidMatrix* getMantidMatrix(const QString& wsName);
  MantidMatrix* newMantidMatrix(const QString& name, int start=-1, int end=-1);
  MultiLayer* plotBin(const QString& wsName, int bin, bool errors = false);
  bool runAlgorithmAsync_PyCallback(const QString & algName);
  bool createPropertyInputDialog(const QString & alg_name, const QString & preset_values,
				 const QString & optional_msg,  const QString & enabled_names);
/// Group selected workspaces
  void groupWorkspaces();
  /// UnGroup selected groupworkspace
  void ungroupWorkspaces();
  /** save the workspace data to nexus file
      This method is useful when a project is saved from mantidplot
  */
  void savedatainNexusFormat(const std::string& fileName,const std::string & wsName);
  
  /** load data from nexus file.This method is useful 
      when a project is opened  from mantidplot
  */
  void loaddataFromNexusFile(const std::string& wsname,const std::string& fileName,bool project=false);
  void loadadataFromRawFile(const std::string& wsname,const std::string& fileName,bool project=false);
  
  MantidMatrix* openMatrixWorkspace(ApplicationWindow* parent,const QString& wsName,int lower,int upper);
  
  void saveProject(bool save);
  void enableSaveNexus(const QString & wsName);

  //This is anoverloaded method toexecute load raw/nexus and  called from Icat interface
  void executeloadAlgorithm(const QString&, const QString&, const QString&);

					      
public slots:
  void cancelAllRunningAlgorithms();

signals:
  //A signal to indicate that we want a script to produce a dialog
  void showPropertyInputDialog(const QString & algName);
  // Broadcast that an algorithm is about to be created
  void algorithmAboutToBeCreated();
  // a signal for getting the file locations from ICat downloaddatafiles algorithm
  void fileLocations(const std::vector<std::string>&);
private:
  Mantid::API::IAlgorithm_sptr findAlgorithmPointer(const QString & algName);
  

  //-----------------------------------

public:

signals:

  // These signals are to be fired from methods run in threads other than the main one
  // (e.g. handlers of algorithm notifications)

  // Signals that the UI needs to be updated.
  void workspace_added(const QString &, Mantid::API::Workspace_sptr);
  void workspace_replaced(const QString &, Mantid::API::Workspace_sptr);
  void workspace_removed(const QString &);
  void workspaces_cleared();
  void algorithms_updated();
  void workspace_renamed(const QString &, const QString);
  void workspaces_grouped(const QStringList&);
  void workspace_ungrouped(const QString&, Mantid::API::Workspace_sptr);

    void needToCreateLoadDAEMantidMatrix(const QString&);

    // Display a critical error dialog box
    void needToShowCritical(const QString&);

public slots:

    void test();

    // Display a message in QtiPlot's results window. Used by MantidLog class to display Mantid log information.
    void logMessage(const Poco::Message& msg);

    // Import the workspace selected in the Workspace dock window
    void importWorkspace();
	// #539: For adding Workspace History display to MantidPlot
	void showAlgorithmHistory();

    // Import a workspace wsName
    void importWorkspace(const QString& wsName, bool showDlg = true, bool makeVisible = true);

  // Create a MantidMatrix from workspace wsName
  MantidMatrix *importMatrixWorkspace(const QString& wsName, int lower = -1, int upper = -1,
				      bool showDlg = true, bool makeVisible = true);

    // Create a MantidMatrix from workspace wsName
    Table *importTableWorkspace(const QString& wsName, bool showDlg = true, bool makeVisible = true);

    void createLoadDAEMantidMatrix(const QString&);

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
    void executeAlgorithm(QString algName, int version = -1);
    //Execute an algorithm with the given parameter list
    void executeAlgorithm(const QString & algName, const QString & paramList);
    // Find the name of the first input workspace for an algorithm
    QString findInputWorkspaceProperty(Mantid::API::IAlgorithm_sptr algorithm) const;
    // Show Qt critical error message box
    void showCritical(const QString&);
    // Show the dialog monitoring currently running algorithms
    void showAlgMonitor();
    // Called from ApplicationWindow to customize the main menu
	void mantidMenuAboutToShow();

	void manageMantidWorkspaces();


  //Python related functions
  InstrumentWindow* getInstrumentView(const QString & wsName);

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

    // Show / hide the FitPropertyBrowser
    void showFitPropertyBrowser(bool on = true);

    // Plot a spectrum in response from a InstrumentWindow signal
    MultiLayer* plotInstrumentSpectrum(const QString&,int);
	MultiLayer* plotInstrumentSpectrumList(const QString&,std::set<int>);

	void importString(const QString &logName, const QString &data);
	void importStrSeriesLog(const QString &logName, const QString &data);
	void importNumSeriesLog(const QString &wsName, const QString &logname, int filter);

	// Clear all Mantid related memory
	void clearAllMemory();
	// Ticket #672
	void saveNexusWorkspace();
	QString saveToString(const std::string &workingDir);

#ifdef _WIN32
public:
    // Shows 2D plot of current memory usage.
    void memoryImage();
    void memoryImage2();
#endif


private:

// Create a pointer to the named algorithm and version
    Mantid::API::IAlgorithm_sptr createAlgorithm(const QString& algName, int version);
    // Execute algorithm asinchronously
    void executeAlgorithmAsync(Mantid::API::IAlgorithm_sptr alg);

    // Notification handlers and corresponding observers.
    void handleLoadDAEFinishedNotification(const Poco::AutoPtr<Mantid::API::Algorithm::FinishedNotification>& pNf);
    Poco::NObserver<MantidUI, Mantid::API::Algorithm::FinishedNotification> m_finishedLoadDAEObserver;

    void handleAddWorkspace(Mantid::API::WorkspaceAddNotification_ptr pNf);
    Poco::NObserver<MantidUI, Mantid::API::WorkspaceAddNotification> m_addObserver;

    void handleReplaceWorkspace(Mantid::API::WorkspaceAfterReplaceNotification_ptr pNf);
    Poco::NObserver<MantidUI, Mantid::API::WorkspaceAfterReplaceNotification> m_replaceObserver;

    void handleDeleteWorkspace(Mantid::API::WorkspaceDeleteNotification_ptr pNf);
    Poco::NObserver<MantidUI, Mantid::API::WorkspaceDeleteNotification> m_deleteObserver;

  void handleClearADS(Mantid::API::ClearADSNotification_ptr pNf);
  Poco::NObserver<MantidUI, Mantid::API::ClearADSNotification> m_clearADSObserver;

  void handleAlgorithmFactoryUpdates(Mantid::API::AlgorithmFactoryUpdateNotification_ptr pNf);
  Poco::NObserver<MantidUI, Mantid::API::AlgorithmFactoryUpdateNotification> m_algUpdatesObserver;

  //handles rename workspace notification
   void handleRenameWorkspace(Mantid::API::WorkspaceRenameNotification_ptr pNf);
    Poco::NObserver<MantidUI, Mantid::API::WorkspaceRenameNotification> m_renameObserver;

  //handles notification send by Groupworkspaces algorithm 
  void handleGroupWorkspaces(Mantid::API::WorkspacesGroupedNotification_ptr pNf);
  Poco::NObserver<MantidUI, Mantid::API::WorkspacesGroupedNotification> m_groupworkspacesObserver;

  //handles notification send by UnGroupworkspaces algorithm 
  void handleUnGroupWorkspace(Mantid::API::WorkspaceUnGroupingNotification_ptr pNf);
  Poco::NObserver<MantidUI, Mantid::API::WorkspaceUnGroupingNotification> m_ungroupworkspaceObserver;

	//#678
    //for savenexus algorithm
	void executeSaveNexus(QString algName,int version);

    void copyWorkspacestoVector(const QList<QTreeWidgetItem*> &list,std::vector<std::string> &inputWS);
	void PopulateData(Mantid::API::Workspace_sptr ws_ptr,QTreeWidgetItem*  wsid_item);

	/// This method executes LoadRaw/LoadNexus algorithm from ICat interface
	 MantidQt::API::AlgorithmDialog * createLoadAlgorithmDialog(Mantid::API::IAlgorithm_sptr alg);

	 /// This method accepts user inputs and executes loadraw/load nexus algorithm
	 void executeLoadAlgorithm(MantidQt::API::AlgorithmDialog* dlg,Mantid::API::IAlgorithm_sptr alg);
	

    // Private variables

    ApplicationWindow *m_appWindow;             // QtiPlot main ApplicationWindow
    MantidDockWidget *m_exploreMantid;          // Dock window for manipulating workspaces
    AlgorithmDockWidget *m_exploreAlgorithms;   // Dock window for using algorithms
    FitPropertyBrowser *m_fitFunction;        // Dock window to set fit function properties

    QAction *actionCopyRowToTable;
    QAction *actionCopyRowToGraph;
    QAction *actionCopyRowToGraphErr;
    QAction *actionCopyColumnToTable;
    QAction *actionCopyColumnToGraph;
    QAction *actionCopyColumnToGraphErr;
    QAction *actionToggleMantid;
    QAction *actionToggleAlgorithms;
    QAction *actionToggleFitFunction;
    QAction *actionCopyDetectorsToTable;
    QAction *actionCopyValues;

	  QMenu *mantidMenu;
    QMenu *menuMantidMatrix;             //  MantidMatrix specific menu
    AlgorithmMonitor *m_algMonitor;      //  Class for monitoring running algorithms

    // Map of <workspace_name,update_interval> pairs. Positive update_intervals mean
    // UpdateDAE must be launched after LoadDAE for this workspace
    QMap<std::string,int> m_DAE_map;

    // Stores dependent mdi windows. If the 'key' window closes, all 'value' ones must be closed as well.
    std::multimap<MdiSubWindow*,MdiSubWindow*> m_mdiDependency;

};


#endif
