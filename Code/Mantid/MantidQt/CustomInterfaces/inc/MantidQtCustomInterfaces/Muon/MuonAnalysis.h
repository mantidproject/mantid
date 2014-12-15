#ifndef MANTIDQTCUSTOMINTERFACES_MUONANALYSIS_H_
#define MANTIDQTCUSTOMINTERFACES_MUONANALYSIS_H_

//----------------------
// Includes
//----------------------
#include "ui_MuonAnalysis.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"

#include "MantidGeometry/Instrument.h"

#include "MantidQtAPI/UserSubWindow.h"
#include "MantidQtMantidWidgets/pythonCalc.h"
#include "MantidQtMantidWidgets/MWDiag.h"

#include <map>

namespace MantidQt
{
namespace CustomInterfaces
{

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace Muon
{
  // Tab classes
  class MuonAnalysisOptionTab;
  class MuonAnalysisFitDataTab;
  class MuonAnalysisResultTableTab;
  struct Grouping;

  struct LoadResult {
    Workspace_sptr loadedWorkspace;
    Workspace_sptr loadedGrouping;
    Workspace_sptr loadedDeadTimes;
    std::string mainFieldDirection;
    double timeZero;
    double firstGoodData;
    std::string label;
  };

  struct GroupResult {
    bool usedExistGrouping;
    boost::shared_ptr<Grouping> groupingUsed;
    Workspace_sptr groupedWorkspace;
  };
}

using namespace Muon;

/** 
This is the main class for the MuonAnalysis interface
see <http://www.mantidproject.org/MuonAnalysis>.    

@author Anders Markvardsen, ISIS, RAL

Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>    
*/


class MuonAnalysis : public MantidQt::API::UserSubWindow
{
  Q_OBJECT

public:
  /// Name of the interface
  static std::string name() { return "Muon Analysis"; }
  // This interface's categories.
  static QString categoryInfo() { return "Muon"; }

  /// Default Constructor
  MuonAnalysis(QWidget *parent = 0);

signals:
  /// Request to hide/show Mantid toolbars
  void setToolbarsHidden(bool isHidden); 

private slots:
  /// Guess Alpha clicked
  void guessAlphaClicked();

  /// Checks whether two specified periods are equal and, if they are, sets second one to None
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
  void userSelectInstrument(const QString& prefix);

  /// Hide/show MantidPlot toolbars
  void doSetToolbarsHidden(bool hidden);

  /// Run the plot button on the home tab.
  void runFrontPlotButton();

  /// Creates a plot of selected group/pair.
  void plotSelectedItem();

  /// Link to the wiki for the home tab
  void muonAnalysisHelpClicked();

  /// Link to the wiki for the grouping tab.
  void muonAnalysisHelpGroupingClicked();

  /// Check to see if the user want to append the previous run and set accordingly
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
  void groupTabUpdatePlot();

  /// Sets plot type combo box on the Home tab to the same value as the one under Group Table
  void syncGroupTablePlotTypeWithHome();

  /// Updates the style of the current plot according to actual parameters on settings tab.
  void updateCurrentPlotStyle();

  /// Checks whether plots should be auto-updated when some settings change.
  bool isAutoUpdateEnabled();

  /// Whether Overwrite option is enabled on the Settings tab.
  bool isOverwriteEnabled();

  /// Checks if the plot for the workspace does exist.
  bool plotExists(const QString& wsName);

  /// Enable PP tool for the plot of the given WS
  void selectMultiPeak(const QString& wsName);

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

  /// Change to the dead time file, make sure graph is updated next time it is plotted.
  void deadTimeFileSelected();

  /// Updates the enabled-state and value of Time Zero using "auto" check-box state
  void setTimeZeroState(int checkBoxState = -1);

  /// Updates the enabled-state and value of First Good Data using "auto" check-box state
  void setFirstGoodDataState(int checkBoxState = -1);

  /// Saves the value of the widget which called the slot
  void saveWidgetValue();

  /// Opens a sequential fit dialog
  void openSequentialFitDialog();

  /// Update front
  void updateFront();

  /// Opens the managed directory dialog for easier access for the user.
  void openDirectoryDialog();

private:
 
  /// Types of entities we are dealing with
  enum ItemType { Pair, Group };
  
  /// Possible plot types users might request
  enum PlotType { Asymmetry, Counts, Logorithm };

  /// Types of periods
  enum PeriodType { First, Second };

  /// Initialize local Python environment
  void initLocalPython();

  /// Initialize the layout
  void initLayout();

  /// Set start up interface look
  void startUpLook();

  /// Change the connected data name
  void setCurrentDataName(const QString& name);

  /// Executed when interface gets hidden or closed
  void hideEvent(QHideEvent *e);
  
  /// Executed when interface gets shown
  void showEvent(QShowEvent *e);

  /// Input file changed - update GUI accordingly
  void inputFileChanged(const QStringList& filenames);

  /// Loads the given list of files
  boost::shared_ptr<LoadResult> load(const QStringList& files) const;

  /// Groups the loaded workspace
  boost::shared_ptr<GroupResult> group(boost::shared_ptr<LoadResult> loadResult) const;

  /// Set whether the loading buttons and MWRunFiles widget are enabled.
  void allowLoading(bool enabled);

  /// Return the pair which is in focus and -1 if none
  int pairInFocus();

