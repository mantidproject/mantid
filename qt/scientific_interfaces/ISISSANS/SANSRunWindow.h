// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_SANSRUNWINDOW_H_
#define MANTIDQTCUSTOMINTERFACES_SANSRUNWINDOW_H_

//----------------------
// Includes
//----------------------
#include "MantidQtWidgets/Common/SaveWorkspaces.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"
#include "SANSAddFiles.h"
#include "SANSConstants.h"
#include "SANSDiagnostics.h"
#include "SANSPlotSpecial.h"
#include "ui_SANSRunWindow.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/ConfigService.h"
#include <Poco/NObserver.h>
#include <QHash>
#include <QSettings>
#include <QStringList>
#include <vector>

namespace Mantid {
namespace Kernel {
class Logger;
}
namespace API {
class MatrixWorkspace;
}
} // namespace Mantid

class QAction;

namespace MantidQt {
namespace CustomInterfaces {
/**
Implements the SANS, small angle neutron scattering, dialog box

@author Martyn Gigg
*/
class SANSRunWindow : public MantidQt::API::UserSubWindow {
  Q_OBJECT
  Q_ENUMS(States)

public:
  /// Name of the interface
  static std::string name() { return "Old ISIS SANS (Deprecated)"; }
  // This interface's categories.
  static QString categoryInfo() { return "SANS"; }

  /// Stores the batch or single run mode selection
  enum States {
    NoSample, ///< No sample workspace has yet been loaded
    Loading,  ///< Workspaces are loading
    Ready,    ///< A sample workspace is loaded and the reduce buttons should be
    /// active
    OneD, ///< Signifies a 1D reduction
    TwoD  ///< For 2D reductions
  };
  /// Default Constructor
  explicit SANSRunWindow(QWidget *parent = nullptr);
  /// Destructor
  ~SANSRunWindow() override;

signals:
  /// Indicate the state of the loaded data.
  void dataReadyToProcess(bool state);
  // signal  to notify mask file loaded
  void userfileLoaded();
  /// signal to send gemoetry information
  void sendGeometryInformation(QString & /*_t1*/, QString & /*_t2*/, QString & /*_t3*/, QString & /*_t4*/);

private:
  /// Stores the batch or single run mode selection
  enum RunMode { SingleMode = 0, BatchMode };

  /// mask type
  enum MaskType { DefaultMask = 0, TimeMask = 1, PixelMask = 2 };

  /// Enumerate the tabs of this interface.
  enum Tab {
    RUN_NUMBERS,
    REDUCTION_SETTINGS,
    GEOMETRY,
    MASKING,
    LOGGING,
    ADD_RUNS,
    DIAGNOSTICS,
    ONE_D_ANALYSIS
  };

  /// Enum for the two states of the Q Resolution aperture selection
  enum QResoluationAperture { CIRCULAR, RECTANGULAR };

