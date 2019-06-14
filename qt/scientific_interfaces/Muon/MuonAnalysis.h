// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_MUONANALYSIS_H_
#define MANTIDQTCUSTOMINTERFACES_MUONANALYSIS_H_

//----------------------
// Includes
//----------------------
#include "ui_MuonAnalysis.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/GroupingLoader.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include "MantidGeometry/Instrument.h"

#include "IO_MuonGrouping.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"
#include "MuonAnalysisDataLoader.h"
#include "MuonAnalysisHelper.h"

#include <boost/optional/optional.hpp>
#include <map>

namespace MantidQt {
namespace MantidWidgets {
class FunctionBrowser;
class MuonFitDataSelector;
} // namespace MantidWidgets
namespace CustomInterfaces {
class MuonAnalysisFitDataPresenter;
class MuonAnalysisFitFunctionPresenter;

namespace Muon {
// Tab classes
class MuonAnalysisOptionTab;
class MuonAnalysisFitDataTab;
class MuonAnalysisResultTableTab;

struct GroupResult {
  bool usedExistGrouping;
  boost::shared_ptr<Mantid::API::Grouping> groupingUsed;
};
} // namespace Muon

/**
This is the main class for the MuonAnalysis interface
see <http://www.mantidproject.org/MuonAnalysis>.

@author Anders Markvardsen, ISIS, RAL
*/

class MuonAnalysis : public MantidQt::API::UserSubWindow {
  Q_OBJECT

public:
  /// Name of the interface
  static std::string name() { return "Muon Analysis"; }
  // This interface's categories.
  static QString categoryInfo() { return "Muon"; }

  /// Default Constructor
  explicit MuonAnalysis(QWidget *parent = nullptr);

  /// Destructor
  ~MuonAnalysis();

  /// Gets current index of the group or pair to plot
  int getGroupOrPairToPlot() const;

  /// Sets the current index of the group or pair to plot
  void setGroupOrPairIndexToPlot(int index);

  /// Plots the currently selected group or pair
  void plotCurrentGroupAndPairs();

signals:
  /// Request to hide/show Mantid toolbars
  void setToolbarsHidden(bool isHidden);

private slots:
  /// Guess Alpha clicked
  void guessAlphaClicked();
  void handleGroupBox();
  void handlePeriodBox();
  /// Checks whether two specified periods are equal and, if they are, sets
  /// second one to None
  void checkForEqualPeriods();

  /// Input file changed in MWRunFiles widget
  void inputFileChanged_MWRunFiles();

  // Load current file.
  void runLoadCurrent();

  /// group table changed
  void groupTableChanged(int row, int column);

  // group table clicked
  void groupTableClicked(int row, int column);

  // group table vertical label clicked
  void groupTableClicked(int row);

  /// group table changed
  void pairTableChanged(int row, int column);

  // pair table clicked
  void pairTableClicked(int row, int column);

  // pair table vertical lable clicked
  void pairTableClicked(int row);

  /// group table plot button
  void runGroupTablePlotButton();

  /// group table plot button
  void runPairTablePlotButton();

  /// Save grouping button
  void runSaveGroupButton();

  /// Load grouping button
  void runLoadGroupButton();

  /// Clear grouping button
  void runClearGroupingButton();

  /// User select instrument
  void userSelectInstrument(const QString &prefix);

  /// Hide/show MantidPlot toolbars
  void doSetToolbarsHidden(bool hidden);

  /// Run the plot button on the home tab.
  void runFrontPlotButton();

  /// Creates a plot of selected group/pair.
  void plotSelectedGroupPair();

  /// Link to the wiki for the home tab
  void muonAnalysisHelpClicked();

  /// Link to the wiki for the grouping tab.
  void muonAnalysisHelpGroupingClicked();

  /// Check to see if the user want to append the previous run and set
  /// accordingly
  void checkAppendingPreviousRun();

  /// Check to see if the user want to append the next run and set accordingly
  void checkAppendingNextRun();

  /// When the tab has changed.
  void changeTab(int newTabIndex);

  /// Update the plot based on changes on the front tab
  void homeTabUpdatePlot();

  /// Update the plot based on changes on the settings tab
  void settingsTabUpdatePlot();

