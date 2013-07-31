//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/MuonAnalysis.h"
#include "MantidQtCustomInterfaces/MuonAnalysisHelper.h"
#include "MantidQtCustomInterfaces/MuonAnalysisOptionTab.h"
#include "MantidQtCustomInterfaces/MuonAnalysisFitDataTab.h"
#include "MantidQtCustomInterfaces/MuonAnalysisResultTableTab.h"
#include "MantidQtCustomInterfaces/IO_MuonGrouping.h"
#include "MantidQtAPI/FileDialogHandler.h"
#include "MantidQtMantidWidgets/FitPropertyBrowser.h"
#include "MantidQtMantidWidgets/MuonFitPropertyBrowser.h"

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidGeometry/Instrument/XMLlogfile.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidKernel/cow_ptr.h"

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

//----------------------
// Public member functions
//----------------------
///Constructor
MuonAnalysis::MuonAnalysis(QWidget *parent) :
  UserSubWindow(parent), m_last_dir(), m_workspace_name("MuonAnalysis"), m_currentDataName("N/A"), m_assigned(false), m_groupTableRowInFocus(0), m_pairTableRowInFocus(0),
  m_tabNumber(0), m_groupNames(), m_settingsGroup("CustomInterfaces/MuonAnalysis/"), 
  m_updating(false), m_loaded(false), m_deadTimesChanged(false), m_textToDisplay(""),
  m_nexusTimeZero(0.0)
{
  try
  {
    Poco::File tempFile(ConfigService::Instance().getInstrumentDirectory());

    // If the instrument directory can't be written to... (linux problem)
    if (tempFile.exists() && !tempFile.canWrite() ) 
    {
      g_log.information() << "Instrument directory is read only, writing temp grouping to system temp.\n";
      m_groupingTempFilename = ConfigService::Instance().getTempDir()+"tempMuonAnalysisGrouping.xml";
    }
    else
    {
      m_groupingTempFilename = ConfigService::Instance().getInstrumentDirectory()+"Grouping/tempMuonAnalysisGrouping.xml";
    }
  }
  catch(...)
  {
    g_log.debug() << "Problem writing temp grouping file";
  }
}