  /// Initialize the layout
  void initLayout() override;
  /// Init Python environment
  void initLocalPython() override;
  /**@name Utility functions */
  //@{
  /// Initialise some of the data and signal connections in the save box
  void setupSaveBox();
  /// connect the buttons click events to signals
  void connectButtonSignals();
  void connectFirstPageSignals();
  void connectAnalysDetSignals();
  /// Create the necessary widget maps
  void initWidgetMaps();
  /// Read previous settings
  void readSettings();
  /// Sets the states of the save box controls to the states read from the
  /// QSettings object
  void readSaveSettings(QSettings &valueStore);
  /// Save settings
  void saveSettings();
  /// Stores the state of the save box controls in QSettings
  void saveSaveSettings(QSettings &valueStore);
  /// Run a reduce function
  QString runReduceScriptFunction(const QString &pycode);
  /// Trim python print markers
  void trimPyMarkers(QString &txt);
  /// Load the user file specified in the text field
  bool loadUserFile();
  /// Load a CSV file
  bool loadCSVFile();
  /// Set limits step and type options
  void setLimitStepParameter(const QString &pname, QString param,
                             QLineEdit *step_value, QComboBox *step_type);
  /// Construct mask table
  void updateMaskTable();
  /// Add spectrum masks to table
  void addSpectrumMasksToTable(const QString &mask_string,
                               const QString &det_name);
  /// Add a time mask string to the mask table
  void addTimeMasksToTable(const QString &mask_string, const QString &det_name);
  /// Append the given information as a new row to the masking table.
  void appendRowToMaskTable(const QString &type, const QString &detector,
                            const QString &details);
  void readNumberOfEntries(const QString &RunStep,
                           API::MWRunFiles *const output);
  QString readUserFileGUIChanges(const States type);
  QString readSampleObjectGUIChanges();
  /// Get the component distances
  void componentLOQDistances(
      boost::shared_ptr<const Mantid::API::MatrixWorkspace> workspace,
      double &lms, double &lsda, double &lsdb);
  /// Enable/disable user interaction
  void setProcessingState(const States action);
  /// Check for workspace name in the AnalysisDataService
  bool workspaceExists(const QString &ws_name) const;
  Mantid::API::MatrixWorkspace_sptr
  getGroupMember(Mantid::API::Workspace_const_sptr in, const int member) const;
  /// Construct a QStringList of the currently loaded workspaces
  QStringList currentWorkspaceList() const;
  /// Is the user file loaded
  bool isUserFileLoaded() const;
  /// Create a mask string
  void addUserMaskStrings(QString &exec_script, const QString &importCommand,
                          enum MaskType mType);
  /// Set geometry details
  void setGeometryDetails();
  /// Set the SANS2D geometry
  void setSANS2DGeometry(
      boost::shared_ptr<const Mantid::API::MatrixWorkspace> workspace,
      int wscode);
  /// Set LOQ geometry
  void setLOQGeometry(
      boost::shared_ptr<const Mantid::API::MatrixWorkspace> workspace,
      int wscode);
  /// Mark an error on a label
  void markError(QLabel *label);
  /// set the name of the output workspace, empty means there is no output
  void resetDefaultOutput(const QString &wsName = "");
  /// Run an assign command
  bool runAssign(int key, QString &logs);
  /// Load a scatter sample file or can run via Python objects using the passed
  /// Python command
  bool assignDetBankRun(API::MWRunFiles &runFile, const QString &assignFn);
  /// runs that contain only monitor counts can be direct or transmission runs
  bool assignMonitorRun(API::MWRunFiles &trans, API::MWRunFiles &direct,
                        const QString &assignFn);
  /// Get the detectors' names
  void fillDetectNames(QComboBox *output);
  QStringList getSaveAlgs();
  /// Handle a delete notification from Mantid
  void handleMantidDeleteWorkspace(
      Mantid::API::WorkspacePostDeleteNotification_ptr p_dnf);
  // Format a double in a string with a specfied colour, format and precision
  QString formatDouble(double value, const QString &colour = "black",
                       char format = 'f', int precision = 3);
  /// Issue a warning
  void raiseOneTimeMessage(const QString &msg, int index = -1);
  /// Reset the geometry box to blank
  void resetGeometryDetailsBox();
  /// Cleanup old raw files
  void cleanup();
  /// Flip the reload flag
  void forceDataReload(bool force = true);
  /// Browse for a file
  bool browseForFile(const QString &box_title, QLineEdit *file_field,
                     QString file_filter = QString());
  /// Add a csv line to the batch grid
  int addBatchLine(QString csv_line, QString separator = "");
  /// Save the batch file
  QString saveBatchGrid(const QString &filename = "");
  /// Check that the workspace can have the zero errors removed
  bool isValidWsForRemovingZeroErrors(QString &originalWorkspaceName);
  //@}
public slots:
  /// apply mask
  void applyMask(const QString &wsName, bool time_pixel);
  /// Create a zero error free clone for the specified workspace
  void createZeroErrorFreeClone(QString &originalWorkspaceName,
                                QString &clonedWorkspaceName);
  /// Destroy a zero error free cloned workspace
  void deleteZeroErrorFreeClone(QString &clonedWorkspaceName);

private slots:
  /// phi masking has changed
  void phiMaskingChanged();
  /// phi masking has changed
  void phiMaskingChanged(int i);
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
  /// deals with the save workspaces dialog box closing
  void saveWorkspacesClosed();
  /// Receive a load button click
  bool handleLoadButtonClick();
  /// Reduce button clicked
  void handleReduceButtonClick(const QString &type);
  /// Find centre button click handler
  void handleRunFindCentre();
  /// Save the output from a single run reduction in the user selected formats
  void handleDefSaveClick();
  void handleWavComboChange(int new_index);
  /// A ComboBox option change
  void handleStepComboChange(int new_index);
  /// Called when the show mask button has been clicked
  void handleShowMaskButtonClick();
  /// Handle the change in instrument
  void handleInstrumentChange();
  /// Record if that user has changed the default filename
  void setUserFname();
  /// Enables or disables the floodFile run widget
  void prepareFlood(int state);
  /// Enable the default save button only if there an output workspace and a
  /// filename to save it to
  void enableOrDisableDefaultSave();
  /// connected to the Multi-period check box it shows or hides the multi-period
  /// boxes on the file widgets
  void disOrEnablePeriods(const int /*tickState*/);
  /// Switch mode
  void switchMode();
  /// Paste to batch table
  void pasteToBatchTable();
  /// Clear the batch table
  void clearBatchTable();
  /// Clear logger
  void clearLogger();
  /// Default trans changed state
  void updateTransInfo(int state);
  /// So user can decide to use fixed q range or not
  void updateFrontDetQrange(int state);
  void updateMergeQRange(int state);
  void checkList();
  /// Adds a warning message to the tab title
  void setLoggerTabTitleToWarn();
  /// Handle selection of the transmission
  void transSelectorChanged(int /*currindex*/);
  void loadTransmissionSettings();