  /// Update the plot based on changes on the grouping options tab
  void groupTabUpdatePlotGroup();
  void groupTabUpdatePlotPair();

  /// Sets plot type combo box on the Home tab to the same value as the one
  /// under Group Table
  void syncGroupTablePlotTypeWithHome();

  /// Updates the style of the current plot according to actual parameters on
  /// settings tab.
  void updateCurrentPlotStyle();

  /// Checks whether plots should be auto-updated when some settings change.
  bool isAutoUpdateEnabled();

  /// Whether Overwrite option is enabled on the Settings tab.
  bool isOverwriteEnabled();

  /// Checks if the plot for the workspace does exist.
  bool plotExists(const QString &wsName);

  /// Enable PP tool for the plot of the given WS and optional filepath
  void selectMultiPeak(const QString &wsName, const bool update,
                       const boost::optional<QString> &filePath);

  /// Enable PP tool for the plot of the given WS overload to take just a ws
  void selectMultiPeak(const QString &wsName);
  /// Enable PP tool for the plot of the given WS overload to take just a ws
  void selectMultiPeakNoUpdate(const QString &wsName);
  /// Disable tools for all the graphs within MantidPlot
  void disableAllTools();

  /// Hides all the plot windows (MultiLayer ones)
  void hideAllPlotWindows();

  /// Shows all the plot windows (MultiLayer ones)
  void showAllPlotWindows();

  /// Called when dead time correction type is changed.
  void onDeadTimeTypeChanged(int choice);

  /// Auto-update the plot after user has changed dead time correction type.
  void deadTimeTypeAutoUpdate(int choice);

  /// Change to the dead time file, make sure graph is updated next time it is
  /// plotted.
  void deadTimeFileSelected();

  /// Updates the enabled-state and value of Time Zero using "auto" check-box
  /// state
  void setTimeZeroState(int checkBoxState = -1);

  /// Updates the enabled-state and value of First Good Data using "auto"
  /// check-box state
  void setFirstGoodDataState(int checkBoxState = -1);

  /// Saves the value of the widget which called the slot
  void saveWidgetValue();

  /// Update front
  void updateFront();

  /// Opens the managed directory dialog for easier access for the user.
  void openDirectoryDialog();

  /// Called when selected workspace/groups/periods to fit changes
  void dataToFitChanged();

  /// Called when "enable multi fit" checkbox is turned on/off
  void multiFitCheckboxChanged(int state);
  bool safeToLoadAllGroupsOrPairs();
  void loadAllGroups(int state = 0);
  void loadAllPairs(int state = 0);

  /// Called when "overwrite" is changed
  void updateDataPresenterOverwrite(int state);
  // update the displayed normalization
  void updateNormalization(QString name);

private:
  void moveUnNormWS(const std::string &name, std::vector<std::string> &wsNames,
                    bool raw);
  bool getIfTFAsymmStore() const;
  /// Initialize local Python environment
  void initLocalPython() override;

  /// Initialize the layout
  void initLayout() override;

  /// Set start up interface look
  void startUpLook();

  /// Change the connected data name
  void setCurrentDataName(const QString &name);

  /// Executed when interface gets hidden or closed
  void hideEvent(QHideEvent *e) override;

  /// Executed when interface gets shown
  void showEvent(QShowEvent *e) override;

  /// Input file changed - update GUI accordingly
  void inputFileChanged(const QStringList &filenames);

  /// Get grouping for the loaded workspace
  boost::shared_ptr<Muon::GroupResult>
  getGrouping(boost::shared_ptr<Muon::LoadResult> loadResult) const;

  /// Set whether the loading buttons and MWRunFiles widget are enabled.
  void allowLoading(bool enabled);

  /// is grouping set
  bool isGroupingSet() const;

  /// Creates workspace for specified group/pair and plots it
  void plotItem(Muon::ItemType itemType, int tableRow, Muon::PlotType plotType);
  std::string addItem(Muon::ItemType itemType, int tableRow,
                      Muon::PlotType plotType);
  /// Creates workspace ready for analysis and plotting
  Mantid::API::Workspace_sptr createAnalysisWorkspace(Muon::ItemType itemType,
                                                      int tableRow,
                                                      Muon::PlotType plotType,
                                                      std::string wsName,
                                                      bool isRaw = false);

