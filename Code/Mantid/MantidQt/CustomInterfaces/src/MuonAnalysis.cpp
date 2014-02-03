//----------------------
// Includes
//----------------------
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/ScopedWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/XMLlogfile.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidQtAPI/FileDialogHandler.h"
#include "MantidQtCustomInterfaces/IO_MuonGrouping.h"
#include "MantidQtCustomInterfaces/MuonAnalysis.h"
#include "MantidQtCustomInterfaces/MuonAnalysisFitDataTab.h"
#include "MantidQtCustomInterfaces/MuonAnalysisOptionTab.h"
#include "MantidQtCustomInterfaces/MuonAnalysisResultTableTab.h"
#include "MantidQtMantidWidgets/FitPropertyBrowser.h"
#include "MantidQtMantidWidgets/MuonFitPropertyBrowser.h"
#include "MantidQtMantidWidgets/MuonSequentialFitDialog.h"

#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/StringTokenizer.h>
#include <boost/lexical_cast.hpp>

#include <algorithm>

#include <QLineEdit>
#include <QVariant>
#include <QtProperty>
#include <QFileDialog>
#include <QHash>
#include <QTextStream>
#include <QTreeWidgetItem>
#include <QSettings>
#include <QMessageBox>
#include <QInputDialog>
#include <QSignalMapper>
#include <QHeaderView>
#include <QApplication>
#include <QTemporaryFile>
#include <QDesktopServices>
#include <QUrl>

#include <fstream>