/// Set up the dialog layout
void MuonAnalysis::initLayout()
{
  m_uiForm.setupUi(this);

  m_uiForm.fitBrowser->init();

  // alow appending files
  m_uiForm.mwRunFiles->allowMultipleFiles(true);

  // Further set initial look
  startUpLook();
  createMicroSecondsLabels(m_uiForm);
  m_uiForm.mwRunFiles->readSettings(m_settingsGroup + "mwRunFilesBrowse");

  connect(m_uiForm.previousRun, SIGNAL(clicked()), this, SLOT(checkAppendingPreviousRun()));
  connect(m_uiForm.nextRun, SIGNAL(clicked()), this, SLOT(checkAppendingNextRun()));

  m_optionTab = new MuonAnalysisOptionTab(m_uiForm, m_settingsGroup);
  m_fitDataTab = new MuonAnalysisFitDataTab(m_uiForm);
  m_resultTableTab = new MuonAnalysisResultTableTab(m_uiForm);

  m_optionTab->initLayout();
  m_fitDataTab->init();

  setConnectedDataText();

  // Add the graphs back to mantid if the user selects not to hide graphs on settings tab.
  connect(m_optionTab, SIGNAL(notHidingGraphs()), this, SIGNAL (showGraphs()));

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
  connect(m_uiForm.frontGroupGroupPairComboBox, SIGNAL(currentIndexChanged(int)), this,
    SLOT(runFrontGroupGroupPairComboBox(int)));

  connect(m_uiForm.hideToolbars, SIGNAL(toggled(bool)), this, SLOT(showHideToolbars(bool)));

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

  // Input check for First Good Data
  connect(m_uiForm.firstGoodBinFront, SIGNAL(lostFocus()), this,
    SLOT(runFirstGoodBinFront()));

  // load previous saved values
  loadAutoSavedValues(m_settingsGroup);

  // connect the fit function widget buttons to their respective slots.
  loadFittings();

  // Detected a workspace change and therefore the peak picker tool needs to be reassigned.
  connect(m_uiForm.fitBrowser, SIGNAL(wsChangePPAssign(const QString &)), this, SLOT(assignPeakPickerTool(const QString &)));

  // Detect when the tab is changed
  connect(m_uiForm.tabWidget, SIGNAL(currentChanged(int)), this, SLOT(changeTab(int)));

  connectAutoUpdate();

  // Muon scientists never fits peaks, hence they want the following parameter, set to a high number
  ConfigService::Instance().setString("curvefitting.peakRadius","99");

  connect(m_uiForm.deadTimeType, SIGNAL(currentIndexChanged(int)), this, SLOT(changeDeadTimeType(int) ) );
  connect(m_uiForm.mwRunDeadTimeFile, SIGNAL(fileEditingFinished()), this, SLOT(deadTimeFileSelected() ) );
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
* Set connected data text.
*/
void MuonAnalysis::setConnectedDataText()
{
  m_uiForm.connectedDataHome->setText("Connected: " + m_currentDataName);
  m_uiForm.connectedDataGrouping->setText("Connected: " + m_currentDataName);
  m_uiForm.connectedDataSettings->setText("Connected: " + m_currentDataName);
}


/**
* Front group/ group pair combobox (slot)
*/
void MuonAnalysis::runFrontGroupGroupPairComboBox(int index)
{
  if ( index >= 0 )
    updateFront();
}


/**
* Check input is valid in input box (slot)
*/
void MuonAnalysis::runFirstGoodBinFront()
{
  try
  {
    boost::lexical_cast<double>(m_uiForm.firstGoodBinFront->text().toStdString());
    
    // if this value updated then also update 'Start at" Plot option if "Start at First Good Data" set
    if (m_uiForm.timeComboBox->currentIndex() == 0 )
    {
      m_uiForm.timeAxisStartAtInput->setText(m_uiForm.firstGoodBinFront->text());
    }
  }
  catch (...)
  {
    QMessageBox::warning(this,"Mantid - MuonAnalysis", "Number not recognised in First Good Data (ms)' input box. Reset to 0.3.");
    m_uiForm.firstGoodBinFront->setText("0.3");
  }
}


/**
* Front plot button (slot)
*/
void MuonAnalysis::runFrontPlotButton()
{
  if (m_deadTimesChanged)
  {
    inputFileChanged(m_previousFilenames);
    return;
  }

  // get current index
  int index = m_uiForm.frontGroupGroupPairComboBox->currentIndex();

  if (index < 0)
  {
    index = 0;
    m_uiForm.frontGroupGroupPairComboBox->setCurrentIndex(index);
  }
  else if (index >= numGroups())
  {
    // i.e. index points to a pair
    m_pairTableRowInFocus = m_pairToRow[index-numGroups()];  // this can be improved
    std::string str = m_uiForm.frontPlotFuncs->currentText().toStdString();
    plotPair(str);
  }
  else
  {
    m_groupTableRowInFocus = m_groupToRow[index];
    std::string str = m_uiForm.frontPlotFuncs->currentText().toStdString();
    plotGroup(str);
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
    saveGroupingTabletoXML(m_uiForm, groupingFile.toStdString());
    
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

  saveGroupingTabletoXML(m_uiForm, m_groupingTempFilename);
  clearTablesAndCombo();

  try
  {
    loadGroupingXMLtoTable(m_uiForm, groupingFile.toStdString());
  }
  catch (Exception::FileError& e)
  {
    g_log.error(e.what());
    g_log.error("Revert to previous grouping");
    loadGroupingXMLtoTable(m_uiForm, m_groupingTempFilename);
  }

  // add number of detectors column to group table
  int numRows = m_uiForm.groupTable->rowCount();
  for (int i = 0; i < numRows; i++)
  {
    QTableWidgetItem *item = m_uiForm.groupTable->item(i,1);
    if (!item)
      break;
    if ( item->text().isEmpty() )
      break;

    std::stringstream detNumRead;
    try
    {
      detNumRead << numOfDetectors(item->text().toStdString());
      m_uiForm.groupTable->setItem(i, 2, new QTableWidgetItem(detNumRead.str().c_str()));
    }
    catch (...)
    {
      m_uiForm.groupTable->setItem(i, 2, new QTableWidgetItem("Invalid"));
    }
  }
  updateFront();
  m_updating = false;
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
  if (m_deadTimesChanged)
  {
    inputFileChanged(m_previousFilenames);
    return;
  }
  plotGroup(m_uiForm.groupTablePlotChoice->currentText().toStdString());
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
  {
    std::stringstream idstr;
    idstr << "1-" << matrix_workspace->getNumberHistograms();
    m_uiForm.groupTable->setItem(0, 0, new QTableWidgetItem("NoGroupingDetected"));
    m_uiForm.groupTable->setItem(0, 1, new QTableWidgetItem(idstr.str().c_str()));
    updateFrontAndCombo();
  }

  if ( !applyGroupingToWS(m_workspace_name, m_workspace_name+"Grouped") )
    return;

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

  // Populate period information
  std::stringstream periodLabel;
  periodLabel << "Data collected in " << numPeriods << " Periods. "
    << "Plot/analyse Period:";
  m_uiForm.homePeriodsLabel->setText(periodLabel.str().c_str());

  while ( m_uiForm.homePeriodBox1->count() != 0 )
    m_uiForm.homePeriodBox1->removeItem(0);
  while ( m_uiForm.homePeriodBox2->count() != 0 )
    m_uiForm.homePeriodBox2->removeItem(0);

  m_uiForm.homePeriodBox2->addItem("None");
  for ( int i = 1; i <= numPeriods; i++ )
  {
    std::stringstream strInt;
    strInt << i;
    m_uiForm.homePeriodBox1->addItem(strInt.str().c_str());
    m_uiForm.homePeriodBox2->addItem(strInt.str().c_str());
  }

  if (wsPeriods)
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
 * Pair table plot button (slot)
 */
void MuonAnalysis::runPairTablePlotButton()
{
  if (m_deadTimesChanged)
  {
    inputFileChanged(m_previousFilenames);
    return;
  }

  m_uiForm.frontPlotFuncs->setCurrentIndex(m_uiForm.pairTablePlotChoice->currentIndex());
  // if something sensible in row then update front
  int currentSelection(m_uiForm.pairTable->currentRow());
  if (currentSelection >= 0)
  {
    int index (numGroups() + currentSelection);
    if (m_uiForm.frontGroupGroupPairComboBox->count() >= index)
    {
      m_uiForm.frontGroupGroupPairComboBox->setCurrentIndex(index);
      plotPair(m_uiForm.pairTablePlotChoice->currentText().toStdString());
    }
  }
  else
  {
    m_uiForm.frontGroupGroupPairComboBox->setCurrentIndex(numGroups()); //if two groups then index 2 will be pair group
    plotPair(m_uiForm.pairTablePlotChoice->currentText().toStdString());
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
    updateFront();
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
    updateFront();
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
  applyGroupingToWS(m_workspace_name, m_workspace_name+"Grouped");
  updatePairTable();
  updateFrontAndCombo();
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
  else
    m_updating = false;
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

  std::string mainFieldDirection("");
  double timeZero(0.0);
  double firstGoodData(0.0);
  std::vector<double> deadTimes;

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
        m_uiForm.instrSelector->setCurrentIndex(j);
        foundInst = true;
        break;
      }
    }
    if ( !foundInst )
    {
      QMessageBox::warning(this,"Mantid - MuonAnalysis", "Muon file " + filename + " not recognised.");
      deleteRangedWorkspaces();
      m_uiForm.tabWidget->setTabEnabled(3, true);
      return;
    }

    // Setup Load Nexus Algorithm
    Mantid::API::IAlgorithm_sptr loadMuonAlg = Mantid::API::AlgorithmManager::Instance().create("LoadMuonNexus");
    loadMuonAlg->setPropertyValue("Filename", filename.toStdString() );
    if (i > 0)
    {
      QString tempRangeNum;
      tempRangeNum.setNum(i);
      loadMuonAlg->setPropertyValue("OutputWorkspace", m_workspace_name + tempRangeNum.toStdString() );
    }
    else
    {
      loadMuonAlg->setPropertyValue("OutputWorkspace", m_workspace_name);
    }
    loadMuonAlg->setProperty("AutoGroup", false);
    if (loadMuonAlg->execute() )
    {
      mainFieldDirection = loadMuonAlg->getPropertyValue("MainFieldDirection");
      timeZero = loadMuonAlg->getProperty("TimeZero");
      firstGoodData = loadMuonAlg->getProperty("FirstGoodData");
      if (m_uiForm.instrSelector->currentText().toUpper().toStdString() != "ARGUS")
        deadTimes = loadMuonAlg->getProperty("DeadTimes");
    }
    else
    {
      QMessageBox::warning(this,"Mantid - MuonAnalysis", "Problem when executing LoadMuonNexus algorithm.");
      deleteRangedWorkspaces();
      m_uiForm.tabWidget->setTabEnabled(3, true);
      return;
    }
  }

  if (m_previousFilenames.size() > 1)
    plusRangeWorkspaces();

  if (m_uiForm.instrSelector->currentText().toUpper().toStdString() != "ARGUS")
  {
    // Get dead times from data.
    if (m_uiForm.deadTimeType->currentIndex() == 1)
    {
      getDeadTimeFromData(deadTimes);
    }
    // Get dead times from file.
    else if (m_uiForm.deadTimeType->currentIndex() == 2)
    {
      QString deadTimeFile(m_uiForm.mwRunDeadTimeFile->getFirstFilename() );

      try
      {
        getDeadTimeFromFile(deadTimeFile);
      }
      catch (std::exception&)
      {
        QMessageBox::information(this, "Mantid - MuonAnalysis", "A problem occurred while applying dead times.");
      }
    }
  }
  else if (m_uiForm.deadTimeType->currentIndex() != 0)
  {
    QMessageBox::information(this, "Mantid - Muon Analysis", "Dead times are currently not implemented in ARGUS files."
                          + QString("\nAs a result, no dead times will be applied.") );
  }

  // Make the options available
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

  // if grouping not set, first see if grouping defined in Nexus
  if ( !isGroupingSet() )
    setGroupingFromNexus(files[0]);
  // if grouping still not set, then take grouping from IDF
  if ( !isGroupingSet() )
    setGroupingFromIDF(mainFieldDirection, matrix_workspace);
  // finally if nothing else works set dummy grouping and display
  // message to user
  if ( !isGroupingSet() )
    setDummyGrouping(static_cast<int>(matrix_workspace->getInstrument()->getDetectorIDs().size()));

  if ( !applyGroupingToWS(m_workspace_name, m_workspace_name+"Grouped") )
  {
    m_uiForm.tabWidget->setTabEnabled(3, true);
    return;
  }

  // Populate instrument fields
  std::stringstream str;
  str << "Description: ";
  int nDet = static_cast<int>(matrix_workspace->getInstrument()->getDetectorIDs().size());
  str << nDet;
  str << " detector spectrometer, main field ";
  str << QString(mainFieldDirection.c_str()).toLower().toStdString();
  str << " to muon polarisation";
  m_uiForm.instrumentDescription->setText(str.str().c_str());

  m_uiForm.timeZeroFront->setText(QString::number(timeZero, 'g',2));
  // I want the nexus time to equal exactly how it is stored in time zero text box
  // so that later I can check if user has altered it
  m_nexusTimeZero = boost::lexical_cast<double>(m_uiForm.timeZeroFront->text().toStdString());
  m_uiForm.firstGoodBinFront->setText(QString::number(firstGoodData-timeZero,'g',2));

  // since content of first-good-bin changed run this slot
  runFirstGoodBinFront();

  std::string infoStr("");
  
  // Populate run information with the run number
  QString run(getGroupName());
  if (m_previousFilenames.size() > 1)
    infoStr += "Runs: ";
  else
    infoStr += "Run: ";

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
  infoStr += run.toStdString();

  // Populate run information text field
  m_title = matrix_workspace->getTitle();
  infoStr += "\nTitle: ";
  infoStr += m_title;
  
  // Add the comment to run information
  infoStr += "\nComment: ";
  infoStr += matrix_workspace->getComment();
  
  const Run& runDetails = matrix_workspace->run();
  Mantid::Kernel::DateAndTime start, end;

  // Add the start time for the run
  infoStr += "\nStart: ";
  if ( runDetails.hasProperty("run_start") )
  {
    start = runDetails.getProperty("run_start")->value();
    infoStr += runDetails.getProperty("run_start")->value();
  }

  // Add the end time for the run
  infoStr += "\nEnd: ";
  if ( runDetails.hasProperty("run_end") )
  {
    end = runDetails.getProperty("run_end")->value();
    infoStr += runDetails.getProperty("run_end")->value();
  }

  // Add counts to run information
  infoStr += "\nCounts: ";
  double counts(0.0);
  for (size_t i=0; i<matrix_workspace->getNumberHistograms(); ++i)
  {
    for (size_t j=0; j<matrix_workspace->blocksize(); ++j)
    {
      counts += matrix_workspace->dataY(i)[j];
    }
  }
  std::ostringstream ss;
  ss << std::fixed << std::setprecision(12) << counts/1000000;
  infoStr += ss.str();
  infoStr += " MEv";

  // Add average temperature.
  infoStr += "\nAverage Temperature: ";
  if ( runDetails.hasProperty("Temp_Sample") )
  {
    // Filter the temperatures by the start and end times for the run.
    runDetails.getProperty("Temp_Sample")->filterByTime(start, end);
    QString allRuns = QString::fromStdString(runDetails.getProperty("Temp_Sample")->value() );
    QStringList runTemp = allRuns.split("\n");
    int tempCount(0);
    double total(0.0);

    // Go through each temperature entry, remove the date and time, and total the temperatures.
    for (int i=0; i<runTemp.size(); ++i)
    {
      if (runTemp[i].contains("  ") )
      {
        QStringList dateTimeTemperature = runTemp[i].split("  ");
        total += dateTimeTemperature[dateTimeTemperature.size() - 1].toDouble();
        ++tempCount;
      }
    }

    // Find the average and display it.
    double average(total/tempCount);
    if (average != 0.0)
    {
      std::ostringstream ss;
      ss << std::fixed << std::setprecision(12) << average;
      infoStr += ss.str();
    }
    else // Show appropriate error message.
      infoStr += "Errror - Not set in data file.";
  }
  else // Show appropriate error message.
    infoStr += "Errror - Not found in data file.";

  // Include all the run information.
  m_uiForm.infoBrowser->setText(infoStr.c_str());

  // Populate period information
  std::stringstream periodLabel;
  periodLabel << "Data collected in " << numPeriods << " Periods. "
    << "Plot/analyse Period:";
  m_uiForm.homePeriodsLabel->setText(periodLabel.str().c_str());

  while ( m_uiForm.homePeriodBox1->count() != 0 )
    m_uiForm.homePeriodBox1->removeItem(0);
  while ( m_uiForm.homePeriodBox2->count() != 0 )
    m_uiForm.homePeriodBox2->removeItem(0);

  m_uiForm.homePeriodBox2->addItem("None");
  for ( int i = 1; i <= numPeriods; i++ )
  {
    std::stringstream strInt;
    strInt << i;
    m_uiForm.homePeriodBox1->addItem(strInt.str().c_str());
    m_uiForm.homePeriodBox2->addItem(strInt.str().c_str());
  }

  if (wsPeriods)
  {
    m_uiForm.homePeriodBox2->setEnabled(true);
    m_uiForm.homePeriodBoxMath->setEnabled(true);
  }
  else
  {
    m_uiForm.homePeriodBox2->setEnabled(false);
    m_uiForm.homePeriodBoxMath->setEnabled(false);
  }

  // Populate bin width info in Plot options
  double binWidth = matrix_workspace->dataX(0)[1]-matrix_workspace->dataX(0)[0];
  static const QChar MU_SYM(956);
  m_uiForm.optionLabelBinWidth->setText(QString("Data collected with histogram bins of ") + QString::number(binWidth) + QString(" %1s").arg(MU_SYM));

  m_deadTimesChanged = false;

  // finally the preferred default by users are to by default
  // straight away plot the data
  if (m_uiForm.frontPlotButton->isEnabled() )
    runFrontPlotButton();
  
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
* Create a table of dead times and apply them to the data.
*
* @param deadTimes :: a vector of all the dead times starting at spectrum 1.
*/
void MuonAnalysis::getDeadTimeFromData(const std::vector<double> & deadTimes)
{
  int numData(0); // Number of data sets under muon analysis.
  if (Mantid::API::AnalysisDataService::Instance().doesExist(m_workspace_name) )
  {
    ++numData;
    int loop(1);
    while(loop == numData)
    {
      std::stringstream ss; //create a stringstream
      ss << (numData + 1);
      if (Mantid::API::AnalysisDataService::Instance().doesExist(m_workspace_name + '_' + ss.str() ) )
      {
        ++numData;
      }
      ++loop;
    }
  }

  // Setup the dead time table.
  for (int i=1; i<=numData; ++i)
  {
    std::string workspaceName("");
    Mantid::API::ITableWorkspace_sptr deadTimeTable = Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
    deadTimeTable->addColumn("int","spectrum");
    deadTimeTable->addColumn("double","dead-time");

    Mantid::API::MatrixWorkspace_sptr muonData;

    if (i==1 && 1==numData)
    {
      workspaceName = m_workspace_name;
      muonData = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(workspaceName) );
    }
    else
    {
      std::stringstream ss; //create a stringstream
      ss << i;
      workspaceName = m_workspace_name + '_' + ss.str();
      muonData = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(workspaceName) );
    }

    //check dead time size
    if (deadTimes.size() >= (muonData->getNumberHistograms() + (i-1)*muonData->getNumberHistograms() ) )
    {
      for (size_t j=0; j<muonData->getNumberHistograms(); ++j)
      {
        Mantid::API::TableRow row = deadTimeTable->appendRow();
        row << boost::lexical_cast<int>(j+1) << deadTimes[j+((i-1)*muonData->getNumberHistograms() ) ];
      }
    }

    // Add to the ADS for use with algorithm. (Unique name chosen so not to cause conflict)
    Mantid::API::AnalysisDataService::Instance().addOrReplace("tempMuonDeadTime123qwe", deadTimeTable);

    // Setup and run the ApplyDeadTimeCorr algorithm.
    Mantid::API::IAlgorithm_sptr applyDeadTimeAlg = Mantid::API::AlgorithmManager::Instance().create("ApplyDeadTimeCorr");
    applyDeadTimeAlg->setPropertyValue("InputWorkspace", workspaceName );
    applyDeadTimeAlg->setProperty("DeadTimeTable", deadTimeTable);
    applyDeadTimeAlg->setPropertyValue("OutputWorkspace", workspaceName );
    if (!applyDeadTimeAlg->execute())
      throw std::runtime_error("Error in applying dead time.");
    
    // Make sure to remove the table from the ADS because it isn't used anymore.
    Mantid::API::AnalysisDataService::Instance().remove("tempMuonDeadTime123qwe");
  }
}