  /// Returns PlotType as chosen using given selector
  Muon::PlotType parsePlotType(QComboBox *selector);

  /// Finds a name for new analysis workspace
  std::string getNewAnalysisWSName(Muon::ItemType itemType, int tableRow,
                                   Muon::PlotType plotType);

  /// Update front and pair combo box
  void updateFrontAndCombo(bool updateIndexAndPlot);

  /// Updates widgets related to period algebra
  void updatePeriodWidgets(size_t numPeriods);

  /// Calculate number of detectors from string of type 1-3, 5, 10-15
  int numOfDetectors(const std::string &str) const;

  /// Clear tables and front combo box
  void clearTablesAndCombo();

  /// Clear run info and loaded run
  void clearLoadedRun();

  /// Deletes a workspace _or_ a workspace group with the given name, if one
  /// exists
  void deleteWorkspaceIfExists(const std::string &wsName);

  /// Return number of pairs
  int numPairs();

  /// Return number of groups defined (not including pairs)
  int numGroups();

  /// Returns custom dead time table file name as set on the interface
  std::string deadTimeFilename() const;

  /// Plots specific WS spectrum (used by plotPair and plotGroup)
  void plotSpectrum(const QString &wsName, bool logScale = false);

  /// set labels for a single data set
  void updateLabels(std::string &name);

  void setGroupsAndPairs();

  /// Get current plot style parameters. wsName and wsIndex are used to get
  /// default values if
  /// something is not specified
  QMap<QString, QString> getPlotStyleParams(const QString &wsName);

  /// get period labels
  std::string getPeriodLabels() const;

  // The form generated by Qt Designer
  Ui::MuonAnalysis m_uiForm;

  /// group plot functions
  QStringList m_groupPlotFunc;

  /// pair plot functions
  QStringList m_pairPlotFunc;

  /// The last directory that was viewed
  QString m_last_dir;

  /// Name of the loaded workspace
  std::string m_workspace_name;

  /// Name of the loaded AND grouped workspace
  std::string m_grouped_name;

  /// name of the loaded data
  QString m_currentDataName;

  /// which group table row has the user last clicked on
  int m_groupTableRowInFocus;

  /// which pair table row has the user last clicked on
  int m_pairTableRowInFocus;

  /// Widget of the current tab
  QWidget *m_currentTab;

  /// used to test that a new filename has been entered
  QStringList m_previousFilenames;

  /// List of current group names
  std::vector<std::string> m_groupNames;

  /// Deal with input file changes.
  void handleInputFileChanges();

  ///
  void updatePairTable();

  /// Currently selected instrument
  QString m_curInterfaceSetup;

  /// tell which group is in which row
  std::vector<int> m_pairToRow;

  /// tell which group is in which row
  std::vector<int> m_groupToRow;

  /// Return the group-number for the group in a row.
  /// Return -1 if invalid group in row
  int getGroupNumberFromRow(int row);

  /// Return the pair-number for the pair in a row.
  /// Return -1 if invalid pair in row
  int getPairNumberFromRow(int row);

  /// first good bin returned in ms
  double firstGoodBin() const;

  /// Returns start X value as specified by user
  double startTime() const;

  /// Return finish X value as specified by user
  double finishTime() const;

  /// time zero returned in ms
  double timeZero();

  /// Returns params string which can be passed to Rebin, according to what user
  /// specified
  std::string rebinParams(Mantid::API::Workspace_sptr wsForRebin);

  /// Updates rebin params in the fit data presenter
  void updateRebinParams();

  /// title of run
  std::string m_title;

  /// group defaults are saved to
  QString m_settingsGroup;

  /// Boolean to show whether the gui is being updated
  bool m_updating;

  /// Flag to indicate that grouping table is being updated
  bool m_updatingGrouping;

  /// Boolean to show when data has been loaded. (Can't auto-update data that
  /// hasn't been loaded)
  bool m_loaded;

  /// If the dead times have changed.
  bool m_deadTimesChanged;

  /// The working run or directory displayed in mwRunFiles
  QString m_textToDisplay;

  /// Load auto saved values
  void loadAutoSavedValues(const QString &group);