//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
namespace CustomInterfaces
{
  DECLARE_SUBWINDOW(MuonAnalysis);

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace MantidQt::MantidWidgets;
using namespace MantidQt::CustomInterfaces;
using namespace MantidQt::CustomInterfaces::Muon;
using namespace Mantid::Geometry;

// Initialize the logger
Logger& MuonAnalysis::g_log = Logger::get("MuonAnalysis");

// Static constants
const QString MuonAnalysis::NOT_AVAILABLE("N/A");

//----------------------
// Public member functions
//----------------------
///Constructor
MuonAnalysis::MuonAnalysis(QWidget *parent) :
  UserSubWindow(parent), 
  m_last_dir(), 
  m_workspace_name("MuonAnalysis"), m_grouped_name(m_workspace_name + "Grouped"), 
  m_currentDataName(),
  m_groupTableRowInFocus(0), m_pairTableRowInFocus(0), 
  m_currentTab(NULL),
  m_groupNames(), 
  m_settingsGroup("CustomInterfaces/MuonAnalysis/"),
  m_updating(false), m_loaded(false), m_deadTimesChanged(false), 
  m_textToDisplay(""), 
  m_dataTimeZero(0.0), m_dataFirstGoodData(0.0)
{}

/**
 * Initialize local Python environmnet. 
 */
void MuonAnalysis::initLocalPython()
{
  QString code;

  code += "from mantid.simpleapi import *\n";

  // Needed for Python GUI API
  code += "from PyQt4.QtGui import QPen, QBrush, QColor\n"
          "from PyQt4.QtCore import QSize\n";

  runPythonCode(code);

  // TODO: Following shouldn't be here. It is now because ApplicationWindow sets up the Python 
  // environment only after the UserSubWindow is shown.

  // Hide the toolbars, if user wants to
  if(m_uiForm.hideToolbars->isChecked())
    emit setToolbarsHidden(true);
}

/// Set up the dialog layout
void MuonAnalysis::initLayout()
{
  m_uiForm.setupUi(this);

  std::set<std::string> supportedFacilities;
  supportedFacilities.insert("ISIS");
  supportedFacilities.insert("SmuS");

  const std::string userFacility = ConfigService::Instance().getFacility().name();

  // Allow to load current run for ISIS only 
  if ( userFacility != "ISIS" )
    m_uiForm.loadCurrent->setDisabled(true);

  // If facility if not supported by the interface - show a warning, but still open it
  if ( supportedFacilities.find(userFacility) == supportedFacilities.end() )
  {
    const std::string supportedFacilitiesStr = Strings::join(supportedFacilities.begin(), 
      supportedFacilities.end(), ", ");

    const QString errorTemplate = 
      "Your facility (%1) is not supported by MuonAnalysis, so you will not be able to load any files. \n\n"
      "Supported facilities are: %2. \n\n" 
      "Please use Preferences -> Mantid -> Instrument to update your facility information.";

    const QString error = errorTemplate.arg( userFacility.c_str(), supportedFacilitiesStr.c_str() );

    QMessageBox::warning(this, "Unsupported facility", error);
  }

  m_uiForm.fitBrowser->init();
  connect( m_uiForm.fitBrowser, SIGNAL(sequentialFitRequested()), 
           this, SLOT(openSequentialFitDialog()) );

  // alow appending files
  m_uiForm.mwRunFiles->allowMultipleFiles(true);

  // Further set initial look
  startUpLook();
  m_uiForm.mwRunFiles->readSettings(m_settingsGroup + "mwRunFilesBrowse");

  connect(m_uiForm.previousRun, SIGNAL(clicked()), this, SLOT(checkAppendingPreviousRun()));
  connect(m_uiForm.nextRun, SIGNAL(clicked()), this, SLOT(checkAppendingNextRun()));

  m_optionTab = new MuonAnalysisOptionTab(m_uiForm, m_settingsGroup);
  m_optionTab->initLayout();

  m_fitDataTab = new MuonAnalysisFitDataTab(m_uiForm);
  m_fitDataTab->init();

  m_resultTableTab = new MuonAnalysisResultTableTab(m_uiForm);
  connect(m_resultTableTab, SIGNAL(runPythonCode(const QString&, bool)),
                      this, SIGNAL(runAsPythonScript(const QString&, bool)));

  setCurrentDataName(NOT_AVAILABLE);

  // connect guess alpha
  connect(m_uiForm.guessAlphaButton, SIGNAL(clicked()), this, SLOT(guessAlphaClicked()));

  // signal/slot connections to respond to changes in instrument selection combo boxes
  connect(m_uiForm.instrSelector, SIGNAL(instrumentSelectionChanged(const QString&)), this, SLOT(userSelectInstrument(const QString&)));

  // Load current
  connect(m_uiForm.loadCurrent, SIGNAL(clicked()), this, SLOT(runLoadCurrent()));

  // If group table change
  // currentCellChanged ( int currentRow, int currentColumn, int previousRow, int previousColumn )
  connect(m_uiForm.groupTable, SIGNAL(cellChanged(int, int)), this, SLOT(groupTableChanged(int, int)));
  connect(m_uiForm.groupTable, SIGNAL(cellClicked(int, int)), this, SLOT(groupTableClicked(int, int)));
  connect(m_uiForm.groupTable->verticalHeader(), SIGNAL(sectionClicked(int)), SLOT(groupTableClicked(int)));

  // group table plot button
  connect(m_uiForm.groupTablePlotButton, SIGNAL(clicked()), this, SLOT(runGroupTablePlotButton()));

  // If pair table change
  connect(m_uiForm.pairTable, SIGNAL(cellChanged(int, int)), this, SLOT(pairTableChanged(int, int)));
  connect(m_uiForm.pairTable, SIGNAL(cellClicked(int, int)), this, SLOT(pairTableClicked(int, int)));
  connect(m_uiForm.pairTable->verticalHeader(), SIGNAL(sectionClicked(int)), SLOT(pairTableClicked(int)));
  // Pair table plot button
  connect(m_uiForm.pairTablePlotButton, SIGNAL(clicked()), this, SLOT(runPairTablePlotButton()));

  // save grouping
  connect(m_uiForm.saveGroupButton, SIGNAL(clicked()), this, SLOT(runSaveGroupButton()));

  // load grouping
  connect(m_uiForm.loadGroupButton, SIGNAL(clicked()), this, SLOT(runLoadGroupButton()));

  // clear grouping
  connect(m_uiForm.clearGroupingButton, SIGNAL(clicked()), this, SLOT(runClearGroupingButton()));

  // front plot button
  connect(m_uiForm.frontPlotButton, SIGNAL(clicked()), this, SLOT(runFrontPlotButton()));

  // front group/ group pair combobox
  connect(m_uiForm.frontGroupGroupPairComboBox, SIGNAL( currentIndexChanged(int) ), this, SLOT( updateFront() ));

  // Synchronize plot function selector on the Home tab with the one under the Group Table
  connect(m_uiForm.frontPlotFuncs, SIGNAL( activated(int) ),m_uiForm.groupTablePlotChoice, SLOT( setCurrentIndex(int) ) );
  connect(m_uiForm.groupTablePlotChoice, SIGNAL( activated(int) ), this, SLOT( syncGroupTablePlotTypeWithHome() ) );

  connect(m_uiForm.homePeriodBox1, SIGNAL( currentIndexChanged(int) ), this, SLOT( checkForEqualPeriods() ));
  connect(m_uiForm.homePeriodBox2, SIGNAL( currentIndexChanged(int) ), this, SLOT( checkForEqualPeriods() ));

  connect(m_uiForm.hideToolbars, SIGNAL( toggled(bool) ), this, SIGNAL( setToolbarsHidden(bool) ));

  // connect "?" (Help) Button
  connect(m_uiForm.muonAnalysisHelp, SIGNAL(clicked()), this, SLOT(muonAnalysisHelpClicked()));
  connect(m_uiForm.muonAnalysisHelpGrouping, SIGNAL(clicked()), this, SLOT(muonAnalysisHelpGroupingClicked()));

  // add combo boxes to pairTable
  for (int i = 0; i < m_uiForm.pairTable->rowCount(); i++)
  {
    m_uiForm.pairTable->setCellWidget(i,1, new QComboBox);
    m_uiForm.pairTable->setCellWidget(i,2, new QComboBox);
  }

  // file input
  connect(m_uiForm.mwRunFiles, SIGNAL(fileFindingFinished()), this, SLOT(inputFileChanged_MWRunFiles()));

  connect(m_uiForm.timeZeroAuto, SIGNAL(stateChanged(int)), this, SLOT(setTimeZeroState(int)));
  connect(m_uiForm.firstGoodDataAuto, SIGNAL(stateChanged(int)), this, SLOT(setFirstGoodDataState(int)));

  // load previous saved values
  loadAutoSavedValues(m_settingsGroup);

  // connect the fit function widget buttons to their respective slots.
  loadFittings();

  // Detect when the tab is changed
  connect(m_uiForm.tabWidget, SIGNAL(currentChanged(int)), this, SLOT(changeTab(int)));

  connectAutoUpdate();

  connectAutoSave();

  // Muon scientists never fits peaks, hence they want the following parameter, set to a high number
  ConfigService::Instance().setString("curvefitting.peakRadius","99");

  connect(m_uiForm.deadTimeType, SIGNAL( currentIndexChanged(int) ),
          this, SLOT( onDeadTimeTypeChanged(int) ));

  connect(m_uiForm.mwRunDeadTimeFile, SIGNAL( fileFindingFinished() ),
          this, SLOT( deadTimeFileSelected() ));

  m_currentTab = m_uiForm.tabWidget->currentWidget();

  connect(this, SIGNAL( setToolbarsHidden(bool) ), this, SLOT( doSetToolbarsHidden(bool) ), 
    Qt::QueuedConnection ); // We dont' neet this to happen instantly, prefer safer way
}

/**
* Muon Analysis help (slot)
*/
void MuonAnalysis::muonAnalysisHelpClicked()
{
  QDesktopServices::openUrl(QUrl(QString("http://www.mantidproject.org/") +
            "MuonAnalysis"));
}

/**
* Muon Analysis Grouping help (slot)
*/
void MuonAnalysis::muonAnalysisHelpGroupingClicked()
{
  QDesktopServices::openUrl(QUrl(QString("http://www.mantidproject.org/") +
            "MuonAnalysisGrouping"));
}


/**
 * Set the connected workspace name.
 * @param name The new connected ws name
 */
void MuonAnalysis::setCurrentDataName(const QString& name)
{
  m_currentDataName = name;

  // Update labels
  m_uiForm.connectedDataHome->setText("Connected: " + m_currentDataName);
  m_uiForm.connectedDataGrouping->setText("Connected: " + m_currentDataName);
  m_uiForm.connectedDataSettings->setText("Connected: " + m_currentDataName);
}

/**
* Front plot button (slot)
*/
void MuonAnalysis::runFrontPlotButton()
{
  if(m_updating)
    return;

  if (m_deadTimesChanged)
  {
    inputFileChanged(m_previousFilenames);
    return;
  }

  plotSelectedItem();
}

/**
 * Creates a plot of selected group/pair.
 */
void MuonAnalysis::plotSelectedItem()
{
  ItemType itemType;
  int tableRow; 

  int index = m_uiForm.frontGroupGroupPairComboBox->currentIndex();

  if (index < 0)
    return; // Nothing to plot

  if (index >= numGroups())
  {
    itemType = Pair;
    tableRow = m_pairToRow[index-numGroups()];
  }
  else
  {
    itemType = Group;
    tableRow = m_groupToRow[index];
  }

  PlotType plotType = parsePlotType(m_uiForm.frontPlotFuncs);

  plotItem(itemType, tableRow, plotType);  
}

/**
 * Creates workspace for specified group/pair and plots it;
 * @param itemType :: Whether it's a group or pair
 * @param tableRow :: Row in the group/pair table which contains the item
 * @param plotType :: What kind of plot we want to analyse
 */ 
void MuonAnalysis::plotItem(ItemType itemType, int tableRow, PlotType plotType)
{
  m_updating = true;

  AnalysisDataServiceImpl& ads = AnalysisDataService::Instance();

  try 
  {
    // Name of the group currently used to store plot workspaces. Depends on loaded data.
    const std::string groupName = getGroupName().toStdString();

    // Create workspace and a raw (unbinned) version of it
    MatrixWorkspace_sptr ws = createAnalysisWorkspace(itemType, tableRow, plotType);
    MatrixWorkspace_sptr wsRaw = createAnalysisWorkspace(itemType, tableRow, plotType, true);

    // Find names for new workspaces
    const std::string wsName = getNewAnalysisWSName(groupName, itemType, tableRow, plotType); 
    const std::string wsRawName = wsName + "_Raw"; 

    // Make sure they end up in the ADS
    ads.addOrReplace(wsName, ws);
    ads.addOrReplace(wsRawName, wsRaw);

    // Make sure they are in the right group
    if ( ! ads.retrieveWS<WorkspaceGroup>(groupName)->contains(wsName) )
    {
      ads.addToGroup(groupName, wsName);
      ads.addToGroup(groupName, wsRawName);
    }

    QString wsNameQ = QString::fromStdString(wsName);

    // Hide all the previous plot windows, if requested by user
    if (m_uiForm.hideGraphs->isChecked())
      hideAllPlotWindows();

    // Plot the workspace
    plotSpectrum( wsNameQ, (plotType == Logorithm) );

    setCurrentDataName( wsNameQ );
  }
  catch(...)
  { 
    QMessageBox::critical( this, "MuonAnalysis - Error", "Unable to plot the item. Check log for details." ); 
  }

  m_updating = false;
}

/**
 * Finds a name for new analysis workspace.
 * @param runLabel :: String describing the run we are working with
 * @param itemType :: Whether it's a group or pair
 * @param tableRow :: Row in the group/pair table which contains the item
 * @param plotType :: What kind of plot we want to analyse
 * @return New name
 */ 
std::string MuonAnalysis::getNewAnalysisWSName(const std::string& runLabel, ItemType itemType, int tableRow, 
  PlotType plotType)
{
  std::string plotTypeName;

  switch(plotType)
  {
    case Asymmetry:
      plotTypeName = "Asym"; break;
    case Counts:
      plotTypeName = "Counts"; break;
    case Logorithm:
      plotTypeName = "Logs"; break;
  }

  std::string itemTypeName;
  std::string itemName;

  if ( itemType == Pair )
  {
    itemTypeName = "Pair";
    itemName = m_uiForm.pairTable->item(tableRow,0)->text().toStdString();
  }
  else if ( itemType == Group )
  {
    itemTypeName = "Group";
    itemName = m_uiForm.groupTable->item(tableRow,0)->text().toStdString();
  }

  const std::string firstPart = runLabel + "; " + itemTypeName + "; " + itemName + "; " + plotTypeName + "; #";

  std::string newName;

  if ( isOverwriteEnabled() )
  {
    // If ovewrite is enabled, can use the same name again and again 
    newName = firstPart + "1";
  }
  else
  { 
    // If overwrite is disabled, need to find unique name for the new workspace
    int plotNum(1);
    do
    {
      newName = firstPart + boost::lexical_cast<std::string>(plotNum++);
    }
    while ( AnalysisDataService::Instance().doesExist(newName) );
  }

  return newName;
}

/**
 * Returns PlotType as chosen using given selector.
 * @param selector :: Widget to use for parsing
 * @return PlotType as selected using the widget
 */ 
MuonAnalysis::PlotType MuonAnalysis::parsePlotType(QComboBox* selector)
{
  std::string plotTypeName = selector->currentText().toStdString();

  if ( plotTypeName == "Asymmetry" )
  {
    return Asymmetry;
  }
  else if ( plotTypeName == "Counts" )
  {
    return Counts;
  }
  else if ( plotTypeName == "Logorithm" )
  {
    return Logorithm;
  }
  else
  {
    throw std::runtime_error("Unknown plot type name: " + plotTypeName);
  }
}

/**
 * Creates workspace ready for analysis and plotting. 
 * @param itemType :: Whether it's a group or pair
 * @param tableRow :: Row in the group/pair table which contains the item
 * @param plotType :: What kind of plot we want to analyse
 * @param isRaw    :: Whether binning should be applied to the workspace
 * @return Created workspace
 */ 
MatrixWorkspace_sptr MuonAnalysis::createAnalysisWorkspace(ItemType itemType, int tableRow, PlotType plotType,
  bool isRaw)
{
  IAlgorithm_sptr alg = AlgorithmManager::Instance().createUnmanaged("MuonCalculateAsymmetry");

  alg->initialize();

  auto loadedWS = AnalysisDataService::Instance().retrieveWS<Workspace>(m_grouped_name);

  if ( auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(loadedWS) )
  {
    // If is a group, will need to handle periods
    
    if ( MatrixWorkspace_sptr ws1 = getPeriodWorkspace(First, group) )
    {
      alg->setProperty( "FirstPeriodWorkspace", prepareAnalysisWorkspace(ws1, isRaw) );
    }
    else
    {
      // First period should be selected no matter what
      throw std::runtime_error("First period should be specified");
    }

    if ( MatrixWorkspace_sptr ws2 = getPeriodWorkspace(Second, group) )
    {
      // If second period was selected, set it up together with selected period arithmetics

      alg->setProperty("SecondPeriodWorkspace", prepareAnalysisWorkspace(ws2, isRaw) );
 
      // Parse selected operation
      const std::string op = m_uiForm.homePeriodBoxMath->currentText().toStdString();
      alg->setProperty("PeriodOperation", op);
    }
  }
  else if ( auto ws = boost::dynamic_pointer_cast<MatrixWorkspace>(loadedWS) )
  {
    alg->setProperty( "FirstPeriodWorkspace", prepareAnalysisWorkspace(ws, isRaw) );
  }
  else
  {
    throw std::runtime_error("Usupported workspace type");
  }

  if ( itemType == Group )
  {
    std::string outputType;

    switch(plotType)
    {
      case Counts:
      case Logorithm:
        outputType = "GroupCounts"; break;
      case Asymmetry:
        outputType = "GroupAsymmetry"; break;
      default:
        throw std::invalid_argument("Unsupported plot type");
    }

    alg->setProperty("OutputType", outputType);

    int groupNum = getGroupNumberFromRow(tableRow);
    alg->setProperty("GroupIndex", groupNum);
  }
  else if ( itemType == Pair )
  {
    if ( plotType == Asymmetry )
      alg->setProperty("OutputType", "PairAsymmetry");
    else
      throw std::invalid_argument("Pairs support asymmetry plot type only");

    QTableWidget* t = m_uiForm.pairTable;

    double alpha = t->item(tableRow,3)->text().toDouble();
    int index1 = static_cast<QComboBox*>( t->cellWidget(tableRow,1) )->currentIndex();
    int index2 = static_cast<QComboBox*>( t->cellWidget(tableRow,2) )->currentIndex();

    alg->setProperty("PairFirstIndex", index1);
    alg->setProperty("PairSecondIndex", index2);
    alg->setProperty("Alpha", alpha);
  }
  else
  {
    throw std::invalid_argument("Unsupported item type");
  }

  // We don't want workspace in the ADS so far
  alg->setChild(true);

  // Name is not used, as is child algorithm, so just to make validator happy
  alg->setPropertyValue("OutputWorkspace", "__IAmNinjaYouDontSeeMe");

  alg->execute();

  return alg->getProperty("OutputWorkspace");
}

/**
 * Crop/rebins/offsets the workspace according to interface settings.  
 * @param ws    :: Loaded data which to prepare
 * @param isRaw :: If true, Rebin is not applied
 * @return Prepared workspace
 */ 
MatrixWorkspace_sptr MuonAnalysis::prepareAnalysisWorkspace(MatrixWorkspace_sptr ws, bool isRaw)
{
  // Adjust for time zero if necessary
  if ( m_dataTimeZero != timeZero())
  {
      double shift = m_dataTimeZero - timeZero();

      Mantid::API::IAlgorithm_sptr alg = AlgorithmManager::Instance().createUnmanaged("ChangeBinOffset");
      alg->initialize();
      alg->setChild(true);
      alg->setProperty("InputWorkspace", ws);
      alg->setProperty("Offset", shift);
      alg->setPropertyValue("OutputWorkspace", "__IAmNinjaYouDontSeeMe"); // Is not used
      alg->execute();    

      ws = alg->getProperty("OutputWorkspace");
  }

  // Crop workspace
  Mantid::API::IAlgorithm_sptr cropAlg = AlgorithmManager::Instance().createUnmanaged("CropWorkspace");
  cropAlg->initialize();
  cropAlg->setChild(true);
  cropAlg->setProperty("InputWorkspace", ws);
  cropAlg->setProperty("Xmin", plotFromTime());
  if ( !m_uiForm.timeAxisFinishAtInput->text().isEmpty() )
    cropAlg->setProperty("Xmax", plotToTime());
  cropAlg->setPropertyValue("OutputWorkspace", "__IAmNinjaYouDontSeeMe"); // Is not used
  cropAlg->execute();

  ws = cropAlg->getProperty("OutputWorkspace");

  // Rebin data if option set in Plot Options and we don't want raw workspace
  if ( !isRaw && m_uiForm.rebinComboBox->currentIndex() != 0)
  {
    std::string rebinParams;
    double binSize = ws->dataX(0)[1] - ws->dataX(0)[0];

    if(m_uiForm.rebinComboBox->currentIndex() == 1) // Fixed
    {
      double bunchedBinSize = binSize * m_uiForm.optionStepSizeText->text().toDouble();
      rebinParams = boost::lexical_cast<std::string>(bunchedBinSize);
    }
    else // Variable
    {
      rebinParams = m_uiForm.binBoundaries->text().toStdString();
    }

    // Rebin data
    IAlgorithm_sptr rebinAlg = AlgorithmManager::Instance().createUnmanaged("Rebin");
    rebinAlg->initialize();
    rebinAlg->setChild(true);
    rebinAlg->setProperty("InputWorkspace", ws);
    rebinAlg->setProperty("Params", rebinParams);
    rebinAlg->setPropertyValue("OutputWorkspace", "__IAmNinjaYouDontSeeMe"); // Is not used
    rebinAlg->execute();

    ws = rebinAlg->getProperty("OutputWorkspace");

    // TODO: The following should be moved to Rebin as additional option

    // However muon group don't want last bin if shorter than previous bins
    binSize = ws->dataX(0)[1] - ws->dataX(0)[0]; 
    double firstX = ws->dataX(0)[0];
    double lastX = ws->dataX(0)[ws->dataX(0).size()-1];
    double numberOfFullBunchedBins =  std::floor((lastX - firstX) / binSize );

    if ( numberOfFullBunchedBins )
    {
      lastX = firstX + numberOfFullBunchedBins * binSize;

      IAlgorithm_sptr cropAlg = AlgorithmManager::Instance().createUnmanaged("CropWorkspace");
      cropAlg->initialize();
      cropAlg->setChild(true);
      cropAlg->setProperty("InputWorkspace", ws);
      cropAlg->setProperty("Xmax", lastX);
      cropAlg->setPropertyValue("OutputWorkspace", "__IAmNinjaYouDontSeeMe"); // Is not used
      cropAlg->execute();

      ws = cropAlg->getProperty("OutputWorkspace");
    }
  }

  return ws;
}


/**
 * Selects a workspace from the group according to what is selected on the interface for the period.
 * @param periodType :: Which period we want
 * @param group      :: Workspace group as loaded from the data file
 * @return Selected workspace
 */ 
MatrixWorkspace_sptr MuonAnalysis::getPeriodWorkspace(PeriodType periodType, WorkspaceGroup_sptr group)
{
  QComboBox* periodSelector;
 
  switch(periodType)
  {
    case First:
      periodSelector = m_uiForm.homePeriodBox1; break;
    case Second:
      periodSelector = m_uiForm.homePeriodBox2; break;
    default:
      throw std::invalid_argument("Unsupported period type");
  }

  const QString periodLabel = periodSelector->currentText();

  if ( periodLabel != "None" )
  {
    int periodNumber = periodLabel.toInt();
    size_t periodIndex = static_cast<size_t>(periodNumber - 1);

    if ( periodNumber < 1 || periodIndex >= group->size() )
      throw std::runtime_error("Loaded group doesn't seem to have period " + periodLabel.toStdString());

    return boost::dynamic_pointer_cast<MatrixWorkspace>( group->getItem(periodIndex) );
  }
  else
  {
    return MatrixWorkspace_sptr();
  }
}

/**
* If the instrument selection has changed (slot)
*
* @param prefix :: instrument name from QComboBox object
*/
void MuonAnalysis::userSelectInstrument(const QString& prefix)
{
  if ( prefix != m_curInterfaceSetup )
  {
    runClearGroupingButton();
    m_curInterfaceSetup = prefix;

    // save this new choice
    QSettings group;
    group.beginGroup(m_settingsGroup + "instrument");
    group.setValue("name", prefix);
  }
}


/**
 * Save grouping button (slot)
 */
void MuonAnalysis::runSaveGroupButton()
{
  if ( numGroups() <= 0 )
  {
    QMessageBox::warning(this, "MantidPlot - MuonAnalysis", "No grouping to save.");
    return;
  }

  QSettings prevValues;
  prevValues.beginGroup(m_settingsGroup + "SaveOutput");

  // Get value for "dir". If the setting doesn't exist then use
  // the the path in "defaultsave.directory"
  QString prevPath = prevValues.value("dir", QString::fromStdString(
    ConfigService::Instance().getString("defaultsave.directory"))).toString();

  QString filter;
  filter.append("Files (*.xml *.XML)");
  filter += ";;AllFiles (*.*)";
  QString groupingFile = MantidQt::API::FileDialogHandler::getSaveFileName(this,
                                   "Save Grouping file as", prevPath, filter);

  // Add extension if the groupingFile specified doesn't have one. (Solving Linux problem).
  if (!groupingFile.endsWith(".xml"))
    groupingFile += ".xml";

  if( ! groupingFile.isEmpty() )
  {
    Grouping groupingToSave;
    parseGroupingTable(m_uiForm, groupingToSave);
    saveGroupingToXML(groupingToSave, groupingFile.toStdString());
    
    QString directory = QFileInfo(groupingFile).path();
    prevValues.setValue("dir", directory);
  }
}


/**
 * Load grouping button (slot)
 */
void MuonAnalysis::runLoadGroupButton()
{
  m_updating = true;

  // Get grouping file
  QSettings prevValues;
  prevValues.beginGroup(m_settingsGroup + "LoadGroupFile");

  // Get value for "dir". If the setting doesn't exist then use
  // the the path in "defaultsave.directory"
  QString prevPath = prevValues.value("dir", QString::fromStdString(
    ConfigService::Instance().getString("defaultload.directory"))).toString();

  QString filter;
  filter.append("Files (*.xml *.XML)");
  filter += ";;AllFiles (*.*)";
  QString groupingFile = QFileDialog::getOpenFileName(this, "Load Grouping file", prevPath, filter);
  if( groupingFile.isEmpty() || QFileInfo(groupingFile).isDir() ) 
    return;
    
  QString directory = QFileInfo(groupingFile).path();
  prevValues.setValue("dir", directory);

  Grouping loadedGrouping;

  try
  {
    loadGroupingFromXML(groupingFile.toStdString(), loadedGrouping);
  }
  catch (Exception::FileError& e)
  {
    g_log.error("Unable to load grouping. Data left unchanged");
    g_log.error(e.what());
    m_updating = false;
    return;
  }

  clearTablesAndCombo();
  fillGroupingTable(loadedGrouping, m_uiForm);

  m_updating = false;

  if ( m_loaded )
  {
    try
    {
      groupLoadedWorkspace();
    }
    catch(std::exception& e)
    {
      g_log.error( e.what() );
      QMessageBox::critical(this, "MantidPlot - MuonAnalysis",
          "Unable to group the workspace. See log for details.");
    }
  }
}

/**
 * Clear grouping button (slot)
 */
void MuonAnalysis::runClearGroupingButton()
{
  clearTablesAndCombo();
}

/**
 * Group table plot button (slot)
 */
void MuonAnalysis::runGroupTablePlotButton()
{
  if(m_updating)
    return;

  if (m_deadTimesChanged)
  {
    inputFileChanged(m_previousFilenames);
    return;
  }

  if ( getGroupNumberFromRow(m_groupTableRowInFocus) != -1 )
  {
    PlotType plotType = parsePlotType(m_uiForm.groupTablePlotChoice);
    plotItem(Group, m_groupTableRowInFocus, plotType);
  }
}

/**
 * Load current (slot)
 */
void MuonAnalysis::runLoadCurrent()
{
  QString instname = m_uiForm.instrSelector->currentText().toUpper();

  // If Argus data then simple
  if ( instname == "ARGUS" )
  {
    QString argusDAE = "\\\\ndw828\\argusdata\\current cycle\\nexus\\argus0000000.nxs";
    Poco::File l_path( argusDAE.toStdString() );
    try
    {
      if ( !l_path.exists() )
      {
        QMessageBox::warning(this,"Mantid - MuonAnalysis",
          QString("Can't load ARGUS Current data since\n") +
          argusDAE + QString("\n") +
          QString("does not seem to exist"));
        return;
      }
    }
    catch(Poco::Exception&)
    {
       QMessageBox::warning(this, "MantidPlot - MuonAnalysis", "Can't read from the selected directory, either the computer you are trying"
         "\nto access is down or your computer is not currently connected to the network.");
       return;
    }
    m_uiForm.mwRunFiles->setUserInput(argusDAE);
    m_uiForm.mwRunFiles->setText("CURRENT RUN");
    return;
  }

  if ( instname == "EMU" || instname == "HIFI" || instname == "MUSR")
  {
    std::string autosavePointsTo = "";
    std::string autosaveFile = "\\\\" + instname.toStdString() + "\\data\\autosave.run";
    Poco::File pathAutosave( autosaveFile );
    
    try // check if exists
    {
      if ( pathAutosave.exists() )
      {
        std::ifstream autofileIn(autosaveFile.c_str(), std::ifstream::in);
        autofileIn >> autosavePointsTo;
      }
    }
    catch(Poco::Exception&)
    {
       QMessageBox::warning(this, "MantidPlot - MuonAnalysis", "Can't read from the selected directory, either the computer you are trying"
         "\nto access is down or your computer is not currently connected to the network.");
       return;
    }

    QString psudoDAE;
    if ( autosavePointsTo.empty() )
      psudoDAE = "\\\\" + instname + "\\data\\" + instname + "auto_A.tmp";
    else
      psudoDAE = "\\\\" + instname + "\\data\\" + autosavePointsTo.c_str();

    Poco::File l_path( psudoDAE.toStdString() );
    try
    {
      if ( !l_path.exists() )
      {
        QMessageBox::warning(this,"Mantid - MuonAnalysis",
          QString("Can't load ") + "Current data since\n" +
          psudoDAE + QString("\n") +
          QString("does not seem to exist"));
        return;
      }
    }
    catch(Poco::Exception&)
    {
      QMessageBox::warning(this,"Mantid - MuonAnalysis",
        QString("Can't load ") + "Current data since\n" +
        psudoDAE + QString("\n") +
        QString("does not seem to exist"));
      return;
    }
    m_uiForm.mwRunFiles->setUserInput(psudoDAE);
    m_uiForm.mwRunFiles->setText("CURRENT RUN");
    return;
  }

  QString daename = "NDX" + instname;

  // Load dae file
  AnalysisDataService::Instance().remove(m_workspace_name);

   //   "  " +  QString(m_workspace_name.c_str()) + "LoadDAE('" + daename + "')\n"

  QString pyString =
      "import sys\n"
      "try:\n"
      "  " +  QString(m_workspace_name.c_str()) + "LoadDAE('" + daename + "')\n"
      "except SystemExit, message:\n"
      "  print str(message)";
  QString pyOutput = runPythonCode( pyString ).trimmed();

  // if output is none empty something has gone wrong
  if ( !pyOutput.toStdString().empty() )
  {
    m_optionTab->noDataAvailable();
    QMessageBox::warning(this, "MantidPlot - MuonAnalysis", "Can't read from " + daename + ". Plotting disabled");
    return;
  }

  m_optionTab->nowDataAvailable();

  // Get hold of a pointer to a matrix workspace and apply grouping if applicatable
  Workspace_sptr workspace_ptr = AnalysisDataService::Instance().retrieve(m_workspace_name);
  WorkspaceGroup_sptr wsPeriods = boost::dynamic_pointer_cast<WorkspaceGroup>(workspace_ptr);
  MatrixWorkspace_sptr matrix_workspace;
  int numPeriods = 1;   // 1 may mean either a group with one period or simply just 1 normal matrix workspace
  if (wsPeriods)
  {
    numPeriods = wsPeriods->getNumberOfEntries();

    Workspace_sptr workspace_ptr1 = AnalysisDataService::Instance().retrieve(m_workspace_name + "_1");
    matrix_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(workspace_ptr1);
  }
  else
  {
    matrix_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(workspace_ptr);
  }

  if ( !isGroupingSet() )
    setDummyGrouping( matrix_workspace->getInstrument() );

  groupLoadedWorkspace();

  // Populate instrument fields
  std::stringstream str;
  str << "Description: ";
  int nDet = static_cast<int>(matrix_workspace->getInstrument()->getDetectorIDs().size());
  str << nDet;
  str << " detector spectrometer, main field ";
  str << "unknown"; 
  str << " to muon polarisation";
  m_uiForm.instrumentDescription->setText(str.str().c_str());

  // Populate run information text field
  std::string infoStr = "Number of spectra in data = ";
  infoStr += boost::lexical_cast<std::string>(matrix_workspace->getNumberHistograms()) + "\n";
  infoStr += "Title: ";
  infoStr += matrix_workspace->getTitle() + "\n" + "Comment: "
    + matrix_workspace->getComment();
  m_uiForm.infoBrowser->setText(infoStr.c_str());

  // If number of periods has changed -> update period widgets
  if(numPeriods != m_uiForm.homePeriodBox1->count())
    updatePeriodWidgets(numPeriods);
}

/**
 * Pair table plot button (slot)
 */
void MuonAnalysis::runPairTablePlotButton()
{
  if(m_updating)
    return;

  if (m_deadTimesChanged)
  {
    inputFileChanged(m_previousFilenames);
    return;
  }

  if ( getPairNumberFromRow(m_pairTableRowInFocus) != -1 )
  {
    // Sync with selectors on the front
    m_uiForm.frontGroupGroupPairComboBox->setCurrentIndex(numGroups() + m_pairTableRowInFocus);
    m_uiForm.frontPlotFuncs->setCurrentIndex(m_uiForm.pairTablePlotChoice->currentIndex());

    PlotType plotType = parsePlotType(m_uiForm.pairTablePlotChoice);
    plotItem(Pair, m_pairTableRowInFocus, plotType);
  }

}

/**
 * Pair table vertical lable clicked (slot)
 */
void MuonAnalysis::pairTableClicked(int row)
{
  m_pairTableRowInFocus = row;

  // if something sensible in row then update front
  int pNum = getPairNumberFromRow(row);
  if ( pNum >= 0 )
  {
    m_uiForm.frontGroupGroupPairComboBox->setCurrentIndex(pNum+numGroups());
  }
}

/**
 * Pair table clicked (slot)
 */
void MuonAnalysis::pairTableClicked(int row, int column)
{
  (void) column;

  pairTableClicked(row);
}

/**
 * Group table clicked (slot)
 */
void MuonAnalysis::groupTableClicked(int row, int column)
{
  (void) column;

  groupTableClicked(row);
}

/**
* Group table clicked (slot)
*/
void MuonAnalysis::groupTableClicked(int row)
{
  m_groupTableRowInFocus = row;

  // if something sensible in row then update front
  int gNum = getGroupNumberFromRow(row);
  if ( gNum >= 0 )
  {
    m_uiForm.frontGroupGroupPairComboBox->setCurrentIndex(gNum);
    m_uiForm.frontPlotFuncs->setCurrentIndex(m_uiForm.groupTablePlotChoice->currentIndex());
  }
}


/**
* Group table changed, e.g. if:         (slot)
*
*    1) user changed detector sequence
*    2) user type in a group name
*
* @param row :: row number
* @param column :: column number
*/
void MuonAnalysis::groupTableChanged(int row, int column)
{
  // changes to the IDs
  if ( column == 1 )
  {
    QTableWidgetItem* itemNdet = m_uiForm.groupTable->item(row,2);
    QTableWidgetItem *item = m_uiForm.groupTable->item(row,1);

    // if IDs list has been changed to empty string
    if (item->text() == "")
    {
      if (itemNdet)
        itemNdet->setText("");
    }
    else
    {
      int numDet = numOfDetectors(item->text().toStdString());
      std::stringstream detNumRead;
      if (numDet > 0 )
      {
        detNumRead << numDet;
        if (itemNdet == NULL)
          m_uiForm.groupTable->setItem(row,2, new QTableWidgetItem(detNumRead.str().c_str()));
        else
        {
          itemNdet->setText(detNumRead.str().c_str());
        }
      }
      else
      {
        if (itemNdet == NULL)
          m_uiForm.groupTable->setItem(row,2, new QTableWidgetItem("Invalid IDs string"));
        else
          m_uiForm.groupTable->item(row, 2)->setText("Invalid IDs string");
      }
    }
  }

  // Change to group name
  if ( column == 0 )
  {
    QTableWidgetItem *itemName = m_uiForm.groupTable->item(row,0);

    if ( itemName == NULL )  // this should never happen
      m_uiForm.groupTable->setItem(row,0, new QTableWidgetItem(""));
      
    if ( itemName->text() != "" )
    {
      // check that the group name entered does not already exist
      for (int i = 0; i < m_uiForm.groupTable->rowCount(); i++)
      {
        if (i==row)
          continue;

        QTableWidgetItem *item = m_uiForm.groupTable->item(i,0);
        if (item)
        {
          if ( item->text() == itemName->text() )
          {
            QMessageBox::warning(this, "MantidPlot - MuonAnalysis", "Group names must be unique. Please re-enter Group name.");
            itemName->setText("");
            break;
          }
        }
      }
    }
  }
  whichGroupToWhichRow(m_uiForm, m_groupToRow);
  updatePairTable();
  updateFrontAndCombo();
  
  if ( m_loaded && ! m_updating )
  {
    try
    {
      groupLoadedWorkspace();
    }
    catch(std::exception& e)
    {
      g_log.error( e.what() );

      QMessageBox::critical(this, "MantidPlot - MuonAnalysis", 
          "Unable to group the workspace. See log for details");
    }
  }
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
void MuonAnalysis::pairTableChanged(int row, int column)
{
  // alpha been modified
  if ( column == 3 )
  {
    QTableWidgetItem* itemAlpha = m_uiForm.pairTable->item(row,3);

    if ( !itemAlpha->text().toStdString().empty() )
    {
      try
      {
         boost::lexical_cast<double>(itemAlpha->text().toStdString().c_str());
      }  catch (boost::bad_lexical_cast&)
      {
        QMessageBox::warning(this, "MantidPlot - MuonAnalysis", "Alpha must be a number.");
        itemAlpha->setText("");
        return;
      }
    }
    whichPairToWhichRow(m_uiForm, m_pairToRow);
    updateFrontAndCombo();
  }

  // pair name been modified
  if ( column == 0 )
  {
    QTableWidgetItem *itemName = m_uiForm.pairTable->item(row,0);

    if ( itemName == NULL )  // this should never happen
      m_uiForm.pairTable->setItem(row,0, new QTableWidgetItem(""));
      
    if ( itemName->text() != "" )
    {
      // check that the group name entered does not already exist
      for (int i = 0; i < m_uiForm.pairTable->rowCount(); i++)
      {
        if (i==row)
          continue;

        QTableWidgetItem *item = m_uiForm.pairTable->item(i,0);
        if (item)
        {
          if ( item->text() == itemName->text() )
          {
            QMessageBox::warning(this, "MantidPlot - MuonAnalysis", "Pair names must be unique. Please re-enter Pair name.");
            itemName->setText("");
          }
        }
      }
    }

    whichPairToWhichRow(m_uiForm, m_pairToRow);
    updateFrontAndCombo();

    // check to see if alpha is specified (if name!="") and if not
    // assign a default of 1.0
    if ( itemName->text() != "" )
    {
      QTableWidgetItem* itemAlpha = m_uiForm.pairTable->item(row,3);

      if (itemAlpha)
      {
        if ( itemAlpha->text().toStdString().empty() )
        {
          itemAlpha->setText("1.0");
        }
      }
      else
      {
        m_uiForm.pairTable->setItem(row,3, new QTableWidgetItem("1.0"));
      }
    }
  }
}

/**
 * Update pair table
 */
void MuonAnalysis::updatePairTable()
{
  // number of groups has dropped below 2 and pair names specified then
  // clear pair table
  if ( numGroups() < 2 && numPairs() > 0 )
  {
    m_uiForm.pairTable->clearContents();
    for (int i = 0; i < m_uiForm.pairTable->rowCount(); i++)
    {
      m_uiForm.pairTable->setCellWidget(i,1, new QComboBox);
      m_uiForm.pairTable->setCellWidget(i,2, new QComboBox);
    }
    updateFrontAndCombo();
    return;
  }
  else if ( numGroups() < 2 && numPairs() <= 0 )
  {
    return;
  }

  // get previous number of groups as listed in the pair comboboxes
  QComboBox* qwF = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(0,1));
  int previousNumGroups = qwF->count(); // how many groups listed in pair combobox
  int newNumGroups = numGroups();

  // reset context of combo boxes
  for (int i = 0; i < m_uiForm.pairTable->rowCount(); i++)
  {
    QComboBox* qwF = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(i,1));
    QComboBox* qwB = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(i,2));

