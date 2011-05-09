#ifndef MANTIDQTCUSTOMINTERFACES_SANSRUNWINDOW_H_
#define MANTIDQTCUSTOMINTERFACES_SANSRUNWINDOW_H_

//----------------------
// Includes
//----------------------
#include "ui_SANSRunWindow.h"
#include "MantidQtAPI/UserSubWindow.h"
#include "MantidQtCustomInterfaces/SANSAddFiles.h"
#include "MantidQtMantidWidgets/SaveWorkspaces.h"
#include "MantidQtCustomInterfaces/SANSDiagnostics.h"

#include <QHash>
#include <QSettings>
#include <QStringList>
#include <Poco/NObserver.h>
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/ConfigService.h"
#include <vector>

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

class QAction;

namespace MantidQt
{
namespace CustomInterfaces
{
	/** 
    Implements the SANS, small angle neutron scattering, dialog box

    @author Martyn Gigg

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class SANSRunWindow : public MantidQt::API::UserSubWindow
{
  Q_OBJECT

public:
  /// Name of the interface
  static std::string name() { return "ISIS SANS"; }

public:
  /// Default Constructor
  SANSRunWindow(QWidget *parent = 0);
  /// Destructor
  ~SANSRunWindow();  
  
  
signals:
  ///Indicate the state of the loaded data.
  void dataReadyToProcess(bool state);
  //signal  to notify mask file loaded
  void userfileLoaded();

private:
  ///Mode enumeration
  enum RunMode { SingleMode = 0, BatchMode };

  /// mask type
  enum MaskType{ DefaultMask=0,TimeMask=1,PixelMask=2};

  /// Initialize the layout
  virtual void initLayout();
  /// Init Python environment
  virtual void initLocalPython();
  /**@name Utility functions */
  //@{
  /// Initialise some of the data and signal connections in the save box
  void setupSaveBox();
  /// connect the buttons click events to signals
  void connectButtonSignals();
  /// connect signals from the textChanged() signal from text boxes, index changed on ComboBoxes etc.
  void connectChangeSignals();
  /// Create the necessary widget maps
  void initWidgetMaps();
  ///Read previous settings
  void readSettings();
  /// Sets the states of the save box controls to the states read from the QSettings object
  void readSaveSettings(QSettings & valueStore);
  ///Save settings
  void saveSettings();
  ///Stores the state of the save box controls in QSettings
  void saveSaveSettings(QSettings & valueStore);
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
  QString readUserFileGUIChanges(const QString & type);
  QString readSampleObjectGUIChanges();
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
  void addUserMaskStrings(QString & exec_script,const QString& importCommand,enum MaskType mType);
  /// Set geometry details
  void setGeometryDetails(const QString & sample_logs, const QString & can_logs);
  /// Set the SANS2D geometry
  void setSANS2DGeometry(boost::shared_ptr<Mantid::API::MatrixWorkspace> workspace, const QString & logs, int wscode);
  /// Set LOQ geometry
  void setLOQGeometry(boost::shared_ptr<Mantid::API::MatrixWorkspace> workspace, int wscode);
  /// Mark an error on a label
  void markError(QLabel* label);
  /// set the name of the output workspace, empty means there is no output
  void resetDefaultOutput(const QString & wsName="");
  /// Run an assign command
  bool runAssign(int key, QString & logs);
  /// Load a scatter sample file or can run via Python objects using the passed Python command
  bool assignDetBankRun(const MantidWidgets::MWRunFiles & runFile, const QString & assignFn, QString & logs);
  /// runs that contain only monitor counts can be direct or transmission runs
  bool assignMonitorRun(const MantidWidgets::MWRunFiles & runFile, const MantidWidgets::MWRunFiles & canFile, const QString & assignFn);
  /// Get the detectors' names
  void fillDetectNames(QComboBox *output);
  QStringList getSaveAlgs();
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
 
  public slots:
     /// apply mask
  void applyMask(const QString& wsName,bool time_pixel);

private slots:
  /// Select the data directory
  void selectDataDir();
  /// Select the user file
  void selectUserFile();
  /// Select a CSV file
  void selectCSVFile();
  /// Raises a browse dialog to select the filename used to save the output
  void saveFileBrowse();
  /// Raises a dialog that allows people to save multiple workspaces
  void saveWorkspacesDialog();
  ///deals with the save workspaces dialog box closing
  void saveWorkspacesClosed();
  /// A run number has changed
  void runChanged();
  /// Receive a load button click
  bool handleLoadButtonClick();
  /// Reduce button clicked
  void handleReduceButtonClick(const QString & type);
  /// Find centre button click handler
  void handleRunFindCentre();
  ///Save the output from a single run reduction in the user selected formats
  void handleDefSaveClick();
  void handleWavComboChange(int new_index);
  /// A ComboBox option change
  void handleStepComboChange(int new_index);
  /// Called when the show mask button has been clicked
  void handleShowMaskButtonClick();
  ///Handle the change in instrument 
  void handleInstrumentChange();
  ///Record if that user has changed the default filename
  void setUserFname();
  /// Update the centre finding progress
  void updateCentreFindingStatus(const QString & msg);
  /// Enables or disables the floodFile run widget
  void prepareFlood(int state);
  /// Enable the default save button only if there an output workspace and a filename to save it to
  void enableOrDisableDefaultSave();
  /// connected to the Multi-period check box it shows or hides the multi-period boxes on the file widgets
  void disOrEnablePeriods(const int);
  /// Append log message to log window
  void updateLogWindow(const QString & msg);
  /// Switch mode
  void switchMode();
  ///Paste to batch table
  void pasteToBatchTable();
  ///Clear the batch table
  void clearBatchTable();
  ///Clear logger
  void clearLogger();
  ///Default trans changed state
  void updateTransInfo(int state);
  void checkList();
  
private:
  /// used to specify the range of validation to do
  enum ValCheck
  {
    ALL,                                                    ///< for checking all validators
    LOAD,                                                   ///< for checking the load validators only
    RUN                                                     ///< for checking the run validators only
  };