/**
* Load up a dead time table or a group of dead time tables and apply them to the workspace.
*
* @param fileName :: The file where the dead times are kept.
*/
void MuonAnalysis::getDeadTimeFromFile(const QString & fileName)
{
  Mantid::API::IAlgorithm_sptr loadDeadTimes = Mantid::API::AlgorithmManager::Instance().create("LoadNexusProcessed");
  loadDeadTimes->setPropertyValue("Filename", fileName.toStdString() );
  loadDeadTimes->setPropertyValue("OutputWorkspace", "tempMuonDeadTime123qwe");
  if (loadDeadTimes->execute() )
  {
    Mantid::API::ITableWorkspace_sptr deadTimeTable = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve("tempMuonDeadTime123qwe") );
    if (deadTimeTable)
    {
      // Must be deadtime
      Mantid::API::IAlgorithm_sptr applyDeadTimeAlg = Mantid::API::AlgorithmManager::Instance().create("ApplyDeadTimeCorr");
      applyDeadTimeAlg->setPropertyValue("InputWorkspace", m_workspace_name );
      applyDeadTimeAlg->setProperty("DeadTimeTable", deadTimeTable);
      applyDeadTimeAlg->setPropertyValue("OutputWorkspace", m_workspace_name );
      if (!applyDeadTimeAlg->execute())
        throw std::runtime_error("Error in applying dead time.");
      Mantid::API::AnalysisDataService::Instance().remove("tempMuonDeadTime123qwe");
    }
    else
    {
      // Check to see if it is a group of dead time tables
      Mantid::API::WorkspaceGroup_sptr deadTimeTables = boost::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(Mantid::API::AnalysisDataService::Instance().retrieve("tempMuonDeadTime123qwe") );
      if (deadTimeTables)
      {
        std::vector<std::string> groupNames(deadTimeTables->getNames() );

        size_t numData(0); // Number of data sets under muon analysis.
        if (Mantid::API::AnalysisDataService::Instance().doesExist(m_workspace_name) )
        {
          ++numData;
          size_t loop(1);
          while(loop == numData)
          {
            std::stringstream ss; //create a stringstream
            ss << (numData + 1);
            if (Mantid::API::AnalysisDataService::Instance().doesExist(m_workspace_name + '_' + ss.str() ) )
            {
              ++numData;
            }
            ++loop;
          }
        }

        if (numData == groupNames.size() )
        {
          bool allTables(true);
          for (size_t i=0; i<groupNames.size(); ++i)
          {
            deadTimeTable = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(groupNames[i] ) );
            if (!deadTimeTable)
              allTables = false;
          }
          if (allTables == true)
          {
            for (size_t i=0; i<groupNames.size(); ++i)
            {
              std::string workspaceName("");

              Mantid::API::MatrixWorkspace_sptr muonData;

              if (i==0 && 1==numData)
              {
                workspaceName = m_workspace_name;
              }
              else
              {
                std::stringstream ss; //create a stringstream
                ss << (i+1);
                workspaceName = m_workspace_name + '_' + ss.str();
              }
              deadTimeTable = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(groupNames[i] ) );
              Mantid::API::IAlgorithm_sptr applyDeadTimeAlg = Mantid::API::AlgorithmManager::Instance().create("ApplyDeadTimeCorr");
              applyDeadTimeAlg->setPropertyValue("InputWorkspace", workspaceName );
              applyDeadTimeAlg->setProperty("DeadTimeTable", deadTimeTable);
              applyDeadTimeAlg->setPropertyValue("OutputWorkspace", workspaceName );
              if (!applyDeadTimeAlg->execute())
                throw std::runtime_error("Error in applying dead time.");
            }
          }
        }
      }
      else
      {
        Mantid::API::AnalysisDataService::Instance().remove("tempMuonDeadTime123qwe");
        QMessageBox::information(this, "Mantid - Muon Analysis", "This kind of workspace is not compatible with applying dead times");
        return;
      }
      Mantid::API::AnalysisDataService::Instance().remove("tempMuonDeadTime123qwe");
    }
  }
  else
  {
    Mantid::API::AnalysisDataService::Instance().remove("tempMuonDeadTime123qwe");
    QMessageBox::information(this, "Mantid - Muon Analysis", "Failed to load dead times from the file " + fileName);
    return;
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
    firstFile.chop(firstFile.size()-firstFile.find('.') );

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
  groupTabUpdatePair();
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
  if (numG)
  {
    if (index >= numG && numG >= 2)
    {
      // i.e. index points to a pair
      m_uiForm.frontPlotFuncs->addItems(m_pairPlotFunc);

      m_uiForm.frontAlphaLabel->setVisible(true);
      m_uiForm.frontAlphaNumber->setVisible(true);

      m_uiForm.frontAlphaNumber->setText(m_uiForm.pairTable->item(m_pairToRow[index-numG],3)->text());
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

  updateFront();
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
}


/**
 * Create WS contained the data for a plot
 * Take the MuonAnalysisGrouped WS and reduce(crop) histograms according to Plot Options.
 * If period data then the resulting cropped WS is on for the period, or sum/difference of, selected
 * by the user on the front panel. Also create raw workspace for fitting against if the user wants.
 * @param groupName  WorkspaceGroup name to add created workspace to
 * @param inputWS Name of the input workspace equal unprocessed data, but which have been groupped  
 * @param outWS Name of output workspace created by this method
 */
void MuonAnalysis::createPlotWS(const std::string& groupName, 
                                const std::string& inputWS, const std::string& outWS)
{
  m_loaded = true;
  // adjust for time zero if necessary
  if ( m_nexusTimeZero != timeZero())
  {
    try {
      double shift = m_nexusTimeZero - timeZero();
      Mantid::API::IAlgorithm_sptr rebinAlg = Mantid::API::AlgorithmManager::Instance().create("ChangeBinOffset");
      rebinAlg->setPropertyValue("InputWorkspace", inputWS);
      rebinAlg->setPropertyValue("OutputWorkspace", outWS);
      rebinAlg->setProperty("Offset", shift);
      rebinAlg->execute();    
    }
    catch(...) {
      QMessageBox::information(this, "Mantid - Muon Analysis", "The workspace couldn't be corrected for time zero.");
    }
  }

  Mantid::API::IAlgorithm_sptr cropAlg = Mantid::API::AlgorithmManager::Instance().create("CropWorkspace");
  if ( m_nexusTimeZero != timeZero() )
    cropAlg->setPropertyValue("InputWorkspace", outWS);
  else 
    cropAlg->setPropertyValue("InputWorkspace", inputWS);
  cropAlg->setPropertyValue("OutputWorkspace", outWS);
  cropAlg->setProperty("Xmin", plotFromTime());
  if ( !m_uiForm.timeAxisFinishAtInput->text().isEmpty() )
    cropAlg->setProperty("Xmax", plotToTime());
  cropAlg->execute();

  // Copy the data and keep as raw for later
  m_fitDataTab->makeRawWorkspace(outWS);

  // rebin data if option set in Plot Options
  if (m_uiForm.rebinComboBox->currentIndex() != 0)
  {
    try
    {
      Mantid::API::MatrixWorkspace_sptr tempWs =  boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(outWS));
      std::string rebinParams("");
      double binSize = tempWs->dataX(0)[1]-tempWs->dataX(0)[0];
      if(m_uiForm.rebinComboBox->currentIndex() == 1) // Fixed
      {
        double bunchedBinSize = binSize*m_uiForm.optionStepSizeText->text().toDouble();
        rebinParams = boost::lexical_cast<std::string>(bunchedBinSize);
      }
      else // Variable
      {
        rebinParams = m_uiForm.binBoundaries->text().toStdString();
      }
      // bunch data
      Mantid::API::IAlgorithm_sptr rebinAlg = Mantid::API::AlgorithmManager::Instance().create("Rebin");
      rebinAlg->setPropertyValue("InputWorkspace", outWS);
      rebinAlg->setPropertyValue("OutputWorkspace", outWS);
      rebinAlg->setPropertyValue("Params", rebinParams);
      rebinAlg->execute();

      // however muon group don't want last bin if shorter than previous bins
      tempWs =  boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(outWS));
      binSize = tempWs->dataX(0)[1]-tempWs->dataX(0)[0]; 
      double firstX = tempWs->dataX(0)[0];
      double lastX = tempWs->dataX(0)[tempWs->dataX(0).size()-1];
      double numberOfFullBunchedBins =  std::floor((lastX - firstX) / binSize );

      if ( numberOfFullBunchedBins )
      {
        lastX = firstX + numberOfFullBunchedBins*binSize;

        Mantid::API::IAlgorithm_sptr cropAlg = Mantid::API::AlgorithmManager::Instance().create("CropWorkspace");
        cropAlg->setPropertyValue("InputWorkspace", outWS);
        cropAlg->setPropertyValue("OutputWorkspace", outWS);
        cropAlg->setPropertyValue("Xmax", boost::lexical_cast<std::string>(lastX));
        cropAlg->setProperty("Xmax", lastX);
        cropAlg->execute();
      }

    }
    catch(std::exception&)
    {
      QMessageBox::information(this, "Mantid - Muon Analysis", "The workspace couldn't be rebunched.");
    }
  }

  // Make group to display more organised in Mantidplot workspace list
  if ( !AnalysisDataService::Instance().doesExist(groupName) )
  {
    QString rubbish = "boevsMoreBoevs";
    QString groupStr = rubbish + QString("=CloneWorkspace(InputWorkspace='") + outWS.c_str() + "')\n";
    groupStr += groupName.c_str() + QString("=GroupWorkspaces(InputWorkspaces='") + outWS.c_str() + "," + rubbish
      + "')\n";
    runPythonCode( groupStr ).trimmed();
    AnalysisDataService::Instance().remove(rubbish.toStdString());
  }
  else
  {
    QString groupStr = QString(groupName.c_str()) + QString("=GroupWorkspaces(InputWorkspaces='") + outWS.c_str() + "," + groupName.c_str()
      + "')\n";
    runPythonCode( groupStr ).trimmed();
  }


  // Group the raw workspace
  std::vector<std::string> groupWorkspaces;
  groupWorkspaces.push_back(groupName);
  groupWorkspaces.push_back(outWS + "_Raw");
  m_fitDataTab->groupWorkspaces(groupWorkspaces, groupName);
}