  void handleSlicePushButton();
  /// Open the help page of whichever tab the user is currently viewing.
  void openHelpPage();
  /// Transmission setting for M3
  void onTransmissionM3CheckboxChanged();
  /// Transmission setting for M4
  void onTransmissionM4CheckboxChanged();
  /// Transmission setting for Radius
  void onTransmissionRadiusCheckboxChanged();
  /// Transmission setting for ROI files
  void onTransmissionROIFilesCheckboxChanged();
  /// React to change in Left/Right checkbox
  void onLeftRightCheckboxChanged();
  /// React to change in Up/Down checkbox
  void onUpDownCheckboxChanged();
  /// Handle a change of the aperture geometry for QResolution
  void handleQResolutionApertureChange(int aperture);
  ///
  void onUpdateGeometryRequest();

private:
  /// used to specify the range of validation to do
  enum ValCheck {
    ALL,  ///< for checking all validators
    LOAD, ///< for checking the load validators only
    RUN   ///< for checking the run validators only
  };

  enum TransSettings { M3, M4, RADIUS, ROI };

  /// holds pointer to validators and their locations
  using ValMap = std::map<QWidget *const, std::pair<QWidget *, QWidget *>>;
  /// The form generated by Qt Designer
  Ui::SANSRunWindow m_uiForm;
  /// this object holds the functionality in the Add Files tab
  SANSAddFiles *m_addFilesTab;
  /// this object holds the functionality/ui for the "Display" tab
  SANSPlotSpecial *m_displayTab;