  /// is grouping set
  bool isGroupingSet() const;

  /// Crop/rebins/offsets the workspace according to interface settings. 
  MatrixWorkspace_sptr prepareAnalysisWorkspace(MatrixWorkspace_sptr ws, bool isRaw);

  /// Creates workspace for specified group/pair and plots it 
  void plotItem(ItemType itemType, int tableRow, PlotType plotType);
  
  /// Creates workspace ready for analysis and plotting
  MatrixWorkspace_sptr createAnalysisWorkspace(ItemType itemType, int tableRow, PlotType type,
    bool isRaw = false);

  /// Returns PlotType as chosen using given selector 
  PlotType parsePlotType(QComboBox* selector);

  /// Finds a name for new analysis workspace 
  std::string getNewAnalysisWSName(ItemType itemType, int tableRow, PlotType plotType);

  /// Selects a workspace from the group according to what is selected on the interface for the period
  MatrixWorkspace_sptr getPeriodWorkspace(PeriodType periodType, WorkspaceGroup_sptr group);

  /// Update front anc pair combo box
  void updateFrontAndCombo();

  /// Updates widgets related to period algebra
  void updatePeriodWidgets(size_t numPeriods);

  /// Calculate number of detectors from string of type 1-3, 5, 10-15
  int numOfDetectors(const std::string& str) const;

  /// is string a number?
  bool isNumber(const std::string& s) const;

  /// Clear tables and front combo box
  void clearTablesAndCombo();

  /// Deletes a workspace _or_ a workspace group with the given name, if one exists
  void deleteWorkspaceIfExists(const std::string& wsName);

  ///Return true if data are loaded
  bool areDataLoaded();

  /// Return number of pairs
  int numPairs();

  /// Return number of groups defined (not including pairs)
  int numGroups();

  /// Returns custom dead time table file name as set on the interface
  std::string deadTimeFilename() const;

  /// Loads dead time table (group of tables) from the file.
  Workspace_sptr loadDeadTimes(const std::string& filename) const;

  /// Applies dead time correction to the loaded workspace
  void applyDeadTimeCorrection(boost::shared_ptr<LoadResult> loadResult) const;

  /// Creates and algorithm with all the properties set according to widget values on the interface
  Algorithm_sptr createLoadAlgorithm();

  /// Plots specific WS spectrum (used by plotPair and plotGroup)
  void plotSpectrum(const QString& wsName, bool logScale = false);

  /// Get current plot style parameters. wsName and wsIndex are used to get default values if 
  /// something is not specified
  QMap<QString, QString> getPlotStyleParams(const QString& wsName);

  /// get period labels
  QStringList getPeriodLabels() const;

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
  QWidget* m_currentTab;

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

  /// Returns params string which can be passed to Rebin, according to what user specified
  std::string rebinParams(Workspace_sptr wsForRebin);

  /// title of run
  std::string m_title;

  /// group defaults are saved to
  QString m_settingsGroup;

  /// Boolean to show whether the gui is being updated
  bool m_updating;

  /// Flag to indicate that grouping table is being updated
  bool m_updatingGrouping;

  /// Boolean to show when data has been loaded. (Can't auto-update data that hasn't been loaded)
  bool m_loaded;

  /// If the dead times have changed.
  bool m_deadTimesChanged;

  /// The working run or directory displayed in mwRunFiles
  QString m_textToDisplay;

  /// Load auto saved values
  void loadAutoSavedValues(const QString& group);

  /// connect the settings for the fit function to their respective slots
  void loadFittings();

  /// Add or take one away from the range of files
  void setAppendingRun(int inc);

  /// change and load the run depending on the value passed as a parameter
  void changeRun(int amountToChange);

  /// Separate the muon file. The current File will remove the path (i.e MUSR002413.nxs)
  void separateMuonFile(QString & filePath, QString & currentFile, QString & run, int & runSize);

  /// Include the 0's fromt eh beginning of the file that were lost in conversion from QString to int
  void getFullCode(int originalSize, QString & run);

  /// Setup the signals for updating
  void connectAutoUpdate();

  /// Setup connects for saving values using QSettings
  void connectAutoSave();

  /// Saves the value of the widget which called the slot
  void loadWidgetValue(QWidget* target, const QVariant& defaultValue);

  /// Groups the workspace
  Workspace_sptr groupWorkspace(Workspace_sptr ws, Workspace_sptr grouping) const;

  /// Groups the workspace
  Workspace_sptr groupWorkspace(const std::string& wsName, const std::string& groupingName) const;

  /// Groups loaded workspace using information from Grouping Options tab
  void groupLoadedWorkspace();

  /// Parses grouping information from the UI table.
  ITableWorkspace_sptr parseGrouping();  

  /// When no data loaded set various buttons etc to inactive
  void noDataAvailable();

  /// When data loaded set various buttons etc to active
  void nowDataAvailable();

  /// handles option tab work
  MantidQt::CustomInterfaces::Muon::MuonAnalysisOptionTab* m_optionTab;
  /// handles fit data work
  MantidQt::CustomInterfaces::Muon::MuonAnalysisFitDataTab* m_fitDataTab;
  /// handles result table tab work
  MantidQt::CustomInterfaces::Muon::MuonAnalysisResultTableTab* m_resultTableTab;

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

};

}
}

#endif //MANTIDQTCUSTOMINTERFACES_MUONANALYSIS_H_