    if (previousNumGroups < newNumGroups)
    {
      // then need to increase the number of entrees in combo box
      for (int ii = 1; ii <= newNumGroups-previousNumGroups; ii++)
      {
        qwF->addItem(""); // effectively here just allocate space for extra items
        qwB->addItem("");
      }
    }
    else if (previousNumGroups > newNumGroups)
    {
      // then need to decrease the number of entrees in combo box
      for (int ii = 1; ii <= previousNumGroups-newNumGroups; ii++)
      {
        qwF->removeItem(qwF->count()-1); // remove top items
        qwB->removeItem(qwB->count()-1);
      }

      // further for this case check that none of the current combo box
      // indexes are larger than the number of groups
      if ( qwF->currentIndex()+1 > newNumGroups || qwB->currentIndex()+1 > newNumGroups )
      {
        qwF->setCurrentIndex(0);
        qwB->setCurrentIndex(1);
      }
    }

    if ( qwF->currentIndex() == 0 && qwB->currentIndex() == 0 )
      qwB->setCurrentIndex(1);

    // re-populate names in combo boxes with group names
    for (int ii = 0; ii < newNumGroups; ii++)
    {
      qwF->setItemText(ii, m_uiForm.groupTable->item(m_groupToRow[ii],0)->text());
      qwB->setItemText(ii, m_uiForm.groupTable->item(m_groupToRow[ii],0)->text());
    }
  }
}

/**
 * Slot called when the input file is changed.
 */
void MuonAnalysis::inputFileChanged_MWRunFiles()
{
  // Handle changed input, then turn buttons back on.
  handleInputFileChanges();
  allowLoading(true);
}

/**
 * Do some check when reading from MWRun, before actually reading new data file, to see if file is valid
 */
void MuonAnalysis::handleInputFileChanges()
{ 

  if ( m_uiForm.mwRunFiles->getText().isEmpty() )
    return;

  if ( !m_uiForm.mwRunFiles->isValid() )
  { 
    QMessageBox::warning(this,"Mantid - MuonAnalysis", m_uiForm.mwRunFiles->getFileProblem() );
    if (m_textToDisplay == "")
      m_uiForm.mwRunFiles->setFileProblem("Error. No File specified.");
    else
      m_uiForm.mwRunFiles->setFileProblem("Error finding file. Reset to last working data.");
    m_uiForm.mwRunFiles->setText(m_textToDisplay);
    return;
  }

  if (!m_updating)
  {
    QStringList runFiles = m_uiForm.mwRunFiles->getFilenames();
  
    m_previousFilenames.clear();
    m_previousFilenames = runFiles;
    m_textToDisplay =  m_uiForm.mwRunFiles->getText();

    // save selected browse file directory to be reused next time interface is started up
    m_uiForm.mwRunFiles->saveSettings(m_settingsGroup + "mwRunFilesBrowse");

    inputFileChanged(m_previousFilenames);
  }
}