  /// holds pointer to validators and their locations
  typedef std::map<QWidget * const, std::pair<QWidget *, QWidget *> > ValMap;
  /// The form generated by Qt Designer
  Ui::SANSRunWindow m_uiForm;
  /// this object holds the functionality in the Add Files tab
  SANSAddFiles *m_addFilesTab;

  SANSDiagnostics* m_diagnosticsTab;
  /// this points to a saveWorkspaces, which allows users to save any workspace, when one is opened
  MantidWidgets::SaveWorkspaces *m_saveWorkspaces;
  /// The data directory (as an absolute path)
  QString m_data_dir;
  /// The instrument definition directory
  QString m_ins_defdir;
  /// The last directory that was viewed
  QString m_last_dir;
  /// Is the user file loaded
  bool m_cfg_loaded;
  ///True if the user cahnged the default filename text, false otherwise
  bool m_userFname;
  /// The sample that was loaded
  QString m_sample_file;
  /// The workspace containing the experimental run one the sample under investigation
  QString m_experWksp;
  /// The workspace containing the can run
  QString m_experCan;
  /// List of all run entry widgets, which are on tab page 1
  std::vector< MantidWidgets::MWRunFiles * > m_runFiles;
  /// There validators are searched before a reduction begins. Where there is a problem focus goes to the widget linked to a validator whose tab is also stored in the pair. Disabling a validator QLabel disables checking that validator
  ValMap m_validators;
  /// List of all validators searched through before a load operation is possible
  ValMap m_loadValids;
  /// A map for quickly retrieving the different line edits
  QHash<int, QLineEdit*> m_run_no_boxes;
  /// A list of the full workspace names
  std::set<QString> m_workspaceNames;
  /// Stores the last output workspace from single run mode, should be emptied when run in batch mode
  QString m_outputWS;
  /// A signal mapper to pick up various button clicks
  QSignalMapper *m_reducemapper;
  /// A flag to mark that warnings have been issued about geometry issues
  bool m_warnings_issued;
  /// A flag that causes the reload of the data
  bool m_force_reload;
  /// Holds pointers to the check box for each supported save format with the name of its save algorithm
  QHash<const QCheckBox * const, QString> m_savFormats;
  typedef QHash<const QCheckBox * const, QString>::const_iterator SavFormatsConstIt;
  /// A flag indicating there were warning messsages in the log
  bool m_log_warnings;
  /// Get notified when the system input directories have changed
  Poco::NObserver<SANSRunWindow, Mantid::Kernel::ConfigValChangeNotification> m_newInDir;
  /// An observer for a delete notification from Mantid
  Poco::NObserver<SANSRunWindow, Mantid::API::WorkspaceDeleteNotification> m_delete_observer;
  /// A map of S2D detector names to QLabel pointers
  QList<QHash<QString, QLabel*> > m_s2d_detlabels;
  /// A map of LOQ detector names to QLabel pointers
  QList<QHash<QString, QLabel*> > m_loq_detlabels;
  /// A map of allowed batch csv tags to column numbers
  QHash<QString,int> m_allowed_batchtags;
  /// An integer to save the last run reduction type
  int m_lastreducetype;
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
  //Time/Pixel mask string
  QString m_maskScript;
  
  void initAnalysDetTab();
  void makeValidator(QLabel * const newValid, QWidget * control, QWidget * tab, const QString & errorMsg);
  void upDateDataDir();
  void handleInputDirChange(Mantid::Kernel::ConfigValChangeNotification_ptr pDirInfo);
  QString getInstrumentClass() const;
  bool entriesAreValid(const ValCheck check=ALL);
  bool entriesAreValid(ValMap & vals);
  bool runFilesAreValid();
  QString reduceSingleRun() const;

  //A reference to a logger
  static Mantid::Kernel::Logger & g_log;
};

}
}

#endif //MANTIDQTCUSTOMINTERFACES_SANSRUNWINDOW_H_