  SANSDiagnostics *m_diagnosticsTab;
  /// this points to a saveWorkspaces, which allows users to save any workspace,
  /// when one is opened
  MantidWidgets::SaveWorkspaces *m_saveWorkspaces;
  /// The data directory (as an absolute path)
  QString m_data_dir;
  /// The instrument definition directory
  QString m_ins_defdir;
  /// The last directory that was viewed
  QString m_last_dir;
  /// Is the user file loaded
  bool m_cfg_loaded;
  /// True if the user cahnged the default filename text, false otherwise
  bool m_userFname;
  /// The sample that was loaded
  QString m_sample_file;
  /// The workspace containing the experimental run one the sample under
  /// investigation
  QString m_experWksp;
  /// The workspace containing the can run
  QString m_experCan;
  /// List of all run entry widgets, which are on tab page 1
  std::vector<API::MWRunFiles *> m_runFiles;
  /// There validators are searched before a reduction begins. Where there is a
  /// problem focus goes to the widget linked to a validator whose tab is also
  /// stored in the pair. Disabling a validator QLabel disables checking that
  /// validator
  ValMap m_validators;
  /// List of all validators searched through before a load operation is
  /// possible
  ValMap m_loadValids;
  /// A list of the full workspace names
  std::set<QString> m_workspaceNames;
  /// Stores the last output workspace from single run mode, should be emptied
  /// when run in batch mode
  QString m_outputWS;
  /// A signal mapper to pick up various button clicks
  QSignalMapper *m_reducemapper;
  /// A flag to mark that warnings have been issued about geometry issues
  bool m_warnings_issued;
  /// A flag that causes the reload of the data
  bool m_force_reload;
  /// Holds pointers to the check box for each supported save format with the
  /// name of its save algorithm
  QHash<const QCheckBox *const, QString> m_savFormats;
  using SavFormatsConstIt =
      QHash<const QCheckBox *const, QString>::const_iterator;
  /// Get notified when the system input directories have changed
  Poco::NObserver<SANSRunWindow, Mantid::Kernel::ConfigValChangeNotification>
      m_newInDir;
  /// An observer for a delete notification from Mantid
  Poco::NObserver<SANSRunWindow, Mantid::API::WorkspacePostDeleteNotification>
      m_delete_observer;
  /// A map of S2D detector names to QLabel pointers
  QList<QHash<QString, QLabel *>> m_s2d_detlabels;
  /// A map of LOQ detector names to QLabel pointers
  QList<QHash<QString, QLabel *>> m_loq_detlabels;
  /// A map of allowed batch csv tags to column numbers
  QHash<QString, int> m_allowed_batchtags;
  /// Indicate if the reduce module has been loaded?
  bool m_have_reducemodule;
  /// A flag marking if the batch grid has been changed
  bool m_dirty_batch_grid;
  /// If set, the filename specified is removed after a batch run
  QString m_tmp_batchfile;
  /// A paste action for the batch table
  QAction *m_batch_paste;
  /// A clear action for the batch table
  QAction *m_batch_clear;
  // Time/Pixel mask string
  QString m_maskScript;
  /// Stores the URL of each tab's help page.
  QMap<Tab, QString> m_helpPageUrls;
  /// SANS constants
  SANSConstants m_constants;
  /// Validators
  QValidator *m_mustBeDouble;
  QValidator *m_doubleValidatorZeroToMax;
  QValidator *m_intValidatorZeroToMax;

  void initAnalysDetTab();
  void makeValidator(QLabel *const newValid, QWidget *control, QWidget *tab,
                     const QString &errorMsg);
  void upDateDataDir();
  void handleInputDirChange(
      Mantid::Kernel::ConfigValChangeNotification_ptr pDirInfo);
  QString getInstrumentClass() const;
  bool entriesAreValid(const ValCheck check = ALL);
  bool entriesAreValid(ValMap &vals);
  bool runFilesAreValid();
  QString reduceSingleRun() const;
  void setValidators();

