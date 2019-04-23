// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDUI_H
#define MANTIDUI_H

//----------------------------------
// Includes
//----------------------------------
#include "../ApplicationWindow.h"
#include "../Graph.h"
#include "MantidQtWidgets/Common/IProjectSerialisable.h"
#include "MantidQtWidgets/Common/MantidDisplayBase.h"
#include "MantidQtWidgets/Common/MantidWSIndexDialog.h"

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IPeaksWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/Workspace_fwd.h"

#include "MantidQtWidgets/Common/AlgorithmDialog.h"
#include "MantidQtWidgets/Common/MantidAlgorithmMetatype.h"
#include "MantidQtWidgets/Plotting/Qwt/QwtWorkspaceSpectrumData.h"

#include "MantidPlotUtilities.h"

#include <Poco/NObserver.h>

#include <QApplication>
#include <QDockWidget>
#include <QMap>
#include <QMutex>
#include <QProgressDialog>
#include <QTreeWidget>
#include <unordered_map>

#ifdef MAKE_VATES
#include "vtkPVDisplayInformation.h"
#endif

//----------------------------------
// Forward declarations
//----------------------------------
class Graph3D;
class ScriptingEnv;
class MantidMatrix;
class AlgorithmDockWidget;
class RemoteClusterDockWidget;
class AlgorithmMonitor;
class InstrumentWindow;

namespace MantidQt {
namespace MantidWidgets {
class FitPropertyBrowser;
class WorkspaceTreeWidget;
class Message;
} // namespace MantidWidgets
namespace SliceViewer {
class SliceViewerWindow;
}
namespace SpectrumView {
class SpectrumView;
}
} // namespace MantidQt
namespace Ui {
class SequentialFitDialog;
}

namespace Mantid {
namespace API {
class AlgorithmObserver;
}
} // namespace Mantid

/**
MantidUI is the extension of QtiPlot's ApplicationWindow which deals with Mantid
framework.

@author Roman Tolchenov, Tessella Support Services plc
@date 02/07/2008
*/

/// Required by Qt to use Mantid::API::Workspace_sptr as a parameter type in
/// signals
Q_DECLARE_METATYPE(Mantid::API::MatrixWorkspace_sptr)
Q_DECLARE_METATYPE(Mantid::API::MatrixWorkspace_const_sptr)