/**
 * Input file changed. Update GUI accordingly. Note this method does no check of input filename assumed
 * done elsewhere depending on e.g. whether filename came from MWRunFiles or 'get current run' button.
 *
 * @param files :: All file names for the files loading.
 */
void MuonAnalysis::inputFileChanged(const QStringList& files)
{
  if (files.size() <= 0)
    return;

  m_updating = true;
  m_uiForm.tabWidget->setTabEnabled(3, false);

  try
  {
    // Whether the instrument in the file is different from the one used
    bool instrumentChanged = false;

    std::string mainFieldDirection("");
    double timeZero(0.0);
    double firstGoodData(0.0);

    ScopedWorkspace loadedDeadTimes;
    ScopedWorkspace loadedDetGrouping;

    for (int i=0; i<files.size(); ++i)
    {
      QString filename = files[i];
      Poco::File l_path( filename.toStdString() );

      // and check if file is from a recognised instrument and update instrument combo box
      QString filenamePart = (Poco::Path(l_path.path()).getFileName()).c_str();
      filenamePart = filenamePart.toLower();
      bool foundInst = false;
      for (int j=0; j < m_uiForm.instrSelector->count(); j++)
      {
        QString instName = m_uiForm.instrSelector->itemText(j).toLower();
      
        std::string sfilename = filenamePart.toStdString();
        std::string sinstName = instName.toStdString();
        size_t found;
        found = sfilename.find(sinstName);
        if ( found != std::string::npos )
        {
          foundInst = true;

          // If currently used instrument has changed
          if(j != m_uiForm.instrSelector->currentIndex())
          {
            m_uiForm.instrSelector->setCurrentIndex(j);
            instrumentChanged = true;
          }
          
          break;
        }
      }
      if ( !foundInst )
        throw std::runtime_error("Muon file " + filename.toStdString() + " not recognised.");

      // Setup Load Nexus Algorithm
      Mantid::API::IAlgorithm_sptr loadMuonAlg = AlgorithmManager::Instance().createUnmanaged("LoadMuonNexus");
      loadMuonAlg->initialize();
      loadMuonAlg->setLogging(false);
      loadMuonAlg->setPropertyValue("Filename", filename.toStdString() );
      loadMuonAlg->setProperty("AutoGroup", false);

      if ( i == 0 )
      {
        // Get dead times/grouping from first file only
        loadMuonAlg->setPropertyValue( "DeadTimeTable", loadedDeadTimes.name() );
        loadMuonAlg->setPropertyValue( "DetectorGroupingTable", loadedDetGrouping.name() );

        loadMuonAlg->setPropertyValue("OutputWorkspace", m_workspace_name);
      }
      else
      {
        QString tempRangeNum;
        tempRangeNum.setNum(i);
        loadMuonAlg->setPropertyValue("OutputWorkspace", m_workspace_name + tempRangeNum.toStdString() );
      }

      if (loadMuonAlg->execute() )
      {
        
        timeZero = loadMuonAlg->getProperty("TimeZero");
        firstGoodData = loadMuonAlg->getProperty("FirstGoodData");


        if (m_uiForm.instrSelector->currentText().toUpper() == "ARGUS")
        {
          // ARGUS doesn't support dead time correction, so leave deadTimes empty.

          // Some of the ARGUS data files contain wrong information about the instrument main field
          // direction. It is alway longitudinal.
          mainFieldDirection = "longitudinal";
        }
        else
        {
          mainFieldDirection = loadMuonAlg->getPropertyValue("MainFieldDirection");
        }
      }
      else
      {
        throw std::runtime_error("Problem when executing LoadMuonNexus algorithm.");
      }
    }

    if (m_previousFilenames.size() > 1)
      plusRangeWorkspaces();

    if (m_uiForm.deadTimeType->currentIndex() != 0)
    {
      try // ... to apply dead time correction
      {
        // ARGUS does not support dead time corr.
        if (m_uiForm.instrSelector->currentText().toUpper() == "ARGUS") 
            throw std::runtime_error("Dead times are currently not implemented in ARGUS files.");

        ScopedWorkspace deadTimes;

        if (m_uiForm.deadTimeType->currentIndex() == 1) // From Run Data
        {
          if( ! loadedDeadTimes )
            throw std::runtime_error("Data file doesn't appear to contain dead time values");

          Workspace_sptr ws = loadedDeadTimes.retrieve();
          loadedDeadTimes.remove();

          deadTimes.set(ws);
        }
        else if (m_uiForm.deadTimeType->currentIndex() == 2) // From Specified File
        {
          Workspace_sptr ws = loadDeadTimes( deadTimeFilename() );
          deadTimes.set(ws);
        }

        IAlgorithm_sptr applyCorrAlg = AlgorithmManager::Instance().create("ApplyDeadTimeCorr");
        applyCorrAlg->setRethrows(true);
        applyCorrAlg->setLogging(false);
        applyCorrAlg->setPropertyValue("InputWorkspace", m_workspace_name); 
        applyCorrAlg->setPropertyValue("OutputWorkspace", m_workspace_name);
        applyCorrAlg->setPropertyValue("DeadTimeTable", deadTimes.name());
        applyCorrAlg->execute();
      }
      catch(std::exception& e)
      {
        QString errorMsg(e.what());
        errorMsg += "\n\nNo Dead Time correction applied.\n\nReset to None.";

        // Set DTC type to None
        m_uiForm.deadTimeType->setCurrentIndex(0);

        QMessageBox::warning(this, "Mantid - MuonAnalysis", errorMsg);
      }
    }

    // Get hold of a pointer to a matrix workspace
    MatrixWorkspace_sptr matrix_workspace;
    int numPeriods;

    Workspace_sptr loadedWS = AnalysisDataService::Instance().retrieve(m_workspace_name);

    if ( auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(loadedWS) )
    {
      numPeriods = static_cast<int>( group->size() );
      matrix_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>( group->getItem(0) );
    }
    else 
    {
      numPeriods = 1;
      matrix_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(loadedWS);
    }

    if ( isGroupingSet() )
    {
      // If grouping set already - it means it wasn't reset and we can use it
      g_log.information("Using custom grouping");
      groupLoadedWorkspace();
    }
    else
    {
      setGroupingFromIDF( matrix_workspace->getInstrument(), mainFieldDirection );

      if ( isGroupingSet() )
      {
        g_log.information("Using grouping loaded from IDF");
        groupLoadedWorkspace();
      }
      else if ( loadedDetGrouping )
      {
        g_log.information("Using grouping loaded from Nexus file");

        Workspace_sptr groupingWS = loadedDetGrouping.retrieve();
        loadedDetGrouping.remove(); // Don't need it in the ADS any more

        ITableWorkspace_sptr groupingTable;

        if ( auto table = boost::dynamic_pointer_cast<ITableWorkspace>(groupingWS) )
        {
          groupingTable = table;
        }
        else if ( auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(groupingWS) )
        {
          g_log.information("Multi-period grouping loaded from the Nexus file. Using the first one.");
          groupingTable = boost::dynamic_pointer_cast<ITableWorkspace>( group->getItem(0) );
        }

        setGrouping(groupingTable);
        groupLoadedWorkspace(groupingTable);
      }
      else 
      {
        g_log.information("Using dummy grouping");
        setDummyGrouping( matrix_workspace->getInstrument() );
        groupLoadedWorkspace();
      }
    }

    // Make the options available
    m_optionTab->nowDataAvailable();

    // Populate instrument fields
    std::stringstream str;
    str << "Description: ";
    int nDet = static_cast<int>(matrix_workspace->getInstrument()->getDetectorIDs().size());
    str << nDet;
    str << " detector spectrometer, main field ";
    str << QString(mainFieldDirection.c_str()).toLower().toStdString();
    str << " to muon polarisation";
    m_uiForm.instrumentDescription->setText(str.str().c_str());

    // Save loaded values
    m_dataTimeZero = timeZero;
    m_dataFirstGoodData = firstGoodData - timeZero;

    if(instrumentChanged)
    {
      // When instrument changes we use information from data no matter what user has chosen before
      m_uiForm.timeZeroAuto->setCheckState(Qt::Checked);
      m_uiForm.firstGoodDataAuto->setCheckState(Qt::Checked);
    }

    // Update boxes, as values have been changed
    setTimeZeroState();
    setFirstGoodDataState();

    std::ostringstream infoStr;

    // Set display style for floating point values
    infoStr << std::fixed << std::setprecision(12);
    
    // Populate run information with the run number
    QString run(getGroupName());
    if (m_previousFilenames.size() > 1)
      infoStr << "Runs: ";
    else
      infoStr << "Run: ";

    // Remove instrument and leading zeros
    int zeroCount(0);
    for (int i=0; i<run.size(); ++i)
    {
      if ( (run[i] == '0') || (run[i].isLetter() ) )
        ++zeroCount;
      else
      {
        run = run.right(run.size() - zeroCount);
        break;
      }
    }

    // Add to run information.
    infoStr << run.toStdString();

    // Populate run information text field
    m_title = matrix_workspace->getTitle();
    infoStr << "\nTitle: ";
    infoStr << m_title;
    
    // Add the comment to run information
    infoStr << "\nComment: ";
    infoStr << matrix_workspace->getComment();
    
    const Run& runDetails = matrix_workspace->run();

    Mantid::Kernel::DateAndTime start, end;

    // Add the start time for the run
    infoStr << "\nStart: ";
    if ( runDetails.hasProperty("run_start") )
    {
      start = runDetails.getProperty("run_start")->value();
      infoStr << start.toSimpleString();
    }

    // Add the end time for the run
    infoStr << "\nEnd: ";
    if ( runDetails.hasProperty("run_end") )
    {
      end = runDetails.getProperty("run_end")->value();
      infoStr << end.toSimpleString();
    }

    // Add counts to run information
    infoStr << "\nCounts: ";
    double counts(0.0);
    for (size_t i=0; i<matrix_workspace->getNumberHistograms(); ++i)
    {
      for (size_t j=0; j<matrix_workspace->blocksize(); ++j)
      {
        counts += matrix_workspace->dataY(i)[j];
      }
    }
    infoStr << counts/1000000 << " MEv";

    // Add average temperature.
    infoStr << "\nAverage Temperature: ";
    if ( runDetails.hasProperty("Temp_Sample") )
    {
      // Filter the temperatures by the start and end times for the run.
      runDetails.getProperty("Temp_Sample")->filterByTime(start, end);

      // Get average of the values
      double average = runDetails.getPropertyAsSingleValue("Temp_Sample");

      if (average != 0.0)
      {
        infoStr << average;
      }
      else
      {
        infoStr << "Not set";
      }
    }
    else
    {
      infoStr << "Not found";
    }

    // Add sample temperature
    infoStr << "\nSample Temperature: ";
    if ( runDetails.hasProperty("sample_temp") )
    {
      auto temp = runDetails.getPropertyValueAsType<double>("sample_temp");
      infoStr << temp;
    }
    else
    {
      infoStr << "Not found";
    }

    // Add sample magnetic field
    infoStr << "\nSample Magnetic Field: ";
    if ( runDetails.hasProperty("sample_magn_field") )
    {
      auto temp = runDetails.getPropertyValueAsType<double>("sample_magn_field");
      infoStr << temp;
    }
    else
    {
      infoStr << "Not found";
    }

    // Include all the run information.
    m_uiForm.infoBrowser->setText( QString::fromStdString(infoStr.str()) );

    // If instrument or number of periods has changed -> update period widgets
    if(instrumentChanged || numPeriods != m_uiForm.homePeriodBox1->count())
      updatePeriodWidgets(numPeriods);

    // Populate bin width info in Plot options
    double binWidth = matrix_workspace->dataX(0)[1]-matrix_workspace->dataX(0)[0];
    static const QChar MU_SYM(956);
    m_uiForm.optionLabelBinWidth->setText(QString("Data collected with histogram bins of ") + QString::number(binWidth) + QString(" %1s").arg(MU_SYM));

    m_uiForm.tabWidget->setTabEnabled(3, true);

    m_updating = false;
    m_deadTimesChanged = false;

    m_loaded = true;

    // Create a group for new data, if it doesn't exist
    const std::string groupName = getGroupName().toStdString();
    if ( ! AnalysisDataService::Instance().doesExist(groupName) )
    {
      AnalysisDataService::Instance().add( groupName, boost::make_shared<WorkspaceGroup>() );
    }

    if(m_uiForm.frontPlotButton->isEnabled())
      plotSelectedItem();
  }
  catch(std::exception& e)
  {
    deleteRangedWorkspaces();

    QMessageBox::warning(this,"Mantid - MuonAnalysis", e.what());
  }

  m_updating = false;
  m_uiForm.tabWidget->setTabEnabled(3, true);
}


/**
* Uses the algorithm plus to add all the workspaces from a range.
*/
void MuonAnalysis::plusRangeWorkspaces()
{
  // Start at 1 because 0 is MuonAnalysis without a number
  for (int i=1; i<m_previousFilenames.size(); ++i)
  {
    QString tempNum;
    tempNum.setNum(i);

    Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("Plus");
    alg->setPropertyValue("LHSWorkspace", m_workspace_name);
    alg->setPropertyValue("RHSWorkspace", m_workspace_name + tempNum.toStdString());
    alg->setPropertyValue("OutputWorkspace", m_workspace_name);
    if (!alg->execute())
      throw std::runtime_error("Error in adding range together.");
  }
  deleteRangedWorkspaces();
}


/**
* Delete ranged workspaces.
*/
void MuonAnalysis::deleteRangedWorkspaces()
{
  // Start at 1 because 0 is MuonAnalysis without a number
  for (int i=1; i<m_previousFilenames.size(); ++i)
  {
    QString tempNum;
    tempNum.setNum(i);
    if (Mantid::API::AnalysisDataService::Instance().doesExist(m_workspace_name + tempNum.toStdString() ) )
      Mantid::API::AnalysisDataService::Instance().remove(m_workspace_name + tempNum.toStdString() );
    if (Mantid::API::AnalysisDataService::Instance().doesExist(m_workspace_name + tempNum.toStdString() + "_1") )
      Mantid::API::AnalysisDataService::Instance().remove(m_workspace_name + tempNum.toStdString() + "_1");
    if (Mantid::API::AnalysisDataService::Instance().doesExist(m_workspace_name + tempNum.toStdString() + "_2") )
      Mantid::API::AnalysisDataService::Instance().remove(m_workspace_name + tempNum.toStdString() + "_2");
  }
}

/**
* Get the group name for the workspace.
*
* @return wsGroupName :: The name of the group workspace.
*/
QString MuonAnalysis::getGroupName()
{
  std::string workspaceGroupName("");

  // Decide on name for workspaceGroup
  if (m_previousFilenames.size() == 1)
  {
    Poco::File l_path( m_previousFilenames[0].toStdString() );
    workspaceGroupName = Poco::Path(l_path.path()).getFileName();
    changeCurrentRun(workspaceGroupName);
  }
  else
  {
    workspaceGroupName = getRangedName();
  }

  std::size_t extPos = workspaceGroupName.find(".");
  if ( extPos!=std::string::npos)
    workspaceGroupName = workspaceGroupName.substr(0,extPos);

  QString wsGroupName(workspaceGroupName.c_str());
  wsGroupName = wsGroupName.toUpper();
  return wsGroupName;
}

