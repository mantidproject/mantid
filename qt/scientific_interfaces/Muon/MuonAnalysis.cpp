// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MuonAnalysis.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/ScopedWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidQtWidgets/Common/ManageUserDirectories.h"
#include "MantidQtWidgets/Common/MuonFitDataSelector.h"
#include "MantidQtWidgets/Common/MuonFitPropertyBrowser.h"
#include "MantidQtWidgets/Common/MuonFunctionBrowser.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertybrowser.h"
#include "MuonAnalysisFitDataPresenter.h"
#include "MuonAnalysisFitDataTab.h"
#include "MuonAnalysisFitFunctionPresenter.h"
#include "MuonAnalysisOptionTab.h"
#include "MuonAnalysisResultTableTab.h"
#include "MuonSequentialFitDialog.h"

#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/StringTokenizer.h>
#include <boost/lexical_cast.hpp>
#include <math.h>

#include <algorithm>

#include <QApplication>
#include <QFileDialog>
#include <QHash>
#include <QHeaderView>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QSettings>
#include <QSignalMapper>
#include <QTemporaryFile>
#include <QTextStream>
#include <QTreeWidgetItem>
#include <QVariant>

#include <fstream>

// Add this class to the list of specialised dialogs in this namespace
namespace MantidQt {
namespace CustomInterfaces {
DECLARE_SUBWINDOW(MuonAnalysis)

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace MantidQt::MantidWidgets;
using namespace MantidQt::CustomInterfaces;
using namespace MantidQt::CustomInterfaces::Muon;
using namespace Mantid::Geometry;
using namespace MuonAnalysisHelper;
using Mantid::API::Grouping;
using Mantid::API::Workspace_sptr;

namespace {
/// static logger
Mantid::Kernel::Logger g_log("MuonAnalysis");

void zoomYAxis(const QString &wsName, QMap<QString, QString> &params) {
  Workspace_sptr ws_ptr =
      AnalysisDataService::Instance().retrieve(wsName.toStdString());
  MatrixWorkspace_sptr matrix_workspace =
      boost::dynamic_pointer_cast<MatrixWorkspace>(ws_ptr);
  const auto &xData = matrix_workspace->x(0);

  const auto xMin = *min_element(xData.begin(), xData.end());
  const auto xMax = *max_element(xData.begin(), xData.end());
  // make our own y limits for plot (not all of the data)
  if (xMin < params["XAxisMin"].toDouble() ||
      xMax > params["XAxisMin"].toDouble()) {

    std::vector<double> yPlusEData, yMinusEData;
    for (size_t index = 0; index < matrix_workspace->e(0).size(); index++) {
      const auto &yData = matrix_workspace->y(0)[index];
      const auto &eData = matrix_workspace->e(0)[index];
      yPlusEData.push_back(yData + eData);
      yMinusEData.push_back(yData - eData);
    }

    auto xN = std::distance(xData.begin(),
                            std::upper_bound(xData.begin(), xData.end(),
                                             params["XAxisMax"].toDouble()));
    auto x0 = std::distance(xData.begin(),
                            std::lower_bound(xData.begin(), xData.end(),
                                             params["XAxisMin"].toDouble()));
    params["YAxisMax"] = QString::number(
        *max_element(yPlusEData.begin() + x0, yPlusEData.begin() + xN));
    params["YAxisMin"] = QString::number(
        *min_element(yMinusEData.begin() + x0, yMinusEData.begin() + xN));
    params["YAxisAuto"] = "False";
  } else {
    // make sure auto scale is on
    params["YAxisAuto"] = "True";
  }
}
} // namespace

// Static constants
const QString MuonAnalysis::NOT_AVAILABLE("N/A");
const QString MuonAnalysis::TIME_ZERO_DEFAULT("0.2");
const QString MuonAnalysis::FIRST_GOOD_BIN_DEFAULT("0.3");
const std::string MuonAnalysis::PEAK_RADIUS_CONFIG("curvefitting.peakRadius");

//----------------------
// Public member functions
//----------------------
/// Constructor
MuonAnalysis::MuonAnalysis(QWidget *parent)
    : UserSubWindow(parent), m_last_dir(), m_workspace_name("MuonAnalysis"),
      m_grouped_name(m_workspace_name + "Grouped"), m_currentDataName(),
      m_groupTableRowInFocus(0), m_pairTableRowInFocus(0),
      m_currentTab(nullptr), m_groupNames(),
      m_settingsGroup("CustomInterfaces/MuonAnalysis/"), m_updating(false),
      m_updatingGrouping(false), m_loaded(false), m_deadTimesChanged(false),
      m_textToDisplay(""), m_optionTab(nullptr), m_fitDataTab(nullptr),
      m_resultTableTab(nullptr), // Will be created in initLayout()
      m_dataTimeZero(0.0), m_dataFirstGoodData(0.0),
      m_currentLabel("NoLabelSet"), m_numPeriods(0),
      m_groupingHelper(this->m_uiForm), m_functionBrowser(nullptr),
      m_dataSelector(nullptr),
      m_dataLoader(Muon::DeadTimesType::None, // will be replaced by correct
                                              // instruments later
                   {"MUSR", "HIFI", "EMU", "ARGUS", "CHRONUS"}),
      m_deadTimeIndex(-1), m_useDeadTime(true) {}

/**
 * Destructor
 */
MuonAnalysis::~MuonAnalysis() {}

/**
 * Initialize local Python environmnet.
 */
void MuonAnalysis::initLocalPython() {
  QString code;

  code += "from mantid.simpleapi import *\n";

  // Needed for Python GUI API
  code += "from PyQt4.QtGui import QPen, QBrush, QColor\n"
          "from PyQt4.QtCore import QSize\n";

  runPythonCode(code);

  // TODO: Following shouldn't be here. It is now because ApplicationWindow sets
  // up the Python
  // environment only after the UserSubWindow is shown.

  // Hide the toolbars, if user wants to
  if (m_uiForm.hideToolbars->isChecked())
    emit setToolbarsHidden(true);
}

/// Set up the dialog layout
void MuonAnalysis::initLayout() {
  m_uiForm.setupUi(this);

  std::set<std::string> supportedFacilities;
  supportedFacilities.insert("ISIS");
  supportedFacilities.insert("SmuS");

  const std::string userFacility =
      ConfigService::Instance().getFacility().name();

  // Allow loading current run, provided platform and facility support this
  setLoadCurrentRunEnabled(true);

  // If facility is not supported by the interface - show a warning, but still
  // open it
  if (supportedFacilities.find(userFacility) == supportedFacilities.end()) {
    const std::string supportedFacilitiesStr = Strings::join(
        supportedFacilities.begin(), supportedFacilities.end(), ", ");

    const QString errorTemplate =
        "Your facility (%1) is not supported by MuonAnalysis, so you will not "
        "be able to load any files. \n\n"
        "Supported facilities are: %2. \n\n"
        "Please use Preferences -> Mantid -> Instrument to update your "
        "facility information.";

    const QString error =
        errorTemplate.arg(userFacility.c_str(), supportedFacilitiesStr.c_str());

    QMessageBox::warning(this, "Unsupported facility", error);
  }

  m_uiForm.fitBrowser->init();

  // alow appending files
  m_uiForm.mwRunFiles->allowMultipleFiles(true);

  // Further set initial look
  startUpLook();
  m_uiForm.mwRunFiles->readSettings(m_settingsGroup + "mwRunFilesBrowse");

  connect(m_uiForm.previousRun, SIGNAL(clicked()), this,
          SLOT(checkAppendingPreviousRun()));
  connect(m_uiForm.nextRun, SIGNAL(clicked()), this,
          SLOT(checkAppendingNextRun()));

  m_optionTab = new MuonAnalysisOptionTab(m_uiForm, m_settingsGroup);
  m_optionTab->initLayout();

  m_fitDataTab = new MuonAnalysisFitDataTab(m_uiForm);
  m_fitDataTab->init();
  m_resultTableTab = new MuonAnalysisResultTableTab(m_uiForm);
  connect(m_resultTableTab, SIGNAL(runPythonCode(const QString &, bool)), this,
          SIGNAL(runAsPythonScript(const QString &, bool)));

  setCurrentDataName(NOT_AVAILABLE);

  // Now we know the facility, update supported instruments
  m_dataLoader.setSupportedInstruments(getSupportedInstruments());

  // connect guess alpha
  connect(m_uiForm.guessAlphaButton, SIGNAL(clicked()), this,
          SLOT(guessAlphaClicked()));

  // signal/slot connections to respond to changes in instrument selection combo
  // boxes
  connect(m_uiForm.instrSelector,
          SIGNAL(instrumentSelectionChanged(const QString &)), this,
          SLOT(userSelectInstrument(const QString &)));

  // Load current
  connect(m_uiForm.loadCurrent, SIGNAL(clicked()), this,
          SLOT(runLoadCurrent()));

  // If group table change
  // currentCellChanged ( int currentRow, int currentColumn, int previousRow,
  // int previousColumn )
  connect(m_uiForm.groupTable, SIGNAL(cellChanged(int, int)), this,
          SLOT(groupTableChanged(int, int)));
  connect(m_uiForm.groupTable, SIGNAL(cellClicked(int, int)), this,
          SLOT(groupTableClicked(int, int)));
  connect(m_uiForm.groupTable->verticalHeader(), SIGNAL(sectionClicked(int)),
          SLOT(groupTableClicked(int)));

  // group table plot button
  connect(m_uiForm.groupTablePlotButton, SIGNAL(clicked()), this,
          SLOT(runGroupTablePlotButton()));

  // If pair table change
  connect(m_uiForm.pairTable, SIGNAL(cellChanged(int, int)), this,
          SLOT(pairTableChanged(int, int)));
  connect(m_uiForm.pairTable, SIGNAL(cellClicked(int, int)), this,
          SLOT(pairTableClicked(int, int)));
  connect(m_uiForm.pairTable->verticalHeader(), SIGNAL(sectionClicked(int)),
          SLOT(pairTableClicked(int)));
  // Pair table plot button
  connect(m_uiForm.pairTablePlotButton, SIGNAL(clicked()), this,
          SLOT(runPairTablePlotButton()));

  // save grouping
  connect(m_uiForm.saveGroupButton, SIGNAL(clicked()), this,
          SLOT(runSaveGroupButton()));

  // load grouping
  connect(m_uiForm.loadGroupButton, SIGNAL(clicked()), this,
          SLOT(runLoadGroupButton()));

  // clear grouping
  connect(m_uiForm.clearGroupingButton, SIGNAL(clicked()), this,
          SLOT(runClearGroupingButton()));

  // front plot button
  connect(m_uiForm.frontPlotButton, SIGNAL(clicked()), this,
          SLOT(runFrontPlotButton()));

  // front group/ group pair combobox
  connect(m_uiForm.frontGroupGroupPairComboBox,
          SIGNAL(currentIndexChanged(int)), this, SLOT(updateFront()));

  // Synchronize plot function selector on the Home tab with the one under the
  // Group Table
  connect(m_uiForm.frontPlotFuncs, SIGNAL(activated(int)),
          m_uiForm.groupTablePlotChoice, SLOT(setCurrentIndex(int)));
  connect(m_uiForm.groupTablePlotChoice, SIGNAL(activated(int)), this,
          SLOT(syncGroupTablePlotTypeWithHome()));

  connect(m_uiForm.homePeriodBox1, SIGNAL(textChanged(const QString &)), this,
          SLOT(checkForEqualPeriods()));
  connect(m_uiForm.homePeriodBox2, SIGNAL(textChanged(const QString &)), this,
          SLOT(checkForEqualPeriods()));

  connect(m_uiForm.hideToolbars, SIGNAL(toggled(bool)), this,
          SIGNAL(setToolbarsHidden(bool)));

  // connect "?" (Help) Button
  connect(m_uiForm.muonAnalysisHelp, SIGNAL(clicked()), this,
          SLOT(muonAnalysisHelpClicked()));
  connect(m_uiForm.muonAnalysisHelpGrouping, SIGNAL(clicked()), this,
          SLOT(muonAnalysisHelpGroupingClicked()));

  // add combo boxes to pairTable
  for (int i = 0; i < m_uiForm.pairTable->rowCount(); i++) {
    m_uiForm.pairTable->setCellWidget(i, 1, new QComboBox);
    m_uiForm.pairTable->setCellWidget(i, 2, new QComboBox);
  }

  // file input
  connect(m_uiForm.mwRunFiles, SIGNAL(fileFindingFinished()), this,
          SLOT(inputFileChanged_MWRunFiles()));

  connect(m_uiForm.timeZeroAuto, SIGNAL(stateChanged(int)), this,
          SLOT(setTimeZeroState(int)));
  connect(m_uiForm.firstGoodDataAuto, SIGNAL(stateChanged(int)), this,
          SLOT(setFirstGoodDataState(int)));

  // load previous saved values
  loadAutoSavedValues(m_settingsGroup);

  // connect the fit function widget buttons to their respective slots.
  loadFittings();

  // Detect when the tab is changed
  connect(m_uiForm.tabWidget, SIGNAL(currentChanged(int)), this,
          SLOT(changeTab(int)));

  connectAutoUpdate();

  connectAutoSave();

  connect(m_uiForm.deadTimeType, SIGNAL(currentIndexChanged(int)), this,
          SLOT(onDeadTimeTypeChanged(int)));

  connect(m_uiForm.mwRunDeadTimeFile, SIGNAL(fileFindingFinished()), this,
          SLOT(deadTimeFileSelected()));

  m_currentTab = m_uiForm.tabWidget->currentWidget();

  connect(this, SIGNAL(setToolbarsHidden(bool)), this,
          SLOT(doSetToolbarsHidden(bool)),
          Qt::QueuedConnection); // We dont' neet this to happen instantly,
                                 // prefer safer way

  // Manage User Directories
  connect(m_uiForm.manageDirectoriesBtn, SIGNAL(clicked()), this,
          SLOT(openDirectoryDialog()));
}

void MuonAnalysis::setChosenGroupAndPeriods(const QString &wsName) {
  const auto wsParams =
      MuonAnalysisHelper::parseWorkspaceName(wsName.toStdString());

  const QString &periodToSet = QString::fromStdString(wsParams.periods);
  const auto &periods = m_dataSelector->getPeriodSelections();

  if (!periodToSet.isEmpty() && !periods.contains(periodToSet)) {
    m_uiForm.fitBrowser->setChosenPeriods(periodToSet);
  }
  return;
}

/**
 * Muon Analysis help (slot)
 */
void MuonAnalysis::muonAnalysisHelpClicked() {
  MantidQt::API::HelpWindow::showCustomInterface(nullptr,
                                                 QString("Muon Analysis"));
}

/**
 * Muon Analysis Grouping help (slot)
 */
void MuonAnalysis::muonAnalysisHelpGroupingClicked() {
  MantidQt::API::HelpWindow::showCustomInterface(
      nullptr, QString("Muon Analysis"), QString("grouping-options"));
}

/**
 * Set the connected workspace name.
 * @param name The new connected ws name
 */
void MuonAnalysis::setCurrentDataName(const QString &name) {
  m_currentDataName = name;

  // Update labels
  m_uiForm.connectedDataHome->setText("Connected: " + m_currentDataName);
  m_uiForm.connectedDataGrouping->setText("Connected: " + m_currentDataName);
  m_uiForm.connectedDataSettings->setText("Connected: " + m_currentDataName);
}

/**
 * Front plot button (slot)
 */
void MuonAnalysis::runFrontPlotButton() {
  if (m_updating)
    return;

  if (m_deadTimesChanged) {
    inputFileChanged(m_previousFilenames);
    return;
  }

  plotSelectedGroupPair();
}

/**
 * Creates a plot of selected group/pair.
 */
void MuonAnalysis::plotSelectedGroupPair() {
  ItemType itemType;
  int tableRow;

  int index = getGroupOrPairToPlot();

  if (index < 0)
    return; // Nothing to plot

  if (index >= numGroups()) {
    itemType = Pair;
    tableRow = m_pairToRow[index - numGroups()];
  } else {
    itemType = Group;
    tableRow = m_groupToRow[index];
  }

  PlotType plotType = parsePlotType(m_uiForm.frontPlotFuncs);

  plotItem(itemType, tableRow, plotType);
}
/**
 * Creates workspace for specified group/pair and adds it to the ADS;
 * @param itemType :: Whether it's a group or pair
 * @param tableRow :: Row in the group/pair table which contains the item
 * @param plotType :: What kind of plot we want to analyse
 */
std::string MuonAnalysis::addItem(ItemType itemType, int tableRow,
                                  PlotType plotType) {
  AnalysisDataServiceImpl &ads = AnalysisDataService::Instance();

  // Find names for new workspaces
  const std::string wsName = getNewAnalysisWSName(itemType, tableRow, plotType);
  const std::string wsRawName = wsName + "_Raw";
  std::vector<std::string> wsNames = {wsName, wsRawName};
  // Create workspace and a raw (unbinned) version of it
  auto ws = createAnalysisWorkspace(itemType, tableRow, plotType, wsName);
  moveUnNormWS(wsName, wsNames, false);

  auto wsRaw =
      createAnalysisWorkspace(itemType, tableRow, plotType, wsRawName, true);
  moveUnNormWS(wsName, wsNames, true); // raw
  // Make sure they end up in the ADS
  ads.addOrReplace(wsName, ws);
  ads.addOrReplace(wsRawName, wsRaw);

  MuonAnalysisHelper::groupWorkspaces(m_currentLabel, wsNames);
  return wsName;
}

void MuonAnalysis::moveUnNormWS(const std::string &name,
                                std::vector<std::string> &wsNames, bool raw) {
  AnalysisDataServiceImpl &ads = AnalysisDataService::Instance();
  std::string unnorm = "_unNorm";
  if (raw) {
    unnorm += "_Raw";
  }
  if (ads.doesExist("tmp_unNorm")) {
    ads.rename("tmp_unNorm", name + unnorm);
    wsNames.push_back(name + unnorm);
  }
}
/**
 * Creates workspace for specified group/pair and plots it;
 * @param itemType :: Whether it's a group or pair
 * @param tableRow :: Row in the group/pair table which contains the item
 * @param plotType :: What kind of plot we want to analyse
 */
void MuonAnalysis::plotItem(ItemType itemType, int tableRow,
                            PlotType plotType) {
  m_updating = true;
  m_uiForm.fitBrowser->clearChosenPeriods();
  try {
    auto wsName = addItem(itemType, tableRow, plotType);

    QString wsNameQ = QString::fromStdString(wsName);

    // Plot the workspace
    plotSpectrum(wsNameQ, (plotType == Logarithm));
    setCurrentDataName(wsNameQ);
  } catch (std::exception &e) {
    g_log.error(e.what());
    QMessageBox::critical(this, "MuonAnalysis - Error",
                          "Unable to plot the item. Check log for details.");
  }
  loadAllGroups();
  loadAllPairs();
  m_updating = false;
}

/**
 * Finds a name for new analysis workspace.
 * Format: "INST00012345; Pair; long; Asym;[ 1;] #1"
 * @param itemType :: Whether it's a group or pair
 * @param tableRow :: Row in the group/pair table which contains the item
 * @param plotType :: What kind of plot we want to analyse
 * @return New name
 */
std::string MuonAnalysis::getNewAnalysisWSName(ItemType itemType, int tableRow,
                                               PlotType plotType) {
  Muon::DatasetParams params;

  params.label = m_currentLabel;
  params.itemType = itemType;
  auto table = (itemType == Pair) ? m_uiForm.pairTable : m_uiForm.groupTable;
  params.itemName = table->item(tableRow, 0)->text().toStdString();
  params.plotType = plotType;
  params.periods = getPeriodLabels();
  bool isItSummed = false;
  if (params.periods.find("+") != std::string::npos ||
      params.periods.find("-") != std::string::npos) {
    isItSummed = true;
  }
  if (params.periods != "" && isItSummed) {
    m_uiForm.fitBrowser->addPeriodCheckboxToMap(
        QString::fromStdString(params.periods));
  }

  // Version - always "#1" if overwrite is on, otherwise increment
  params.version = 1;
  std::string workspaceName = generateWorkspaceName(params);
  if (!isOverwriteEnabled()) {
    // If overwrite is disabled, need to find unique name for the new workspace
    while (AnalysisDataService::Instance().doesExist(workspaceName)) {
      params.version++;
      workspaceName = generateWorkspaceName(params);
    }
  }

  return workspaceName;
}

/**
 * Returns PlotType as chosen using given selector.
 * @param selector :: Widget to use for parsing
 * @return PlotType as selected using the widget
 */
CustomInterfaces::Muon::PlotType
MuonAnalysis::parsePlotType(QComboBox *selector) {
  std::string plotTypeName = selector->currentText().toStdString();

  if (plotTypeName == "Asymmetry") {
    return Asymmetry;
  } else if (plotTypeName == "Counts") {
    return Counts;
  } else if (plotTypeName == "Logarithm") {
    return Logarithm;
  } else {
    throw std::runtime_error("Unknown plot type name: " + plotTypeName);
  }
}

/**
 * Creates workspace ready for analysis and plotting.
 * @param itemType :: Whether it's a group or pair
 * @param tableRow :: Row in the group/pair table which contains the item
 * @param plotType :: What kind of plot we want to analyse
 * @param isRaw    :: Whether binning should be applied to the workspace
 * @param wsName   :: The name of the workspace -> for saving the normalisation
 * @return Created workspace
 */
Workspace_sptr MuonAnalysis::createAnalysisWorkspace(ItemType itemType,
                                                     int tableRow,
                                                     PlotType plotType,
                                                     std::string wsName,
                                                     bool isRaw) {
  auto loadedWS =
      AnalysisDataService::Instance().retrieveWS<Workspace>(m_grouped_name);
  Muon::AnalysisOptions options(m_groupingHelper.parseGroupingTable());
  options.summedPeriods = getSummedPeriods();
  options.subtractedPeriods = getSubtractedPeriods();
  options.timeZero = timeZero();           // user input
  options.loadedTimeZero = m_dataTimeZero; // from file
  options.timeLimits.first = firstGoodBin();
  options.timeLimits.second = finishTime();
  options.rebinArgs = isRaw ? "" : rebinParams(loadedWS);
  options.plotType = plotType;
  options.wsName = wsName;
  const auto *table =
      itemType == ItemType::Group ? m_uiForm.groupTable : m_uiForm.pairTable;
  options.groupPairName = table->item(tableRow, 0)->text().toStdString();
  m_groupPairName = table->item(tableRow, 0)->text().toStdString();
  return m_dataLoader.createAnalysisWorkspace(loadedWS, options);
}

/**
 * If the instrument selection has changed (slot)
 *
 * @param prefix :: instrument name from QComboBox object
 */
void MuonAnalysis::userSelectInstrument(const QString &prefix) {
  // Set file browsing to current instrument
  m_uiForm.mwRunFiles->setInstrumentOverride(prefix);

  if (prefix != m_curInterfaceSetup) {
    runClearGroupingButton();
    m_curInterfaceSetup = prefix;
    clearLoadedRun();

    // save this new choice
    QSettings group;
    group.beginGroup(m_settingsGroup + "instrument");
    group.setValue("name", prefix);
  }
}

/**
 * Save grouping button (slot)
 */
void MuonAnalysis::runSaveGroupButton() {
  if (numGroups() <= 0) {
    QMessageBox::warning(this, "MantidPlot - MuonAnalysis",
                         "No grouping to save.");
    return;
  }

  QSettings prevValues;
  prevValues.beginGroup(m_settingsGroup + "SaveOutput");

  // Get value for "dir". If the setting doesn't exist then use
  // the the path in "defaultsave.directory"
  QString prevPath =
      prevValues
          .value("dir",
                 QString::fromStdString(ConfigService::Instance().getString(
                     "defaultsave.directory")))
          .toString();

  QString filter;
  filter.append("Files (*.xml *.XML)");
  filter += ";;AllFiles (*)";
  QString groupingFile = QFileDialog::getSaveFileName(
      this, "Save Grouping file as", prevPath, filter);

  // Add extension if the groupingFile specified doesn't have one. (Solving
  // Linux problem).
  if (!groupingFile.endsWith(".xml"))
    groupingFile += ".xml";

  if (!groupingFile.isEmpty()) {
    Mantid::API::Grouping groupingToSave =
        m_groupingHelper.parseGroupingTable();
    MuonGroupingHelper::saveGroupingToXML(groupingToSave,
                                          groupingFile.toStdString());

    QString directory = QFileInfo(groupingFile).path();
    prevValues.setValue("dir", directory);
  }
}

/**
 * Load grouping button (slot)
 */
void MuonAnalysis::runLoadGroupButton() {
  m_updating = true;

  // Get grouping file
  QSettings prevValues;
  prevValues.beginGroup(m_settingsGroup + "LoadGroupFile");

  // Get value for "dir". If the setting doesn't exist then use
  // the the path in "defaultsave.directory"
  QString prevPath =
      prevValues
          .value("dir",
                 QString::fromStdString(ConfigService::Instance().getString(
                     "defaultload.directory")))
          .toString();

  QString filter;
  filter.append("Files (*.xml *.XML)");
  filter += ";;AllFiles (*)";
  QString groupingFile = QFileDialog::getOpenFileName(
      this, "Load Grouping file", prevPath, filter);
  if (groupingFile.isEmpty() || QFileInfo(groupingFile).isDir())
    return;

  QString directory = QFileInfo(groupingFile).path();
  prevValues.setValue("dir", directory);

  Mantid::API::Grouping loadedGrouping;

  try {
    Mantid::API::GroupingLoader::loadGroupingFromXML(groupingFile.toStdString(),
                                                     loadedGrouping);
  } catch (Exception::FileError &e) {
    g_log.error("Unable to load grouping. Data left unchanged");
    g_log.error(e.what());
    m_updating = false;
    return;
  }

  clearTablesAndCombo();
  fillGroupingTable(loadedGrouping);

  m_updating = false;

  if (m_loaded) {
    try {
      groupLoadedWorkspace();
    } catch (std::exception &e) {
      g_log.error(e.what());
      QMessageBox::critical(
          this, "MantidPlot - MuonAnalysis",
          "Unable to group the workspace. See log for details.");
    }
  }
}

/**
 * Clear grouping button (slot)
 */
void MuonAnalysis::runClearGroupingButton() { clearTablesAndCombo(); }

/**
 * Load current (slot)
 * N.B. This method will only work if
 * - using Windows
 * - connected to the ISIS network
 */
void MuonAnalysis::runLoadCurrent() {
  QString instname = m_uiForm.instrSelector->currentText().toUpper();

  if (instname == "EMU" || instname == "HIFI" || instname == "MUSR" ||
      instname == "CHRONUS" || instname == "ARGUS") {
    const QString instDirectory = instname;
    std::string autosavePointsTo = "";
    const std::string autosaveFile =
        "\\\\" + instDirectory.toStdString() + "\\data\\autosave.run";
    const Poco::File pathAutosave(autosaveFile);

    try // check if exists
    {
      if (pathAutosave.exists()) {
        std::ifstream autofileIn(autosaveFile.c_str(), std::ifstream::in);
        autofileIn >> autosavePointsTo;
      }
    } catch (const Poco::Exception &) {
      QString message("Can't read from the selected directory, either the "
                      "computer you are trying"
                      "\nto access is down or your computer is not "
                      "currently connected to the network.");
      message.append("\n\nRemote path: ").append(autosaveFile.c_str());
      QMessageBox::warning(this, "MantidPlot - MuonAnalysis", message);
      return;
    }

    // If this directory is not in Mantid's data search list, add it now
    // Must use forward slash format for this list, even on Windows
    const std::string autosaveDir =
        "//" + instDirectory.toStdString() + "/data";
    Mantid::Kernel::ConfigService::Instance().appendDataSearchDir(autosaveDir);

    QString psudoDAE;
    if (autosavePointsTo.empty())
      psudoDAE =
          "\\\\" + instDirectory + "\\data\\" + instDirectory + "auto_A.tmp";
    else
      psudoDAE = "\\\\" + instDirectory + "\\data\\" + autosavePointsTo.c_str();

    const Poco::File l_path(psudoDAE.toStdString());
    try {
      if (!l_path.exists()) {
        QMessageBox::warning(this, "Mantid - MuonAnalysis",
                             QString("Can't load ") + "Current data since\n" +
                                 psudoDAE + QString("\n") +
                                 QString("does not seem to exist"));
        return;
      }
    } catch (const Poco::Exception &) {
      QMessageBox::warning(this, "Mantid - MuonAnalysis",
                           QString("Can't load ") + "Current data since\n" +
                               psudoDAE + QString("\n") +
                               QString("does not seem to exist"));
      return;
    }
    m_uiForm.mwRunFiles->setUserInput(psudoDAE);
    m_uiForm.mwRunFiles->setText("CURRENT RUN");
    return;
  }

  QMessageBox::critical(
      this, "Unsupported instrument",
      "Current run loading is not supported for the selected instrument.");
}

/**
 * Group table plot button (slot)
 */
void MuonAnalysis::runGroupTablePlotButton() {
  runTablePlotButton(ItemType::Group);
}

/**
 * Pair table plot button (slot)
 */
void MuonAnalysis::runPairTablePlotButton() {
  runTablePlotButton(ItemType::Pair);
}

/**
 * Called when one of the "Plot" buttons on the "Grouping Options" tab is
 * pressed
 * Update front tab and plot
 * @param itemType :: [input] Group or pair
 */
void MuonAnalysis::runTablePlotButton(ItemType itemType) {
  if (m_updating) {
    return;
  }

  if (m_deadTimesChanged) {
    inputFileChanged(m_previousFilenames);
    return;
  }

  int plotChoiceIndex(-1), groupPairNumber(-1);
  switch (itemType) {
  case ItemType::Pair:
    plotChoiceIndex = m_uiForm.pairTablePlotChoice->currentIndex();
    if (getPairNumberFromRow(m_pairTableRowInFocus) != -1) {
      groupPairNumber = numGroups() + m_pairTableRowInFocus;
    }
    break;
  case ItemType::Group:
  default:
    plotChoiceIndex = m_uiForm.groupTablePlotChoice->currentIndex();
    groupPairNumber = getGroupNumberFromRow(m_groupTableRowInFocus);
    break;
  }

  if (groupPairNumber != -1 && plotChoiceIndex != -1) {
    // Synchronise with selectors on the front
    m_uiForm.frontGroupGroupPairComboBox->setCurrentIndex(groupPairNumber);
    m_uiForm.frontPlotFuncs->setCurrentIndex(plotChoiceIndex);
    runFrontPlotButton();
  }
}

/**
 * Pair table vertical lable clicked (slot)
 */
void MuonAnalysis::pairTableClicked(int row) { m_pairTableRowInFocus = row; }

/**
 * Pair table clicked (slot)
 */
void MuonAnalysis::pairTableClicked(int row, int column) {
  (void)column;

  pairTableClicked(row);
}

/**
 * Group table clicked (slot)
 */
void MuonAnalysis::groupTableClicked(int row, int column) {
  (void)column;

  groupTableClicked(row);
}

/**
 * Group table clicked (slot)
 */
void MuonAnalysis::groupTableClicked(int row) { m_groupTableRowInFocus = row; }

/**
 * Group table changed, e.g. if:         (slot)
 *
 *    1) user changed detector sequence
 *    2) user type in a group name
 *
 * @param row :: row number
 * @param column :: column number
 */
void MuonAnalysis::groupTableChanged(int row, int column) {
  if (column == 2) {
    // Ignore changes to Ndet column, as they will only be made programmatically
    return;
  }

  // changes to the IDs
  if (column == 1) {
    QTableWidgetItem *itemNdet = m_uiForm.groupTable->item(row, 2);
    QTableWidgetItem *item = m_uiForm.groupTable->item(row, 1);

    // if IDs list has been changed to empty string
    if (item->text() == "") {
      if (itemNdet)
        itemNdet->setText("");
    } else {
      int numDet = numOfDetectors(item->text().toStdString());

      std::stringstream detNumRead;
      if (numDet > 0) {
        detNumRead << numDet;
        if (itemNdet == nullptr)
          m_uiForm.groupTable->setItem(
              row, 2, new QTableWidgetItem(detNumRead.str().c_str()));
        else {
          itemNdet->setText(detNumRead.str().c_str());
        }
      } else {
        if (itemNdet == nullptr)
          m_uiForm.groupTable->setItem(
              row, 2, new QTableWidgetItem("Invalid IDs string"));
        else
          m_uiForm.groupTable->item(row, 2)->setText("Invalid IDs string");
      }
    }
  }

  // Change to group name
  if (column == 0) {
    QTableWidgetItem *itemName = m_uiForm.groupTable->item(row, 0);

    if (itemName == nullptr) // Just in case it wasn't assigned
    {
      itemName = new QTableWidgetItem("");
      m_uiForm.groupTable->setItem(row, 0, itemName);
    }

    if (itemName->text() != "") {
      // check that the group name entered does not already exist
      for (int i = 0; i < m_uiForm.groupTable->rowCount(); i++) {
        if (i == row)
          continue;

        QTableWidgetItem *item = m_uiForm.groupTable->item(i, 0);
        if (item) {
          if (item->text() == itemName->text()) {
            QMessageBox::warning(
                this, "MantidPlot - MuonAnalysis",
                "Group names must be unique. Please re-enter Group name.");
            itemName->setText("");
            break;
          }
        }
      }
    }
  }

  m_groupToRow = m_groupingHelper.whichGroupToWhichRow();
  updatePairTable();

  if (m_loaded && !m_updating) {
    try {
      groupLoadedWorkspace();
    } catch (std::exception &e) {
      g_log.error(e.what());

      QMessageBox::critical(
          this, "MantidPlot - MuonAnalysis",
          "Unable to group the workspace. See log for details");
    }
  }

  // Put this call after grouping. Don't update the current index
  // or replot though (false flag).
  // Note: A bug current exists where if we are re-plotting
  // and the user calls the table changed method before plotting finishes
  // (by clicking the table again) Qt will later crash.
  // This false flag also prevents this (issue: #19701)
  updateFrontAndCombo(false);
}

/**
 * Pair table changed, e.g. if:         (slot)
 *
 *    1) user changed alpha value
 *    2) pair name changed
 *
 * @param row :: row number
 * @param column:: column number
 */
void MuonAnalysis::pairTableChanged(int row, int column) {
  // alpha been modified
  if (column == 3) {
    QTableWidgetItem *itemAlpha = m_uiForm.pairTable->item(row, 3);

    if (!itemAlpha->text().toStdString().empty()) {
      try {
        boost::lexical_cast<double>(itemAlpha->text().toStdString().c_str());
      } catch (boost::bad_lexical_cast &) {
        QMessageBox::warning(this, "MantidPlot - MuonAnalysis",
                             "Alpha must be a number.");
        itemAlpha->setText("");
        return;
      }
    }
    m_pairToRow = m_groupingHelper.whichPairToWhichRow();
    // Don't replot if the pair table has been modified
    updateFrontAndCombo(false);
  }

  // pair name been modified
  if (column == 0) {
    QTableWidgetItem *itemName = m_uiForm.pairTable->item(row, 0);

    if (itemName == nullptr) // Just in case it wasn't assigned
    {
      itemName = new QTableWidgetItem("");
      m_uiForm.pairTable->setItem(row, 0, itemName);
    }

    if (itemName->text() != "") {
      // check that the group name entered does not already exist
      for (int i = 0; i < m_uiForm.pairTable->rowCount(); i++) {
        if (i == row)
          continue;

        QTableWidgetItem *item = m_uiForm.pairTable->item(i, 0);
        if (item) {
          if (item->text() == itemName->text()) {
            QMessageBox::warning(
                this, "MantidPlot - MuonAnalysis",
                "Pair names must be unique. Please re-enter Pair name.");
            itemName->setText("");
          }
        }
      }
    }

    m_pairToRow = m_groupingHelper.whichPairToWhichRow();
    updateFrontAndCombo(false);

    // check to see if alpha is specified (if name!="") and if not
    // assign a default of 1.0
    if (itemName->text() != "") {
      QTableWidgetItem *itemAlpha = m_uiForm.pairTable->item(row, 3);

      if (itemAlpha) {
        if (itemAlpha->text().toStdString().empty()) {
          itemAlpha->setText("1.0");
        }
      } else {
        m_uiForm.pairTable->setItem(row, 3, new QTableWidgetItem("1.0"));
      }
    }
  }
}

/**
 * Update pair table
 */
void MuonAnalysis::updatePairTable() {
  // number of groups has dropped below 2 and pair names specified then
  // clear pair table
  if (numGroups() < 2 && numPairs() > 0) {
    m_uiForm.pairTable->clearContents();
    for (int i = 0; i < m_uiForm.pairTable->rowCount(); i++) {
      m_uiForm.pairTable->setCellWidget(i, 1, new QComboBox);
      m_uiForm.pairTable->setCellWidget(i, 2, new QComboBox);
    }
    updateFrontAndCombo(false);
    return;
  } else if (numGroups() < 2 && numPairs() <= 0) {
    return;
  }

  // get previous number of groups as listed in the pair comboboxes
  QComboBox *qwF =
      static_cast<QComboBox *>(m_uiForm.pairTable->cellWidget(0, 1));
  int previousNumGroups =
      qwF->count(); // how many groups listed in pair combobox
  int newNumGroups = numGroups();

  // reset context of combo boxes
  for (int i = 0; i < m_uiForm.pairTable->rowCount(); i++) {
    qwF = static_cast<QComboBox *>(m_uiForm.pairTable->cellWidget(i, 1));
    QComboBox *qwB =
        static_cast<QComboBox *>(m_uiForm.pairTable->cellWidget(i, 2));

    if (previousNumGroups < newNumGroups) {
      // then need to increase the number of entrees in combo box
      for (int ii = 1; ii <= newNumGroups - previousNumGroups; ii++) {
        qwF->addItem(
            ""); // effectively here just allocate space for extra items
        qwB->addItem("");
      }
    } else if (previousNumGroups > newNumGroups) {
      // then need to decrease the number of entrees in combo box
      for (int ii = 1; ii <= previousNumGroups - newNumGroups; ii++) {
        qwF->removeItem(qwF->count() - 1); // remove top items
        qwB->removeItem(qwB->count() - 1);
      }

      // further for this case check that none of the current combo box
      // indexes are larger than the number of groups
      if (qwF->currentIndex() + 1 > newNumGroups ||
          qwB->currentIndex() + 1 > newNumGroups) {
        qwF->setCurrentIndex(0);
        qwB->setCurrentIndex(1);
      }
    }

    if (qwF->currentIndex() == 0 && qwB->currentIndex() == 0)
      qwB->setCurrentIndex(1);

    // re-populate names in combo boxes with group names
    for (int ii = 0; ii < newNumGroups; ii++) {
      qwF->setItemText(ii,
                       m_uiForm.groupTable->item(m_groupToRow[ii], 0)->text());
      qwB->setItemText(ii,
                       m_uiForm.groupTable->item(m_groupToRow[ii], 0)->text());
    }
  }
}

/**
 * Slot called when the input file is changed.
 */
void MuonAnalysis::inputFileChanged_MWRunFiles() {
  // Handle changed input, then turn buttons back on.
  handleInputFileChanges();
  allowLoading(true);
}

/**
 * Do some check when reading from MWRun, before actually reading new data file,
 * to see if file is valid
 */
void MuonAnalysis::handleInputFileChanges() {

  if (m_uiForm.mwRunFiles->getText().isEmpty())
    return;

  if (!m_uiForm.mwRunFiles->isValid()) {
    QMessageBox::warning(this, "Mantid - MuonAnalysis",
                         m_uiForm.mwRunFiles->getFileProblem());
    if (m_textToDisplay == "")
      m_uiForm.mwRunFiles->setFileProblem("Error. No File specified.");
    else
      m_uiForm.mwRunFiles->setFileProblem(
          "Error finding file. Reset to last working data.");
    m_uiForm.mwRunFiles->setText(m_textToDisplay);
    return;
  }

  if (!m_updating) {
    inputFileChanged(m_uiForm.mwRunFiles->getFilenames());

    m_textToDisplay = m_uiForm.mwRunFiles->getText();
    // save selected browse file directory to be reused next time interface is
    // started up
    m_uiForm.mwRunFiles->saveSettings(m_settingsGroup + "mwRunFilesBrowse");
  }
}

/**
 * Get grouping for the loaded workspace
 * @param loadResult :: Various loaded parameters as returned by load()
 * @return Used grouping for populating grouping table
 */
boost::shared_ptr<GroupResult>
MuonAnalysis::getGrouping(boost::shared_ptr<LoadResult> loadResult) const {
  auto result = boost::make_shared<GroupResult>();

  boost::shared_ptr<Mantid::API::Grouping> groupingToUse;
  Instrument_const_sptr instr =
      firstPeriod(loadResult->loadedWorkspace)->getInstrument();

  Workspace_sptr currentWS;
  if (AnalysisDataService::Instance().doesExist(m_workspace_name)) {
    currentWS =
        AnalysisDataService::Instance().retrieveWS<Workspace>(m_workspace_name);
  } else {
    currentWS = nullptr;
  }

  const bool reloadNecessary =
      isReloadGroupingNecessary(currentWS, loadResult->loadedWorkspace);

  if (!reloadNecessary && isGroupingSet()) {
    // Use grouping currently set
    result->usedExistGrouping = true;
    groupingToUse = boost::make_shared<Mantid::API::Grouping>(
        m_groupingHelper.parseGroupingTable());
  } else {
    // Need to load a new grouping
    result->usedExistGrouping = false;

    // Try to get grouping from IDF
    // If fails, use grouping loaded from file or, if none, dummy grouping
    Mantid::API::GroupingLoader loader(instr, loadResult->mainFieldDirection);
    try {
      groupingToUse = loader.getGroupingFromIDF();
    } catch (std::runtime_error &e) {
      g_log.warning() << "Unable to apply grouping from the IDF: " << e.what()
                      << "\n";

      if (loadResult->loadedGrouping) {
        g_log.warning("Using grouping loaded from NeXus file.");
        ITableWorkspace_sptr groupingTable;

        if (!(groupingTable = boost::dynamic_pointer_cast<ITableWorkspace>(
                  loadResult->loadedGrouping))) {
          auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(
              loadResult->loadedGrouping);
          groupingTable =
              boost::dynamic_pointer_cast<ITableWorkspace>(group->getItem(0));
        }
        groupingToUse =
            boost::make_shared<Mantid::API::Grouping>(groupingTable);
        groupingToUse->description = "Grouping from Nexus file";
      } else {
        g_log.warning(
            "No grouping set in the Nexus file. Using dummy grouping");
        groupingToUse = loader.getDummyGrouping();
      }
    }
  }

  result->groupingUsed = groupingToUse;

  return result;
}

/**
 * Input file changed. Update GUI accordingly. Note this method does no check of
 * input filename assumed
 * done elsewhere depending on e.g. whether filename came from MWRunFiles or
 * 'get current run' button.
 *
 * @param files :: All file names for the files loading.
 */
void MuonAnalysis::inputFileChanged(const QStringList &files) {
  if (m_deadTimeIndex != -1 && m_useDeadTime) {
    QMessageBox::warning(this, "Restoring dead time correction",
                         "Will use previous dead time correction");
    m_uiForm.deadTimeType->setCurrentIndex(m_deadTimeIndex);
    m_deadTimeIndex = -1;
  }
  if (files.size() <= 0)
    return;

  m_updating = true;
  m_uiForm.tabWidget->setTabEnabled(3, false);

  boost::shared_ptr<LoadResult> loadResult;
  boost::shared_ptr<GroupResult> groupResult;
  ITableWorkspace_sptr deadTimes;
  Workspace_sptr correctedGroupedWS;

  try {
    // Load the new file(s)
    loadResult = boost::make_shared<LoadResult>(m_dataLoader.loadFiles(files));

    try // to get the dead time correction
    {
      deadTimes = m_dataLoader.getDeadTimesTable(*loadResult);
    } catch (const std::exception &e) {
      // If dead correction wasn't applied we can still continue, though should
      // make user aware of that
      g_log.warning() << "No dead time correction applied: " << e.what()
                      << "\n";
    }

    // Get the grouping
    groupResult = getGrouping(loadResult);
    ITableWorkspace_sptr groupingTable = groupResult->groupingUsed->toTable();

    // Now apply DTC, if used, and grouping
    correctedGroupedWS =
        m_dataLoader.correctAndGroup(*loadResult, *groupResult->groupingUsed);

  } catch (const std::exception &e) {
    // if it failed try again with no dead time correction
    if (m_deadTimeIndex == -1) {
      m_deadTimeIndex = m_uiForm.deadTimeType->currentIndex();
      if (m_deadTimeIndex != 0) {
        QMessageBox::warning(this, "Loading failed",
                             "Will try without dead time correction");
        m_uiForm.deadTimeType->setCurrentIndex(0);
        // dont use dead time for next run
        m_useDeadTime = false;
        inputFileChanged(files);
        return;
      }
    }
    g_log.error(e.what());
    QMessageBox::critical(this, "Loading failed",
                          "Unable to load the file[s]. See log for details.");

    m_updating = false;
    m_uiForm.tabWidget->setTabEnabled(3, true);

    return;
  }
  // load worked so lets turn dead time on
  m_useDeadTime = true;
  // At this point we are sure that new data was loaded successfully, so we can
  // safely overwrite
  // previous one.

  // This is done explicitly because addOrReplace is not replacing groups
  // properly.
  deleteWorkspaceIfExists(m_workspace_name);
  deleteWorkspaceIfExists(m_grouped_name);

  // Get hold of a pointer to a matrix workspace
  MatrixWorkspace_sptr matrix_workspace =
      firstPeriod(loadResult->loadedWorkspace);

  // Set various instance variables
  m_dataTimeZero = loadResult->timeZero;
  m_fitDataPresenter->setTimeZero(m_dataTimeZero);
  m_dataFirstGoodData = loadResult->firstGoodData - loadResult->timeZero;
  m_title = matrix_workspace->getTitle();
  m_previousFilenames = files;

  int newInstrIndex = m_uiForm.instrSelector->findText(
      QString::fromStdString(matrix_workspace->getInstrument()->getName()));

  bool instrumentChanged =
      newInstrIndex != m_uiForm.instrSelector->currentIndex();

  m_uiForm.instrSelector->setCurrentIndex(newInstrIndex);

  // Add workspaces to ADS *after* changing selected instrument (as that can
  // clear them)
  AnalysisDataService::Instance().add(m_workspace_name,
                                      loadResult->loadedWorkspace);
  AnalysisDataService::Instance().add(m_grouped_name, correctedGroupedWS);

  // Update the grouping table with the used grouping, if new grouping was
  // loaded
  // XXX: this should be done after the instrument was changed, because changing
  // the instrument will
  //      clear the grouping
  if (!groupResult->usedExistGrouping) {
    runClearGroupingButton();
    fillGroupingTable(*(groupResult->groupingUsed));
  }

  // Populate instrument fields
  std::stringstream str;
  str << "Description: ";
  str << matrix_workspace->getInstrument()->getDetectorIDs().size();
  str << " detector spectrometer, main field ";
  str << QString(loadResult->mainFieldDirection.c_str())
             .toLower()
             .toStdString();
  str << " to muon polarisation";
  m_uiForm.instrumentDescription->setText(str.str().c_str());

  if (instrumentChanged) {
    // When instrument changes we use information from data no matter what user
    // has chosen before
    m_uiForm.timeZeroAuto->setCheckState(Qt::Checked);
    m_uiForm.firstGoodDataAuto->setCheckState(Qt::Checked);
  }

  // Update boxes, as values have been changed
  setTimeZeroState();
  setFirstGoodDataState();

  std::ostringstream infoStr;

  std::string label = loadResult->label;

  // Remove instrument and leading zeros
  for (auto it = label.begin(); it != label.end(); ++it) {
    if (!(isalpha(*it) || *it == '0')) {
      // When non-letter and non-zero met - delete everything up to it
      label.erase(label.begin(), it);
      break;
    }
  }

  if (files.size() > 1)
    infoStr << "Runs: ";
  else
    infoStr << "Run: ";

  infoStr << label;

  // Add other information about the run
  printRunInfo(matrix_workspace, infoStr);

  m_uiForm.infoBrowser->setText(QString::fromStdString(infoStr.str()));

  // If instrument or number of periods has changed -> update period widgets
  size_t numPeriods =
      MuonAnalysisHelper::numPeriods(loadResult->loadedWorkspace);
  if (instrumentChanged || numPeriods != m_numPeriods) {
    // if some data has been loaded, update the run number
    // before updating the periods (stops errors)
    if (m_currentDataName != NOT_AVAILABLE) {
      const boost::optional<QString> filePath =
          m_uiForm.mwRunFiles->getUserInput().toString();
      m_fitDataPresenter->setSelectedWorkspace(m_currentDataName, filePath);
    }
    updatePeriodWidgets(numPeriods);
  }

  // Populate bin width info in Plot options
  double binWidth = matrix_workspace->x(0)[1] - matrix_workspace->x(0)[0];
  m_uiForm.optionLabelBinWidth->setText(
      QString("Data collected with histogram bins of %1 %2s")
          .arg(binWidth)
          .arg(QChar(956)));

  m_deadTimesChanged = false;

  m_loaded = true;

  m_updating = false;
  m_uiForm.tabWidget->setTabEnabled(3, true);

  // Make the options available
  nowDataAvailable();

  m_currentLabel = loadResult->label;

  if (m_uiForm.frontPlotButton->isEnabled())
    plotSelectedGroupPair();
}

/**
 * Deletes a workspace _or_ a workspace group with the given name, if one exists
 * @param wsName :: Name of the workspace to delete
 */
void MuonAnalysis::deleteWorkspaceIfExists(const std::string &wsName) {
  if (AnalysisDataService::Instance().doesExist(wsName)) {
    IAlgorithm_sptr deleteAlg =
        AlgorithmManager::Instance().create("DeleteWorkspace");
    deleteAlg->setLogging(false);
    deleteAlg->setPropertyValue("Workspace", wsName);
    deleteAlg->execute();
  }
}

/**
 * Guess Alpha (slot). For now include all data from first good data(bin)
 */
void MuonAnalysis::guessAlphaClicked() {
  m_updating = true;

  if (getPairNumberFromRow(m_pairTableRowInFocus) >= 0) {
    QComboBox *qwF = static_cast<QComboBox *>(
        m_uiForm.pairTable->cellWidget(m_pairTableRowInFocus, 1));
    QComboBox *qwB = static_cast<QComboBox *>(
        m_uiForm.pairTable->cellWidget(m_pairTableRowInFocus, 2));

    if (!qwF || !qwB)
      return;

    // group IDs
    QTableWidgetItem *idsF =
        m_uiForm.groupTable->item(m_groupToRow[qwF->currentIndex()], 1);
    QTableWidgetItem *idsB =
        m_uiForm.groupTable->item(m_groupToRow[qwB->currentIndex()], 1);

    if (!idsF || !idsB)
      return;

    QString inputWS = m_workspace_name.c_str();
    if (m_uiForm.homePeriodBox2->isEnabled())
      inputWS += "_" + m_uiForm.homePeriodBox1->text();

    double alphaValue;

    try {
      IAlgorithm_sptr alphaAlg =
          AlgorithmManager::Instance().create("AlphaCalc");
      alphaAlg->setPropertyValue("InputWorkspace", inputWS.toStdString());
      alphaAlg->setPropertyValue("ForwardSpectra", idsF->text().toStdString());
      alphaAlg->setPropertyValue("BackwardSpectra", idsB->text().toStdString());
      alphaAlg->setProperty("FirstGoodValue", firstGoodBin());
      alphaAlg->execute();

      alphaValue = alphaAlg->getProperty("Alpha");
    } catch (std::exception &e) {
      g_log.error() << "Error when running AlphaCalc: " << e.what() << "\n";
      QMessageBox::critical(this, "Guess alpha error",
                            "Unable to guess alpha value. AlphaCalc failed. "
                            "See log for details.");
      m_updating = false;
      return;
    }

    const QString alpha = QString::number(alphaValue);

    QComboBox *qwAlpha = static_cast<QComboBox *>(
        m_uiForm.pairTable->cellWidget(m_pairTableRowInFocus, 3));
    if (qwAlpha)
      m_uiForm.pairTable->item(m_pairTableRowInFocus, 3)->setText(alpha);
    else
      m_uiForm.pairTable->setItem(m_pairTableRowInFocus, 3,
                                  new QTableWidgetItem(alpha));
  }

  m_updating = false;

  // See if auto-update is on and if so update the plot
  groupTabUpdatePlotPair();
}

/**
 * Return number of groups defined (not including pairs)
 *
 * @return number of groups
 */
int MuonAnalysis::numGroups() {
  m_groupToRow = m_groupingHelper.whichGroupToWhichRow();
  return static_cast<int>(m_groupToRow.size());
}

/**
 * Return number of pairs
 *
 * @return number of pairs
 */
int MuonAnalysis::numPairs() {
  m_pairToRow = m_groupingHelper.whichPairToWhichRow();
  return static_cast<int>(m_pairToRow.size());
}

/**
 * Update front "group / group-pair" combo-box based on what the currentIndex
 * now is
 */
void MuonAnalysis::updateFront() {
  // get current group/pair index
  const int gpIndex = getGroupOrPairToPlot();

  // Cache current selection of plot type
  const int plotType = m_uiForm.frontPlotFuncs->currentIndex();

  m_uiForm.frontPlotFuncs->clear();

  int numG = numGroups();

  if (gpIndex >= 0 && numG) {
    if (gpIndex >= numG && numG >= 2) {
      // i.e. index points to a pair
      m_uiForm.frontPlotFuncs->addItems(m_pairPlotFunc);

      m_uiForm.frontAlphaLabel->setVisible(true);
      m_uiForm.frontAlphaNumber->setVisible(true);

      m_uiForm.frontAlphaNumber->setText(
          m_uiForm.pairTable->item(m_pairToRow[gpIndex - numG], 3)->text());

      m_uiForm.frontAlphaNumber->setCursorPosition(0);
    } else {
      // i.e. index points to a group
      m_uiForm.frontPlotFuncs->addItems(m_groupPlotFunc);

      m_uiForm.frontAlphaLabel->setVisible(false);
      m_uiForm.frontAlphaNumber->setVisible(false);
    }
    // Replace cached value
    if (plotType != -1 && plotType < m_uiForm.frontPlotFuncs->count()) {
      m_uiForm.frontPlotFuncs->setCurrentIndex(plotType);
    }
  }
}

/**
 * Update front including first re-populate pair list combo box
 * Also update multiple fitting
 * Plots changes if requested
 * @param updateIndexAndPlot :: If true updates and plots the current group/pair
 */
void MuonAnalysis::updateFrontAndCombo(bool updateIndexAndPlot) {
  // for now brute force clearing and adding new context
  // could go for softer approach and check if is necessary
  // to completely reset this combo box
  int currentI = getGroupOrPairToPlot();
  if (currentI < 0) // in case this combobox has not been set yet
    currentI = 0;

  m_uiForm.frontGroupGroupPairComboBox->clear();

  int numG = numGroups();
  int numP = numPairs();
  QStringList groupsAndPairs;
  for (int i = 0; i < numG; i++) {
    m_uiForm.frontGroupGroupPairComboBox->addItem(
        m_uiForm.groupTable->item(m_groupToRow[i], 0)->text());
    auto groupName = m_uiForm.groupTable->item(m_groupToRow[i], 0)->text();
    if (groupName.toStdString() != "") {
      groupsAndPairs << groupName;
    }
  }
  for (int i = 0; i < numP; i++) {
    m_uiForm.frontGroupGroupPairComboBox->addItem(
        m_uiForm.pairTable->item(m_pairToRow[i], 0)->text());
    auto pairName = m_uiForm.groupTable->item(m_pairToRow[i], 0)->text();
    if (pairName.toStdString() != "") {
      groupsAndPairs << pairName;
    }
  }
  // If it doesn't match then reset
  if (currentI >= m_uiForm.frontGroupGroupPairComboBox->count()) {
    currentI = 0;
  }
  setGroupsAndPairs();
  loadAllGroups();
  loadAllPairs();
  if (updateIndexAndPlot) {
    setGroupOrPairIndexToPlot(currentI);
    plotCurrentGroupAndPairs();
  }
}
/**
 * sets the selected groups and pairs
 */
void MuonAnalysis::setGroupsAndPairs() {
  auto names = m_groupingHelper.parseGroupingTable().pairNames;
  auto tmp = m_groupingHelper.parseGroupingTable().groupNames;
  names.insert(std::end(names), std::make_move_iterator(tmp.begin()),
               std::make_move_iterator(tmp.end()));
  QStringList GroupsAndPairsNames;
  for (const auto &name : names) {
    GroupsAndPairsNames << QString::fromStdString(name);
  }
  m_uiForm.fitBrowser->setAvailableGroups(GroupsAndPairsNames);
}

/**
 * Updates widgets related to period algebra.
 * @param numPeriods Number of periods available
 */
void MuonAnalysis::updatePeriodWidgets(size_t numPeriods) {
  QString periodLabel = "Data collected in " + QString::number(numPeriods) +
                        " periods. Plot/analyse period(s): ";
  m_uiForm.homePeriodsLabel->setText(periodLabel);

  // Reset the previous text
  m_uiForm.homePeriodBox1->setText("1");
  m_uiForm.homePeriodBox2->clear();

  // We only need period widgets enabled if we have more than 1 period
  if (numPeriods > 1) {
    m_uiForm.homePeriodBox2->setEnabled(true);
  } else {
    m_uiForm.homePeriodBox2->setEnabled(false);
  }

  // cache number of periods
  m_numPeriods = numPeriods;
  m_uiForm.fitBrowser->setNumPeriods(m_numPeriods);
}

/**
 * Return the group-number for the group in a row. Return -1 if
 * invalid group in row
 *
 * @param row :: A row in the group table
 * @return Group number
 */
int MuonAnalysis::getGroupNumberFromRow(int row) {
  m_groupToRow = m_groupingHelper.whichGroupToWhichRow();
  for (unsigned int i = 0; i < m_groupToRow.size(); i++) {
    if (m_groupToRow[i] == row)
      return i;
  }
  return -1;
}

/**
 * Return the pair-number for the pair in a row. Return -1 if
 * invalid pair in row
 *
 * @param row :: A row in the pair table
 * @return Pair number
 */
int MuonAnalysis::getPairNumberFromRow(int row) {
  m_pairToRow = m_groupingHelper.whichPairToWhichRow();
  for (unsigned int i = 0; i < m_pairToRow.size(); i++) {
    if (m_pairToRow[i] == row)
      return i;
  }
  return -1;
}

/**
 * Clear tables and front combo box
 */
void MuonAnalysis::clearTablesAndCombo() {
  m_uiForm.groupTable->clearContents();
  m_uiForm.frontGroupGroupPairComboBox->clear();
  m_uiForm.frontPlotFuncs->clear();

  m_uiForm.pairTable->clearContents();
  for (int i = 0; i < m_uiForm.pairTable->rowCount(); i++) {
    m_uiForm.pairTable->setCellWidget(i, 1, new QComboBox);
    m_uiForm.pairTable->setCellWidget(i, 2, new QComboBox);
  }

  m_uiForm.groupDescription->clear();
}

/**
 * Clear loaded run, run info and delete loaded workspaces
 */
void MuonAnalysis::clearLoadedRun() {
  m_uiForm.mwRunFiles->clear();
  m_uiForm.infoBrowser->clear();
  deleteWorkspaceIfExists(m_workspace_name);
  deleteWorkspaceIfExists(m_grouped_name);
}

/**
 * Get period labels for the periods selected in the GUI
 * Return an empty string for single-period data or all periods
 * @return String for the period(s) selected by user
 */
std::string MuonAnalysis::getPeriodLabels() const {
  std::ostringstream retVal;

  // Change input comma-separated to more readable format
  auto summed = getSummedPeriods();
  std::replace(summed.begin(), summed.end(), ',', '+');
  auto subtracted = getSubtractedPeriods();
  if (!subtracted.empty()) {
    std::replace(subtracted.begin(), subtracted.end(), ',', '+');
  }

  // If single period, or all (1,2,3,...) then leave blank
  // All periods => size of string is 2n-1
  const bool isSinglePeriod = m_numPeriods == 1;
  const bool isAllPeriods =
      summed.size() == 2 * m_numPeriods - 1 && subtracted.empty();

  if (!isSinglePeriod && !isAllPeriods) {
    retVal << summed;
    if (!subtracted.empty()) {
      retVal << "-" << subtracted;
    }
  }

  return retVal.str();
}

/**
 * Plots specific WS spectrum (used by plotPair and plotGroup)
 * This is done with a Python script (there must be a better way!)
 * @param wsName   :: Workspace name
 * @param logScale :: Whether to plot using logarithmic scale
 */
void MuonAnalysis::plotSpectrum(const QString &wsName, bool logScale) {
  // List of script lines which acquire a window and plot in it.
  QStringList acquireWindowScript;

  MuonAnalysisOptionTab::NewPlotPolicy policy = m_optionTab->newPlotPolicy();

  // Hide all the previous plot windows, if creating a new one
  if (policy == MuonAnalysisOptionTab::NewWindow &&
      m_uiForm.hideGraphs->isChecked()) {
    hideAllPlotWindows();
  }

  QStringList &s = acquireWindowScript; // To keep short

  // Get the window to plot in (returns window)
  // ws_name: name of workspace to plot
  // prev_name: name of currently plotted workspace
  // use_prev: whether to plot in existing window or new
  s << "def get_window(ws_name, prev_name, use_prev):";
  s << "  graph_name = ws_name + '-1'";
  s << "  if not use_prev:";
  s << "    return newGraph(graph_name, 0)";
  s << "  existing = graph(graph_name)";
  s << "  if existing is not None and ws_name != prev_name:";
  s << "    existing.close()";
  s << "  window = graph(prev_name + '-1')";
  s << "  if window is None:";
  s << "    window = newGraph(graph_name, 0)";
  s << "  return window";
  s << "";

  // Remove data and difference from given plot (keep fit and guess)
  // num_to_keep: number of previous fits to keep
  s << "def remove_data(window, num_to_keep):";
  s << "  if window is None:";
  s << "    raise ValueError('No plot to remove data from')";
  // Need to keep the last "num_to_keep" curves with
  // "Workspace-Calc" in their name, plus guesses
  s << "  layer = window.activeLayer()";
  s << "  if layer is not None:";
  s << "    kept_fits = 0";
  s << "    for i in range(layer.numCurves() - 1, 0, -1):"; // reversed
  s << "      title = layer.curveTitle(i)";
  s << "      if title == \"CompositeFunction\":";
  s << "        continue"; // keep all guesses
  s << "      if \"Workspace-Calc\" in title and kept_fits < num_to_keep:";
  s << "        kept_fits = kept_fits + 1";
  s << "        continue";           // keep last n fits
  s << "      layer.removeCurve(i)"; // remove everything else

  // Plot data in the given window with given options
  s << "def plot_data(ws_name,errors, connect, window_to_use):";
  if (parsePlotType(m_uiForm.frontPlotFuncs) == PlotType::Asymmetry) {
    // clang-format off
    s << "  w = plotSpectrum(source = ws_name,"
         "indices = 0,"
         "distribution = mantidqtpython.MantidQt.DistributionFalse,"
         "error_bars = errors,"
         "type = connect,"
         "window = window_to_use)";
    // clang-format on
    // set if TFAsymm is on or off
  } else {
    // clang-format off
    s << "  w = plotSpectrum(source = ws_name,"
         "indices = 0,"
         "distribution = mantidqtpython.MantidQt.DistributionDefault,"
         "error_bars = errors,"
         "type = connect,"
         "window = window_to_use)";
    // clang-format on
  }
  s << "  w.setName(ws_name + '-1')";
  s << "  w.setObjectName(ws_name)";
  s << "  w.show()";
  s << "  w.setFocus()";
  s << "  return w";
  s << "";

  // Format the graph scale, title, legends and colours
  // Data (most recently added curve) should be black
  s << "def format_graph(graph, ws_name, log_scale, y_auto, y_min, "
       "y_max,x_min,x_max):";
  s << "  layer = graph.activeLayer()";
  s << "  num_curves = layer.numCurves()";
  s << "  layer.setCurveTitle(num_curves, ws_name)";
  s << "  layer.setTitle(mtd[ws_name].getTitle())";
  s << "  for i in range(0, num_curves):";
  s << "    color = i + 1 if i != num_curves - 1 else 0";
  s << "    layer.setCurveLineColor(i, color)";

  s << "  if log_scale:";
  s << "    layer.logYlinX()";
  s << "  else:";
  s << "    layer.linearAxes()";
  s << "  if y_auto:";
  s << "    layer.setAutoScale()";
  s << "  else:";
  s << "    try:";
  s << "      layer.setAxisScale(Layer.Left, float(y_min), float(y_max))";
  s << "    except ValueError:";
  s << "      layer.setAutoScale()";
  s << "  layer.setScale(2,float(x_min),float(x_max))";

  s << "";

  // Plot the data!
  s << "win = get_window('%WSNAME%', '%PREV%', %USEPREV%)";
  s << "if %FITSTOKEEP% != -1:";
  // leave the 0th layer -> layer is not empty
  s << "  remove_data(win, %FITSTOKEEP%)";
  s << "g = plot_data('%WSNAME%', %ERRORS%, %CONNECT%, win)";
  // if there is more than one layer delete the oldest one manually
  s << "if %FITSTOKEEP% != -1:";
  s << "  layer = win.activeLayer()";
  s << "  if layer.numCurves()>1:";
  s << "     layer.removeCurve(0)";

  s << "format_graph(g, '%WSNAME%', %LOGSCALE%, %YAUTO%, '%YMIN%', "
       "'%YMAX%','%XMIN%','%XMAX%')";

  QString pyS;

  // Add line separators
  pyS += acquireWindowScript.join("\n") + "\n";

  // Get plotting params
  const QMap<QString, QString> &params = getPlotStyleParams(wsName);

  // Insert real values
  QString safeWSName(wsName);
  safeWSName.replace("'", "\'");
  pyS.replace("%WSNAME%", safeWSName);
  pyS.replace("%PREV%", m_currentDataName);
  pyS.replace("%USEPREV%", policy == MuonAnalysisOptionTab::PreviousWindow
                               ? "True"
                               : "False");
  pyS.replace("%ERRORS%", params["ShowErrors"]);
  pyS.replace("%CONNECT%", params["ConnectType"]);
  pyS.replace("%LOGSCALE%", logScale ? "True" : "False");
  pyS.replace("%YAUTO%", params["YAxisAuto"]);
  pyS.replace("%YMIN%", params["YAxisMin"]);
  pyS.replace("%YMAX%", params["YAxisMax"]);
  pyS.replace("%XMIN%", params["XAxisMin"]);
  pyS.replace("%XMAX%", params["XAxisMax"]);

  if (policy == MuonAnalysisOptionTab::PreviousWindow) {
    pyS.replace("%FITSTOKEEP%", m_uiForm.spinBoxNPlotsToKeep->text());
  } else {
    pyS.replace("%FITSTOKEEP%", "-1");
  }

  runPythonCode(pyS);
}

/**
 * Get current plot style parameters. wsName is used to get default values.
 * @param wsName Workspace plot of which we want to style
 * @return Maps of the parameters, see
 MuonAnalysisOptionTab::parsePlotStyleParams for list
           of possible keys
 */
QMap<QString, QString> MuonAnalysis::getPlotStyleParams(const QString &wsName) {
  // Get parameter values from the options tab
  QMap<QString, QString> params = m_optionTab->parsePlotStyleParams();
  auto upper = m_uiForm.timeAxisFinishAtInput->text().toDouble();

  Workspace_const_sptr ws_ptr =
      AnalysisDataService::Instance().retrieve(wsName.toStdString());
  MatrixWorkspace_const_sptr matrix_workspace =
      boost::dynamic_pointer_cast<const MatrixWorkspace>(ws_ptr);
  const auto &xData = matrix_workspace->x(0);

  auto lower = m_uiForm.timeAxisStartAtInput->text().toDouble();
  if (upper > *max_element(xData.begin(), xData.end())) {
    QMessageBox::warning(this, tr("Muon Analysis"),
                         tr("Upper bound is beyond data range.\n"
                            "Setting end time to last time value (minus 1)."),
                         QMessageBox::Ok, QMessageBox::Ok);
    // subtract a small amount off to prevent a crash from using the exact end
    upper = *max_element(xData.begin(), xData.end()) - 1.;
    m_uiForm.timeAxisFinishAtInput->setText(QString::number(upper));
  }
  if (upper < *min_element(xData.begin(), xData.end())) {
    QMessageBox::warning(this, tr("Muon Analysis"),
                         tr("No data in selected range.\n"
                            "Setting end time to last time value (minus 1)."),
                         QMessageBox::Ok, QMessageBox::Ok);
    upper = *max_element(xData.begin(), xData.end()) - 1.;
    m_uiForm.timeAxisFinishAtInput->setText(QString::number(upper));
  }
  params["XAxisMax"] = QString::number(upper);
  if (lower > upper) {
    QMessageBox::warning(this, tr("Muon Analysis"),
                         tr("Time max is less than time min.\n"
                            "Will change time min."),
                         QMessageBox::Ok, QMessageBox::Ok);
    lower = *min_element(xData.begin(), xData.end());
    m_uiForm.timeAxisStartAtInput->setText(QString::number(lower));
  }
  if (lower > *max_element(xData.begin(), xData.end())) {
    QMessageBox::warning(this, tr("Muon Analysis"),
                         tr("No data in selected range.\n"
                            "Setting start time to first time value."),
                         QMessageBox::Ok, QMessageBox::Ok);
    lower = *min_element(xData.begin(), xData.end());
    m_uiForm.timeAxisStartAtInput->setText(QString::number(lower));
  }
  params["XAxisMin"] = QString::number(lower);

  // If autoscale disabled
  if (params["YAxisAuto"] == "False") {
    // Get specified min/max values for Y axis
    QString min = params["YAxisMin"];
    QString max = params["YAxisMax"];

    // If any of those is not specified - get min and max by default
    if (min.isEmpty() || max.isEmpty()) {
      const auto &yData = matrix_workspace->y(0);

      if (min.isEmpty())
        params["YAxisMin"] =
            QString::number(*min_element(yData.begin(), yData.end()));

      if (max.isEmpty())
        params["YAxisMax"] =
            QString::number(*max_element(yData.begin(), yData.end()));
    }
  } else {
    zoomYAxis(wsName, params);
  }

  return params;
}

/**
 * Checks if the plot for the workspace does exist.
 * @param wsName Name of the workspace
 * @return True if exists, false if not
 */
bool MuonAnalysis::plotExists(const QString &wsName) {
  QString code;

  code += "g = graph('%1-1')\n"
          "if g != None:\n"
          "  print('1')\n"
          "else:\n"
          "  print('0')\n";

  QString output = runPythonCode(code.arg(wsName));

  bool ok;
  int outputCode = output.toInt(&ok);

  if (!ok)
    throw std::logic_error("Script should print 0 or 1");

  return (outputCode == 1);
}

/**
 * Enable PP tool for the plot of the given WS.
 * @param wsName Name of the WS which plot PP tool will be attached to.
 * @param update :: [input] If to update the data selector
 * @param filePath :: [input] Optional path to file that is actually used. This
 * is for "load current run" where the data file has a temporary name like
 * MUSRauto_E.tmp
 */
void MuonAnalysis::selectMultiPeak(const QString &wsName, const bool update,
                                   const boost::optional<QString> &filePath) {
  disableAllTools();
  if (!plotExists(wsName)) {
    plotSpectrum(wsName);
    setCurrentDataName(wsName);
  }

  if (wsName != m_fitDataPresenter->getAssignedFirstRun()) {
    // Set the available groups/pairs and periods
    const Grouping groups = m_groupingHelper.parseGroupingTable();
    QStringList groupsAndPairs;
    groupsAndPairs.reserve(
        static_cast<int>(groups.groupNames.size() + groups.pairNames.size()));
    std::transform(groups.groupNames.begin(), groups.groupNames.end(),
                   std::back_inserter(groupsAndPairs), &QString::fromStdString);
    std::transform(groups.pairNames.begin(), groups.pairNames.end(),
                   std::back_inserter(groupsAndPairs), &QString::fromStdString);
    setGroupsAndPairs();
    if (update) {
      // Set the selected run, group/pair and period
      m_fitDataPresenter->setAssignedFirstRun(wsName, filePath);
      setChosenGroupAndPeriods(wsName);
    }
  }

  QString code;

  code += "g = graph('" + wsName +
          "-1')\n"
          "if g != None:\n"
          "  g.show()\n"
          "  g.setFocus()\n"
          "  selectMultiPeak(g)\n";

  runPythonCode(code);
}

/**
 * Pass through to selectMultiPeak(wsName, update, filePath) where filePath is
 * set
 * to blank. Enables connection as a slot without Qt understanding
 * boost::optional.
 * @param wsName Name of the selected workspace
 */
void MuonAnalysis::selectMultiPeak(const QString &wsName) {
  selectMultiPeak(wsName, true, boost::optional<QString>());
}

/**
 * Pass through to selectMultiPeak(wsName, update, filePath) where filePath is
 * set
 * to blank. Enables connection as a slot without Qt understanding
 * boost::optional. This will not update the data selector
 * @param wsName Name of the selected workspace
 */
void MuonAnalysis::selectMultiPeakNoUpdate(const QString &wsName) {
  selectMultiPeak(wsName, false, boost::optional<QString>());
}

/**
 * Disable tools for all the graphs within MantidPlot.
 */
void MuonAnalysis::disableAllTools() { runPythonCode("disableTools()"); }

/**
 * Hides all the plot windows (MultiLayer ones)
 */
void MuonAnalysis::hideAllPlotWindows() {
  QString code;

  code += "for w in windows():\n"
          "  if w.inherits('MultiLayer'):\n"
          "    w.hide()\n";

  runPythonCode(code);
}

/**
 * Shows all the plot windows (MultiLayer ones)
 */
void MuonAnalysis::showAllPlotWindows() {
  QString code;

  code += "for w in windows():\n"
          "  if w.inherits('MultiLayer'):\n"
          "    w.show()\n";

  runPythonCode(code);
}

/**
 * Is Grouping set.
 *
 * @return true if set
 */
bool MuonAnalysis::isGroupingSet() const {
  auto dummy = m_groupingHelper.whichGroupToWhichRow();

  return !dummy.empty();
}

/**
 * Calculate number of detectors from string of type 1-3, 5, 10-15
 *
 * @param str :: String of type "1-3, 5, 10-15"
 * @return Number of detectors. Return 0 if not recognised
 */
int MuonAnalysis::numOfDetectors(const std::string &str) const {
  size_t rangeSize;

  try {
    rangeSize = Strings::parseRange(str).size();
  } catch (...) {
    rangeSize = 0;
  }

  return static_cast<int>(rangeSize);
}

/**
 * Set start up interface look and populate local attributes
 * initiated from info set in QT designer
 */
void MuonAnalysis::startUpLook() {
  // populate group plot functions
  for (int i = 0; i < m_uiForm.groupTablePlotChoice->count(); i++)
    m_groupPlotFunc.append(m_uiForm.groupTablePlotChoice->itemText(i));

  // pair plot functions
  for (int i = 0; i < m_uiForm.pairTablePlotChoice->count(); i++)
    m_pairPlotFunc.append(m_uiForm.pairTablePlotChoice->itemText(i));

  // Set initial front
  m_uiForm.frontAlphaLabel->setVisible(false);
  m_uiForm.frontAlphaNumber->setVisible(false);
  m_uiForm.frontAlphaNumber->setEnabled(false);
  m_uiForm.homePeriodBox2->setEnabled(false);

  // Set validators for number-only boxes
  setDoubleValidator(m_uiForm.timeZeroFront);
  setDoubleValidator(m_uiForm.firstGoodBinFront);

  // set various properties of the group table
  m_uiForm.groupTable->setColumnWidth(0, 100);
  m_uiForm.groupTable->setColumnWidth(1, 200);
  for (int i = 0; i < m_uiForm.groupTable->rowCount(); i++) {
    QTableWidgetItem *item = m_uiForm.groupTable->item(i, 2);
    if (!item) {
      QTableWidgetItem *it = new QTableWidgetItem("");
      it->setFlags(it->flags() & (~Qt::ItemIsEditable));
      m_uiForm.groupTable->setItem(i, 2, it);
    } else {
      item->setFlags(item->flags() & (~Qt::ItemIsEditable));
    }
    item = m_uiForm.groupTable->item(i, 0);
    if (!item) {
      QTableWidgetItem *it = new QTableWidgetItem("");
      m_uiForm.groupTable->setItem(i, 0, it);
    }
  }

  // When first started, no data has yet been loaded
  noDataAvailable();
}

/**
 * Time zero returned in ms
 */
double MuonAnalysis::timeZero() {
  return getValidatedDouble(m_uiForm.timeZeroFront, TIME_ZERO_DEFAULT,
                            "time zero", g_log);
}

/**
 * Returns params string which can be passed to Rebin, according to what user
 * specified. If no rebin
 * requested by user, returns an empty string.
 * @param wsForRebin :: Workspace we are going to rebin. Use to determine bin
 * size
 * @return Params string to pass to rebin
 */
std::string MuonAnalysis::rebinParams(Workspace_sptr wsForRebin) {
  MuonAnalysisOptionTab::RebinType rebinType = m_optionTab->getRebinType();

  if (rebinType == MuonAnalysisOptionTab::NoRebin) {
    return "";
  } else if (rebinType == MuonAnalysisOptionTab::FixedRebin) {
    MatrixWorkspace_sptr ws = firstPeriod(wsForRebin);
    double binSize = ws->x(0)[1] - ws->x(0)[0];

    double stepSize = m_optionTab->getRebinStep();

    return boost::lexical_cast<std::string>(binSize * stepSize);
  } else if (rebinType == MuonAnalysisOptionTab::VariableRebin) {
    return m_optionTab->getRebinParams();
  } else {
    throw std::runtime_error("Unknown rebin type");
  }
}

/**
 * Return first good bin as set on the interface.
 */
double MuonAnalysis::firstGoodBin() const {
  return getValidatedDouble(m_uiForm.firstGoodBinFront, FIRST_GOOD_BIN_DEFAULT,
                            "first good bin", g_log);
}

/**
 * Returns min X value as specified by user.
 * @return Min X value
 */
double MuonAnalysis::startTime() const {
  auto startTimeType = m_optionTab->getStartTimeType();
  double value(0);

  switch (startTimeType) {
  case MuonAnalysisOptionTab::FirstGoodData:
    value = firstGoodBin();
    break;

  case MuonAnalysisOptionTab::TimeZero:
    value = 0;
    break;

  case MuonAnalysisOptionTab::Custom:
    value = m_optionTab->getCustomStartTime();
    break;

  default:
    // Just in case added a new one
    throw std::runtime_error("Unknown start time type");
  }

  return value;
}

/**
 * Returns max X value as specified by user.
 * @return Max X value, or EMPTY_DBL() if not set
 */
double MuonAnalysis::finishTime() const {
  return m_optionTab->getCustomFinishTime();
}

/**
 * Load auto saved values
 */
void MuonAnalysis::loadAutoSavedValues(const QString &group) {

  QSettings prevInstrumentValues;
  prevInstrumentValues.beginGroup(group + "instrument");
  QString instrumentName =
      prevInstrumentValues.value("name", "MUSR").toString();
  m_uiForm.instrSelector->setCurrentIndex(
      m_uiForm.instrSelector->findText(instrumentName));

  // Load dead time options.
  QSettings deadTimeOptions;
  deadTimeOptions.beginGroup(group + "DeadTimeOptions");

  int deadTimeTypeIndex = deadTimeOptions.value("deadTimes", 0).toInt();
  m_uiForm.deadTimeType->setCurrentIndex(deadTimeTypeIndex);

  onDeadTimeTypeChanged(deadTimeTypeIndex);

  QString savedDeadTimeFile = deadTimeOptions.value("deadTimeFile").toString();
  m_uiForm.mwRunDeadTimeFile->setUserInput(savedDeadTimeFile);

  // Load values saved using saveWidgetValue()
  loadWidgetValue(m_uiForm.timeZeroFront, TIME_ZERO_DEFAULT);
  loadWidgetValue(m_uiForm.firstGoodBinFront, FIRST_GOOD_BIN_DEFAULT);
  loadWidgetValue(m_uiForm.timeZeroAuto, Qt::Checked);
  loadWidgetValue(m_uiForm.firstGoodDataAuto, Qt::Checked);
}

/**
 * Loads up the options for fit browser so that it works in muon analysis tab
 * and set up data selector widget and fit data helper
 */
void MuonAnalysis::loadFittings() {
  // Title of the fitting dock widget that now lies within the fittings tab.
  // Should be made dynamic so that the Chi-sq can be displayed alongside like
  // original fittings widget
  m_uiForm.fitBrowser->setWindowTitle("Fit Function");
  // Make sure that the window can't be moved or closed within the tab.
  m_uiForm.fitBrowser->setFeatures(QDockWidget::NoDockWidgetFeatures);
  // Add Function browser widget to the fit tab
  m_functionBrowser = new MuonFunctionBrowser(nullptr, true);
  m_functionBrowser->sizePolicy().setVerticalStretch(10);
  m_uiForm.fitBrowser->addFitBrowserWidget(m_functionBrowser,
                                           m_functionBrowser);
  // Add Data Selector widget to the fit tab
  m_dataSelector = new MuonFitDataSelector(m_uiForm.fitBrowser);
  m_dataSelector->sizePolicy().setVerticalStretch(0);
  m_uiForm.fitBrowser->addExtraWidget(m_dataSelector);
  // Set up fit data and function presenters
  m_fitDataPresenter =
      Mantid::Kernel::make_unique<MuonAnalysisFitDataPresenter>(
          m_uiForm.fitBrowser, m_dataSelector, m_dataLoader,
          m_groupingHelper.parseGroupingTable(), PlotType::Asymmetry,
          m_dataTimeZero);
  updateRebinParams(); // set initial params for fit data presenter
  m_fitFunctionPresenter =
      Mantid::Kernel::make_unique<MuonAnalysisFitFunctionPresenter>(
          nullptr, m_uiForm.fitBrowser, m_functionBrowser);
  // Connect signals
  connect(m_dataSelector, SIGNAL(workspaceChanged()), this,
          SLOT(dataToFitChanged()));
  connect(m_uiForm.plotCreation, SIGNAL(currentIndexChanged(int)), this,
          SLOT(updateDataPresenterOverwrite(int)));
  connect(m_uiForm.fitBrowser, SIGNAL(groupBoxClicked()), this,
          SLOT(handleGroupBox()));
  connect(m_uiForm.fitBrowser, SIGNAL(periodBoxClicked()), this,
          SLOT(handlePeriodBox()));
  connect(m_dataSelector, SIGNAL(nameChanged(QString)), this,
          SLOT(updateNormalization(QString)));

  m_fitDataPresenter->setOverwrite(isOverwriteEnabled());
  // Set multi fit mode on/off as appropriate
  const auto &multiFitState = m_optionTab->getMultiFitState();
  m_fitFunctionPresenter->setMultiFitState(multiFitState);
}
/**
 * Handle "groups" selected/deselected
 * Update stored value
 */
void MuonAnalysis::handleGroupBox() {
  // send the group to dataselector
  m_dataSelector->setGroupsSelected(m_uiForm.fitBrowser->getChosenGroups());
  // update labels for single fit
  auto names = m_fitDataPresenter->generateWorkspaceNames(true);
  if (names.size() == 1) {
    updateLabels(names[0]);
  }
  m_fitDataPresenter->handleSelectedDataChanged(true);
  m_dataSelector->checkForMultiGroupPeriodSelection();
}
/**
 * Handle"periods" selected/deselected
 * Update stored value
 */
void MuonAnalysis::handlePeriodBox() {
  // send the group to dataselector
  m_dataSelector->setPeriodsSelected(m_uiForm.fitBrowser->getChosenPeriods());
  // update labels for single fit
  auto names = m_fitDataPresenter->generateWorkspaceNames(true);
  if (names.size() == 1) {
    updateLabels(names[0]);
  }
  m_fitDataPresenter->handleSelectedDataChanged(true);
}
/**
 * Updates the labels (legend and ws) for
 * a single fit when within the mulit-
 * fit GUI.
 * @param name :: the name for the label.
 */
void MuonAnalysis::updateLabels(std::string &name) {
  m_uiForm.fitBrowser->setOutputName(name);
}
/**
 * Allow/disallow loading.
 */
void MuonAnalysis::allowLoading(bool enabled) {
  m_uiForm.nextRun->setEnabled(enabled);
  m_uiForm.previousRun->setEnabled(enabled);
  m_uiForm.mwRunFiles->setEnabled(enabled);
  setLoadCurrentRunEnabled(enabled);
}

/**
 *   Check to see if the appending option is true when the previous button has
 * been pressed and acts accordingly
 */
void MuonAnalysis::checkAppendingPreviousRun() { checkAppendingRun(-1); }

/**
 *   Check to see if the appending option is true when the next button has been
 * pressed and acts accordingly
 */
void MuonAnalysis::checkAppendingNextRun() { checkAppendingRun(1); }

/**
 * Check to see if the appending option is true when the next/previous button
 * has been pressed, and load accordingly
 * @param direction :: +1 for "next", -1 for "previous"
 */
void MuonAnalysis::checkAppendingRun(const int direction) {
  const auto &runPath = m_uiForm.mwRunFiles->getText();
  if (runPath.isEmpty()) {
    return;
  }

  const int sign = direction < 0 ? -1 : 1;
  allowLoading(false);
  const auto runString = runPath.split(Poco::Path::separator()).last();
  if (runString.contains("-")) {
    setAppendingRun(sign); // append next/previous run
  } else {
    changeRun(sign); // replace with next/previous run
  }
}

/**
 *   This sets up an appending lot of files so that when the user hits enter
 *   all files within the range will open.
 *
 *   @param inc :: The number to increase the run by, this can be
 *   -1 if previous has been selected.
 */
void MuonAnalysis::setAppendingRun(int inc) {
  QString filePath("");

  // Get hold of the files to increment or decrement the range to.
  QStringList currentFiles(m_uiForm.mwRunFiles->getFilenames());
  if (currentFiles.empty())
    currentFiles = m_previousFilenames;

  // Name and size of the run to change.
  QString run("");
  int runSize(-1);

  // The file number that needs to be incremented or decremented.
  int fileNumber(-1);

  if (inc < 0) // If the files list only includes one file.
  {
    fileNumber = 0; // Pick the first file in the list to decrement.
  } else            // must be next that has been clicked.
  {
    fileNumber = currentFiles.size() - 1; // Pick the last file to increment.
  }

  // File path should be the same for both.
  separateMuonFile(filePath, currentFiles[fileNumber], run, runSize);

  int fileExtensionSize(currentFiles[fileNumber].size() -
                        currentFiles[fileNumber].indexOf('.'));
  currentFiles[fileNumber].chop(fileExtensionSize);

  int firstRunNumber = currentFiles[fileNumber].right(runSize).toInt();
  currentFiles[fileNumber].chop(runSize);

  firstRunNumber = firstRunNumber + inc;
  QString newRun("");
  newRun.setNum(firstRunNumber);

  getFullCode(runSize, newRun);

  // Increment is positive (next button)
  if (inc < 0) {
    // Add the file to the beginning of mwRunFiles text box.
    QString lastName = m_previousFilenames.back();
    separateMuonFile(filePath, lastName, run, runSize);
    getFullCode(runSize, run);
    m_uiForm.mwRunFiles->setUserInput(newRun + '-' + run);
  } else // Increment is negative (previous button)
  {
    // Add the file onto the end of mwRunFiles text box
    QString firstName = m_previousFilenames[0];
    separateMuonFile(filePath, firstName, run, runSize);
    getFullCode(runSize, run);
    m_uiForm.mwRunFiles->setUserInput(run + '-' + newRun);
  }
}

/**
 *   Opens up the next file if clicked next or previous on the muon analysis
 *
 *   @param amountToChange :: if clicked next then you need to open the next
 *   file so 1 is passed, -1 is passed if previous was clicked by the user.
 */
void MuonAnalysis::changeRun(int amountToChange) {
  QString filePath("");
  QString currentFile = m_uiForm.mwRunFiles->getFirstFilename();
  if ((currentFile.isEmpty())) {
    if (m_previousFilenames.isEmpty()) {
      // not a valid file, and no previous valid files
      QMessageBox::warning(this, tr("Muon Analysis"),
                           tr("Unable to open the file.\n"
                              "and no previous valid files available."),
                           QMessageBox::Ok, QMessageBox::Ok);
      allowLoading(true);
      return;
    } else {
      // blank box - use previous run
      currentFile = m_previousFilenames[0];
    }
  }

  QString run("");
  int runSize(-1);

  // If load current run get the correct run number.
  if (currentFile.contains("auto") || currentFile.contains("argus0000000")) {
    separateMuonFile(filePath, currentFile, run, runSize);
    currentFile = filePath + QString::fromStdString(m_currentLabel) + ".nxs";
  }

  separateMuonFile(filePath, currentFile, run, runSize);

  int fileExtensionSize(currentFile.size() - currentFile.indexOf('.'));
  QString fileExtension(currentFile.right(fileExtensionSize));
  currentFile.chop(fileExtensionSize);

  int runNumber = currentFile.right(runSize).toInt();
  currentFile.chop(runSize);

  runNumber = runNumber + amountToChange;
  QString newRun("");
  newRun.setNum(runNumber);

  getFullCode(runSize, newRun);

  if (m_textToDisplay.contains("\\") || m_textToDisplay.contains("/") ||
      m_textToDisplay == "CURRENT RUN")
    m_uiForm.mwRunFiles->setUserInput(filePath + currentFile + newRun);
  else
    m_uiForm.mwRunFiles->setUserInput(newRun);
}

/**
 *   Seperates the a given file into instrument, code and size of the code.
 *   i.e c:/data/MUSR0002419.nxs becomes c:/data/, MUSR0002419.nxs, 2419, 7.
 *
 *   @param filePath :: The file path of the data file.
 *   @param currentFile :: This is the file with path. Can be network path.
 * Return as file with extension.
 *   @param run :: The run as a string without 0's at the beginning.
 *   @param runSize :: contains the size of the run number.
 */
void MuonAnalysis::separateMuonFile(QString &filePath, QString &currentFile,
                                    QString &run, int &runSize) {
  int fileStart(-1);
  int firstRunDigit(-1);

  // Find where the file begins
  for (int i = 0; i < currentFile.size(); i++) {
    if (currentFile[i] == '/' || currentFile[i] == '\\') //.isDigit())
    {
      fileStart = i + 1;
    }
  }

  filePath = currentFile.left(fileStart);
  currentFile = currentFile.right(currentFile.size() - fileStart);

  for (int i = 0; i < currentFile.size(); i++) {
    if (currentFile[i].isDigit()) //.isDigit())
    {
      firstRunDigit = i;
      break;
    }
  }

  runSize = 0;
  if (!(firstRunDigit < 0)) {
    // Find where the run number ends
    for (int i = firstRunDigit; i < currentFile.size(); i++) {
      if (currentFile[i] == '.')
        break;
      if (currentFile[i].isDigit()) {
        ++runSize;
      }
    }
  }
  run = currentFile.right(currentFile.size() - firstRunDigit);
  run = run.left(runSize);
}

/**
 * Adds the 0's back onto the run which were lost when converting it to an
 * integer.
 *
 * @param originalSize :: The size of the original run before conversion
 * @param run :: This is the run after it was incremented or decremented.
 */
void MuonAnalysis::getFullCode(int originalSize, QString &run) {
  while (originalSize > run.size()) {
    run = "0" + run;
  }
}

/**
 * Sets the fitting ranges on the dataselectot and fitbrowser
 *
 * @param xmin :: The minimum x value
 * @param xmax :: The maximum x value
 */
void MuonAnalysis::setFittingRanges(double xmin, double xmax) {
  if (xmin == 0.0 && xmax == 0.0) {
    // A previous fitting range of [0,0] means this is the first time the
    // user goes to "Data Analysis" tab
    // We have to initialise the fitting range
    m_dataSelector->setStartTime(
        m_uiForm.timeAxisStartAtInput->text().toDouble());
    m_dataSelector->setEndTime(
        m_uiForm.timeAxisFinishAtInput->text().toDouble());
    m_uiForm.fitBrowser->setStartX(
        m_uiForm.timeAxisStartAtInput->text().toDouble());
    m_uiForm.fitBrowser->setEndX(
        m_uiForm.timeAxisFinishAtInput->text().toDouble());

  }
  // or set it to the previous values provided by the user:
  else {
    // A previous fitting range already exists, so we use it
    m_dataSelector->setStartTime(xmin);
    m_dataSelector->setEndTime(xmax);
    m_uiForm.fitBrowser->setStartX(xmin);
    m_uiForm.fitBrowser->setEndX(xmax);
  }
}

/**
 * Is called every time when tab gets changed
 *
 * @param newTabIndex :: The index of the tab we switch to
 */
void MuonAnalysis::changeTab(int newTabIndex) {
  QWidget *newTab = m_uiForm.tabWidget->widget(newTabIndex);

  // Make sure all toolbars are still not visible. May have brought them back to
  // do a plot.
  if (m_uiForm.hideToolbars->isChecked())
    emit setToolbarsHidden(true);

  if (m_currentTab == m_uiForm.DataAnalysis) // Leaving DA tab
  {
    // Say MantidPlot to use default fit prop. browser
    emit setFitPropertyBrowser(nullptr);

    // Reset cached config option
    ConfigService::Instance().setString(PEAK_RADIUS_CONFIG, m_cachedPeakRadius);

    // Remove PP tool from any plots it was attached to
    disableAllTools();

    // Disconnect to avoid problems when filling list of workspaces in fit prop.
    // browser
    disconnect(m_uiForm.fitBrowser,
               SIGNAL(workspaceNameChanged(const QString &)), this,
               SLOT(selectMultiPeak(const QString &)));
    disconnect(m_uiForm.fitBrowser, SIGNAL(TFPlot(const QString &)), this,
               SLOT(selectMultiPeakNoUpdate(const QString &)));
  }

  if (newTab == m_uiForm.DataAnalysis) // Entering DA tab
  {
    // Save last fitting range
    auto xmin = m_uiForm.fitBrowser->startX();
    auto xmax = m_uiForm.fitBrowser->endX();
    // make sure data selector has same values
    m_dataSelector->setStartTime(xmin);
    m_dataSelector->setEndTime(xmax);

    // Say MantidPlot to use Muon Analysis fit prop. browser
    emit setFitPropertyBrowser(m_uiForm.fitBrowser);

    // Muon scientists never fit peaks, hence they want the following
    // parameter set to a high number
    m_cachedPeakRadius =
        ConfigService::Instance().getString(PEAK_RADIUS_CONFIG);
    ConfigService::Instance().setString(PEAK_RADIUS_CONFIG, "99");

    setFittingRanges(xmin, xmax);

    // If a workspace is selected:
    // - Show connected plot and attach PP tool to it (if has been assigned)
    // - Set input of data selector to selected workspace
    if (m_currentDataName != NOT_AVAILABLE) {
      const boost::optional<QString> filePath =
          m_uiForm.mwRunFiles->getUserInput().toString();
      m_fitDataPresenter->setSelectedWorkspace(m_currentDataName, filePath);
      setChosenGroupAndPeriods(m_currentDataName);
      selectMultiPeak(m_currentDataName, true, filePath);
    }

    // In future, when workspace gets changed, show its plot and attach PP tool
    // to it
    connect(m_uiForm.fitBrowser, SIGNAL(workspaceNameChanged(const QString &)),
            this, SLOT(selectMultiPeak(const QString &)), Qt::QueuedConnection);
    connect(m_uiForm.fitBrowser, SIGNAL(TFPlot(const QString &)), this,
            SLOT(selectMultiPeakNoUpdate(const QString &)),
            Qt::QueuedConnection);
    // repeat setting the fitting ranges as the above code can set them to an
    // unwanted default value
    setFittingRanges(xmin, xmax);
    // work out if data is a group or pair
    Muon::AnalysisOptions options(m_groupingHelper.parseGroupingTable());
    m_uiForm.fitBrowser->setGroupNames(options.grouping.groupNames);
    auto isItGroup = m_dataLoader.isContainedIn(m_groupPairName,
                                                options.grouping.groupNames);
    // make sure groups are not on if single fit
    if (m_optionTab->getMultiFitState() == Muon::MultiFitState::Disabled) {
      m_uiForm.fitBrowser->setSingleFitLabel(m_currentDataName.toStdString());
    } else {
      m_uiForm.fitBrowser->setAllGroupsOrPairs(isItGroup);
      m_uiForm.fitBrowser->updatePeriods();
    }

    m_uiForm.fitBrowser->setTFAsymm(false);

    m_uiForm.fitBrowser->checkFitEnabled();

  } else if (newTab == m_uiForm.ResultsTable) {
    m_resultTableTab->refresh();
  }

  m_currentTab = newTab;
}
void MuonAnalysis::updateNormalization(QString name) {
  m_uiForm.fitBrowser->setNormalization(name.toStdString());
}

/**
 * Set up the signals and slots for auto updating the plots
 */
void MuonAnalysis::connectAutoUpdate() {
  // Home tab Auto Updates
  connect(m_uiForm.frontGroupGroupPairComboBox, SIGNAL(activated(int)), this,
          SLOT(homeTabUpdatePlot()));

  connect(m_uiForm.frontPlotFuncs, SIGNAL(activated(int)), this,
          SLOT(homeTabUpdatePlot()));
  connect(m_uiForm.frontAlphaNumber, SIGNAL(returnPressed()), this,
          SLOT(homeTabUpdatePlot()));

  connect(m_uiForm.timeZeroFront, SIGNAL(returnPressed()), this,
          SLOT(homeTabUpdatePlot()));
  connect(m_uiForm.firstGoodBinFront, SIGNAL(returnPressed()), this,
          SLOT(homeTabUpdatePlot()));

  connect(m_uiForm.homePeriodBox1, SIGNAL(editingFinished()), this,
          SLOT(homeTabUpdatePlot()));
  connect(m_uiForm.homePeriodBox2, SIGNAL(editingFinished()), this,
          SLOT(homeTabUpdatePlot()));

  connect(m_uiForm.deadTimeType, SIGNAL(activated(int)), this,
          SLOT(deadTimeTypeAutoUpdate(int)));

  // Grouping tab Auto Updates
  connect(m_uiForm.groupTablePlotChoice, SIGNAL(activated(int)), this,
          SLOT(groupTabUpdatePlotGroup()));
  connect(m_uiForm.pairTablePlotChoice, SIGNAL(activated(int)), this,
          SLOT(groupTabUpdatePlotPair()));

  // Settings tab Auto Updates
  connect(m_optionTab, SIGNAL(settingsTabUpdatePlot()), this,
          SLOT(settingsTabUpdatePlot()));
  connect(m_optionTab, SIGNAL(plotStyleChanged()), this,
          SLOT(updateCurrentPlotStyle()));
  connect(m_optionTab, SIGNAL(multiFitStateChanged(int)), this,
          SLOT(multiFitCheckboxChanged(int)));
  connect(m_optionTab, SIGNAL(loadAllGroupChanged(int)), this,
          SLOT(loadAllGroups(int)));
  connect(m_optionTab, SIGNAL(loadAllPairsChanged(int)), this,
          SLOT(loadAllPairs(int)));
}

/**
 * Connect widgets to saveWidgetValue() slot so their values are automatically
 * saved when they are
 * getting changed.
 */
void MuonAnalysis::connectAutoSave() {
  connect(m_uiForm.timeZeroFront, SIGNAL(textChanged(const QString &)), this,
          SLOT(saveWidgetValue()));
  connect(m_uiForm.firstGoodBinFront, SIGNAL(textChanged(const QString &)),
          this, SLOT(saveWidgetValue()));

  connect(m_uiForm.timeZeroAuto, SIGNAL(stateChanged(int)), this,
          SLOT(saveWidgetValue()));
  connect(m_uiForm.firstGoodDataAuto, SIGNAL(stateChanged(int)), this,
          SLOT(saveWidgetValue()));
}

/**
 * Saves the value of the widget which called the slot.
 * TODO: should be done using MuonAnalysisHelper::WidgetAutoSaver
 */
void MuonAnalysis::saveWidgetValue() {
  // Get the widget which called the slot
  QWidget *sender = qobject_cast<QWidget *>(QObject::sender());

  if (!sender)
    throw std::runtime_error("Unable to save value of non-widget QObject");

  QString name = sender->objectName();

  QSettings settings;
  settings.beginGroup(m_settingsGroup + "SavedWidgetValues");

  // Save value for QLineEdit
  if (QLineEdit *w = qobject_cast<QLineEdit *>(sender)) {
    settings.setValue(name, w->text());
  }
  // Save value for QCheckBox
  else if (QCheckBox *w = qobject_cast<QCheckBox *>(sender)) {
    settings.setValue(name, static_cast<int>(w->checkState()));
  }
  // ... add more as neccessary
  else
    throw std::runtime_error(
        "Value saving for this widget type is not supported");

  settings.endGroup();
}

/**
 * Load previously saved value for the widget.
 * TODO: should be done using MuonAnalysisHelper::WidgetAutoSaver
 * @param       target :: Widget where the value will be loaded to
 * @param defaultValue :: Values which will be set if there is no saved value
 */
void MuonAnalysis::loadWidgetValue(QWidget *target,
                                   const QVariant &defaultValue) {
  QString name = target->objectName();

  QSettings settings;
  settings.beginGroup(m_settingsGroup + "SavedWidgetValues");

  // Load value for QLineEdit
  if (QLineEdit *w = qobject_cast<QLineEdit *>(target)) {
    w->setText(settings.value(name, defaultValue).toString());
  }
  // Load value for QCheckBox
  else if (QCheckBox *w = qobject_cast<QCheckBox *>(target)) {
    w->setCheckState(static_cast<Qt::CheckState>(
        settings.value(name, defaultValue).toInt()));
  }
  // ... add more as neccessary
  else
    throw std::runtime_error(
        "Value loading for this widget type is not supported");

  settings.endGroup();
}

/**
 * Checks whether two specified period sets are equal and, if they are, unsets
 * second one.
 * At present, no check is made for if the same index appears in both lists -
 * this is down to the user!
 */
void MuonAnalysis::checkForEqualPeriods() {
  if (m_uiForm.homePeriodBox2->text() == m_uiForm.homePeriodBox1->text()) {
    m_uiForm.homePeriodBox2->clear();
  }
}

void MuonAnalysis::homeTabUpdatePlot() {
  if (isAutoUpdateEnabled() && m_currentTab == m_uiForm.Home && m_loaded)
    runFrontPlotButton();
}

/**
 * Update plot based on changes made in "Grouping Options" tab
 * (e.g. plot type combo box changed)
 * For group selected
 */
void MuonAnalysis::groupTabUpdatePlotGroup() {
  if (isAutoUpdateEnabled() && m_currentTab == m_uiForm.GroupingOptions &&
      m_loaded) {
    updateFront();
    runTablePlotButton(ItemType::Group);
  }
}

/**
 * Update plot based on changes made in "Grouping Options" tab
 * (e.g. plot type combo box changed)
 * For pair selected
 */
void MuonAnalysis::groupTabUpdatePlotPair() {
  if (isAutoUpdateEnabled() && m_currentTab == m_uiForm.GroupingOptions &&
      m_loaded) {
    updateFront();
    runTablePlotButton(ItemType::Pair);
  }
}

/**
 * Called when something on the options tab has been changed
 * Update rebin params in fit data presenter and replot if necessary
 */
void MuonAnalysis::settingsTabUpdatePlot() {
  // Update the fit data presenter if rebin options have changed
  updateRebinParams();

  if (isAutoUpdateEnabled() && m_currentTab == m_uiForm.Settings &&
      m_loaded == true) {
    runFrontPlotButton();
  }
}

/**
 * Sets plot type combo box on the Home tab to the same value as the one under
 * Group Table.
 */
void MuonAnalysis::syncGroupTablePlotTypeWithHome() {
  int plotTypeIndex = m_uiForm.groupTablePlotChoice->currentIndex();

  if (m_uiForm.frontPlotFuncs->count() <= plotTypeIndex) {
    // This is not the best solution, but I don't have anything brighter at the
    // moment and it
    // was working like that for some time without anybody complaining.
    setGroupOrPairIndexToPlot(0);
    plotCurrentGroupAndPairs();
  }

  m_uiForm.frontPlotFuncs->setCurrentIndex(plotTypeIndex);
}

/**
 * Updates the style of the current plot according to actual parameters on
 * settings tab.
 */
void MuonAnalysis::updateCurrentPlotStyle() {
  if (isAutoUpdateEnabled() && m_currentDataName != NOT_AVAILABLE) {
    // Replot using new style params
    plotSpectrum(m_currentDataName);
  }
}

bool MuonAnalysis::isAutoUpdateEnabled() {
  int choice(m_uiForm.plotCreation->currentIndex());
  return (choice == 0 || choice == 1);
}

/**
 * Whether Overwrite option is enabled on the Settings tab.
 * @return True if enabled, false if not
 */
bool MuonAnalysis::isOverwriteEnabled() {
  int choice(m_uiForm.plotCreation->currentIndex());
  return (choice == 0 || choice == 2);
}

/**
 * Executed when interface gets hidden or closed
 */
void MuonAnalysis::hideEvent(QHideEvent * /*unused*/) {
  // Show toolbars if were chosen to be hidden by user
  if (m_uiForm.hideToolbars->isChecked())
    emit setToolbarsHidden(false);

  // If closed while on DA tab, reassign fit property browser to default one
  if (m_currentTab == m_uiForm.DataAnalysis)
    emit setFitPropertyBrowser(nullptr);
}

/**
 * Executed when interface gets shown
 */
void MuonAnalysis::showEvent(QShowEvent * /*unused*/) {
  // Hide toolbars if requested by user
  if (m_uiForm.hideToolbars->isChecked())
    emit setToolbarsHidden(true);
}

/**
 * Hide/show MantidPlot toolbars.
 * @param hidden If true, toolbars will be hidden, if false - shown
 */
void MuonAnalysis::doSetToolbarsHidden(bool hidden) {
  QString isVisibleStr = hidden ? "False" : "True";

  runPythonCode(QString("setToolbarsVisible(%1)").arg(isVisibleStr));
}

/**
 * Called when dead time correction type is changed.
 * @param choice :: New index of dead time correction type combo box
 */
void MuonAnalysis::onDeadTimeTypeChanged(int choice) {
  m_deadTimesChanged = true;
  m_dataLoader.clearCache();
  if (choice == 0 || choice == 1) // if choice == none || choice == from file
  {
    m_uiForm.mwRunDeadTimeFile->setVisible(false);
    m_uiForm.dtcFileLabel->setVisible(false);
    if (choice == 0) {
      m_dataLoader.setDeadTimesType(Muon::DeadTimesType::None);
    } else {
      m_dataLoader.setDeadTimesType(Muon::DeadTimesType::FromFile);
    }
  } else // choice must be from workspace
  {
    m_uiForm.mwRunDeadTimeFile->setVisible(true);
    m_uiForm.mwRunDeadTimeFile->setUserInput("");
    m_uiForm.dtcFileLabel->setVisible(true);
    m_dataLoader.setDeadTimesType(Muon::DeadTimesType::FromDisk);
  }

  QSettings group;
  group.beginGroup(m_settingsGroup + "DeadTimeOptions");
  group.setValue("deadTimes", choice);
}

/**
 * Auto-update the plot after user has changed dead time correction type.
 * @param choice :: User selected index of the dead time correction combox box
 */
void MuonAnalysis::deadTimeTypeAutoUpdate(int choice) {
  // We update the plot only if user switches to "None" or "From Data File"
  // correction type, because
  // in case of "From Disk" the file should be specified first.
  if (choice == 0 || choice == 1) {
    homeTabUpdatePlot();
  }
}

/**
 * If the user selects/changes the file to be used to apply the dead times then
 * see if the plot needs updating and make sure next time the user plots that
 * the dead times are applied.
 */
void MuonAnalysis::deadTimeFileSelected() {
  if (!m_uiForm.mwRunDeadTimeFile->isValid())
    return;

  // Remember the filename for the next time interface is opened
  QSettings group;
  group.beginGroup(m_settingsGroup + "DeadTimeOptions");
  group.setValue("deadTimeFile", m_uiForm.mwRunDeadTimeFile->getText());
  m_dataLoader.setDeadTimesType(
      Muon::DeadTimesType::FromDisk,
      m_uiForm.mwRunDeadTimeFile->getText().toStdString());
  m_deadTimesChanged = true;
  homeTabUpdatePlot();
}

/**
 * Updates the enabled-state and value of Time Zero using "auto" check-box
 * state.
 * @param checkBoxState :: State of "auto" check-box. If -1 will retrieve it
 * from the form
 */
void MuonAnalysis::setTimeZeroState(int checkBoxState) {
  m_dataLoader.clearCache();
  if (checkBoxState == -1)
    checkBoxState = m_uiForm.timeZeroAuto->checkState();

  if (checkBoxState == Qt::Checked) // From data file
  {
    m_uiForm.timeZeroFront->setEnabled(false);
    m_uiForm.timeZeroFront->setText(QString::number(m_dataTimeZero, 'g', 2));
    homeTabUpdatePlot(); // Auto-update
  } else                 // Custom
  {
    m_uiForm.timeZeroFront->setEnabled(true);
  }
}

/**
 * Updates the enabled-state and value of First Good Data using "auto" check-box
 * state.
 * @param checkBoxState :: State of "auto" check-box. If -1 will retrieve it
 * from the form
 */
void MuonAnalysis::setFirstGoodDataState(int checkBoxState) {
  m_dataLoader.clearCache();
  if (checkBoxState == -1)
    checkBoxState = m_uiForm.firstGoodDataAuto->checkState();

  if (checkBoxState == Qt::Checked) // From data file
  {
    m_uiForm.firstGoodBinFront->setEnabled(false);
    m_uiForm.firstGoodBinFront->setText(
        QString::number(m_dataFirstGoodData, 'g', 2));
    homeTabUpdatePlot(); // Auto-update
  } else                 // Custom
  {
    m_uiForm.firstGoodBinFront->setEnabled(true);
  }
}

/**
 * Groups detectors in the workspace
 * @param wsName :: ADS name of the workspace to group
 * @param groupingName :: ADS name of the grouping table to use
 * @return Grouped workspace
 */
Workspace_sptr
MuonAnalysis::groupWorkspace(const std::string &wsName,
                             const std::string &groupingName) const {
  ScopedWorkspace outputEntry;
  // Use MuonProcess in "correct and group" mode.
  // No dead time correction so all it does is group the workspaces.
  try {
    auto groupAlg = AlgorithmManager::Instance().createUnmanaged("MuonProcess");
    groupAlg->initialize();
    groupAlg->setRethrows(true);
    groupAlg->setLogging(false);
    groupAlg->setPropertyValue("InputWorkspace", wsName);
    groupAlg->setPropertyValue("Mode", "CorrectAndGroup");
    groupAlg->setProperty("ApplyDeadTimeCorrection", false);
    groupAlg->setProperty(
        "LoadedTimeZero",
        m_dataTimeZero); // won't be used, but property is mandatory
    groupAlg->setPropertyValue("DetectorGroupingTable", groupingName);
    groupAlg->setPropertyValue("OutputWorkspace", outputEntry.name());
    // want to remove data before first good data
    groupAlg->setProperty("xmin", firstGoodBin());
    groupAlg->setProperty("xmax", m_dataSelector->getEndTime());
    groupAlg->execute();

  } catch (std::exception &e) {
    throw std::runtime_error("Unable to group workspace:\n\n" +
                             std::string(e.what()));
  }
  return outputEntry.retrieve();
}

/**
 * Groups loaded workspace using information from Grouping Options tab.
 * I.e. m_workspace_name is grouped with result placed to m_grouped_name
 */
void MuonAnalysis::groupLoadedWorkspace() {
  ITableWorkspace_sptr grouping = parseGrouping();

  if (!grouping)
    throw std::invalid_argument(
        "Unable to parse grouping information from the table, or it is empty.");

  ScopedWorkspace groupingEntry(grouping);

  Workspace_sptr groupedWorkspace =
      groupWorkspace(m_workspace_name, groupingEntry.name());

  deleteWorkspaceIfExists(m_grouped_name);
  AnalysisDataService::Instance().add(m_grouped_name, groupedWorkspace);
}

/**
 * Parses grouping information from the UI table.
 * @return ITableWorkspace of the format returned by LoadMuonNexus
 */
ITableWorkspace_sptr MuonAnalysis::parseGrouping() {
  auto grouping = m_groupingHelper.parseGroupingTable();
  return grouping.toTable();
}

/**
 * Returns custom dead time table file name as set on the interface.
 * @return The filename
 */
std::string MuonAnalysis::deadTimeFilename() const {
  if (!m_uiForm.mwRunDeadTimeFile->isValid())
    throw std::runtime_error("Specified Dead Time file is not valid.");

  return m_uiForm.mwRunDeadTimeFile->getFirstFilename().toStdString();
}

/**
 * When no data loaded set various buttons etc to inactive
 */
void MuonAnalysis::noDataAvailable() {
  m_uiForm.frontPlotButton->setEnabled(false);
  m_uiForm.groupTablePlotButton->setEnabled(false);
  m_uiForm.pairTablePlotButton->setEnabled(false);
  m_uiForm.guessAlphaButton->setEnabled(false);
  setAnalysisTabsEnabled(false);
}

/**
 * When data loaded set various buttons etc to active
 */
void MuonAnalysis::nowDataAvailable() {
  m_uiForm.frontPlotButton->setEnabled(true);
  m_uiForm.groupTablePlotButton->setEnabled(true);
  m_uiForm.pairTablePlotButton->setEnabled(true);
  m_uiForm.guessAlphaButton->setEnabled(true);
  setAnalysisTabsEnabled(true);
}

void MuonAnalysis::openDirectoryDialog() {
  auto ad = new MantidQt::API::ManageUserDirectories(this);
  ad->show();
  ad->setFocus();
}

/**
 * Updates the current choice of which group or group pair to plot
 * Also updates the UI on the front panel
 * The point of using this function is so that the UI is never out of sync
 * @param index :: [input] Index of which group/pair to plot
 */
void MuonAnalysis::setGroupOrPairIndexToPlot(int index) {
  m_uiForm.frontGroupGroupPairComboBox->setCurrentIndex(index);
}

void MuonAnalysis::plotCurrentGroupAndPairs() {
  // Replot, whichever tab we're currently on
  if (m_loaded && isAutoUpdateEnabled()) {
    runFrontPlotButton();
  }
}

/**
 * Current index of which group/pair to plot
 */
int MuonAnalysis::getGroupOrPairToPlot() const {
  return m_uiForm.frontGroupGroupPairComboBox->currentIndex();
}

/**
 * Fills in the grouping table using information from provided Grouping struct.
 *
 * @param grouping :: [input] Grouping struct to use for filling the table
 */
void MuonAnalysis::fillGroupingTable(const Grouping &grouping) {
  int defaultIndex = m_groupingHelper.fillGroupingTable(grouping);
  setGroupOrPairIndexToPlot(defaultIndex);
  plotCurrentGroupAndPairs();
}

/**
 * Returns the set of summed period numbers
 * @returns :: period number string
 */
std::string MuonAnalysis::getSummedPeriods() const {
  auto summed = m_uiForm.homePeriodBox1->text().toStdString();
  summed.erase(std::remove(summed.begin(), summed.end(), ' '), summed.end());
  return summed;
}

/**
 * Returns the set of subtracted period numbers
 * @returns :: period number string
 */
std::string MuonAnalysis::getSubtractedPeriods() const {
  auto subtracted = m_uiForm.homePeriodBox2->text().toStdString();
  subtracted.erase(std::remove(subtracted.begin(), subtracted.end(), ' '),
                   subtracted.end());
  return subtracted;
}

/**
 * Slot: groups/periods/runs to fit changed in data selector widget
 * Pass this information to the fit helper
 */
void MuonAnalysis::dataToFitChanged() {
  if (m_fitDataPresenter && m_loaded) { // Only act if some data is loaded
    m_fitDataPresenter->setGrouping(m_groupingHelper.parseGroupingTable());
    m_fitDataPresenter->setPlotType(parsePlotType(m_uiForm.frontPlotFuncs));
    // Set busy cursor while workspaces are being created
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    m_fitDataPresenter->handleSelectedDataChanged(isOverwriteEnabled());
    QApplication::restoreOverrideCursor();
  }
}

/**
 * Return a list of supported muon instruments
 * @returns :: list of instruments
 */
QStringList MuonAnalysis::getSupportedInstruments() {
  QStringList instruments;
  for (int i = 0; i < m_uiForm.instrSelector->count(); i++) {
    instruments.append(m_uiForm.instrSelector->itemText(i));
  }
  return instruments;
}

/**
 * Gets rebin arguments off the options tab and passes them to the fit data
 * presenter
 */
void MuonAnalysis::updateRebinParams() {
  std::pair<MuonAnalysisOptionTab::RebinType, std::string> rebinParams;
  rebinParams.first = m_optionTab->getRebinType();
  if (rebinParams.first == MuonAnalysisOptionTab::RebinType::FixedRebin) {
    rebinParams.second = std::to_string(m_optionTab->getRebinStep());
  } else if (rebinParams.first ==
             MuonAnalysisOptionTab::RebinType::VariableRebin) {
    rebinParams.second = m_optionTab->getRebinParams();
  }
  m_fitDataPresenter->setRebinArgs(rebinParams);
}

/**
 * Set the "load current run" button enabled/disabled.
 * To be set enabled, the following must be true:
 * 1) this is a Windows machine
 * 2) facility is ISIS
 * @param enabled :: [input] Whether to enable/disable button
 */
void MuonAnalysis::setLoadCurrentRunEnabled(bool enabled) {
  if (enabled) {
#ifdef _WIN32
    // "Load current run" is only possible at ISIS
    if (ConfigService::Instance().getFacility().name() != "ISIS") {
      enabled = false;
    }
#else
    enabled = false;
#endif
  }
  m_uiForm.loadCurrent->setEnabled(enabled);
}

/**
 * Called when the "enable multiple fitting" checkbox is changed (settings tab.)
 * Forward this to the fit function presenter.
 */
void MuonAnalysis::multiFitCheckboxChanged(int state) {
  const Muon::MultiFitState multiFitState = state == Qt::CheckState::Checked
                                                ? Muon::MultiFitState::Enabled
                                                : Muon::MultiFitState::Disabled;
  m_fitFunctionPresenter->setMultiFitState(multiFitState);
}
/**
 * Checks if the run is set and if the plot name is valid.
 * If they are not valid then the loadAllGroups and loadAllPairs
 * methods should do nothing.
 */

bool MuonAnalysis::safeToLoadAllGroupsOrPairs() {
  std::string plotTypeName =
      m_uiForm.frontPlotFuncs->currentText().toStdString();
  if (m_currentLabel == "NoLabelSet") {
    return false;
  } else if (plotTypeName != "Asymmetry" && plotTypeName != "Counts" &&
             plotTypeName != "Logarithm") {
    return false;
  }
  return true;
}

/**
 * Load all of the pairs if the all pairs tickbox is ticked
 * @param state :: [input] (not used) Setting of combo box
 */
void MuonAnalysis::loadAllGroups(int state) {
  Q_UNUSED(state);

  if (m_uiForm.loadAllGroupsCheckBox->isChecked() &&
      safeToLoadAllGroupsOrPairs()) {
    ItemType itemType = Group;
    PlotType plotType = parsePlotType(m_uiForm.frontPlotFuncs);
    for (int j = 0; j < numGroups(); j++) {
      addItem(itemType, j, plotType);
    }
  }
}
/**
 * Load all of the pairs if the all pairs tickbox is ticked
 * @param state :: [input] (not used) Setting of combo box
 */
void MuonAnalysis::loadAllPairs(int state) {

  Q_UNUSED(state);
  if (m_uiForm.loadAllPairsCheckBox->isChecked() &&
      safeToLoadAllGroupsOrPairs()) {
    ItemType itemType = Pair;
    PlotType plotType = parsePlotType(m_uiForm.frontPlotFuncs);
    for (int j = 0; j < numPairs(); j++) {

      addItem(itemType, j, plotType);
    }
  }
}
/**
 * Update the fit data presenter with current overwrite setting
 * @param state :: [input] (not used) Setting of combo box
 */
void MuonAnalysis::updateDataPresenterOverwrite(int state) {
  Q_UNUSED(state);
  if (m_fitDataPresenter) {
    m_fitDataPresenter->setOverwrite(isOverwriteEnabled());
  }
}

/**
 * Set the following tabs enabled/disabled:
 * - Grouping Options
 * - Data Analysis
 * (based on whether data is available or not)
 * @param enabled :: [input] Whether to enable or disable tabs
 */
void MuonAnalysis::setAnalysisTabsEnabled(const bool enabled) {
  const std::vector<QWidget *> tabs{m_uiForm.DataAnalysis,
                                    m_uiForm.GroupingOptions};
  for (auto *tab : tabs) {
    const auto &index = m_uiForm.tabWidget->indexOf(tab);
    m_uiForm.tabWidget->setTabEnabled(index, enabled);
  }
}

bool MuonAnalysis::getIfTFAsymmStore() const {
  Muon::AnalysisOptions options(m_groupingHelper.parseGroupingTable());
  bool value =
      m_dataLoader.isContainedIn(m_groupPairName, options.grouping.groupNames);
  return value;
}

} // namespace CustomInterfaces
} // namespace MantidQt
