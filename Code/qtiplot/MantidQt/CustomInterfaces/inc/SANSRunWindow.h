#ifndef MANTIDQTCUSTOMINTERFACES_SANSRUNWINDOW_H_
#define MANTIDQTCUSTOMINTERFACES_SANSRUNWINDOW_H_

//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/ui_SANSRunWindow.h"
#include "MantidQtAPI/UserSubWindow.h"
#include "MantidQtCustomInterfaces/SANSAddFiles.h"

#include <QHash>
#include <QStringList>
#include "Poco/NObserver.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid
{
  namespace Kernel
  {
    class Logger;
  }
  namespace API
  {
    class MatrixWorkspace;
  }
}

//---------------------------
// Qt Forward Declarations
//---------------------------
class QLineEdit;
class QSignalMapper;
class QLabel;
class QAction;

namespace MantidQt
{
namespace CustomInterfaces
{
//-----------------------------
// Forward declaration
//-----------------------------


class SANSRunWindow : public MantidQt::API::UserSubWindow
{
  Q_OBJECT

public:
  /// Default Constructor
  SANSRunWindow(QWidget *parent = 0);
  /// Destructor
  ~SANSRunWindow();

signals:
  ///Indicate the state of the loaded data.
  void dataReadyToProcess(bool state);

private:
  ///Mode enumeration
  enum RunMode { SingleMode = 0, BatchMode };
  /// Initialize the layout
  virtual void initLayout();
  /// Init Python environment
  virtual void initLocalPython();
  /**@name Utility functions */
  //@{
  /// connect the buttons click events to signals
  void connectButtonSignals();
  /// Create the necessary widget maps
  void initWidgetMaps();
  ///Read previous settings
  void readSettings();
  ///Save settings
  void saveSettings();
  /// Run a reduce function
  QString runReduceScriptFunction(const QString & pycode);
  /// Trim python print markers
  void trimPyMarkers(QString & txt);
  /// Load the user file specified in the text field
  bool loadUserFile();
  /// Load a CSV file
  bool loadCSVFile();
  /// Set limits step and type options
  void setLimitStepParameter(const QString & pname, QString param, QLineEdit* step_value,  QComboBox* step_type);
  ///Construct mask table
  void updateMaskTable();
  /// Add spectrum masks to table
  void addSpectrumMasksToTable(const QString & mask_string, const QString & det_name);
  /// Add a time mask string to the mask table
  void addTimeMasksToTable(const QString & mask_string, const QString & det_name);
  /// Construct the reduction code from the Python script template
  QString createAnalysisDetailsScript(const QString & type);
  /// Get the component distances
  void componentLOQDistances(boost::shared_ptr<Mantid::API::MatrixWorkspace> workspace, double & lms, double & lsda, double & lsdb);
  /// Enable/disable user interaction
  void setProcessingState(bool running, int type);
  ///Check for workspace name in the AnalysisDataService
  bool workspaceExists(const QString & ws_name) const;
  Mantid::API::MatrixWorkspace_sptr getGroupMember(Mantid::API::Workspace_const_sptr in, const int member) const;
  ///Construct a QStringList of the currently loaded workspaces
  QStringList currentWorkspaceList() const;
  ///Is the user file loaded
  bool isUserFileLoaded() const;
  /// Create a mask string
  void addUserMaskStrings(QString & exec_script);
  /// Set geometry details
  void setGeometryDetails(const QString & sample_logs, const QString & can_logs);
  /// Set the SANS2D geometry
  void setSANS2DGeometry(boost::shared_ptr<Mantid::API::MatrixWorkspace> workspace, const QString & logs, int wscode);
  /// Set LOQ geometry
  void setLOQGeometry(boost::shared_ptr<Mantid::API::MatrixWorkspace> workspace, int wscode);
  /// Mark an error on a label
  void markError(QLabel* label);
  /// Run an assign command
  bool runAssign(int key, QString & logs);
  int getPeriod(const int key);
  void setNumberPeriods(const int key, const int num);
  void unSetPeriods(const int key);
  QString getWorkspaceName(int key);
  /// Handle a delete notification from Mantid
  void handleMantidDeleteWorkspace(Mantid::API::WorkspaceDeleteNotification_ptr p_dnf);
  // Format a double in a string with a specfied colour, format and precision
  QString formatDouble(double value, const QString & colour = "black", char format = 'f', int precision = 3);
  /// Issue a warning 
  void raiseOneTimeMessage(const QString & msg, int index = -1);
  /// Reset the geometry box to blank
  void resetGeometryDetailsBox();
  ///Cleanup old raw files
  void cleanup();
  /// Flip the reload flag
  void forceDataReload(bool force = true);
  /// Browse for a file
  bool browseForFile(const QString & box_title, QLineEdit* file_field, QString file_filter = QString());
  /// Add a csv line to the batch grid
  int addBatchLine(QString csv_line, QString separator = "");
  ///Save the batch file
  QString saveBatchGrid(const QString & filename = "");
  ///Reset the log flags
  void checkLogFlags();
  //@}

private slots:
  /// Select the data directory
  void selectDataDir();
  /// Select the user file
  void selectUserFile();
  /// Select a CSV file
  void selectCSVFile();
  /// A run number has changed
  void runChanged();
  /// Receive a load button click
  bool handleLoadButtonClick();
  /// Reduce button clicked
  void handleReduceButtonClick(const QString & type);
  /// Plot button has been clicked
  void handlePlotButtonClick();
  /// Find centre button click handler
  void handleRunFindCentre();
  ///Handle save button click
  void handleSaveButtonClick();
  /// A ComboBox option change
  void handleStepComboChange(int new_index);
  /// Called when the show mask button has been clicked
  void handleShowMaskButtonClick();
  ///Handle the change in instrument 
  void handleInstrumentChange(int);
  /// Update the centre finding progress
  void updateCentreFindingStatus(const QString & msg);
  /// Append log message to log window
  void updateLogWindow(const QString & msg);
  /// Switch mode
  void switchMode(int mode_id);
  ///Paste to batch table
  void pasteToBatchTable();
  ///Clear the batch table
  void clearBatchTable();
  ///Clear logger
  void clearLogger();
  ///Verbose mode checked/unchecked
  void verboseMode(int state);
  ///Default trans changed state
  void updateTransInfo(int state);

private:
  /// The form generated by Qt Designer
  Ui::SANSRunWindow m_uiForm;
  /// this object holds the functionality in the Add Files tab
  SANSAddFiles *m_addFilesTab;
  /// The data directory (as an absolute path)
  QString m_data_dir;
  /// The instrument definition directory
  QString m_ins_defdir;
  /// The last directory that was viewed
  QString m_last_dir;
  /// Is the user file loaded
  bool m_cfg_loaded;
  /// The sample that was loaded
  QString m_sample_no;
  /// A map for quickly retrieving the different line edits
  QHash<int, QLineEdit*> m_run_no_boxes;
  /// A hash for quickly retrieving the different label fields
  QHash<int, QLabel*> m_period_lbls;
  /// A list of the full workspace names
  QHash<int, QString> m_workspace_names;
  // A signal mapper to pick up various button clicks
  QSignalMapper *m_reducemapper;
  // A flag to mark that warnings have been issued about geometry issues
  bool m_warnings_issued;
  // A flag that causes the reload of the data
  bool m_force_reload;
  /// A flag indicating there were warning messsages in the log
  bool m_log_warnings;
  // An observer for a delete notification from Mantid
  Poco::NObserver<SANSRunWindow, Mantid::API::WorkspaceDeleteNotification> m_delete_observer;
  /// A map of S2D detector names to QLabel pointers
  QList<QHash<QString, QLabel*> > m_s2d_detlabels;
  /// A map of LOQ detector names to QLabel pointers
  QList<QHash<QString, QLabel*> > m_loq_detlabels;
  /// A map of allowed batch csv tags to column numbers
  QHash<QString,int> m_allowed_batchtags;
  /// An integer to save the last run reduction type
  int m_lastreducetype;
  /// A signal mapper for the mode switches
  QSignalMapper *m_mode_mapper;
  /// Indicate if the reduce module has been loaded?
  bool m_have_reducemodule;
  /// A flag marking if the batch grid has been changed
  bool m_dirty_batch_grid;
  /// If set, the filename specified is removed after a batch run
  QString m_tmp_batchfile;
  /// A paste action for the batch table
  QAction *m_batch_paste;
  ///A clear action for the batch table
  QAction *m_batch_clear;
  //A reference to a logger
  static Mantid::Kernel::Logger & g_log;
};

}
}

#endif //MANTIDQTCUSTOMINTERFACES_SANSRUNWINDOW_H_