class MantidUI : public QObject,
                 public MantidQt::MantidWidgets::MantidDisplayBase {
  Q_OBJECT

public:
  // Constructor
  explicit MantidUI(ApplicationWindow *aw);

  // Destructor
  ~MantidUI() override;

  // Clear the framework
  void shutdown();

  // Save settings to a persistent store
  void saveSettings() const;

  // Initialization
  void init();

  // Insert relevant items into a menu
  void addMenuItems(QMenu *menu);

  // Pointer to QtiPLot main window
  ApplicationWindow *appWindow() { return m_appWindow; }

  // Returns a list of open workspaces
  QStringList getWorkspaceNames();

  // Returns a list of registered algorithms
  QStringList getAlgorithmNames();

  // Returns the number of algorithms currently executing
  int runningAlgCount() const;

  void updateProject() override;
  // Create an algorithm using Mantid FrameworkManager
  // Create a pointer to the named algorithm and version
  Mantid::API::IAlgorithm_sptr createAlgorithm(const QString &algName,
                                               int version = -1) override;

  // Execute algorithm asinchronously
  bool executeAlgorithmAsync(Mantid::API::IAlgorithm_sptr alg,
                             const bool wait = false) override;

  // Gets a pointer to workspace workspaceName
  Mantid::API::Workspace_const_sptr
  getWorkspace(const QString &workspaceName) override;

  QWidget *getParent() override;

  // Deletes workspace from QtiPlot
  void deleteWorkspace(const QString &workspaceName);

  // Delete multiple workspaces from QtiPlot
  void deleteWorkspaces(const QStringList &wsNames = QStringList()) override;

  // Returns the name of selected workspace in exploreMantid window
  QString getSelectedWorkspaceName();

  // Returns the pointer of workspace selected in exploreMantid window
  Mantid::API::Workspace_const_sptr getSelectedWorkspace();

  // Convenience method for updating the list of recent filenames in
  // ApplicationWindow
  void updateRecentFilesList(const QString &fname) override;

  // Returns the name and version of the algorithm selected in algorithm dock
  // window
  void getSelectedAlgorithm(QString &algName, int &version);

  // Adjusts QtiPlot's main menu if a MantidMatrix becomes active (receives
  // focus)
  bool menuAboutToShow(MdiSubWindow *w);

  // Prepares the contex menu for MantidMatrix
  void showContextMenu(QMenu &cm, MdiSubWindow *w);

  // Check if drop event can be accepted
  bool canAcceptDrop(QDragEnterEvent *e);
  // Handles workspace drop operation to QtiPlot (imports the workspace to
  // MantidMatrix)
  bool drop(QDropEvent *e);

  //  *****      Plotting Methods     *****  //

  // Creates a 3D plot in QtiPlot if the active window is a MantidMatrix
  Graph3D *plot3DMatrix(int style);

  // Creates a 2D plot in QtiPlot if the active window is a MantidMatrix
  MultiLayer *plotSpectrogram(GraphOptions::CurveType type);

  /// Create a Table form specified spectra in a MatrixWorkspace
  Table *createTableFromSpectraList(const QString &tableName,
                                    const QString &workspaceName,
                                    QList<int> indexList, bool errs = true,
                                    bool binCentres = false);

  // Copies selected rows from MantidMatrix to Y and errY columns of a new
  // Table.
  Table *createTableFromSelectedRows(MantidMatrix *m, bool errs = true,
                                     bool binCentres = false);

  // Create dialog box for what to plot and how
  MantidQt::MantidWidgets::MantidWSIndexDialog *
  createWorkspaceIndexDialog(int flags, const QStringList &wsNames,
                             bool showWaterfall, bool showPlotAll,
                             bool showTiledOpt, bool isAdvanced) override;

  /// Create a 1d graph form a Table
  MultiLayer *createGraphFromTable(Table *t, int type = 0);

  // Shows 1D graphs of the spectra (rows) selected in a MantidMatrix
  MultiLayer *plotSelectedRows(
      const MantidMatrix *const m,
      MantidQt::DistributionFlag distr = MantidQt::DistributionDefault,
      bool errs = true);
  // Shows 1D graphs of the columns (bins) selected in a MantidMatrix
  MultiLayer *plotSelectedColumns(const MantidMatrix *const m,
                                  bool errs = true);

  AlgorithmMonitor *getAlgMonitor() { return m_algMonitor; }
  /// updates the algorithms tree
  void updateAlgorithms();
  /// updates the workspaces tree
  void updateWorkspaces();

  /// Plot a 1D graph for an integrated mdworkspace
  MultiLayer *plotMDList(const QStringList &wsNames, const int plotAxis,
                         const Mantid::API::MDNormalization normalization,
                         const bool showError, MultiLayer *plotWindow = nullptr,
                         bool clearWindow = false);

  /// Plot a "tiled" plot (with subplots)
  MultiLayer *
  plotSubplots(const QMultiMap<QString, std::set<int>> &toPlot,
               MantidQt::DistributionFlag distr = MantidQt::DistributionDefault,
               bool errs = false, MultiLayer *plotWindow = nullptr) override;

  MultiLayer *
  plotSubplots(const QMultiMap<QString, int> &toPlot,
               MantidQt::DistributionFlag distr = MantidQt::DistributionDefault,
               bool errs = false, MultiLayer *plotWindow = nullptr);

  MultiLayer *
  plotSubplots(const QStringList &wsNames, const QList<int> &indexList,
               MantidQt::DistributionFlag distr = MantidQt::DistributionDefault,
               bool errs = false, MultiLayer *plotWindow = nullptr);

#ifdef MAKE_VATES
  bool doesVatesSupportOpenGL() override;
#endif

public slots:
  // Create a 1d graph form specified MatrixWorkspace and index
  MultiLayer *
  plot1D(const QStringList &wsnames, const QList<int> &indexList,
         bool spectrumPlot,
         MantidQt::DistributionFlag distr = MantidQt::DistributionDefault,
         bool errs = true,
         GraphOptions::CurveType style = GraphOptions::Unspecified,
         MultiLayer *plotWindow = nullptr, bool clearWindow = false,
         bool waterfallPlot = false);

  MultiLayer *
  plot1D(const QString &wsName, const std::set<int> &indexList,
         bool spectrumPlot,
         MantidQt::DistributionFlag distr = MantidQt::DistributionDefault,
         bool errs = false, MultiLayer *plotWindow = nullptr,
         bool clearWindow = false, bool waterfallPlot = false);

  MultiLayer *
  plot1D(const QMultiMap<QString, int> &toPlot, bool spectrumPlot,
         MantidQt::DistributionFlag distr = MantidQt::DistributionDefault,
         bool errs = false,
         GraphOptions::CurveType style = GraphOptions::Unspecified,
         MultiLayer *plotWindow = nullptr, bool clearWindow = false,
         bool waterfallPlot = false, const QString &log = "",
         const std::set<double> &customLogValues = std::set<double>(),
         const bool multipleSpectra = false);

  MultiLayer *
  plot1D(const QMultiMap<QString, std::set<int>> &toPlot, bool spectrumPlot,
         MantidQt::DistributionFlag distr = MantidQt::DistributionDefault,
         bool errs = false, MultiLayer *plotWindow = nullptr,
         bool clearWindow = false, bool waterfallPlot = false,
         const QString &log = "",
         const std::set<double> &customLogValues = std::set<double>()) override;

  /// Plot contour
  void plotContour(bool accepted, int plotIndex, const QString &axisName,
                   const QString &logName,
                   const std::set<double> &customLogValues,
                   const QList<QString> &workspaceNames) override;

  /// Plot surface
  void plotSurface(bool accepted, int plotIndex, const QString &axisName,
                   const QString &logName,
                   const std::set<double> &customLogValues,
                   const QList<QString> &workspaceNames) override;

  /// Draw a color fill plot for each of the listed workspaces
  void drawColorFillPlots(
      const QStringList &wsNames,
      GraphOptions::CurveType curveType = GraphOptions::ColorMap) override;
  /// Draw a color fill plot for the named workspace
  MultiLayer *drawSingleColorFillPlot(
      const QString &wsName,
      GraphOptions::CurveType curveType = GraphOptions::ColorMap,
      MultiLayer *window = nullptr, bool hidden = false);

  // Create a 1d graph form specified spectra in a MatrixWorkspace
  MultiLayer *plotSpectraRange(
      const QString &wsName, int i0, int i1,
      MantidQt::DistributionFlag distr = MantidQt::DistributionDefault,
      bool errs = true);

  // Set properties of a 1d graph which plots data from a workspace
  static void setUpBinGraph(MultiLayer *ml, const QString &wsName,
                            Mantid::API::MatrixWorkspace_const_sptr workspace);

  // Copy to a Table Y-values (and Err-values if errs==true) of bins with
  // indeces from i0 to i1 (inclusive) from a workspace
  Table *createTableFromBins(const QString &wsName,
                             Mantid::API::MatrixWorkspace_const_sptr workspace,
                             const QList<int> &bins, bool errs = true,
                             int fromRow = -1, int toRow = -1);

  // Copies selected columns (time bins) in a MantidMatrix to a Table
  Table *createTableFromSelectedColumns(MantidMatrix *m, bool errs);

  // Creates and shows a Table with detector ids for the workspace in the
  // MantidMatrix
  Table *createTableDetectors(MantidMatrix *m);

  /// create and shows a Table for the workspace from the Python script
  Table *createDetectorTable(const QString &wsName);

  /// Create a table showing detector information for the given workspace and
  /// indices and optionally the data for that detector
  Table *createDetectorTable(const QString &wsName,
                             const std::vector<int> &indices,
                             bool include_data = false) override;
  /// Create the instrument detector table from a MatrixWorkspace
  Table *createDetectorTable(const QString &wsName,
                             const Mantid::API::MatrixWorkspace_sptr &ws,
                             const std::vector<int> &indices,
                             bool include_data = false);
  /// Create a table of detectors from a PeaksWorkspace
  Table *createDetectorTable(const QString &wsName,
                             const Mantid::API::IPeaksWorkspace_sptr &ws);
  /// Create a string of the style "1, 2...(100 more)...102, 103"
  QString createTruncatedList(const std::set<int> &elements);

  /// Triggers a workspace delete check
  void deletePressEvent();

  // Determine whether the workspace has a UB matrix
  bool hasUB(const QString &wsName);
  // Clear the UB via the ClearUB algorithm
  void clearUB(const QStringList &workspaces);
  //  *****                            *****  //
  void renameWorkspace(QStringList = QStringList()) override;

  /**
   * Set the currently used fit property browser. Is needed because e.g. Muon
   * Analysis is using its
   * own fit browser.
   * @param newBrowser The browser to be used. If is null, is set to default
   * one.
   */
  void setFitFunctionBrowser(
      MantidQt::MantidWidgets::FitPropertyBrowser *newBrowser);

public:
  // Return pointer to the fit function property browser
  MantidQt::MantidWidgets::FitPropertyBrowser *fitFunctionBrowser() {
    return m_fitFunction;
  }

  MultiLayer *mergePlots(MultiLayer *g1, MultiLayer *g2);
  MantidMatrix *getMantidMatrix(const QString &wsName);

  void setIsRunning(bool running);
  bool createScriptInputDialog(const QString &alg_name,
                               const QString &preset_values,
                               const QString &optional_msg,
                               const QStringList &enabled,
                               const QStringList &disabled);
  /// Group selected workspaces
  void groupWorkspaces();
  /// UnGroup selected groupworkspace
  void ungroupWorkspaces();
  /** save the workspace data to nexus file
  This method is useful when a project is saved from mantidplot
  */
  void savedatainNexusFormat(const std::string &fileName,
                             const std::string &wsName);

  void loadWSFromFile(const std::string &wsname, const std::string &fileName);

  void saveProject(bool save);
  void enableSaveNexus(const QString &wsName) override;
  void disableSaveNexus() override;

signals:
  // A signal to indicate that we want a script to produce a dialog
  void showPropertyInputDialog(const QString &algName);
  // Broadcast that an algorithm is about to be created
  void algorithmAboutToBeCreated();

public:
signals:

  // These signals are to be fired from methods run in threads other than the
  // main one
  // (e.g. handlers of algorithm notifications)

  void needToCreateLoadDAEMantidMatrix(const QString &);

  // Display a critical error dialog box
  void needToShowCritical(const QString &);

  // Signals that the fit property browser has updated its X range
  void x_range_update(double, double);

public slots:

  // Receives a new X range from a PeakPickerTool and re-emits it.
  void x_range_from_picker(double, double);
  void test();

  void
  showSequentialPlot(Ui::SequentialFitDialog *ui,
                     MantidQt::MantidWidgets::FitPropertyBrowser *fitbrowser);

  // Import the workspace selected in the Workspace dock window
  void importWorkspace() override;
  void importBoxDataTable() override;
  void importTransposed() override;

  // Invoke the Vates Simple User Interface
  void showVatesSimpleInterface() override;

  // Invoke the plot of MD intensity vs non-integrated dimension.
  void showMDPlot() override;

  // Invoke a grid showing a table of MD summary list data.
  void showListData() override;

  // SpectrumViewer
  void showSpectrumViewer() override;

  // SliceViewer
  void showSliceViewer() override;

  // #539: For adding Workspace History display to MantidPlot
  void showAlgorithmHistory() override;

  // Import a workspace wsName
  void importWorkspace(const QString &wsName, bool showDlg = true,
                       bool makeVisible = true) override;

  // Create a MantidMatrix from workspace wsName
  MantidMatrix *importMatrixWorkspace(const QString &wsName, int lower = -1,
                                      int upper = -1, bool showDlg = true,
                                      bool makeVisible = true);

  // Create a MantidMatrix from workspace
  MantidMatrix *
  importMatrixWorkspace(const Mantid::API::MatrixWorkspace_sptr workspace,
                        int lower = -1, int upper = -1,
                        bool showDlg = true) override;

  // Create a Table from workspace wsName
  Table *importTableWorkspace(const QString &wsName, bool showDlg = true,
                              bool makeVisible = true, bool transpose = false);

  void createLoadDAEMantidMatrix(const QString &);

  // Slots responding to MantidMatrix context menu commands
  void copyRowToTable();
  void copyColumnToTable();
  void copyRowToGraph();
  void copyColumnToGraph();
  void copyRowToGraphErr();
  void copyColumnToGraphErr();
  void copyDetectorsToTable();
  void copyValues();
  void copyRowsToWaterfall();
  // Slot callable from Workspace menu item
  void plotWholeAsWaterfall();
  /// Common method to convert a plot of a set of spectra into a waterfall
  /// plot
  void convertToWaterfall(MultiLayer *ml);

  // Execute algorithm given name and version
  void showAlgorithmDialog(const QString &algName, int version = -1) override;
  // Execute an algorithm with the given parameter list
  void showAlgorithmDialog(const QString &algName,
                           QHash<QString, QString> paramList,
                           Mantid::API::AlgorithmObserver *obs = nullptr,
                           int version = -1) override;
  // Execute an algorithm
  void executeAlgorithm(Mantid::API::IAlgorithm_sptr alg) override;

  // Find the name of the first input workspace for an algorithm
  QString
  findInputWorkspaceProperty(Mantid::API::IAlgorithm_sptr algorithm) const;
  // Show Qt critical error message box
  void showCritical(const QString &) override;
  // Show the dialog monitoring currently running algorithms
  void showAlgMonitor();
  // Called from ApplicationWindow to customize the main menu
  void mantidMenuAboutToShow();

  void manageMantidWorkspaces();

  // Python related functions
  InstrumentWindow *getInstrumentView(const QString &wsName, int tab = -1);

  void showMantidInstrument();

  // Show instrument for the selected workspace
  void showMantidInstrumentSelected() override;

  // Show instrument. Workspace name is passed as the argument
  void showMantidInstrument(const QString &);

  // Show log files for selected workspace
  void showLogFileWindow() override;

  // Show sample material window for selected workspace
  void showSampleMaterialWindow() override;

  void insertMenu();

  // Customize MantidMatrix menu.
  void menuMantidMatrixAboutToShow();

  // Show / hide the FitPropertyBrowser
  void showFitPropertyBrowser(bool on = true);

  // Plot a spectrum in response from a InstrumentWindow signal
  MultiLayer *plotInstrumentSpectrum(const QString &, int);
  MultiLayer *plotInstrumentSpectrumList(const QString &,
                                         const std::set<int> &);

  void importString(const QString &logName, const QString &data);
  void importString(const QString &logName, const QString &data,
                    const QString &sep, const QString &wsName = QString());
  void importStrSeriesLog(const QString &logName, const QString &data,
                          const QString &wsName = QString());
  void importNumSeriesLog(const QString &wsName, const QString &logname,
                          int filter);

  // Clear all Mantid related memory
  void clearAllMemory(const bool prompt = true);
  // Ticket #672
  void saveNexusWorkspace();

  void setVatesSubWindow(QMdiSubWindow *vatesUI) { m_vatesSubWindow = vatesUI; }

#ifdef _WIN32
public:
  // Shows 2D plot of current memory usage.
  void memoryImage();
  void memoryImage2();
#endif

private slots:
  // respond to the global Mantid properties being modified
  void configModified();

  // slot for file open dialogs created from the main app menu, or the
  // workspaces dock window
  void loadFileDialogAccept();

private:
  // Notification handlers and corresponding observers.
  void handleLoadDAEFinishedNotification(
      const Poco::AutoPtr<Mantid::API::Algorithm::FinishedNotification> &pNf);
  Poco::NObserver<MantidUI, Mantid::API::Algorithm::FinishedNotification>
      m_finishedLoadDAEObserver;

  // handles notification send by ConfigService, change on
  // pythonscripts.directories
  void handleConfigServiceUpdate(
      Mantid::Kernel::ConfigValChangeNotification_ptr pNf);
  Poco::NObserver<MantidUI, Mantid::Kernel::ConfigValChangeNotification>
      m_configServiceObserver;

  //#678
  // for savenexus algorithm
  void executeSaveNexus();

  void copyWorkspacestoVector(const QList<QTreeWidgetItem *> &list,
                              std::vector<std::string> &inputWS);
  void PopulateData(Mantid::API::Workspace_sptr ws_ptr,
                    QTreeWidgetItem *wsid_item);

  /// This creates an algorithm dialog.
  MantidQt::API::AlgorithmDialog *
  createAlgorithmDialog(Mantid::API::IAlgorithm_sptr alg);

  /// This method accepts user inputs and executes loadraw/load nexus
  /// algorithm
  std::string extractLogTime(Mantid::Types::Core::DateAndTime value,
                             bool useAbsoluteDate,
                             Mantid::Types::Core::DateAndTime start);

  // Whether new plots shoul re-use the same plot instance (for every
  // different
  // type of plot).
  // The name comes from: these plots are normally opened from the context
  // menu
  // of the workspaces dock window
  bool workspacesDockPlot1To1();

  /// Get the title to use for a plot - name of selected group
  QString getSelectedGroupName() const;

  /// Set initial autoscale for graph, then reset user autoscale option
  void setInitialAutoscale(Graph *graph);

  /// Plot a layer in a multilayer plot
  void plotLayerOfMultilayer(MultiLayer *multi, const bool plotErrors,
                             const bool plotDist, int &row, int &col,
                             const QString &wsName,
                             const std::set<int> &spectra);

  /// Get log values and put into a curve spec list
  void putLogsIntoCurveSpecs(
      std::vector<CurveSpec> &curveSpecList,
      const QMultiMap<QString, int> &toPlot, const QString &log,
      const std::set<double> &customLogValues = std::set<double>());

  // Private variables

  ApplicationWindow *m_appWindow; // QtiPlot main ApplicationWindow
  QDockWidget *m_workspaceDockWidget;
  MantidQt::MantidWidgets::WorkspaceTreeWidget
      *m_exploreMantid; // Widget for manipulating workspaces
  AlgorithmDockWidget *m_exploreAlgorithms; // Dock window for using algorithms
  RemoteClusterDockWidget
      *m_exploreRemoteTasks; // Dock window for using remote tasks
  /// Current fit property browser being used
  MantidQt::MantidWidgets::FitPropertyBrowser *m_fitFunction;
  /// Default fit property browser (the one docked on the left)
  MantidQt::MantidWidgets::FitPropertyBrowser *m_defaultFitFunction;

  QAction *actionCopyRowToTable;
  QAction *actionCopyRowToGraph;
  QAction *actionCopyRowToGraphErr;
  QAction *actionWaterfallPlot;
  QAction *actionCopyColumnToTable;
  QAction *actionCopyColumnToGraph;
  QAction *actionCopyColumnToGraphErr;
  QAction *actionToggleMantid;
  QAction *actionToggleAlgorithms;
  QAction *actionToggleRemoteTasks;
  QAction *actionToggleFitFunction;
  QAction *actionCopyDetectorsToTable;
  QAction *actionCopyValues;

  QMenu *mantidMenu;
  QMenu *menuMantidMatrix;        //  MantidMatrix specific menu
  AlgorithmMonitor *m_algMonitor; //  Class for monitoring running algorithms

  // keep track of the last shown, which will be refreshed or killed/rebuilt
  // if
  // showing only one inst. window
  // QPointer handles when events, etc. destroy these windows
  QPointer<InstrumentWindow> m_lastShownInstrumentWin;
  QPointer<MantidQt::SliceViewer::SliceViewerWindow> m_lastShownSliceViewWin;
  QPointer<MantidQt::SpectrumView::SpectrumView> m_lastShownSpectrumViewerWin;
  QPointer<MultiLayer> m_lastShownColorFillWin;
  QPointer<MultiLayer> m_lastShown1DPlotWin;

  // Map of <workspace_name,update_interval> pairs. Positive update_intervals
  // mean
  // UpdateDAE must be launched after LoadDAE for this workspace
  QMap<std::string, int> m_DAE_map;

  // Stores dependent mdi windows. If the 'key' window closes, all 'value'
  // ones
  // must be closed as well.
  std::unordered_multimap<MdiSubWindow *, MdiSubWindow *> m_mdiDependency;
  QMdiSubWindow
      *m_vatesSubWindow; ///< Holder for the Vates interface sub-window

  // prevents some repeated code realtating to log names
  void formatLogName(QString &label, const QString &wsName);
};

/**
 * This object sets the "busy" cursor while it is in scope, then restores the
 * original cursor when destroyed.
 */
class ScopedOverrideCursor {
public:
  /// Constructor sets wait cursor
  ScopedOverrideCursor() { QApplication::setOverrideCursor(Qt::WaitCursor); }
  /// Destructor restores original cursor
  virtual ~ScopedOverrideCursor() { QApplication::restoreOverrideCursor(); }
};

#endif