/**
 * Used by plotGroup and plotPair. 
 */
void MuonAnalysis::handlePeriodChoice(const QString wsName, const QStringList& periodLabel, const QString& /*groupName*/)
{
  if ( periodLabel.size() == 2 )
  {
    if ( m_uiForm.homePeriodBoxMath->currentText()=="+" )
    {
      Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("Plus");
      alg->setPropertyValue("LHSWorkspace", wsName.toStdString() + periodLabel.at(0).toStdString());
      alg->setPropertyValue("RHSWorkspace", wsName.toStdString() + periodLabel.at(1).toStdString());
      alg->setPropertyValue("OutputWorkspace", wsName.toStdString() + periodLabel.at(0).toStdString());
      alg->execute();

      alg = Mantid::API::AlgorithmManager::Instance().create("Plus");
      alg->setPropertyValue("LHSWorkspace", wsName.toStdString() + periodLabel.at(0).toStdString() + "_Raw");
      alg->setPropertyValue("RHSWorkspace", wsName.toStdString() + periodLabel.at(1).toStdString() + "_Raw");
      alg->setPropertyValue("OutputWorkspace", wsName.toStdString() + periodLabel.at(0).toStdString() + "_Raw");
      alg->execute();
    }
    else
    {
      Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("Minus");
      alg->setPropertyValue("LHSWorkspace", wsName.toStdString() + periodLabel.at(0).toStdString());
      alg->setPropertyValue("RHSWorkspace", wsName.toStdString() + periodLabel.at(1).toStdString());
      alg->setPropertyValue("OutputWorkspace", wsName.toStdString() + periodLabel.at(0).toStdString());
      alg->execute();

      alg = Mantid::API::AlgorithmManager::Instance().create("Minus");
      alg->setPropertyValue("LHSWorkspace", wsName.toStdString() + periodLabel.at(0).toStdString() + "_Raw");
      alg->setPropertyValue("RHSWorkspace", wsName.toStdString() + periodLabel.at(1).toStdString() + "_Raw");
      alg->setPropertyValue("OutputWorkspace", wsName.toStdString() + periodLabel.at(0).toStdString() + "_Raw");
      alg->execute();
    }

    Mantid::API::AnalysisDataService::Instance().remove((wsName + periodLabel.at(1)).toStdString() );
    Mantid::API::AnalysisDataService::Instance().remove((wsName + periodLabel.at(1) + "_Raw").toStdString() ); 

    Mantid::API::AnalysisDataService::Instance().rename((wsName + periodLabel.at(0)).toStdString(), wsName.toStdString());
    Mantid::API::AnalysisDataService::Instance().rename((wsName + periodLabel.at(0)+"_Raw").toStdString(), (wsName+"_Raw").toStdString()); 
  }
  else
  {
    if ( periodLabel.at(0) != "" )
    {
      Mantid::API::AnalysisDataService::Instance().rename((wsName + periodLabel.at(0)).toStdString(), wsName.toStdString());
      Mantid::API::AnalysisDataService::Instance().rename((wsName + periodLabel.at(0)+"_Raw").toStdString(), (wsName+"_Raw").toStdString());      
    }
  }
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
 * @param wsName workspace name
 * @param wsIndex workspace index
 */
void MuonAnalysis::plotSpectrum(const QString& wsName, const int wsIndex, const bool ylogscale)
{
    // create first part of plotting Python string
    QString gNum = QString::number(wsIndex);
    QString pyS;
    if ( m_uiForm.showErrorBars->isChecked() )
      pyS = "gs = plotSpectrum(\"" + wsName + "\"," + gNum + ",True)\n";
    else
      pyS = "gs = plotSpectrum(\"" + wsName + "\"," + gNum + ")\n";
    // Add the objectName for the peakPickerTool to find
    pyS += "gs.setObjectName(\"" + wsName + "\")\n"
           "l = gs.activeLayer()\n"
           "l.setCurveTitle(0, \"" + wsName + "\")\n"
           "l.setTitle(\"" + m_title.c_str() + "\")\n";
      
    Workspace_sptr ws_ptr = AnalysisDataService::Instance().retrieve(wsName.toStdString());
    MatrixWorkspace_sptr matrix_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(ws_ptr);
    if ( !m_uiForm.yAxisAutoscale->isChecked() )
    {
      const Mantid::MantidVec& dataY = matrix_workspace->readY(wsIndex);
      double min = 0.0; double max = 0.0;

      if (m_uiForm.yAxisMinimumInput->text().isEmpty())
      {
        min = *min_element(dataY.begin(), dataY.end());
      }
      else
      {
        min = boost::lexical_cast<double>(m_uiForm.yAxisMinimumInput->text().toStdString());
      }

      if (m_uiForm.yAxisMaximumInput->text().isEmpty())
      {
        max = *max_element(dataY.begin(), dataY.end());
      }
      else
      {
        max = boost::lexical_cast<double>(m_uiForm.yAxisMaximumInput->text().toStdString());
      }     
      pyS += "l.setAxisScale(Layer.Left," + QString::number(min) + "," + QString::number(max) + ")\n";
    }

    if ( ylogscale )
      pyS += "l.logYlinX()\n";

    // plot the spectrum
    runPythonCode( pyS );
}


/**
 * Plot group
 */
void MuonAnalysis::plotGroup(const std::string& plotType)
{
  if (plotToTime() <= plotFromTime())
    return;

  m_updating = true;

  int groupNum = getGroupNumberFromRow(m_groupTableRowInFocus);
  if ( groupNum >= 0 )
  {
    QTableWidgetItem *itemName = m_uiForm.groupTable->item(m_groupTableRowInFocus,0);
    QString groupName = itemName->text();
    QString wsGroupName(getGroupName());

    // create plot workspace name
    QString plotTypeTitle("");
    if (plotType == "Asymmetry")
    {
      plotTypeTitle = "Asym";
    }
    else
    {
      if(plotType == "Counts")
        plotTypeTitle = "Counts";
      else
        plotTypeTitle = "Logs";
    }
    QString cropWSfirstPart = wsGroupName + "; Group="
      + groupName + "; " + plotTypeTitle + "";

    // decide on name for workspace to be plotted
    QString cropWS(getNewPlotName(cropWSfirstPart));
    
    // curve plot label
    QString titleLabel = cropWS;

    // Should we hide all graphs except for the one specified
    // Note the "-1" is added when a new graph for a WS is created
    // for the first time. The 2nd time a plot is created for the 
    // same WS I believe this number is "-2". Currently do not 
    // fully understand the purpose of this exception here. 
    if (m_uiForm.hideGraphs->isChecked() )
      emit hideGraphs(titleLabel + "-1"); 

    // check if user specified periods - if multiple period data 
    QStringList periodLabel = getPeriodLabels();

    // create the binned etc plot workspace for the first period
    QString cropWS_1 = cropWS + periodLabel.at(0);
    QString cropWS_2 = ""; // user may not have selected a 2nd period in GUI
    createPlotWS(wsGroupName.toStdString(), 
                 m_workspace_name + "Grouped" + periodLabel.at(0).toStdString(), 
                 cropWS_1.toStdString());
    // and the binned etc plot workspace for the second period
    if (periodLabel.size() == 2)
    {
      cropWS_2 = cropWS + periodLabel.at(1);
      createPlotWS(wsGroupName.toStdString(),
                   m_workspace_name + "Grouped" + periodLabel.at(1).toStdString(), 
                   cropWS_2.toStdString());
    }

    // set to true if plot y axis on log scale
    bool plotOnLogScale = false;

    if (plotType.compare("Counts") == 0)
    {
      // nothing to do
    }
    else if (plotType.compare("Asymmetry") == 0)
    {
      Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("RemoveExpDecay");
      alg->setPropertyValue("InputWorkspace", cropWS_1.toStdString());
      alg->setPropertyValue("OutputWorkspace", cropWS_1.toStdString());
      alg->execute();
      alg = Mantid::API::AlgorithmManager::Instance().create("RemoveExpDecay");
      alg->setPropertyValue("InputWorkspace", cropWS_1.toStdString() + "_Raw");
      alg->setPropertyValue("OutputWorkspace", cropWS_1.toStdString() + "_Raw");
      alg->execute();

      if (periodLabel.size() == 2)  
      {    
        alg = Mantid::API::AlgorithmManager::Instance().create("RemoveExpDecay");
        alg->setPropertyValue("InputWorkspace", cropWS_2.toStdString());
        alg->setPropertyValue("OutputWorkspace", cropWS_2.toStdString());
        alg->execute();
        alg = Mantid::API::AlgorithmManager::Instance().create("RemoveExpDecay");
        alg->setPropertyValue("InputWorkspace", cropWS_2.toStdString() + "_Raw");
        alg->setPropertyValue("OutputWorkspace", cropWS_2.toStdString() + "_Raw");
        alg->execute();  
      }
    }
    else if (plotType.compare("Logorithm") == 0)
    {
      // nothing to do since plot as count but with the y-axis set to logarithm scale
      plotOnLogScale = true;
    }
    else
    {
      g_log.error("Unknown group table plot function");
      m_updating = false;
      return;
    }
    
    // If user has specified this do algebra on periods
    // note after running this method you are just left with the processed cropWS (and curresponding _Raw)
    handlePeriodChoice(cropWS, periodLabel, wsGroupName);

    // set the workspace Y Unit label
    Workspace_sptr ws_ptr = AnalysisDataService::Instance().retrieve(cropWS.toStdString());
    MatrixWorkspace_sptr matrix_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(ws_ptr);
    matrix_workspace->setYUnitLabel(plotType);
    ws_ptr = AnalysisDataService::Instance().retrieve((cropWS+"_Raw").toStdString());
    matrix_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(ws_ptr);
    matrix_workspace->setYUnitLabel(plotType);

    // plot the spectrum
    plotSpectrum(cropWS, groupNum, plotOnLogScale);

    // Change the plot style of the graph so that it matches what is selected on
    // the plot options tab.
    QStringList plotDetails = m_fitDataTab->getAllPlotDetails(titleLabel);
    changePlotType(plotDetails);

    m_currentDataName = titleLabel;
    setConnectedDataText();
  }
  m_updating = false;
}

/**
 * Plot pair
 *
 * @param plotType :: Whether it is Asym, count, etc
 */
void MuonAnalysis::plotPair(const std::string& plotType)
{
  if (plotToTime() <= plotFromTime())
    return;

  m_updating = true;

  int pairNum = getPairNumberFromRow(m_pairTableRowInFocus);
  if ( pairNum >= 0 )
  {
    QTableWidgetItem *itemAlpha = m_uiForm.pairTable->item(m_pairTableRowInFocus,3);
    QTableWidgetItem *itemName = m_uiForm.pairTable->item(m_pairTableRowInFocus,0);
    QString pairName = itemName->text();
    QString wsGroupName(getGroupName());

    // create plot workspace name
    QString plotTypeTitle("");
    if (plotType == "Asymmetry")
    {
      plotTypeTitle = "Asym";
    }    
    QString cropWSfirstPart = wsGroupName + "; Group=" + pairName + "; " + plotTypeTitle + "";
    
    // decide on name for workspace to be plotted
    QString cropWS(getNewPlotName(cropWSfirstPart));

    // curve plot label
    QString titleLabel = cropWS;

    // Should we hide all graphs except for the one specified
    // Note the "-1" is added when a new graph for a WS is created
    // for the first time. The 2nd time a plot is created for the 
    // same WS I believe this number is "-2". Currently do not 
    // fully understand the purpose of this exception here. 
    if (m_uiForm.hideGraphs->isChecked() )
      emit hideGraphs(titleLabel + "-1"); 

    // check if user specified periods - if multiple period data 
    QStringList periodLabel = getPeriodLabels();
    
    // create the binned etc plot workspace for the first period
    QString cropWS_1 = cropWS + periodLabel.at(0);
    QString cropWS_2 = ""; // user may not have selected a 2nd period in GUI
    createPlotWS(wsGroupName.toStdString(), 
                 m_workspace_name + "Grouped" + periodLabel.at(0).toStdString(), 
                 cropWS_1.toStdString());
    // and the binned etc plot workspace for the second period
    if (periodLabel.size() == 2)
    {
      cropWS_2 = cropWS + periodLabel.at(1);
      createPlotWS(wsGroupName.toStdString(),
                   m_workspace_name + "Grouped" + periodLabel.at(1).toStdString(), 
                   cropWS_2.toStdString());
    }

    if (plotType.compare("Asymmetry") == 0)
    {
      QComboBox* qw1 = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(m_pairTableRowInFocus,1));
      QComboBox* qw2 = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(m_pairTableRowInFocus,2));

      Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("AsymmetryCalc");
      alg->setPropertyValue("InputWorkspace", cropWS_1.toStdString());
      alg->setPropertyValue("OutputWorkspace", cropWS_1.toStdString());
      alg->setPropertyValue("ForwardSpectra", QString::number(qw1->currentIndex()).toStdString());
      alg->setPropertyValue("BackwardSpectra", QString::number(qw2->currentIndex()).toStdString());
      alg->setPropertyValue("Alpha", itemAlpha->text().toStdString());
      alg->execute();
      alg = Mantid::API::AlgorithmManager::Instance().create("AsymmetryCalc");
      alg->setPropertyValue("InputWorkspace", cropWS_1.toStdString() + "_Raw");
      alg->setPropertyValue("OutputWorkspace", cropWS_1.toStdString() + "_Raw");
      alg->setPropertyValue("ForwardSpectra", QString::number(qw1->currentIndex()).toStdString());
      alg->setPropertyValue("BackwardSpectra", QString::number(qw2->currentIndex()).toStdString());
      alg->setPropertyValue("Alpha", itemAlpha->text().toStdString());
      alg->execute();

      if (periodLabel.size() == 2)  
      {    
        Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("AsymmetryCalc");
        alg->setPropertyValue("InputWorkspace", cropWS_2.toStdString());
        alg->setPropertyValue("OutputWorkspace", cropWS_2.toStdString());
        alg->setPropertyValue("ForwardSpectra", QString::number(qw1->currentIndex()).toStdString());
        alg->setPropertyValue("BackwardSpectra", QString::number(qw2->currentIndex()).toStdString());
        alg->setPropertyValue("Alpha", itemAlpha->text().toStdString());
        alg->execute();
        alg = Mantid::API::AlgorithmManager::Instance().create("AsymmetryCalc");
        alg->setPropertyValue("InputWorkspace", cropWS_2.toStdString() + "_Raw");
        alg->setPropertyValue("OutputWorkspace", cropWS_2.toStdString() + "_Raw");
        alg->setPropertyValue("ForwardSpectra", QString::number(qw1->currentIndex()).toStdString());
        alg->setPropertyValue("BackwardSpectra", QString::number(qw2->currentIndex()).toStdString());
        alg->setPropertyValue("Alpha", itemAlpha->text().toStdString());
        alg->execute(); 
      }
    }
    else
    {
      g_log.error("Unknown pair table plot function");
      m_updating = false;
      return;
    }

    // If user has specified this do algebra on periods
    // note after running this method you are just left with the processed cropWS (and curresponding _Raw)
    handlePeriodChoice(cropWS, periodLabel, wsGroupName);

    // set the workspace Y Unit label
    Workspace_sptr ws_ptr = AnalysisDataService::Instance().retrieve(cropWS.toStdString());
    MatrixWorkspace_sptr matrix_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(ws_ptr);
    matrix_workspace->setYUnitLabel(plotType);
    ws_ptr = AnalysisDataService::Instance().retrieve((cropWS+"_Raw").toStdString());
    matrix_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(ws_ptr);
    matrix_workspace->setYUnitLabel(plotType);
    
    // plot the spectrum
    plotSpectrum(cropWS, 0);

    // Change the plot style of the graph so that it matches what is selected on
    // the plot options tab. Default is set to line (0).
    QStringList plotDetails = m_fitDataTab->getAllPlotDetails(titleLabel);
    changePlotType(plotDetails);
    
    m_currentDataName = titleLabel;
    setConnectedDataText();
  }
  m_updating = false;
}