/**
* Get ranged name.
*
* @return rangedName :: The name to be used to identify the workspace.
*/
std::string MuonAnalysis::getRangedName()
{
  QString filePath("");
  QString firstFile(m_previousFilenames[0]);
  QString lastFile(m_previousFilenames[m_previousFilenames.size()-1]);

  QString firstRun("");
  QString lastRun("");
  int runSize(-1);
  
  separateMuonFile(filePath, firstFile, firstRun, runSize);

  separateMuonFile(filePath, lastFile, lastRun, runSize);
  
  for (int i=0; i<lastRun.size(); ++i)
  {
    if (firstRun[i] != lastRun[i])
    {
      lastRun = lastRun.right(lastRun.size() - i);
      break;
    }
  }

  if (firstFile.contains('.') )
    firstFile.chop(firstFile.size()-firstFile.indexOf('.') );

  return (firstFile.toStdString() + '-' + lastRun.toStdString());
}


/**
 * Guess Alpha (slot). For now include all data from first good data(bin)
 */
void MuonAnalysis::guessAlphaClicked()
{
  m_updating = true;

  if ( getPairNumberFromRow(m_pairTableRowInFocus) >= 0 )
  {
    QComboBox* qwF = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(m_pairTableRowInFocus,1));
    QComboBox* qwB = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(m_pairTableRowInFocus,2));

    if (!qwF || !qwB)
      return;

    // group IDs
    QTableWidgetItem *idsF = m_uiForm.groupTable->item(m_groupToRow[qwF->currentIndex()],1);
    QTableWidgetItem *idsB = m_uiForm.groupTable->item(m_groupToRow[qwB->currentIndex()],1);

    if (!idsF || !idsB)
      return;

    QString inputWS = m_workspace_name.c_str();
    if ( m_uiForm.homePeriodBox2->isEnabled() )
      inputWS += "_" + m_uiForm.homePeriodBox1->currentText();

    Mantid::API::IAlgorithm_sptr alphaAlg = Mantid::API::AlgorithmManager::Instance().create("AlphaCalc");
    alphaAlg->setPropertyValue("InputWorkspace", inputWS.toStdString());
    alphaAlg->setPropertyValue("ForwardSpectra", idsF->text().toStdString());
    alphaAlg->setPropertyValue("BackwardSpectra", idsB->text().toStdString());
    alphaAlg->setPropertyValue("FirstGoodValue", firstGoodBin().toStdString());
    alphaAlg->execute();  

    const QString alpha(alphaAlg->getPropertyValue("Alpha").c_str());

    QComboBox* qwAlpha = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(m_pairTableRowInFocus,3));
    if (qwAlpha)
      m_uiForm.pairTable->item(m_pairTableRowInFocus,3)->setText(alpha);
    else
      m_uiForm.pairTable->setItem(m_pairTableRowInFocus,3, new QTableWidgetItem(alpha));
  }

  m_updating = false;

  // See if auto-update is on and if so update the plot
  groupTabUpdatePlot();
}

/**
 * Return number of groups defined (not including pairs)
 *
 * @return number of groups
 */
int MuonAnalysis::numGroups()
{
  whichGroupToWhichRow(m_uiForm, m_groupToRow);
  return static_cast<int>(m_groupToRow.size());
}

/**
 * Return number of pairs
 *
 * @return number of pairs
 */
int MuonAnalysis::numPairs()
{
  whichPairToWhichRow(m_uiForm, m_pairToRow);
  return static_cast<int>(m_pairToRow.size());
}

/**
 * Update front "group / group-pair" combo-box based on what the currentIndex now is
 */
void MuonAnalysis::updateFront()
{
  // get current index
  int index = m_uiForm.frontGroupGroupPairComboBox->currentIndex();

  m_uiForm.frontPlotFuncs->clear();

  int numG = numGroups();

  if (index >= 0 && numG)
  {
    if (index >= numG && numG >= 2)
    {
      // i.e. index points to a pair
      m_uiForm.frontPlotFuncs->addItems(m_pairPlotFunc);

      m_uiForm.frontAlphaLabel->setVisible(true);
      m_uiForm.frontAlphaNumber->setVisible(true);

      m_uiForm.frontAlphaNumber->setText(m_uiForm.pairTable->item(m_pairToRow[index-numG],3)->text());

      m_uiForm.frontAlphaNumber->setCursorPosition(0);
    }
    else
    {
      // i.e. index points to a group
      m_uiForm.frontPlotFuncs->addItems(m_groupPlotFunc);

      m_uiForm.frontAlphaLabel->setVisible(false);
      m_uiForm.frontAlphaNumber->setVisible(false);
    }
  }
}


/**
 * Update front including first re-populate pair list combo box
 */
void MuonAnalysis::updateFrontAndCombo()
{
  // for now brute force clearing and adding new context
  // could go for softer approach and check if is necessary
  // to complete reset this combo box
  int currentI = m_uiForm.frontGroupGroupPairComboBox->currentIndex();
  if (currentI < 0)  // in case this combobox has not been set yet
    currentI = 0;
  m_uiForm.frontGroupGroupPairComboBox->clear();

  int numG = numGroups();
  int numP = numPairs();
  for (int i = 0; i < numG; i++)
    m_uiForm.frontGroupGroupPairComboBox->addItem(
      m_uiForm.groupTable->item(m_groupToRow[i],0)->text());
  for (int i = 0; i < numP; i++)
    m_uiForm.frontGroupGroupPairComboBox->addItem(
      m_uiForm.pairTable->item(m_pairToRow[i],0)->text());
  
  if ( currentI >= m_uiForm.frontGroupGroupPairComboBox->count() )
    m_uiForm.frontGroupGroupPairComboBox->setCurrentIndex(0);
  else
    m_uiForm.frontGroupGroupPairComboBox->setCurrentIndex(currentI);
}

/**
 * Updates widgets related to period algebra.
 * @param numPeriods Number of periods available
 */
void MuonAnalysis::updatePeriodWidgets(int numPeriods)
{
  QString periodLabel = "Data collected in " + QString::number(numPeriods)
                        + " periods. Plot/analyse period: ";
  m_uiForm.homePeriodsLabel->setText(periodLabel);

  // Remove all the previous items
  m_uiForm.homePeriodBox1->clear();
  m_uiForm.homePeriodBox2->clear();

  m_uiForm.homePeriodBox2->addItem("None");

  for ( int i = 1; i <= numPeriods; i++ )
  {
    m_uiForm.homePeriodBox1->addItem(QString::number(i));
    m_uiForm.homePeriodBox2->addItem(QString::number(i));
  }

  // We only need period widgets enabled if we have more than 1 period
  if(numPeriods > 1)
  {
    m_uiForm.homePeriodBox2->setEnabled(true);
    m_uiForm.homePeriodBoxMath->setEnabled(true);
  }
  else
  {
    m_uiForm.homePeriodBox2->setEnabled(false);
    m_uiForm.homePeriodBoxMath->setEnabled(false);
  }  
}


/**
 * Return the group-number for the group in a row. Return -1 if
 * invalid group in row
 *
 * @param row :: A row in the group table
 * @return Group number
 */