  /// connect the settings for the fit function to their respective slots
  void loadFittings();

  /// Add or take one away from the range of files
  void setAppendingRun(int inc);

  /// change and load the run depending on the value passed as a parameter
  void changeRun(int amountToChange);

  /// Separate the muon file. The current File will remove the path (i.e
  /// MUSR002413.nxs)
  void separateMuonFile(QString &filePath, QString &currentFile, QString &run,
                        int &runSize);

  /// Include the 0's fromt eh beginning of the file that were lost in
  /// conversion from QString to int
  void getFullCode(int originalSize, QString &run);

  /// Sets the fitting ranges
  void setFittingRanges(double xmin, double xmax);

  /// Setup the signals for updating
  void connectAutoUpdate();

  /// Setup connects for saving values using QSettings
  void connectAutoSave();

  /// Saves the value of the widget which called the slot
  void loadWidgetValue(QWidget *target, const QVariant &defaultValue);

  /// Groups the workspace
  Mantid::API::Workspace_sptr
  groupWorkspace(const std::string &wsName,
                 const std::string &groupingName) const;

  /// Groups loaded workspace using information from Grouping Options tab
  void groupLoadedWorkspace();

  /// Parses grouping information from the UI table.
  Mantid::API::ITableWorkspace_sptr parseGrouping();

  /// When no data loaded set various buttons etc to inactive
  void noDataAvailable();

  /// When data loaded set various buttons etc to active
  void nowDataAvailable();

  /// Fills in the grouping table using information from provided Grouping
  void fillGroupingTable(const Mantid::API::Grouping &grouping);

  /// handles option tab work
  MantidQt::CustomInterfaces::Muon::MuonAnalysisOptionTab *m_optionTab;
  /// handles fit data work
  MantidQt::CustomInterfaces::Muon::MuonAnalysisFitDataTab *m_fitDataTab;
  /// handles result table tab work
  MantidQt::CustomInterfaces::Muon::MuonAnalysisResultTableTab
      *m_resultTableTab;

  /// Time Zero as loaded from Data file
  double m_dataTimeZero;

  /// First Good Data time as loaded from Data file
  double m_dataFirstGoodData;

  /// The label to use for naming / grouping all the new workspaces
  std::string m_currentLabel;

  /// Default widget values
  static const QString TIME_ZERO_DEFAULT;
  static const QString FIRST_GOOD_BIN_DEFAULT;

  static const QString NOT_AVAILABLE;

  /// Current number of periods
  size_t m_numPeriods;

  /// Grouping helper class
  Muon::MuonGroupingHelper m_groupingHelper;

  /// Get period number string in summed set
  std::string getSummedPeriods() const;

  /// Get period number string in subtracted set
  std::string getSubtractedPeriods() const;

  /// Run "Plot" button from group or pair table
  void runTablePlotButton(Muon::ItemType itemType);

  /// Cached value of config setting
  std::string m_cachedPeakRadius;
  static const std::string PEAK_RADIUS_CONFIG;

  /// Function browser widget for fit tab
  MantidQt::MantidWidgets::FunctionBrowser *m_functionBrowser;

  /// Data selector widget for fit tab
  MantidQt::MantidWidgets::MuonFitDataSelector *m_dataSelector;

  /// Presenter to get data to fit
  std::unique_ptr<MuonAnalysisFitDataPresenter> m_fitDataPresenter;

  /// Presenter to get fit function
  std::unique_ptr<MuonAnalysisFitFunctionPresenter> m_fitFunctionPresenter;

  /// Helper class to load data
  MuonAnalysisDataLoader m_dataLoader;

  /// Get list of supported instruments
  QStringList getSupportedInstruments();

  /// Enable/disable "load current run" - if allowed
  void setLoadCurrentRunEnabled(bool enabled);

  /// Check if next/previous run should be appended
  void checkAppendingRun(const int direction);

  /// Set the Grouping and Data Analysis tabs enabled/disabled
  void setAnalysisTabsEnabled(const bool enabled);

  void setChosenGroupAndPeriods(const QString &wsName);

  /// set the group/pair name
  std::string m_groupPairName;
  int m_deadTimeIndex;
  bool m_useDeadTime;
};
} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_MUONANALYSIS_H_