  /// set logic for M3 or M4 selection
  void setM3M4Logic(TransSettings setting, bool isNowChecked);
  /// set logic for beam stop selection
  void setBeamStopLogic(TransSettings setting, bool isNowChecked);
  /// set logic for radius and mask
  void setRadiusAndMaskLogic(bool isNowChecked);
  /// set logic for ROI and mask
  void setROIAndMaskLogic(bool isNowChecked);
  /// validate float input
  // void validateNumericInput(QString numericInput);
  /// validate file input
  // void valideateFileInput(QString fileInput);
  /// set the transmission settings
  // void sendTransmissionSettings();
  /// get the transmission settings
  void setTransmissionSettingsFromUserFile();
  /// write the transmission settings to a python script
  void writeTransmissionSettingsToPythonScript(QString &pythonCode);
  /// initialize the connections for the transmission settings
  void initTransmissionSettings();
  /// Set all trans fields to a certain enabled state
  void resetAllTransFields();
  /// Reset the to M3
  void resetToM3IfNecessary();
  /// Check the validty of inputs
  bool areSettingsValid(States type);
  /// Check setting for wavelengths and Q values
  void checkWaveLengthAndQValues(bool &isValid, QString &message,
                                 QLineEdit *min, QLineEdit *max,
                                 QComboBox *selection, QString type);
  /// Checks if the save settings are valid for a particular workspace
  bool areSaveSettingsValid(const QString &workspaceName);
  /// Update the beam center fields
  void updateBeamCenterCoordinates();
  /// Set the beam finder details
  void setBeamFinderDetails();
  /// Gets the QResolution settings and shows them in the GUI
  void retrieveQResolutionSettings();
  /// Gets the QResolution settings for the aperture, decides for the the
  /// correct setting and displays it
  void retrieveQResolutionAperture();
  /// General getter for aperture settings of Q Resolution
  QString retrieveQResolutionGeometry(QString command);
  /// Write the QResolution GUI changes to a python script
  void writeQResolutionSettingsToPythonScript(QString &pythonCode);
  /// Write single line for Q Resolution
  void writeQResolutionSettingsToPythonScriptSingleEntry(
      QString value, QString code_entry, const QString lineEnding,
      QString &py_code) const;
  /// Sets the cirucular aperture, ie labels and populates the values with what
  /// is available from the user file
  void setupQResolutionCircularAperture();
  /// Sets the rectuanular aperture
  void setupQResolutionRectangularAperture(QString h1, QString w1, QString h2,
                                           QString w2);
  /// Set the rectangular aperture
  void setupQResolutionRectangularAperture();
  /// Set the aperture type
  void setQResolutionApertureType(QResoluationAperture apertureType,
                                  QString a1H1Label, QString a2H2Label,
                                  QString a1H1, QString a2H2,
                                  QString toolTipA1H1, QString toolTipA2H2,
                                  bool w1W2Disabled);
  /// Initialize the QResolution settings
  void initQResolutionSettings();

  /// Gets the BackgroundCorrection settings
  void retrieveBackgroundCorrection();
  /// Get Background runner
  SANSBackgroundCorrectionSettings
  retrieveBackgroundCorrectionSetting(bool isTime, bool isMon);
  /// Initialize the background correction
  void initializeBackgroundCorrection();
  /// Sets the BackgroundCorrection settings
  void writeBackgroundCorrectionToPythonScript(QString &pythonCode);
  /// Generic addition of background correction to python script
  void addBackgroundCorrectionToPythonScript(
      QString &pythonCode,
      MantidQt::CustomInterfaces::SANSBackgroundCorrectionSettings setting,
      bool isTimeBased);
  /// Check if the user file has a valid user file extension
  bool hasUserFileValidFileExtension();
  /// Check if the user file is valid
  bool isValidUserFile();
  /// Update IDF file path
  void updateIDFFilePath();
  /// Update IDF file path when running in Batch mode
  void updateIDFFilePathForBatch();
  //// Update IDF information
  void updateIDFInfo(const QString &command);

  UserSubWindow *slicingWindow;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_SANSRUNWINDOW_H_