int MuonAnalysis::getGroupNumberFromRow(int row)
{
  whichGroupToWhichRow(m_uiForm, m_groupToRow);
  for (unsigned int i = 0; i < m_groupToRow.size(); i++)
  {
    if ( m_groupToRow[i] == row )
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
int MuonAnalysis::getPairNumberFromRow(int row)
{
  whichPairToWhichRow(m_uiForm, m_pairToRow);
  for (unsigned int i = 0; i < m_pairToRow.size(); i++)
  {
    if ( m_pairToRow[i] == row )
      return i;
  }
  return -1;
}


/**
 * Return the pair which is in focus and -1 if none
 */
int MuonAnalysis::pairInFocus()
{
  // plus some code here which double checks that pair
  // table in focus actually sensible
  return m_pairTableRowInFocus;
}


/**
 * Clear tables and front combo box
 */
void MuonAnalysis::clearTablesAndCombo()
{
  m_uiForm.groupTable->clearContents();
  m_uiForm.frontGroupGroupPairComboBox->clear();
  m_uiForm.frontPlotFuncs->clear();

  m_uiForm.pairTable->clearContents();
  for (int i = 0; i < m_uiForm.pairTable->rowCount(); i++)
  {
    m_uiForm.pairTable->setCellWidget(i,1, new QComboBox);
    m_uiForm.pairTable->setCellWidget(i,2, new QComboBox);
  }

  m_uiForm.groupDescription->clear();
}

/**
 * Get period labels for the periods selected in the GUI
 * @return Return empty string if no periods (well just one period). If more 
 *         one period then return "_#" string for the periods selected by user
 */
QStringList MuonAnalysis::getPeriodLabels() const
{
  QStringList retVal;
  if ( m_uiForm.homePeriodBox2->isEnabled() && m_uiForm.homePeriodBox2->currentText()!="None" )
  {
    retVal.append( "_" + m_uiForm.homePeriodBox1->currentText());
    retVal.append( "_" + m_uiForm.homePeriodBox2->currentText());
  }
  else if ( m_uiForm.homePeriodBox2->isEnabled() )
  {
    retVal.append( "_" + m_uiForm.homePeriodBox1->currentText());
  }
  else
    retVal.append("");

  return retVal;
}

/**
 * plots specific WS spectrum (used by plotPair and plotGroup)
 * @param wsName   :: Workspace name
 * @param logScale :: Whether to plot using logarithmic scale
 */
void MuonAnalysis::plotSpectrum(const QString& wsName, bool logScale)
{
    // Get plotting params
    const QMap<QString, QString>& params = getPlotStyleParams(wsName);

    QString pyS;

    // Try to find existing graph window
    pyS = "w = graph('%1-1')\n";

    // If doesn't exist - plot it
    pyS += "if w == None:\n"
           "  w = plotSpectrum('%1', 0, %2, %3)\n"
           "  w.setObjectName('%1')\n";

    // If plot does exist already, it should've just been updated automatically, so we just
    // need to make sure it is visible
    pyS += "else:\n"
          "  plotSpectrum('%1', 0, %2, %3, window = w, clearWindow = True)\n"
          "  w.show()\n"
          "  w.setFocus()\n";

    pyS = pyS.arg(wsName).arg(params["ShowErrors"]).arg(params["ConnectType"]);
  
    // Update titles
    pyS += "l = w.activeLayer()\n"
           "l.setCurveTitle(0, '%1')\n"
           "l.setTitle('%2')\n";

    pyS = pyS.arg(wsName).arg(m_title.c_str());

    // Set logarithmic scale if required
    if ( logScale )
      pyS += "l.logYlinX()\n";

    // Set scaling
    if( params["YAxisAuto"] == "True" )
    {
      pyS += "l.setAutoScale()\n";
    }
    else
    {
      pyS += "l.setAxisScale(Layer.Left, %1, %2)\n";
      pyS = pyS.arg(params["YAxisMin"]).arg(params["YAxisMax"]);
    }

    runPythonCode( pyS );
}

/**
 * Get current plot style parameters. wsName is used to get default values. 
 * @param wsName Workspace plot of which we want to style
 * @return Maps of the parameters, see MuonAnalysisOptionTab::parsePlotStyleParams for list
           of possible keys
 */
QMap<QString, QString> MuonAnalysis::getPlotStyleParams(const QString& wsName)
{
  // Get parameter values from the options tab
  QMap<QString, QString> params = m_optionTab->parsePlotStyleParams();

  // If autoscale disabled
  if(params["YAxisAuto"] == "False")
  {
    // Get specified min/max values for Y axis
    QString min = params["YAxisMin"];
    QString max = params["YAxisMax"];

    // If any of those is not specified - get min and max by default
    if(min.isEmpty() || max.isEmpty())
    {
      Workspace_sptr ws_ptr = AnalysisDataService::Instance().retrieve(wsName.toStdString());
      MatrixWorkspace_sptr matrix_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(ws_ptr);
      const Mantid::MantidVec& dataY = matrix_workspace->readY(0);

      if(min.isEmpty())
        params["YAxisMin"] = QString::number(*min_element(dataY.begin(), dataY.end()));

      if(max.isEmpty())
        params["YAxisMax"] = QString::number(*max_element(dataY.begin(), dataY.end()));
    }
  }

  return params;
}

/**
 * Checks if the plot for the workspace does exist.
 * @param wsName Name of the workspace
 * @return True if exists, false if not
 */
bool MuonAnalysis::plotExists(const QString& wsName)
{
  QString code;

  code += "g = graph('%1-1')\n"
          "if g != None:\n"
          "  print('1')\n"
          "else:\n"
          "  print('0')\n";
  
  QString output = runPythonCode(code.arg(wsName));

  bool ok;
  int outputCode = output.toInt(&ok);

  if(!ok)
    throw std::logic_error("Script should print 0 or 1");

  return (outputCode == 1);
}

/**
 * Enable PP tool for the plot of the given WS.
 * @param wsName Name of the WS which plot PP tool will be attached to.
 */
void MuonAnalysis::selectMultiPeak(const QString& wsName)
{
  disableAllTools();

  if( ! plotExists(wsName) )
    plotSpectrum(wsName);

  QString code;

  code += "g = graph('" + wsName + "-1')\n"
          "if g != None:\n"
          "  g.show()\n"
          "  g.setFocus()\n"
          "  selectMultiPeak(g)\n";

  runPythonCode(code);
}

/**
 * Disable tools for all the graphs within MantidPlot.
 */
void MuonAnalysis::disableAllTools()
{
  runPythonCode("disableTools()");
}

/**
 * Hides all the plot windows (MultiLayer ones)
 */
void MuonAnalysis::hideAllPlotWindows()
{
  QString code;

  code += "for w in windows():\n"
          "  if w.inherits('MultiLayer'):\n"
          "    w.hide()\n";

  runPythonCode(code);
}

/**
 * Shows all the plot windows (MultiLayer ones)
 */
void MuonAnalysis::showAllPlotWindows()
{
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
bool MuonAnalysis::isGroupingSet()
{
  std::vector<int> dummy;
  whichGroupToWhichRow(m_uiForm, dummy);

  if (dummy.empty())
    return false;
  else
    return true;
}

/**
 * Calculate number of detectors from string of type 1-3, 5, 10-15
 *
 * @param str :: String of type "1-3, 5, 10-15"
 * @return Number of detectors. Return 0 if not recognised
 */
int MuonAnalysis::numOfDetectors(const std::string& str) const
{
  return static_cast<int>(Strings::parseRange(str).size());
}

/**
* Change the workspace group name to the instrument and run number if load current run was pressed.
*
* @param workspaceGroupName :: The name of the group that needs to be changed or is already in correct format.
*/
void MuonAnalysis::changeCurrentRun(std::string & workspaceGroupName)
{
  QString tempGroupName(QString::fromStdString(workspaceGroupName) );

  if ( (tempGroupName.contains("auto") ) || (tempGroupName.contains("argus0000000") ) )
  {
    Workspace_sptr workspace_ptr = AnalysisDataService::Instance().retrieve(m_workspace_name);
    MatrixWorkspace_sptr matrix_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(workspace_ptr);
    if(!matrix_workspace) // Data collected in periods.
    {
      // Get run number from first period data.
      workspace_ptr = AnalysisDataService::Instance().retrieve(m_workspace_name + "_1");
      matrix_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(workspace_ptr);
      if(!matrix_workspace)
      {
        QMessageBox::information(this, "Mantid - Muon Analysis", "Mantid expected period data but no periods were found.\n"
                      "Default plot name will be used insead of run number.");
        return;
      }
    }
    const Run& runDetails = matrix_workspace->run();
    
    std::string runNumber = runDetails.getProperty("run_number")->value();
    QString instname = m_uiForm.instrSelector->currentText().toUpper();

    size_t zeroPadding(8);

    if (instname == "ARGUS")
      zeroPadding = 7;  

    for (size_t i=runNumber.size(); i<zeroPadding; ++i)
    {
      runNumber = '0' + runNumber;
    }

    workspaceGroupName = instname.toStdString() + runNumber;
  }
}


/** Is input string a number?
 *
 *  @param s :: The input string
 *  @return True is input string is a number
 */
bool MuonAnalysis::isNumber(const std::string& s) const
{
  if( s.empty() )
  {
    return false;
  }

  const std::string allowed("0123456789");

  for (unsigned int i = 0; i < s.size(); i++)
  {
    if (allowed.find_first_of(s[i]) == std::string::npos)
    {
      return false;
    }
  }

  return true;
}


/**
* Return true if data are loaded
*/
bool MuonAnalysis::areDataLoaded()
{
  return AnalysisDataService::Instance().doesExist(m_workspace_name);
}

/**
* Set start up interface look and populate local attributes
* initiated from info set in QT designer
*/
void MuonAnalysis::startUpLook()
{
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
  m_uiForm.homePeriodBox2->setEditable(false);
  m_uiForm.homePeriodBox2->setEnabled(false);

  // Set validators for number-only boxes
  m_uiForm.timeZeroFront->setValidator(createDoubleValidator(m_uiForm.timeZeroFront));
  m_uiForm.firstGoodBinFront->setValidator(createDoubleValidator(m_uiForm.firstGoodBinFront));

  // set various properties of the group table
  m_uiForm.groupTable->setColumnWidth(0, 100);
  m_uiForm.groupTable->setColumnWidth(1, 200);
  for (int i = 0; i < m_uiForm.groupTable->rowCount(); i++)
  {
    QTableWidgetItem* item = m_uiForm.groupTable->item(i,2);
    if (!item)
    {
      QTableWidgetItem* it = new QTableWidgetItem("");
      it->setFlags(it->flags() & (~Qt::ItemIsEditable));
      m_uiForm.groupTable->setItem(i,2, it);
    }
    else
    {
      item->setFlags(item->flags() & (~Qt::ItemIsEditable));
    }
    item = m_uiForm.groupTable->item(i,0);
    if (!item)
    {
      QTableWidgetItem* it = new QTableWidgetItem("");
      m_uiForm.groupTable->setItem(i,0, it);
    }
  }
}

/**
 * If nothing else work set dummy grouping and display comment to user
 */
void MuonAnalysis::setDummyGrouping(Instrument_const_sptr instrument)
{
  // if no grouping in nexus then set dummy grouping and display warning to user
  std::stringstream idstr;
  idstr << "1-" << instrument->getNumberDetectors();
  m_uiForm.groupTable->setItem( 0, 0, new QTableWidgetItem("NoGroupingDetected") );
  m_uiForm.groupTable->setItem( 0, 1, new QTableWidgetItem( QString::fromStdString(idstr.str()) ) );

  updateFrontAndCombo();
}


/**
 * Try to load default grouping file specified in IDF
 */
void MuonAnalysis::setGroupingFromIDF(Instrument_const_sptr instrument, const std::string& mainFieldDirection) 
{
  std::string parameterName = "Default grouping file";

  // Special case for MUSR, because it has two possible groupings
  if (instrument->getName() == "MUSR")
  {
    parameterName.append(" - " + mainFieldDirection);
  }

  std::vector<std::string> groupingFiles = instrument->getStringParameter(parameterName);

  // Get search directory for XML instrument definition files (IDFs)
  std::string directoryName = ConfigService::Instance().getInstrumentDirectory();

  if ( groupingFiles.size() == 1 )
  {
    const std::string groupingFile = groupingFiles[0];

    try
    {
      Grouping loadedGrouping;
      loadGroupingFromXML(directoryName + groupingFile, loadedGrouping);
      fillGroupingTable(loadedGrouping, m_uiForm);
    }
    catch (...)
    {
      g_log.error("Can't load default grouping file:  " + groupingFile);
    }
  }
}


 /**
 * Time zero returend in ms
 */
double MuonAnalysis::timeZero()
{
  QString boxText = m_uiForm.timeZeroFront->text();
  double timeZero = 0.0;
  try
  {
    timeZero = boost::lexical_cast<double>(boxText.toStdString());
  }
  catch(boost::bad_lexical_cast&)
  {
    QMessageBox::warning(this, "MantidPlot - Muon Analysis", "Unable to interpret time zero as number, setting to 0.0");
    m_uiForm.timeZeroFront->setText("0.0");
  }
  return timeZero;
}

 /**
 * first good bin returend in ms
 * returned as the absolute value of first-good-bin minus time zero
 */
QString MuonAnalysis::firstGoodBin()
{
  return m_uiForm.firstGoodBinFront->text();
}

 /**
 * According to Plot Options what time should we plot from in ms
 * @return time to plot from in ms
 */
double MuonAnalysis::plotFromTime()
{
  QLineEdit* startTimeBox;
  double defaultValue;

  // If is first good bin used - we use a different box
  if(m_uiForm.timeComboBox->currentIndex() == 0)
  {
    startTimeBox = m_uiForm.firstGoodBinFront;
    defaultValue = 0.3;
  }
  else
  {
    startTimeBox = m_uiForm.timeAxisStartAtInput;
    defaultValue = 0.0;
  }

  bool ok;
  double returnValue = startTimeBox->text().toDouble(&ok);

  if(!ok)
  {
    returnValue = defaultValue;

    startTimeBox->setText(QString::number(defaultValue));

    QMessageBox::warning(this, "Mantid - MuonAnalysis", 
      QString("Start time number not recognized. Reset to default of %1").arg(defaultValue));
  }
  
  return returnValue;
}


 /**
 * According to Plot Options what time should we plot to in ms
 * @return time to plot to in ms
 */
double MuonAnalysis::plotToTime()
{
  double retVal;
  try
  {
    retVal = boost::lexical_cast<double>(m_uiForm.timeAxisFinishAtInput->text().toStdString());
  }
  catch (...)
  {
    retVal = 1.0;
    QMessageBox::warning(this,"Mantid - MuonAnalysis", "Number not recognised in Plot Option 'Finish at (ms)' input box. Plot to time=1.0.");
  }
  return retVal;
}


/**
* Check if grouping in table is consistent with data file
*
* @return empty string if OK otherwise a complaint
*/
std::string MuonAnalysis::isGroupingAndDataConsistent()
{
  std::string complaint = "Grouping inconsistent with data file. Plotting disabled.\n";

  // should probably farm the getting of matrix workspace out into separate method or store
  // as attribute assigned in inputFileChanged
  Workspace_sptr workspace_ptr = AnalysisDataService::Instance().retrieve(m_workspace_name);
  WorkspaceGroup_sptr wsPeriods = boost::dynamic_pointer_cast<WorkspaceGroup>(workspace_ptr);
  MatrixWorkspace_sptr matrix_workspace;
  if (wsPeriods)
  {
    Workspace_sptr workspace_ptr1 = AnalysisDataService::Instance().retrieve(m_workspace_name + "_1");
    matrix_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(workspace_ptr1);
  }
  else
  {
    matrix_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(workspace_ptr);
  }

  int nDet = static_cast<int>(matrix_workspace->getNumberHistograms());

  complaint += "Number of spectra in data = " + boost::lexical_cast<std::string>(nDet) + ". ";

  int numG = numGroups();
  bool returnComplaint = false;
  for (int iG = 0; iG < numG; iG++)
  {
    typedef Poco::StringTokenizer tokenizer;
    tokenizer values(m_uiForm.groupTable->item(m_groupToRow[iG],1)->text().toStdString(), ",", tokenizer::TOK_TRIM);

    for (int i = 0; i < static_cast<int>(values.count()); i++)
    {
      std::size_t found= values[i].find("-");
      if (found!=std::string::npos)
      {
        tokenizer aPart(values[i], "-", tokenizer::TOK_TRIM);

        int rightInt;
        std::stringstream rightRead(aPart[1]);
        rightRead >> rightInt;

        if ( rightInt > nDet )
        {
          complaint += " Group-table row " + boost::lexical_cast<std::string>(m_groupToRow[iG]+1) + " refers to spectrum "
            + boost::lexical_cast<std::string>(rightInt) + ".";
          returnComplaint = true;
          break;
        }
      }
      else
      {
        if ( (boost::lexical_cast<int>(values[i].c_str()) > nDet) || (boost::lexical_cast<int>(values[i].c_str()) < 1) )
        {
          complaint += " Group-table row " + boost::lexical_cast<std::string>(m_groupToRow[iG]+1) + " refers to spectrum "
            + values[i] + ".";
          returnComplaint = true;
          break;
        }
      }
    }
  }
  if ( returnComplaint )
    return complaint;
  else
    return std::string("");
}


/**
* Check if dublicate ID between different rows
*/
void MuonAnalysis::checkIf_ID_dublicatesInTable(const int row)
{
  QTableWidgetItem *item = m_uiForm.groupTable->item(row,1);

  // row of IDs to compare against
  std::vector<int> idsNew = Strings::parseRange(item->text().toStdString());

  int numG = numGroups();
  int rowInFocus = getGroupNumberFromRow(row);
  for (int iG = 0; iG < numG; iG++)
  {
    if (iG != rowInFocus)
    {
      std::vector<int> ids = Strings::parseRange(m_uiForm.groupTable->item(m_groupToRow[iG],1)->text().toStdString());

      for (unsigned int i = 0; i < ids.size(); i++)
      {
        for (unsigned int j = 0; j < idsNew.size(); j++)
        {
          if ( ids[i] == idsNew[j] )
          {
            item->setText(QString("Dublicate ID: " + item->text()));
            return;
          }
        }
      }
    }
  }
}


/**
 * Load auto saved values
 */
void MuonAnalysis::loadAutoSavedValues(const QString& group)
{
  QSettings prevInstrumentValues;
  prevInstrumentValues.beginGroup(group + "instrument");
  QString instrumentName = prevInstrumentValues.value("name", "MUSR").toString();
  m_uiForm.instrSelector->setCurrentIndex(m_uiForm.instrSelector->findText(instrumentName));

  // load Plot Style options
  QSettings prevPlotStyle;
  prevPlotStyle.beginGroup(group + "plotStyleOptions");

  double timeAxisStart = prevPlotStyle.value("timeAxisStart", 0.3).toDouble();
  double timeAxisFinish = prevPlotStyle.value("timeAxisFinish", 16.0).toDouble();

  m_uiForm.timeAxisStartAtInput->setText(QString::number(timeAxisStart));
  m_uiForm.timeAxisFinishAtInput->setText(QString::number(timeAxisFinish));

  m_optionTab->setStoredCustomTimeValue(prevPlotStyle.value("customTimeValue").toString());
  
  int timeComboBoxIndex = prevPlotStyle.value("timeComboBoxIndex", 0).toInt();
  m_uiForm.timeComboBox->setCurrentIndex(timeComboBoxIndex);
  m_optionTab->runTimeComboBox(timeComboBoxIndex);

  bool axisAutoScaleOnOff = prevPlotStyle.value("axisAutoScaleOnOff", 1).toBool();
  m_uiForm.yAxisAutoscale->setChecked(axisAutoScaleOnOff);
  m_optionTab->runyAxisAutoscale(axisAutoScaleOnOff);

  QStringList kusse = prevPlotStyle.childKeys();
  if ( kusse.contains("yAxisStart") )
  {
    if( ! m_uiForm.yAxisAutoscale->isChecked() )
    {
      double yAxisStart = prevPlotStyle.value("yAxisStart").toDouble();
      m_uiForm.yAxisMinimumInput->setText(QString::number(yAxisStart));
    }
    else
    {
      m_optionTab->setStoredYAxisMinimum(prevPlotStyle.value("yAxisStart").toString());
    }
  }
  if ( kusse.contains("yAxisFinish") )
  {
    if( ! m_uiForm.yAxisAutoscale->isChecked() )
    {
      double yAxisFinish = prevPlotStyle.value("yAxisFinish").toDouble();
      m_uiForm.yAxisMaximumInput->setText(QString::number(yAxisFinish));
    }
    else
    {
      m_optionTab->setStoredYAxisMaximum(prevPlotStyle.value("yAxisFinish").toString());
    }
  }

  // Load Plot Binning Options
  QSettings prevPlotBinning;
  prevPlotBinning.beginGroup(group + "BinningOptions");
  int rebinFixed = prevPlotBinning.value("rebinFixed", 1).toInt();
  m_uiForm.optionStepSizeText->setText(QString::number(rebinFixed));
  m_uiForm.binBoundaries->setText(prevPlotBinning.value("rebinVariable", 1).toString());

  int rebinComboBoxIndex = prevPlotBinning.value("rebinComboBoxIndex", 0).toInt();
  m_uiForm.rebinComboBox->setCurrentIndex(rebinComboBoxIndex);
  m_optionTab->runRebinComboBox(rebinComboBoxIndex);

  // Load Setting tab options
  QSettings prevSettingTabOptions;
  prevSettingTabOptions.beginGroup(group + "SettingOptions");

  int plotCreationIndex = prevSettingTabOptions.value("plotCreation", 0).toInt();
  m_uiForm.plotCreation->setCurrentIndex(plotCreationIndex);

  int connectPlotStyleIndex = prevSettingTabOptions.value("connectPlotStyle", 0).toInt();
  m_uiForm.connectPlotType->setCurrentIndex(connectPlotStyleIndex);

  bool errorBars = prevSettingTabOptions.value("errorBars", 1).toBool();
  m_uiForm.showErrorBars->setChecked(errorBars);

  bool hideTools = prevSettingTabOptions.value("toolbars", 1).toBool();
  m_uiForm.hideToolbars->setChecked(hideTools);

  bool hideGraphs = prevSettingTabOptions.value("hiddenGraphs", 1).toBool();
  m_uiForm.hideGraphs->setChecked(hideGraphs);

  // Load dead time options.
  QSettings deadTimeOptions;
  deadTimeOptions.beginGroup(group + "DeadTimeOptions");

  int deadTimeTypeIndex = deadTimeOptions.value("deadTimes", 0).toInt();
  m_uiForm.deadTimeType->setCurrentIndex(deadTimeTypeIndex);

  onDeadTimeTypeChanged(deadTimeTypeIndex);

  QString savedDeadTimeFile = deadTimeOptions.value("deadTimeFile").toString();
  m_uiForm.mwRunDeadTimeFile->setUserInput(savedDeadTimeFile);

  // Load values saved using saveWidgetValue()
  loadWidgetValue(m_uiForm.timeZeroFront, 0.2);
  loadWidgetValue(m_uiForm.firstGoodBinFront, 0.3);
  loadWidgetValue(m_uiForm.timeZeroAuto, Qt::Checked);
  loadWidgetValue(m_uiForm.firstGoodDataAuto, Qt::Checked);
}


/**
*   Loads up the options for the fit browser so that it works in a muon analysis tab
*/
void MuonAnalysis::loadFittings()
{
  // Title of the fitting dock widget that now lies within the fittings tab. Should be made
  // dynamic so that the Chi-sq can be displayed alongside like original fittings widget
  m_uiForm.fitBrowser->setWindowTitle("Fit Function");
  // Make sure that the window can't be moved or closed within the tab.
  m_uiForm.fitBrowser->setFeatures(QDockWidget::NoDockWidgetFeatures);
}

/**
 * Allow/disallow loading.
 */
void MuonAnalysis::allowLoading(bool enabled)
{
  m_uiForm.nextRun->setEnabled(enabled);
  m_uiForm.previousRun->setEnabled(enabled);
  m_uiForm.loadCurrent->setEnabled(enabled);
  m_uiForm.mwRunFiles->setEnabled(enabled);
}

/**
*   Check to see if the appending option is true when the previous button has been pressed and acts accordingly
*/
void MuonAnalysis::checkAppendingPreviousRun()
{
  if ( m_uiForm.mwRunFiles->getText().isEmpty() )
  {
    return;
  }
  
  allowLoading(false);
  
  if (m_uiForm.mwRunFiles->getText().contains("-"))
  {
    setAppendingRun(-1);
  }
  else
  {
    //Subtact one from the current run and load
    changeRun(-1);
  }
}

/**
*   Check to see if the appending option is true when the next button has been pressed and acts accordingly
*/
void MuonAnalysis::checkAppendingNextRun()
{
  if (m_uiForm.mwRunFiles->getText().isEmpty() )
    return;

  allowLoading(false);

  if (m_uiForm.mwRunFiles->getText().contains("-"))
  {
    setAppendingRun(1);
  }
  else
  {
    //Add one to current run and laod
    changeRun(1);
  }
}


/**
*   This sets up an appending lot of files so that when the user hits enter
*   all files within the range will open.
*
*   @param inc :: The number to increase the run by, this can be
*   -1 if previous has been selected.
*/
void MuonAnalysis::setAppendingRun(int inc)
{
  QString filePath("");

  // Get hold of the files to increment or decrement the range to.
  QStringList currentFiles(m_uiForm.mwRunFiles->getFilenames() );
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
  }
  else // must be next that has been clicked.
  {
    fileNumber = currentFiles.size() - 1; // Pick the last file to increment.
  }

  // File path should be the same for both.
  separateMuonFile(filePath, currentFiles[fileNumber], run, runSize);

  int fileExtensionSize(currentFiles[fileNumber].size()-currentFiles[fileNumber].indexOf('.') );
  QString fileExtension = currentFiles[fileNumber].right(fileExtensionSize);
  currentFiles[fileNumber].chop(fileExtensionSize);

  int firstRunNumber = currentFiles[fileNumber].right(runSize).toInt();
  currentFiles[fileNumber].chop(runSize);

  firstRunNumber = firstRunNumber + inc;
  QString newRun("");
  newRun.setNum(firstRunNumber);

  getFullCode(runSize, newRun);

  // Increment is positive (next button)
  if (inc < 0)
  {
    // Add the file to the beginning of mwRunFiles text box.
    QString lastName = m_previousFilenames[m_previousFilenames.size()-1];
    separateMuonFile(filePath, lastName, run, runSize);
    getFullCode(runSize, run);
    m_uiForm.mwRunFiles->setUserInput(newRun + '-' + run);
  }
  else // Increment is negative (previous button)
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
void MuonAnalysis::changeRun(int amountToChange)
{
  QString filePath("");
  QString currentFile = m_uiForm.mwRunFiles->getFirstFilename();
  if ( (currentFile.isEmpty() ) )
    currentFile = m_previousFilenames[0];
  
  QString run("");
  int runSize(-1);

  // If load current run get the correct run number.
  if (currentFile.contains("auto") || currentFile.contains("argus0000000"))
  {
    separateMuonFile(filePath, currentFile, run, runSize);
    currentFile = filePath + getGroupName() + ".nxs";
  }
    
  separateMuonFile(filePath, currentFile, run, runSize);

  int fileExtensionSize(currentFile.size()-currentFile.indexOf('.') );
  QString fileExtension(currentFile.right(fileExtensionSize) );
  currentFile.chop(fileExtensionSize);

  int runNumber = currentFile.right(runSize).toInt();
  currentFile.chop(runSize);
  
  runNumber = runNumber + amountToChange;
  QString newRun("");
  newRun.setNum(runNumber);

  getFullCode(runSize, newRun);

  if (m_textToDisplay.contains("\\") || m_textToDisplay.contains("/") || m_textToDisplay == "CURRENT RUN")
    m_uiForm.mwRunFiles->setUserInput(filePath + currentFile + newRun);
  else
    m_uiForm.mwRunFiles->setUserInput(newRun);
}


/**
*   Seperates the a given file into instrument, code and size of the code.
*   i.e c:/data/MUSR0002419.nxs becomes c:/data/, MUSR0002419.nxs, 2419, 7.
*
*   @param filePath :: The file path of the data file.
*   @param currentFile :: This is the file with path. Can be network path. Return as file with extension.
*   @param run :: The run as a string without 0's at the beginning.
*   @param runSize :: contains the size of the run number.
*/
void MuonAnalysis::separateMuonFile(QString &filePath, QString &currentFile, QString &run, int &runSize)
{
  int fileStart(-1);
  int firstRunDigit(-1);

  //Find where the file begins
  for (int i = 0; i<currentFile.size(); i++)
  {
    if(currentFile[i] == '/' || currentFile[i] == '\\')  //.isDigit())
    {
      fileStart = i+1;
    }
  }

  filePath = currentFile.left(fileStart);
  currentFile = currentFile.right(currentFile.size() - fileStart);

  for (int i = 0; i<currentFile.size(); i++)
  {
    if(currentFile[i].isDigit())  //.isDigit())
    {
      firstRunDigit = i;
      break;
    }
  }

  runSize = 0;
  if (! (firstRunDigit < 0) )
  {
    //Find where the run number ends
    for (int i = firstRunDigit; i<currentFile.size(); i++)
    {
      if(currentFile[i] == '.')
        break;
      if(currentFile[i].isDigit())
      {
        ++runSize;
      }
    }
  }
  run = currentFile.right(currentFile.size()-firstRunDigit);
  run = run.left(runSize);
}


/**
* Adds the 0's back onto the run which were lost when converting it to an integer.
*
* @param originalSize :: The size of the original run before conversion
* @param run :: This is the run after it was incremented or decremented.
*/
void MuonAnalysis::getFullCode(int originalSize, QString & run)
{
  while (originalSize > run.size())
  {
    run = "0" + run;
  }
}


/**
 * Is called every time when tab gets changed
 *
 * @param newTabIndex :: The index of the tab we switch to
 */
void MuonAnalysis::changeTab(int newTabIndex)
{
  QWidget* newTab = m_uiForm.tabWidget->widget(newTabIndex);

  // Make sure all toolbars are still not visible. May have brought them back to do a plot.
  if (m_uiForm.hideToolbars->isChecked())
    emit setToolbarsHidden(true);

  m_uiForm.fitBrowser->setStartX(m_uiForm.timeAxisStartAtInput->text().toDouble());
  m_uiForm.fitBrowser->setEndX(m_uiForm.timeAxisFinishAtInput->text().toDouble());

  if(m_currentTab == m_uiForm.DataAnalysis) // Leaving DA tab
  {
    // Say MantidPlot to use default fit prop. browser
    emit setFitPropertyBrowser(NULL);

    // Remove PP tool from any plots it was attached to
    disableAllTools();

    // Disconnect to avoid problems when filling list of workspaces in fit prop. browser
    disconnect(m_uiForm.fitBrowser, SIGNAL(workspaceNameChanged(const QString&)),
                              this, SLOT(selectMultiPeak(const QString&)));
  }

  if(newTab == m_uiForm.DataAnalysis) // Entering DA tab
  {
    // Say MantidPlot to use Muon Analysis fit prop. browser
    emit setFitPropertyBrowser(m_uiForm.fitBrowser);

    // Show connected plot and attach PP tool to it (if has been assigned)
    if(m_currentDataName != NOT_AVAILABLE)
      selectMultiPeak(m_currentDataName);
    
    // In future, when workspace gets changed, show its plot and attach PP tool to it
    connect(m_uiForm.fitBrowser, SIGNAL(workspaceNameChanged(const QString&)),
                           this, SLOT(selectMultiPeak(const QString&)), Qt::QueuedConnection);
  }
  else if(newTab == m_uiForm.ResultsTable)
  {
    m_resultTableTab->refresh();
  }

  m_currentTab = newTab;
}

/**
* Set up the signals and slots for auto updating the plots
*/
void MuonAnalysis::connectAutoUpdate()
{
  // Home tab Auto Updates
  connect(m_uiForm.frontGroupGroupPairComboBox, SIGNAL( activated(int) ), this, SLOT( homeTabUpdatePlot() ));

  connect(m_uiForm.frontPlotFuncs, SIGNAL( activated(int) ), this, SLOT( homeTabUpdatePlot() ));
  connect(m_uiForm.frontAlphaNumber, SIGNAL( returnPressed() ), this, SLOT( homeTabUpdatePlot() ));

  connect(m_uiForm.timeZeroFront, SIGNAL(returnPressed()), this, SLOT(homeTabUpdatePlot()));
  connect(m_uiForm.firstGoodBinFront, SIGNAL(returnPressed ()), this, SLOT(homeTabUpdatePlot()));

  connect(m_uiForm.homePeriodBox1, SIGNAL( activated(int) ), this, SLOT( homeTabUpdatePlot() ));
  connect(m_uiForm.homePeriodBoxMath, SIGNAL( activated(int) ), this, SLOT( homeTabUpdatePlot() ));
  connect(m_uiForm.homePeriodBox2, SIGNAL( activated(int) ), this, SLOT( homeTabUpdatePlot() ));

  connect(m_uiForm.deadTimeType, SIGNAL( activated(int) ), this, SLOT( deadTimeTypeAutoUpdate(int) ));

  // Grouping tab Auto Updates
  connect(m_uiForm.groupTablePlotChoice, SIGNAL(activated(int)), this, SLOT(groupTabUpdatePlot()));
  connect(m_uiForm.pairTablePlotChoice, SIGNAL(activated(int)), this, SLOT(groupTabUpdatePlot()));

  // Settings tab Auto Updates
  connect(m_optionTab, SIGNAL(settingsTabUpdatePlot()), this, SLOT(settingsTabUpdatePlot()));
  connect(m_optionTab, SIGNAL(plotStyleChanged()), this, SLOT(updateCurrentPlotStyle()));
}

/**
 * Connect widgets to saveWidgetValue() slot so their values are automatically saved when they are
 * getting changed.
 */
void MuonAnalysis::connectAutoSave()
{
  connect(m_uiForm.timeZeroFront, SIGNAL(textChanged(const QString&)), this, SLOT(saveWidgetValue()));
  connect(m_uiForm.firstGoodBinFront, SIGNAL(textChanged(const QString&)), this, SLOT(saveWidgetValue()));

  connect(m_uiForm.timeZeroAuto, SIGNAL(stateChanged(int)), this, SLOT(saveWidgetValue()));
  connect(m_uiForm.firstGoodDataAuto, SIGNAL(stateChanged(int)), this, SLOT(saveWidgetValue()));
}

/**
 * Saves the value of the widget which called the slot.
 */
void MuonAnalysis::saveWidgetValue()
{
  // Get the widget which called the slot
  QWidget* sender = qobject_cast<QWidget*>(QObject::sender());

  if(!sender)
    throw std::runtime_error("Unable to save value of non-widget QObject");

  QString name = sender->objectName();

  QSettings settings;
  settings.beginGroup(m_settingsGroup + "SavedWidgetValues");

  // Save value for QLineEdit
  if(QLineEdit* w = qobject_cast<QLineEdit*>(sender))
  {
    settings.setValue(name, w->text());
  }
  // Save value for QCheckBox
  else if(QCheckBox* w = qobject_cast<QCheckBox*>(sender))
  {
    settings.setValue(name, static_cast<int>(w->checkState()));
  }
  // ... add more as neccessary
  else
    throw std::runtime_error("Value saving for this widget type is not supported");

  settings.endGroup();
}

/**
 * Load previously saved value for the widget.
 * @param       target :: Widget where the value will be loaded to
 * @param defaultValue :: Values which will be set if there is no saved value
 */
void MuonAnalysis::loadWidgetValue(QWidget* target, const QVariant& defaultValue)
{
  QString name = target->objectName();

  QSettings settings;
  settings.beginGroup(m_settingsGroup + "SavedWidgetValues");


  // Load value for QLineEdit
  if(QLineEdit* w = qobject_cast<QLineEdit*>(target))
  {
    w->setText(settings.value(name, defaultValue).toString());
  }
  // Load value for QCheckBox
  else if(QCheckBox* w = qobject_cast<QCheckBox*>(target))
  {
    w->setCheckState(static_cast<Qt::CheckState>(settings.value(name, defaultValue).toInt()));
  }
  // ... add more as neccessary
  else
    throw std::runtime_error("Value loading for this widget type is not supported");

  settings.endGroup();
}

/**
 * Checks whether two specified periods are equal and, if they are, sets second one to None.
 */
void MuonAnalysis::checkForEqualPeriods()
{
  if ( m_uiForm.homePeriodBox2->currentText() == m_uiForm.homePeriodBox1->currentText() )
  {
    m_uiForm.homePeriodBox2->setCurrentIndex(0);
  }
}

void MuonAnalysis::homeTabUpdatePlot()
{
  if (isAutoUpdateEnabled() && m_currentTab == m_uiForm.Home && m_loaded)
      runFrontPlotButton();
}

void MuonAnalysis::groupTabUpdatePlot()
{
  if (isAutoUpdateEnabled() && m_currentTab == m_uiForm.GroupingOptions && m_loaded)
      runFrontPlotButton();
}

void MuonAnalysis::settingsTabUpdatePlot()
{
  if (isAutoUpdateEnabled() && m_currentTab == m_uiForm.Settings && m_loaded == true)
    runFrontPlotButton();
}

/**
 * Sets plot type combo box on the Home tab to the same value as the one under Group Table.
 */
void MuonAnalysis::syncGroupTablePlotTypeWithHome()
{
  int plotTypeIndex = m_uiForm.groupTablePlotChoice->currentIndex();

  if ( m_uiForm.frontPlotFuncs->count() <= plotTypeIndex )
  {
    // This is not the best solution, but I don't have anything brighter at the moment and it
    // was working like that for some time without anybody complaining.
    m_uiForm.frontGroupGroupPairComboBox->setCurrentIndex(0);
  }

  m_uiForm.frontPlotFuncs->setCurrentIndex(plotTypeIndex);
}

/**
 * Updates the style of the current plot according to actual parameters on settings tab.
 */
void MuonAnalysis::updateCurrentPlotStyle()
{
  if (isAutoUpdateEnabled() && m_currentDataName != NOT_AVAILABLE)
  {
    // Replot using new style params
    plotSpectrum(m_currentDataName);
  }
}

bool MuonAnalysis::isAutoUpdateEnabled()
{
  int choice(m_uiForm.plotCreation->currentIndex());
  return (choice == 0 || choice == 1);
}

/**
 * Whether Overwrite option is enabled on the Settings tab.
 * @return True if enabled, false if not
 */
bool MuonAnalysis::isOverwriteEnabled()
{
  int choice(m_uiForm.plotCreation->currentIndex());
  return (choice == 0 || choice == 2);
}

/**
 * Executed when interface gets hidden or closed
 */
void MuonAnalysis::hideEvent(QHideEvent *)
{
  // Show toolbars if were chosen to be hidden by user
  if (m_uiForm.hideToolbars->isChecked())
    emit setToolbarsHidden(false);
  
  // If closed while on DA tab, reassign fit property browser to default one
  if(m_currentTab == m_uiForm.DataAnalysis)
    emit setFitPropertyBrowser(NULL);
}


/**
 * Executed when interface gets shown
 */
void MuonAnalysis::showEvent(QShowEvent *)
{
  // Hide toolbars if requested by user
  if (m_uiForm.hideToolbars->isChecked() )
    emit setToolbarsHidden(true);
}

/**
 * Hide/show MantidPlot toolbars.
 * @param hidden If true, toolbars will be hidden, if false - shown
 */
void MuonAnalysis::doSetToolbarsHidden(bool hidden)
{
  QString isVisibleStr = hidden ? "False" : "True";

  runPythonCode( QString("setToolbarsVisible(%1)").arg(isVisibleStr) );
}


/**
 * Called when dead time correction type is changed.
 * @param choice :: New index of dead time correction type combo box
 */
void MuonAnalysis::onDeadTimeTypeChanged(int choice)
{
  m_deadTimesChanged = true;

  if (choice == 0 || choice == 1) // if choice == none || choice == from file
  {
    m_uiForm.mwRunDeadTimeFile->setVisible(false);
    m_uiForm.dtcFileLabel->setVisible(false);
  }
  else // choice must be from workspace
  {
    m_uiForm.mwRunDeadTimeFile->setVisible(true);
    m_uiForm.mwRunDeadTimeFile->setUserInput("");
    m_uiForm.dtcFileLabel->setVisible(true);
  }

  QSettings group;
  group.beginGroup(m_settingsGroup + "DeadTimeOptions");
  group.setValue("deadTimes", choice);
}

/**
 * Auto-update the plot after user has changed dead time correction type.
 * @param choice :: User selected index of the dead time correction combox box
 */
void MuonAnalysis::deadTimeTypeAutoUpdate(int choice)
{
  // We update the plot only if user switches to "None" or "From Data File" correction type, because
  // in case of "From Disk" the file should be specified first.
  if ( choice == 0 || choice == 1 )
  {
    homeTabUpdatePlot();
  }
}

/**
* If the user selects/changes the file to be used to apply the dead times then 
* see if the plot needs updating and make sure next time the user plots that the
* dead times are applied.
*/
void MuonAnalysis::deadTimeFileSelected()
{
  if(!m_uiForm.mwRunDeadTimeFile->isValid())
    return;

  // Remember the filename for the next time interface is opened
  QSettings group;
  group.beginGroup(m_settingsGroup + "DeadTimeOptions");
  group.setValue("deadTimeFile", m_uiForm.mwRunDeadTimeFile->getText());

  m_deadTimesChanged = true;
  homeTabUpdatePlot();
}

/**
 * Creates new double validator which accepts numbers in standard notation only.
 * @param parent :: Parent of the new validator
 * @return New created validator
 */
QDoubleValidator* MuonAnalysis::createDoubleValidator(QObject* parent)
{
  QDoubleValidator* newValidator = new QDoubleValidator(parent);
  newValidator->setNotation(QDoubleValidator::StandardNotation);
  return newValidator;
}

/**
 * Updates the enabled-state and value of Time Zero using "auto" check-box state.
 * @param checkBoxState :: State of "auto" check-box. If -1 will retrieve it from the form
 */
void MuonAnalysis::setTimeZeroState(int checkBoxState)
{
  if(checkBoxState == -1)
    checkBoxState = m_uiForm.timeZeroAuto->checkState();

  if(checkBoxState == Qt::Checked) // From data file
  {
    m_uiForm.timeZeroFront->setEnabled(false);
    m_uiForm.timeZeroFront->setText(QString::number(m_dataTimeZero, 'g', 2));
    homeTabUpdatePlot(); // Auto-update
  }
  else // Custom
  {
    m_uiForm.timeZeroFront->setEnabled(true);
  }
}

/**
 * Updates the enabled-state and value of First Good Data using "auto" check-box state.
 * @param checkBoxState :: State of "auto" check-box. If -1 will retrieve it from the form
 */
void MuonAnalysis::setFirstGoodDataState(int checkBoxState)
{
  if(checkBoxState == -1)
    checkBoxState = m_uiForm.firstGoodDataAuto->checkState();

  if(checkBoxState == Qt::Checked) // From data file
  {
    m_uiForm.firstGoodBinFront->setEnabled(false);
    m_uiForm.firstGoodBinFront->setText(QString::number(m_dataFirstGoodData, 'g', 2));
    homeTabUpdatePlot(); // Auto-update
  }
  else // Custom
  {
    m_uiForm.firstGoodBinFront->setEnabled(true);
  }
}

/**
 * Groups loaded workspace (m_workspace_name). Grouped workspace is stored under m_grouped_name.
 * @param detGroupingTable :: Grouping information to use. If null - info from table widget is used
 */
void MuonAnalysis::groupLoadedWorkspace(ITableWorkspace_sptr detGroupingTable)
{
  if ( ! detGroupingTable )
  {
    auto groupingFromUI = parseGrouping();

    if ( ! groupingFromUI )
      throw std::invalid_argument("Unable to parse grouping information from the table, or it is empty.");

    detGroupingTable = groupingFromUI;
  }

  // Make sure grouping table is in the ADS
  ScopedWorkspace table(detGroupingTable);

  try
  {
    IAlgorithm_sptr groupAlg = AlgorithmManager::Instance().createUnmanaged("MuonGroupDetectors"); 
    groupAlg->initialize();
    groupAlg->setLogging(false); // Don't want to clutter the log
    groupAlg->setRethrows(true);
    groupAlg->setPropertyValue("InputWorkspace", m_workspace_name);
    groupAlg->setPropertyValue("OutputWorkspace", m_grouped_name);
    groupAlg->setPropertyValue("DetectorGroupingTable", table.name());
    groupAlg->execute();
  }
  catch(std::exception& e)
  {
    throw std::runtime_error( "Unable to group loaded workspace:\n\n" + std::string(e.what()) );
  }
}

/**
 * Parses grouping information from the UI table.
 * @return ITableWorkspace of the format returned by LoadMuonNexus
 */
ITableWorkspace_sptr MuonAnalysis::parseGrouping()
{
  std::vector<int> groupRows;
  whichGroupToWhichRow(m_uiForm, groupRows); 

  if ( groupRows.size() == 0 )
    return ITableWorkspace_sptr();

  auto newTable = boost::dynamic_pointer_cast<ITableWorkspace>(
      WorkspaceFactory::Instance().createTable("TableWorkspace") );

  newTable->addColumn("vector_int", "Detectors");

  for ( auto it = groupRows.begin(); it != groupRows.end(); ++it )
  {
    const std::string detectorsString = m_uiForm.groupTable->item(*it,1)->text().toStdString();

    TableRow newRow = newTable->appendRow(); 
    newRow << Strings::parseRange(detectorsString);
  }

  return newTable;
}

/**
 * Updated UI table using the grouping information provided.
 * @param detGroupingTable :: Grouping information in the format as returned by LoadMuonNexus
 */
void MuonAnalysis::setGrouping(ITableWorkspace_sptr detGroupingTable)
{
  for ( size_t row = 0; row < detGroupingTable->rowCount(); ++row )
  {
    std::vector<int> detectors = detGroupingTable->cell< std::vector<int> >(row,0);

    // toString() expects the sequence to be sorted
    std::sort( detectors.begin(), detectors.end() );

    // Convert to a range string, i.e. 1-5,6-8,9
    const std::string& detectorRange = Strings::toString(detectors);

    m_uiForm.groupTable->setItem( static_cast<int>(row), 0, 
        new QTableWidgetItem( QString::number(row + 1) ) );

    m_uiForm.groupTable->setItem( static_cast<int>(row), 1, 
        new QTableWidgetItem( QString::fromStdString(detectorRange) ) );
  }

  if ( numGroups() == 2 && numPairs() <= 0 )
  {
    m_uiForm.pairTable->setItem( 0, 0, new QTableWidgetItem("long") );
    m_uiForm.pairTable->setItem( 0, 3, new QTableWidgetItem("1.0") );
  }

  updatePairTable();
  updateFrontAndCombo();
}

/**
 * Opens a sequential fit dialog.
 */
void MuonAnalysis::openSequentialFitDialog()
{
  Algorithm_sptr loadAlg;

  try
  {
    loadAlg = createLoadAlgorithm();
  }
  catch(...)
  {
    QMessageBox::critical(this, "Unable to open dialog", "Error while setting load properties");
    return;
  }

  m_uiForm.fitBrowser->blockSignals(true);

  MuonSequentialFitDialog* dialog = new MuonSequentialFitDialog(m_uiForm.fitBrowser, loadAlg);
  dialog->exec();

  m_uiForm.fitBrowser->blockSignals(false);
}

/**
 * Returns custom dead time table file name as set on the interface.
 * @return The filename
 */
std::string MuonAnalysis::deadTimeFilename()
{
  if(!m_uiForm.mwRunDeadTimeFile->isValid())
    throw std::runtime_error("Specified Dead Time file is not valid.");

  return m_uiForm.mwRunDeadTimeFile->getFirstFilename().toStdString();
}

/**
 * Loads dead time table (group of tables) from the file.
 * @param filename :: File to load dead times from
 * @return Table (group of tables) with dead times
 */
Workspace_sptr MuonAnalysis::loadDeadTimes(const std::string& filename)
{
  try
  {
    IAlgorithm_sptr loadDeadTimes = AlgorithmManager::Instance().create("LoadNexusProcessed");
    loadDeadTimes->setChild(true);
    loadDeadTimes->setPropertyValue("Filename", filename);
    loadDeadTimes->setPropertyValue("OutputWorkspace", "__NotUsed");
    loadDeadTimes->execute();

    return loadDeadTimes->getProperty("OutputWorkspace");
  }
  catch(...)
  {
    throw std::runtime_error("Unable to load dead times from the spefied file");
  }
}

/**
 * Creates and algorithm with all the properties set according to widget values on the interface.
 * @return The algorithm with properties set
 */
Algorithm_sptr MuonAnalysis::createLoadAlgorithm()
{
  Algorithm_sptr loadAlg = AlgorithmManager::Instance().createUnmanaged("MuonLoad");
  loadAlg->initialize();

  // -- Dead Time Correction --------------------------------------------------

  if (m_uiForm.deadTimeType->currentIndex() != 0)
  {
    loadAlg->setProperty("ApplyDeadTimeCorrection", true);

    if (m_uiForm.deadTimeType->currentIndex() == 2) // From Specified File
    {

      Workspace_sptr deadTimes = loadDeadTimes( deadTimeFilename() );

      loadAlg->setProperty("CustomDeadTimeTable", deadTimes);
    }
  }

  // -- Grouping --------------------------------------------------------------

  ITableWorkspace_sptr grouping = parseGrouping(); 
  loadAlg->setProperty("DetectorGroupingTable", grouping);

  // -- X axis options --------------------------------------------------------

  double Xmin = m_uiForm.timeAxisStartAtInput->text().toDouble();
  loadAlg->setProperty("Xmin", Xmin);

  double Xmax = m_uiForm.timeAxisFinishAtInput->text().toDouble();
  loadAlg->setProperty("Xmax", Xmax);

  double timeZero = m_uiForm.timeZeroFront->text().toDouble(); 
  loadAlg->setProperty("TimeZero", timeZero);

  // -- Rebin options ---------------------------------------------------------

  if ( m_uiForm.rebinComboBox->currentIndex() != 0)
  {
    std::string rebinParams;

    if(m_uiForm.rebinComboBox->currentIndex() == 1) // Fixed
    {
      auto loadedWS = AnalysisDataService::Instance().retrieveWS<Workspace>(m_grouped_name);
      MatrixWorkspace_sptr ws;

      if ( ! ( ws = boost::dynamic_pointer_cast<MatrixWorkspace>(loadedWS) ) )
      {
        auto group = boost::dynamic_pointer_cast<WorkspaceGroup>(loadedWS);
        ws = boost::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(0));
      }

      double binSize = ws->dataX(0)[1] - ws->dataX(0)[0];

      double bunchedBinSize = binSize * m_uiForm.optionStepSizeText->text().toDouble();

      rebinParams = boost::lexical_cast<std::string>(bunchedBinSize);
    }
    else // Variable
    {
      rebinParams = m_uiForm.binBoundaries->text().toStdString();
    }

    loadAlg->setPropertyValue("RebinParams", rebinParams);
  }

  // -- Group/pair properties -------------------------------------------------

  int index = m_uiForm.frontGroupGroupPairComboBox->currentIndex();

  if (index >= numGroups())
  {
    loadAlg->setProperty("OutputType", "PairAsymmetry");
    int tableRow = m_pairToRow[index - numGroups()];

    QTableWidget* t = m_uiForm.pairTable;

    double alpha = t->item(tableRow,3)->text().toDouble();
    int index1 = static_cast<QComboBox*>( t->cellWidget(tableRow,1) )->currentIndex();
    int index2 = static_cast<QComboBox*>( t->cellWidget(tableRow,2) )->currentIndex();

    loadAlg->setProperty("PairFirstIndex", index1);
    loadAlg->setProperty("PairSecondIndex", index2);
    loadAlg->setProperty("Alpha", alpha);
  }
  else
  {
    if ( parsePlotType(m_uiForm.frontPlotFuncs) == Asymmetry )
      loadAlg->setProperty("OutputType", "GroupAsymmetry");
    else
      loadAlg->setProperty("OutputType", "GroupCounts");

    int groupIndex = getGroupNumberFromRow(m_groupToRow[index]);
    loadAlg->setProperty("GroupIndex", groupIndex);
  }

  // -- Period options --------------------------------------------------------

  QString periodLabel1 = m_uiForm.homePeriodBox1->currentText();

  int periodIndex1 = periodLabel1.toInt() - 1;
  loadAlg->setProperty("FirstPeriod", periodIndex1);

  QString periodLabel2 = m_uiForm.homePeriodBox2->currentText();
  if ( periodLabel2 != "None" )
  {
    int periodIndex2 = periodLabel2.toInt() - 1;
    loadAlg->setProperty("SecondPeriod", periodIndex2);

    std::string op = m_uiForm.homePeriodBoxMath->currentText().toStdString();
    loadAlg->setProperty("PeriodOperation", op);
  }

  return loadAlg;
}

}//namespace MantidQT
}//namespace CustomInterfaces