QString MuonAnalysis::getNewPlotName(const QString & cropWSfirstPart)
{
  // check if this workspace already exist to avoid replotting an existing workspace
  QString cropWS("");
  int plotNum = 1;
  while (1==1)
  {
    cropWS = cropWSfirstPart + "; #" + boost::lexical_cast<std::string>(plotNum).c_str();
    if ( AnalysisDataService::Instance().doesExist(cropWS.toStdString()) ) 
    {
      if((m_uiForm.plotCreation->currentIndex() == 0) || (m_uiForm.plotCreation->currentIndex() == 2) )
      {
        emit closeGraph(cropWS + "-1");
        AnalysisDataService::Instance().remove(cropWS.toStdString());
        break;
      }
      else
        plotNum++;
    }
    else
    {
      break;
    }
  }
  return cropWS;
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
 * Apply grouping specified in xml file to workspace
 *
 * @param inputWS :: The input workspace to apply grouping
 * @param outputWS :: The resulting workspace
 * @param filename :: Name of grouping file
 */
bool MuonAnalysis::applyGroupingToWS( const std::string& inputWS,  const std::string& outputWS,
   const std::string& filename)
{
  if ( AnalysisDataService::Instance().doesExist(inputWS) )
  {
    AnalysisDataService::Instance().remove(outputWS);

    Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("GroupDetectors");
    alg->setPropertyValue("InputWorkspace", inputWS);
    alg->setPropertyValue("OutputWorkspace", outputWS);
    alg->setPropertyValue("MapFile", filename);
    try
    {
      alg->execute();
      return true;
    }
    catch(...)
    {
      m_optionTab->noDataAvailable();
      QMessageBox::warning(this, "MantidPlot - MuonAnalysis", "Can't group data file according to group-table. Plotting disabled.");
      return false;
    }
  }
  return false;
}

/**
 * Apply whatever grouping is specified in GUI tables to workspace.
 */
bool MuonAnalysis::applyGroupingToWS( const std::string& inputWS,  const std::string& outputWS)
{
  if ( isGroupingSet() && AnalysisDataService::Instance().doesExist(inputWS) )
  {

    std::string complaint = isGroupingAndDataConsistent();
    if (!( complaint.empty() ) )
    {
      if (m_uiForm.frontPlotButton->isEnabled() )
        QMessageBox::warning(this, "MantidPlot - MuonAnalysis", complaint.c_str());
      m_optionTab->noDataAvailable();
      return false;
    }
    {
      if (!m_uiForm.frontPlotButton->isEnabled() )
        m_optionTab->nowDataAvailable();
    }

    saveGroupingTabletoXML(m_uiForm, m_groupingTempFilename);
    return applyGroupingToWS(inputWS, outputWS, m_groupingTempFilename);
  }
  return false;
}

/**
 * Calculate number of detectors from string of type 1-3, 5, 10-15
 *
 * @param str :: String of type "1-3, 5, 10-15"
 * @return Number of detectors. Return 0 if not recognised
 */
int MuonAnalysis::numOfDetectors(const std::string& str) const
{
  return static_cast<int>(spectrumIDs(str).size());
}


/**
 * Return a vector of IDs for row number from string of type 1-3, 5, 10-15
 *
 * @param str :: String of type "1-3, 5, 10-15"
 * @return Vector of IDs
 */
std::vector<int> MuonAnalysis::spectrumIDs(const std::string& str) const
{
  std::vector<int> retVal;

  if (str.empty())
    return retVal;

  typedef Poco::StringTokenizer tokenizer;
  tokenizer values(str, ",", tokenizer::TOK_TRIM);

  for (int i = 0; i < static_cast<int>(values.count()); i++)
  {
    std::size_t found= values[i].find("-");
    if (found!=std::string::npos)
    {
      tokenizer aPart(values[i], "-", tokenizer::TOK_TRIM);

      if ( aPart.count() != 2 )
      {
        retVal.clear();
        return retVal;
      }
      else
      {
        if ( !(isNumber(aPart[0]) && isNumber(aPart[1])) )
        {
          retVal.clear();
          return retVal;
        }
      }

      int leftInt;
      std::stringstream leftRead(aPart[0]);
      leftRead >> leftInt;
      int rightInt;
      std::stringstream rightRead(aPart[1]);
      rightRead >> rightInt;

      if (leftInt > rightInt)
      {
        retVal.clear();
        return retVal;
      }
      for (int step = leftInt; step <= rightInt; step++)
        retVal.push_back(step);
    }
    else
    {

      if (isNumber(values[i]))
        retVal.push_back(boost::lexical_cast<int>(values[i].c_str()));
      else
      {
        retVal.clear();
        return retVal;
      }
    }
  }
  return retVal;
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

  // Only allow numbers in the time zero text box
  m_uiForm.timeZeroFront->setValidator(new QDoubleValidator(m_uiForm.timeZeroFront));

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
* set grouping in table from information from nexus raw file
*/
void MuonAnalysis::setGroupingFromNexus(const QString& nexusFile)
{
  // for now do try to set grouping from nexus file if it is already set
  if ( isGroupingSet() )
    return;

  std::string groupedWS = m_workspace_name+"Grouped";

  // Setup Load Nexus Algorithm
  Mantid::API::IAlgorithm_sptr loadMuonAlg = Mantid::API::AlgorithmManager::Instance().create("LoadMuonNexus");
  loadMuonAlg->setPropertyValue("Filename", nexusFile.toStdString());
  loadMuonAlg->setPropertyValue("OutputWorkspace", groupedWS);
  loadMuonAlg->setProperty("AutoGroup", true);
  if (! (loadMuonAlg->execute() ) )
  {
    QMessageBox::warning(this,"Mantid - MuonAnalysis", "Problem when executing LoadMuonNexus algorithm.");
  }

  // get hold of a matrix-workspace. If period data assume each period has
  // the same grouping
  Workspace_sptr ws_ptr = AnalysisDataService::Instance().retrieve(groupedWS);
  WorkspaceGroup_sptr wsPeriods = boost::dynamic_pointer_cast<WorkspaceGroup>(ws_ptr);
  MatrixWorkspace_sptr matrix_workspace;
  if (wsPeriods)
  {
    Workspace_sptr ws_ptr1 = AnalysisDataService::Instance().retrieve(groupedWS + "_1");
    matrix_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(ws_ptr1);
  }
  else
  {
    matrix_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(ws_ptr);
  }

  // check if there is any grouping in file
  bool thereIsGrouping = false;
  int numOfHist = static_cast<int>(matrix_workspace->getNumberHistograms()); //Qt has no size_t understanding
  for (int wsIndex = 0; wsIndex < numOfHist; wsIndex++)
  {
    IDetector_const_sptr det;
    try // for some bizarry reason when reading EMUautorun_A.tmp this
        // underlying nexus file think there are more histogram than there is
        // hence the reason for this try/catch here
    {
      det = matrix_workspace->getDetector(wsIndex);
    }
    catch (...)
    {
      break;
    }

    if( boost::dynamic_pointer_cast<const DetectorGroup>(det) )
    {
      // prepare IDs string

      boost::shared_ptr<const DetectorGroup> detG = boost::dynamic_pointer_cast<const DetectorGroup>(det);
      std::vector<Mantid::detid_t> detIDs = detG->getDetectorIDs();
      if (detIDs.size() > 1)
      {
        thereIsGrouping = true;
        break;
      }
    }
  }

  // if no grouping in nexus then return
  if ( thereIsGrouping == false )
  {
    return;
  }

  // Add info about grouping from Nexus file to group table
  for (int wsIndex = 0; wsIndex < numOfHist; wsIndex++)
  {
    IDetector_const_sptr det = matrix_workspace->getDetector(wsIndex);

    if( boost::dynamic_pointer_cast<const DetectorGroup>(det) )
    {
      // prepare IDs string

      boost::shared_ptr<const DetectorGroup> detG = boost::dynamic_pointer_cast<const DetectorGroup>(det);
      std::vector<Mantid::detid_t> detIDs = detG->getDetectorIDs();
      std::stringstream idstr;
      int leftInt = detIDs[0];  // meaning left as in the left number of the range 8-18 for instance
      int numIDs = static_cast<int>(detIDs.size());
      idstr << detIDs[0];
      for (int i = 1; i < numIDs; i++)
      {
        if (detIDs[i] != detIDs[i-1]+1 )
        {
          if (detIDs[i-1] == leftInt)
          {
              idstr << ", " << detIDs[i];
              leftInt = detIDs[i];
          }
          else
            {
              idstr << "-" << detIDs[i-1] << ", " << detIDs[i];
              leftInt = detIDs[i];
            }
          }
        else if ( i == numIDs-1 )
        {
          idstr << "-" << detIDs[i];
        }
      }

      // prepare group name string

      std::stringstream gName;
      gName << wsIndex;

      // create table row
      QTableWidgetItem* it = m_uiForm.groupTable->item(wsIndex, 0);
      if (it)
        it->setText(gName.str().c_str());
      else
      {
        m_uiForm.groupTable->setItem(wsIndex, 0, new QTableWidgetItem(gName.str().c_str()));
      }

      it = m_uiForm.groupTable->item(wsIndex, 1);
      if (it)
        it->setText(idstr.str().c_str());
      else
        m_uiForm.groupTable->setItem(wsIndex, 1, new QTableWidgetItem(idstr.str().c_str()));
    }
  }  // end loop over wsIndex

  // check if exactly two groups added in which case assume these are forward/backward groups
  // and automatically then create a pair from which, where the first group is assumed to be
  // the forward group

  updatePairTable();
  if ( numGroups() == 2 && numPairs() <= 0 )
  {
      QTableWidgetItem* it = m_uiForm.pairTable->item(0, 0);
      if (it)
        it->setText("pair");
      else
      {
        m_uiForm.pairTable->setItem(0, 0, new QTableWidgetItem("long"));
      }
      it = m_uiForm.pairTable->item(0, 3);
      if (it)
        it->setText("1.0");
      else
      {
        m_uiForm.pairTable->setItem(0, 3, new QTableWidgetItem("1.0"));
      }
      updatePairTable();
      updateFrontAndCombo();
      m_uiForm.frontGroupGroupPairComboBox->setCurrentIndex(2);
      runFrontGroupGroupPairComboBox(2);
  }
  updatePairTable();
  updateFrontAndCombo();
}


/**
 * If nothing else work set dummy grouping and display comment to user
 */
void MuonAnalysis::setDummyGrouping(const int numDetectors)
{
  // if no grouping in nexus then set dummy grouping and display warning to user
  std::stringstream idstr;
  idstr << "1-" << numDetectors;
  m_uiForm.groupTable->setItem(0, 0, new QTableWidgetItem("NoGroupingDetected"));
  m_uiForm.groupTable->setItem(0, 1, new QTableWidgetItem(idstr.str().c_str()));

  updateFrontAndCombo();

  QMessageBox::warning(this, "MantidPlot - MuonAnalysis", QString("No grouping detected in Nexus file.\n")
    + "and no default grouping file specified in IDF\n"
    + "therefore dummy grouping created.");
}


/**
 * Try to load default grouping file specified in IDF
 */
void MuonAnalysis::setGroupingFromIDF(const std::string& mainFieldDirection, MatrixWorkspace_sptr matrix_workspace)
{
  Instrument_const_sptr inst = matrix_workspace->getInstrument();

  QString instname = m_uiForm.instrSelector->currentText().toUpper();

  QString groupParameter = "Default grouping file";
  // for now hard coded in the special case of MUSR
  if (instname == "MUSR")
  {
    if ( mainFieldDirection == "Transverse" )
      groupParameter += " - Transverse";
    else
      groupParameter += " - Longitudinal";
  }

  std::vector<std::string> groupFile = inst->getStringParameter(groupParameter.toStdString());

  // get search directory for XML instrument definition files (IDFs)
  std::string directoryName = ConfigService::Instance().getInstrumentDirectory();

  if ( groupFile.size() == 1 )
  {
    try
    {
      loadGroupingXMLtoTable(m_uiForm, directoryName+groupFile[0]);
    }
    catch (...)
    {
      QMessageBox::warning(this, "MantidPlot - MuonAnalysis", QString("Can't load default grouping file in IDF.\n")
        + "with name: " + groupFile[0].c_str());
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
  catch(boost::bad_lexical_cast)
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
  double retVal;
  try
  {
    retVal = boost::lexical_cast<double>(m_uiForm.timeAxisStartAtInput->text().toStdString());
  }
  catch (...)
  {
    retVal = 0.0;
    QMessageBox::warning(this,"Mantid - MuonAnalysis", "Number not recognised in Plot Option 'Start at (ms)' input box. Plot from time zero.");
  }
  return retVal;
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
  std::vector<int> idsNew = spectrumIDs(item->text().toStdString());

  int numG = numGroups();
  int rowInFocus = getGroupNumberFromRow(row);
  for (int iG = 0; iG < numG; iG++)
  {
    if (iG != rowInFocus)
    {
      std::vector<int> ids = spectrumIDs(m_uiForm.groupTable->item(m_groupToRow[iG],1)->text().toStdString());

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
  m_uiForm.binBoundaries->setText(prevPlotBinning.value("rebinVariable", 1).toCString());

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
  if (deadTimeTypeIndex != 2)
    m_uiForm.mwRunDeadTimeFile->setVisible(false);
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

  int fileExtensionSize(currentFiles[fileNumber].size()-currentFiles[fileNumber].find('.') );
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

  int fileExtensionSize(currentFile.size()-currentFile.find('.') );
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
* Everytime the tab is changed this is called to decide whether the peakpicker
* tool needs to be associated with a plot or deleted from a plot
*
* @param tabNumber :: The index value of the current tab (3 = data analysis)
*/
void MuonAnalysis::changeTab(int tabNumber)
{
  // Make sure all toolbars are still not visible. May have brought them back to do a plot.
  if (m_uiForm.hideToolbars->isChecked())
    emit hideToolbars();

  m_tabNumber = tabNumber;
  m_uiForm.fitBrowser->setStartX(m_uiForm.timeAxisStartAtInput->text().toDouble());
  m_uiForm.fitBrowser->setEndX(m_uiForm.timeAxisFinishAtInput->text().toDouble());

  // If data analysis tab is chosen by user, assign peak picker tool to the current data if not done so already.
  if (tabNumber == 3)
  {
    m_assigned = false;
    // Update the peak picker tool with the current workspace.
    m_uiForm.fitBrowser->updatePPTool(m_currentDataName);
  }
  else
  {
    if (tabNumber == 4)
    {
      m_resultTableTab->populateTables(m_uiForm.fitBrowser->getWorkspaceNames());
    }
    // delete the peak picker tool because it is no longer needed.
    emit fittingRequested(m_uiForm.fitBrowser, "");
  }
}


/**
*   Emits a signal containing the fitBrowser and the name of the
*   workspace we want to attach a peak picker tool to
*
*   @param workspaceName :: The QString name of the workspace the user wishes
*   to attach a plot picker tool to.
*/
void MuonAnalysis::assignPeakPickerTool(const QString & workspaceName)
{
  if ((m_tabNumber == 3 && !m_assigned) || (m_tabNumber == 3 && m_currentDataName != workspaceName))
  {
    m_assigned = true;
    m_currentDataName = workspaceName;
    emit fittingRequested(m_uiForm.fitBrowser, workspaceName + "-1");
  }
}


/**
* Set up the signals and slots for auto updating the plots
*/
void MuonAnalysis::connectAutoUpdate()
{
  // Home tab Auto Updates
  connect(m_uiForm.firstGoodBinFront, SIGNAL(returnPressed ()), this, SLOT(homeTabUpdatePlot()));
  connect(m_uiForm.homePeriodBox1, SIGNAL(currentIndexChanged(int)), this, SLOT(firstPeriodSelectionChanged()));
  connect(m_uiForm.homePeriodBoxMath, SIGNAL(currentIndexChanged(int)), this, SLOT(homeTabUpdatePlot()));
  connect(m_uiForm.homePeriodBox2, SIGNAL(currentIndexChanged(int)), this, SLOT(secondPeriodSelectionChanged()));
  connect(m_uiForm.frontPlotFuncs, SIGNAL(currentIndexChanged(int)), this, SLOT(changeHomeFunction()));
  connect(m_uiForm.frontAlphaNumber, SIGNAL(returnPressed ()), this, SLOT(homeTabUpdatePlot()));

  // Grouping tab Auto Updates
  // Group Table
  connect(m_uiForm.groupTablePlotChoice, SIGNAL(currentIndexChanged(int)), this, SLOT(groupTabUpdateGroup()));
  // Pair Table (Guess Alpha button attached in another function)
  connect(m_uiForm.pairTablePlotChoice, SIGNAL(currentIndexChanged(int)), this, SLOT(groupTabUpdatePair()));

  // Settings tab Auto Updates
  connect(m_uiForm.binBoundaries, SIGNAL(editingFinished()), this, SLOT(settingsTabUpdatePlot()));
  connect(m_uiForm.optionStepSizeText, SIGNAL(editingFinished()), this, SLOT(settingsTabUpdatePlot()));

  connect(m_optionTab, SIGNAL(settingsTabUpdatePlot()), this, SLOT(settingsTabUpdatePlot()));
}

void MuonAnalysis::changeHomeFunction()
{
  if (m_tabNumber == 0)
  {
    m_uiForm.groupTablePlotChoice->setCurrentIndex(m_uiForm.frontPlotFuncs->currentIndex());
    homeTabUpdatePlot();
  }
}

void MuonAnalysis::firstPeriodSelectionChanged()
{
  if ( m_uiForm.homePeriodBox2->currentText() == m_uiForm.homePeriodBox1->currentText() )
  {
    m_uiForm.homePeriodBox2->setCurrentIndex(0);
  }
  else
    homeTabUpdatePlot();
}

void MuonAnalysis::secondPeriodSelectionChanged()
{
  if ( m_uiForm.homePeriodBox2->currentText() == m_uiForm.homePeriodBox1->currentText() )
  {
    m_uiForm.homePeriodBox2->setCurrentIndex(0);
  }
  else
    homeTabUpdatePlot();
}


void MuonAnalysis::homeTabUpdatePlot()
{
  int choice(m_uiForm.plotCreation->currentIndex());
  if ((choice == 0 || choice == 1) && (!m_updating) && (m_tabNumber == 0) )
  {
    if (m_loaded == true)
      runFrontPlotButton();
  }
}

void MuonAnalysis::groupTabUpdateGroup()
{
  if (m_tabNumber == 1)
  {
    if (m_uiForm.frontPlotFuncs->count() <= 1)
    {
      m_uiForm.frontGroupGroupPairComboBox->setCurrentIndex(0);
      updateFront();
    }
    m_uiForm.frontPlotFuncs->setCurrentIndex(m_uiForm.groupTablePlotChoice->currentIndex());
    int choice(m_uiForm.plotCreation->currentIndex()); 
    if ((choice == 0 || choice == 1) && (!m_updating) )
    {
      if (m_loaded == true)
        runGroupTablePlotButton();
    }
  }
}

void MuonAnalysis::groupTabUpdatePair()
{
  if (m_tabNumber == 1)
  {
    int choice(m_uiForm.plotCreation->currentIndex());
    if ((choice == 0 || choice == 1) && (!m_updating) )
    {
      if (m_loaded == true)
        runPairTablePlotButton();
    }
  }
}

void MuonAnalysis::settingsTabUpdatePlot()
{
  int choice(m_uiForm.plotCreation->currentIndex());
  if ((choice == 0 || choice == 1) && (!m_updating) && (m_tabNumber == 2) )
  {
    if (m_loaded == true)
      runFrontPlotButton();
  }
}


/**
* Re-open the toolbars after closing MuonAnalysis
*/
void MuonAnalysis::closeEvent(QCloseEvent *e)
{
  // Show the toolbar
  if (m_uiForm.hideToolbars->isChecked())
    emit showToolbars();
  // delete the peak picker tool because it is no longer needed.
  emit fittingRequested(m_uiForm.fitBrowser, "");
  e->accept();
}


/**
* Hide the toolbar after opening MuonAnalysis if setting requires it
*/
void MuonAnalysis::showEvent(QShowEvent *e)
{
  const std::string facility = ConfigService::Instance().getFacility().name();
  if (facility != "ISIS")
  {
    QMessageBox::critical(this, "Unsupported facility", QString("Only the ISIS facility is supported by this interface.\n")
                         + "Select ISIS as your default facility in View->Preferences...->Mantid to continue.");
    m_uiForm.loadCurrent->setDisabled(true);
  }
  else
  {
    m_uiForm.loadCurrent->setDisabled(false);
  }
  // Hide the toolbar
  if (m_uiForm.hideToolbars->isChecked() )
    emit hideToolbars();
  e->accept();
}


/**
* Show/Hide Toolbar
*/
void MuonAnalysis::showHideToolbars(bool state)
{
  if (state == true)
    emit hideToolbars();
  else
    emit showToolbars();
}


/**
* Change what type of deadtime to use and the options available for the user's choice.
*
* @param choice :: The current index of dead time type combo box.
*/
void MuonAnalysis::changeDeadTimeType(int choice)
{
  m_deadTimesChanged = true;
  if (choice == 0 || choice == 1) // if choice == none ||choice == from file
  {
    m_uiForm.mwRunDeadTimeFile->setVisible(false);
    homeTabUpdatePlot();
  }
  else // choice must be from workspace
  {
    m_uiForm.mwRunDeadTimeFile->setText("");
    m_uiForm.mwRunDeadTimeFile->setVisible(true);
  }

  QSettings group;
  group.beginGroup(m_settingsGroup + "DeadTimeOptions");
  group.setValue("deadTimes", choice);
}


/**
* If the user selects/changes the file to be used to apply the dead times then 
* see if the plot needs updating and make sure next time the user plots that the
* dead times are applied.
*/
void MuonAnalysis::deadTimeFileSelected()
{
  m_deadTimesChanged = true;
  homeTabUpdatePlot();
}


}//namespace MantidQT
}//namespace CustomInterfaces
