//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/MuonAnalysis.h"
#include "MantidQtCustomInterfaces/IO_MuonGrouping.h"
#include "MantidQtAPI/FileDialogHandler.h"

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/Run.h"
#include "MantidGeometry/IInstrument.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/V3D.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/Instrument/XMLlogfile.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include <Poco/StringTokenizer.h>

#include "Poco/File.h"
#include "Poco/Path.h"

#include <algorithm>

#include <QLineEdit>
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
#include <QClipboard>
#include <QTemporaryFile>
#include <QDateTime>

#include <QDesktopServices>
#include <QUrl>

#include "boost/lexical_cast.hpp"

//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
namespace CustomInterfaces
{
  DECLARE_SUBWINDOW(MuonAnalysis);

using namespace Mantid::Kernel;
using namespace MantidQt::MantidWidgets;
using namespace MantidQt::CustomInterfaces;
using namespace MantidQt::CustomInterfaces::Muon;
using namespace Mantid::API;
using namespace Mantid::Geometry;

// Initialize the logger
Logger& MuonAnalysis::g_log = Logger::get("MuonAnalysis");

//----------------------
// Public member functions
//----------------------
///Constructor
MuonAnalysis::MuonAnalysis(QWidget *parent) :
  UserSubWindow(parent), m_last_dir(), m_workspace_name("MuonAnalysis"), m_groupTableRowInFocus(0), m_pairTableRowInFocus(0),
  m_groupNames(), m_groupingTempFilename("tempMuonAnalysisGrouping.xml")
{
}

/// Set up the dialog layout
void MuonAnalysis::initLayout()
{
  m_uiForm.setupUi(this);

  // Further set initial look
  startUpLook();


  // connect exit button
  //connect(m_uiForm.exitButton, SIGNAL(clicked()), this, SLOT(exitClicked())); 

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

  // connect "?" (Help) Button
  connect(m_uiForm.muonAnalysisHelp, SIGNAL(clicked()), this, SLOT(muonAnalysisHelpClicked()));
  connect(m_uiForm.muonAnalysisHelpGrouping, SIGNAL(clicked()), this, SLOT(muonAnalysisHelpGroupingClicked()));

  // add combo boxes to pairTable
  for (int i = 0; i < m_uiForm.pairTable->rowCount(); i++)
  {
    m_uiForm.pairTable->setCellWidget(i,1, new QComboBox);
    m_uiForm.pairTable->setCellWidget(i,2, new QComboBox);
  }

  QString filter;
  filter.append("Files (*.NXS *.nxs)");
  filter.append(";;All Files (*.*)");

  // file input 
  connect(m_uiForm.mwRunFiles, SIGNAL(fileEditingFinished()), this, SLOT(inputFileChanged()));

  // Input check for First Good Data
  connect(m_uiForm.firstGoodBinFront, SIGNAL(lostFocus()), this, 
    SLOT(runFirstGoodBinFront()));

  ////////////// Plot Option ///////////////
  connect(m_uiForm.timeComboBox, SIGNAL(currentIndexChanged(int)), this, 
    SLOT(runTimeComboBox(int)));
  connect(m_uiForm.timeAxisStartAtInput, SIGNAL(lostFocus()), this, 
    SLOT(runTimeAxisStartAtInput()));
  connect(m_uiForm.timeAxisFinishAtInput, SIGNAL(lostFocus()), this, 
    SLOT(runTimeAxisFinishAtInput()));
  connect(m_uiForm.yAxisAutoscale, SIGNAL(toggled(bool)), this, SLOT(runyAxisAutoscale(bool)));
  connect(m_uiForm.yAxisMinimumInput, SIGNAL(lostFocus()), this, 
    SLOT(runyAxisMinimumInput()));
  connect(m_uiForm.yAxisMaximumInput, SIGNAL(lostFocus()), this, 
    SLOT(runyAxisMaximumInput()));
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
* Front group/ group pair combobox (slot)
*/
void MuonAnalysis::runFrontGroupGroupPairComboBox(int index)
{
  if ( index >= 0 )
    updateFront();
}


/**
* According to Plot Options what is the time to plot from in ms (slot)
*/
void MuonAnalysis::runTimeComboBox(int index)
{
  if ( index == 0 ) // Start at Time Zero
  {
    m_uiForm.timeAxisStartAtInput->setEnabled(false);
    m_uiForm.timeAxisStartAtInput->setText("0");
  }

  if ( index == 1 ) // Start at First Good Data
  {
    m_uiForm.timeAxisStartAtInput->setEnabled(true);
    m_uiForm.timeAxisStartAtInput->setText(m_uiForm.firstGoodBinFront->text());
  }
}

/**
* Check input is valid in input box (slot)
*/
void MuonAnalysis::runTimeAxisStartAtInput()
{
  try 
  {
    double boevs = boost::lexical_cast<double>(m_uiForm.timeAxisStartAtInput->text().toStdString());
  }
  catch (...)
  {
    QMessageBox::warning(this,"Mantid - MuonAnalysis", "Number not recognised in Plot Option 'Start at (ns)' input box. Reset to zero.");
    m_uiForm.timeAxisStartAtInput->setText("0");
  }
}


/**
* Check input is valid in input box (slot)
*/
void MuonAnalysis::runFirstGoodBinFront()
{
  try 
  {
    double boevs = boost::lexical_cast<double>(m_uiForm.firstGoodBinFront->text().toStdString());
  }
  catch (...)
  {
    QMessageBox::warning(this,"Mantid - MuonAnalysis", "Number not recognised in First Good Data (ns)' input box. Reset to 300.");
    m_uiForm.firstGoodBinFront->setText("300");
  }
}


/**
* Check input is valid in input box (slot)
*/
void MuonAnalysis::runTimeAxisFinishAtInput()
{
  if (m_uiForm.timeAxisFinishAtInput->text().isEmpty())
    return;

  try 
  {
    double boevs = boost::lexical_cast<double>(m_uiForm.timeAxisFinishAtInput->text().toStdString());
  }
  catch (...)
  {
    QMessageBox::warning(this,"Mantid - MuonAnalysis", "Number not recognised in Plot Option 'Finish at (ns)' input box.");
    m_uiForm.timeAxisFinishAtInput->setText("");
  }
}


/**
* Check input is valid in input box (slot)
*/
void MuonAnalysis::runyAxisMinimumInput()
{
  if (m_uiForm.yAxisMinimumInput->text().isEmpty())
    return;

  try 
  {
    double boevs = boost::lexical_cast<double>(m_uiForm.yAxisMinimumInput->text().toStdString());
  }
  catch (...)
  {
    QMessageBox::warning(this,"Mantid - MuonAnalysis", "Number not recognised in Plot Option 'Minimum:' input box.");
    m_uiForm.yAxisMinimumInput->setText("");
  }
}


/**
* Check input is valid in input box (slot)
*/
void MuonAnalysis::runyAxisMaximumInput()
{
  if (m_uiForm.yAxisMaximumInput->text().isEmpty())
    return;

  try 
  {
    double boevs = boost::lexical_cast<double>(m_uiForm.yAxisMaximumInput->text().toStdString());
  }
  catch (...)
  {
    QMessageBox::warning(this,"Mantid - MuonAnalysis", "Number not recognised in Plot Option 'Maximum:' input box.");
    m_uiForm.yAxisMaximumInput->setText("");
  }
}


/**
* When clicking autoscale (slot)
*/
void MuonAnalysis::runyAxisAutoscale(bool state)
{
  m_uiForm.yAxisMinimumInput->setEnabled(!state);
  m_uiForm.yAxisMaximumInput->setEnabled(!state);
}



/**
* Front plot button (slot)
*/
void MuonAnalysis::runFrontPlotButton()
{
  // get current index
  int index = m_uiForm.frontGroupGroupPairComboBox->currentIndex();

  if (index >= numGroups())
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
* If the instrument selection has changed, calls instrumentSelectChanged (slot)
*
* @param prefix :: instrument name from QComboBox object
*/
void MuonAnalysis::userSelectInstrument(const QString& prefix) 
{
	if ( prefix != m_curInterfaceSetup )
	{
		//instrumentSelectChanged(prefix);
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
  prevValues.beginGroup("CustomInterfaces/MuonAnalysis/SaveOutput");

  //use their previous directory first and go to their default if that fails
  QString prevPath = prevValues.value("dir", QString::fromStdString(
    ConfigService::Instance().getString("defaultsave.directory"))).toString();

  QString filter;
  filter.append("Files (*.XML *.xml)");
  filter += ";;AllFiles (*.*)";
  QString groupingFile = API::FileDialogHandler::getSaveFileName(this,
                                   "Save Grouping file as", prevPath, filter);

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
  // Get grouping file

  QSettings prevValues;
  prevValues.beginGroup("CustomInterfaces/MuonAnalysis/LoadGroupFile");


  //use their previous directory first and go to their default if that fails

  QString prevPath = prevValues.value("dir", QString::fromStdString(
    ConfigService::Instance().getString("defaultload.directory"))).toString();

  QString filter;
  filter.append("Files (*.XML *.xml)");
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
  plotGroup(m_uiForm.groupTablePlotChoice->currentText().toStdString());
}

/**
 * Load current (slot)
 */
void MuonAnalysis::runLoadCurrent()
{
  QString instname = m_uiForm.instrSelector->currentText().toUpper();
  QString daename = "NDX" + instname;

  // Load dae file
  AnalysisDataService::Instance().remove(m_workspace_name);

  QString pyString = 
      "from mantidsimple import *\n"
      "import sys\n"
      "try:\n"
      "  LoadDAE('" + daename + "','" + m_workspace_name.c_str() + "')\n"
      "except SystemExit, message:\n"
      "  print str(message)";
  QString pyOutput = runPythonCode( pyString ).trimmed();

  // if output is none empty something has gone wrong
  if ( !pyOutput.toStdString().empty() )
  {
    noDataAvailable();
    QMessageBox::warning(this, "MantidPlot - MuonAnalysis", "Can't read from " + daename + ". Plotting disabled");
    return;
  }

  nowDataAvailable();

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
  int nDet = static_cast<int>(matrix_workspace->getInstrument()->getDetectors().size());
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
  plotPair(m_uiForm.pairTablePlotChoice->currentText().toStdString());
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
  }
}


/**
 * Group table changed, e.g. if:         (slot)
 *
 *    1) user changed detector sequence 
 *    2) user type in a group name
 *
 * @param row :: 
 * @param column
::  */
void MuonAnalysis::groupTableChanged(int row, int column)
{
 // if ( column == 2 )
 //   return;

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
        //checkIf_ID_dublicatesInTable(row);
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
 * @param row :: 
 * @param column
::  */
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
         double alpha = boost::lexical_cast<double>(itemAlpha->text().toStdString().c_str());
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


  // get previous number of groups
  QComboBox* qwF = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(0,1));
  int previousNumGroups = qwF->count();
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
 * Input file changed. Update information accordingly (slot)
 */
void MuonAnalysis::inputFileChanged()
{
  if ( !m_uiForm.mwRunFiles->isValid() )
  {
    QMessageBox::warning(this,"Mantid - MuonAnalysis", "Muon file not recognised");
    return;
  }

  if ( m_previousFilename.compare(m_uiForm.mwRunFiles->getFirstFilename()) == 0 )
    return;

  m_previousFilename = m_uiForm.mwRunFiles->getFirstFilename();

  // in case file is selected from browser button check that it actually exist
  Poco::File l_path( m_previousFilename.toStdString() );
  if ( !l_path.exists() )
  {
    QMessageBox::warning(this,"Mantid - MuonAnalysis", "Specified data file does not exist.");
    return;
  }
  // and check if file is from a recognised instrument and update instrument combo box
  QString filenamePart = (Poco::Path(l_path.path()).getFileName()).c_str();
  filenamePart = filenamePart.toLower();
  bool foundInst = false;
  for (int i = 0; i < m_uiForm.instrSelector->count(); i++)
  {
    QString instName = m_uiForm.instrSelector->itemText(i).toLower();
    
    std::string sfilename = filenamePart.toStdString();
    std::string sinstName = instName.toStdString();
    size_t found;
    found = sfilename.find(sinstName);
    if ( found != std::string::npos )
    {
      m_uiForm.instrSelector->setCurrentIndex(i);
      foundInst = true;
      break;
    }
  }
  if ( !foundInst )
  {
    QMessageBox::warning(this,"Mantid - MuonAnalysis", "Muon file not recognised.");
    return;
  }

  QString pyString =      "from mantidsimple import *\n"
      "import sys\n"
      "try:\n"
      "  alg = LoadMuonNexus('" + m_previousFilename+"','" + m_workspace_name.c_str() + "', AutoGroup='0')\n"
      "  print alg.getPropertyValue('MainFieldDirection'), alg.getPropertyValue('TimeZero'), alg.getPropertyValue('FirstGoodData')\n"
      "except SystemExit, message:\n"
      "  print ''";
  QString outputParams = runPythonCode( pyString ).trimmed();

  if ( outputParams.isEmpty() )
  {
    QMessageBox::warning(this,"Mantid - MuonAnalysis", "Problem when executing LoadMuonNexus algorithm.");
    return;
  }

  nowDataAvailable();

  if ( !isGroupingSet() )
    setGroupingFromNexus(m_previousFilename);

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

  if ( !applyGroupingToWS(m_workspace_name, m_workspace_name+"Grouped") )
    return;

  // get hold of output parameters
  std::stringstream strParam(outputParams.toStdString());
  std::string mainFieldDirection;
  double timeZero;
  double firstGoodData;
  strParam >> mainFieldDirection >> timeZero >> firstGoodData;
  
  timeZero *= 1000.0;      // convert to ns
  firstGoodData *= 1000.0;


  // Populate instrument fields

  std::stringstream str;
  str << "Description: ";
  int nDet = static_cast<int>(matrix_workspace->getInstrument()->getDetectors().size());
  str << nDet;
  str << " detector spectrometer, main field ";
  str << QString(mainFieldDirection.c_str()).toLower().toStdString(); 
  str << " to muon polarisation";
  m_uiForm.instrumentDescription->setText(str.str().c_str());

  m_uiForm.timeZeroFront->setText((boost::lexical_cast<std::string>(static_cast<int>(timeZero))).c_str());
  m_uiForm.firstGoodBinFront->setText((boost::lexical_cast<std::string>(static_cast<int>(firstGoodData-timeZero))).c_str());


  // Populate run information text field
  m_title = matrix_workspace->getTitle();
  std::string infoStr = "Title = ";
  infoStr += m_title + "\n" + "Comment: "
    + matrix_workspace->getComment() + "\n";
  const Run& runDetails = matrix_workspace->run();
  infoStr += "Start: ";
  if ( runDetails.hasProperty("run_start") )
  {
    infoStr += runDetails.getProperty("run_start")->value();
  }
  infoStr += "\nEnd: ";
  if ( runDetails.hasProperty("run_end") )
  {
    infoStr += runDetails.getProperty("run_end")->value();
  }
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
 * Exit the interface (slot)
 */
/*
void MuonAnalysis::exitClicked()
{
  close();
  this->close();
  QObject * obj=parent();
  QWidget * widget = qobject_cast<QWidget*>(obj);
  if (widget)
  {
    widget->close();
  }
}*/

/**
 * Guess Alpha (slot). For now include all data from first good data(bin)
 */
void MuonAnalysis::guessAlphaClicked()
{
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

    QString inputWS = m_workspace_name.c_str() + QString("Grouped");
    if ( m_uiForm.homePeriodBox2->isEnabled() )
      inputWS += "_" + m_uiForm.homePeriodBox1->currentText();


    QString pyString;

    pyString += "alg=AlphaCalc(\"" + inputWS + "\",\"" 
        + idsF->text() + "\",\""
        + idsB->text() + "\",\"" 
        + firstGoodBin() + "\")\n"
        + "print alg.getPropertyValue('Alpha')";

    std::cout << pyString.toStdString() << std::endl;

    // run python script
    QString pyOutput = runPythonCode( pyString ).trimmed();
    pyOutput.truncate(5);

    QComboBox* qwAlpha = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(m_pairTableRowInFocus,3));
    if (qwAlpha)
      m_uiForm.pairTable->item(m_pairTableRowInFocus,3)->setText(pyOutput);
    else
      m_uiForm.pairTable->setItem(m_pairTableRowInFocus,3, new QTableWidgetItem(pyOutput));
  }
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

      m_uiForm.frontAlphaNumber->setText(m_uiForm.pairTable->item(index-numG,3)->text());
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
  m_uiForm.frontGroupGroupPairComboBox->clear();

  int numG = numGroups();
  int numP = numPairs();
  for (int i = 0; i < numG; i++)
    m_uiForm.frontGroupGroupPairComboBox->addItem(
      m_uiForm.groupTable->item(m_groupToRow[i],0)->text());
  for (int i = 0; i < numP; i++)
    m_uiForm.frontGroupGroupPairComboBox->addItem(
      m_uiForm.pairTable->item(m_pairToRow[i],0)->text());
  
  m_uiForm.frontGroupGroupPairComboBox->setCurrentIndex(0);

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
 * by the user on the front panel
 */
void MuonAnalysis::createPlotWS(const std::string& groupName, const std::string& wsname)
{
  QString inputWS = m_workspace_name.c_str() + QString("Grouped");

  if ( m_uiForm.homePeriodBox2->isEnabled() && m_uiForm.homePeriodBox2->currentText()!="None" )
  {
    QString pyS;
    if ( m_uiForm.homePeriodBoxMath->currentText()=="+" )
    {
      pyS += "Plus(\"" + inputWS + "_" + m_uiForm.homePeriodBox1->currentText()
        + "\",\"" + inputWS + "_" + m_uiForm.homePeriodBox2->currentText() + "\",\""
        + wsname.c_str() + "\")";
    }
    else 
    {
      pyS += "Minus(\"" + inputWS + "_" + m_uiForm.homePeriodBox1->currentText()
        + "\",\"" + inputWS + "_" + m_uiForm.homePeriodBox2->currentText() + "\",\""
        + wsname.c_str() + "\")";
    }
    runPythonCode( pyS ).trimmed();
    inputWS = wsname.c_str();
  }
  else
  {
    if ( m_uiForm.homePeriodBox2->isEnabled() ) 
      inputWS += "_" + m_uiForm.homePeriodBox1->currentText();
  }


  QString cropStr = "CropWorkspace(\"";
  cropStr += inputWS;
  cropStr += "\",\"";
  cropStr += wsname.c_str();
  cropStr += "\"," + firstGoodBin() + ");";
  runPythonCode( cropStr ).trimmed();


  if ( !AnalysisDataService::Instance().doesExist(groupName) )
  {
    QString rubbish = "boevsMoreBoevs";
    QString groupStr = QString("CloneWorkspace('") + wsname.c_str() + "','"+rubbish+"')\n";
    groupStr += QString("GroupWorkspaces(InputWorkspaces='") + wsname.c_str() + "," + rubbish
      + "',OutputWorkspace='"+groupName.c_str()+"')\n";
    runPythonCode( groupStr ).trimmed();
    AnalysisDataService::Instance().remove(rubbish.toStdString());
  }
  else
  {
    QString groupStr = QString("GroupWorkspaces(InputWorkspaces='") + wsname.c_str() + "," + groupName.c_str()
      + "',OutputWorkspace='" + groupName.c_str() + "')\n";
    runPythonCode( groupStr ).trimmed();
  }

}




/**
 * Plot group
 */
void MuonAnalysis::plotGroup(const std::string& plotType)
{
  int groupNum = getGroupNumberFromRow(m_groupTableRowInFocus);
  if ( groupNum >= 0 )
  {
    QTableWidgetItem *itemName = m_uiForm.groupTable->item(m_groupTableRowInFocus,0);
    QString groupName = itemName->text();

    // Decide on name for workspaceGroup
    Poco::File l_path( m_previousFilename.toStdString() );
    std::string workspaceGroupName = Poco::Path(l_path.path()).getFileName();
    std::size_t extPos = workspaceGroupName.find(".");
    if ( extPos!=std::string::npos)
      workspaceGroupName = workspaceGroupName.substr(0,extPos);

    // decide on name for workspace to be plotted
    QString cropWS;
    QString cropWSfirstPart = QString(workspaceGroupName.c_str()) + "; Group="
      + groupName + "";

    // check if this workspace already exist to avoid replotting an existing workspace
    int plotNum = 1;
    while (1==1)
    {
      cropWS = cropWSfirstPart + "; #" + boost::lexical_cast<std::string>(plotNum).c_str();
      if ( AnalysisDataService::Instance().doesExist(cropWS.toStdString()) ) 
        plotNum++;
      else
        break;
    }

    // create the plot workspace
    createPlotWS(workspaceGroupName,cropWS.toStdString());

    // curve plot label
    QString titleLabel = cropWS;

    // create first part of plotting Python string
    QString gNum = QString::number(groupNum);
    QString pyS;
    if ( m_uiForm.showErrorBars->isChecked() )
      pyS = "gs = plotSpectrum(\"" + cropWS + "\"," + gNum + ",true)\n";
    else
      pyS = "gs = plotSpectrum(\"" + cropWS + "\"," + gNum + ")\n";
    pyS += "l = gs.activeLayer()\n"
      "l.setCurveTitle(0, \"" + titleLabel + "\")\n"
      "l.setTitle(\"" + m_title.c_str() + "\")\n"
      "l.setAxisTitle(Layer.Bottom, \"Time / microsecond\")\n";

    if ( !m_uiForm.yAxisAutoscale->isChecked() )
    {
      Workspace_sptr ws_ptr = AnalysisDataService::Instance().retrieve(cropWS.toStdString());
      MatrixWorkspace_sptr matrix_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(ws_ptr);
      const Mantid::MantidVec& dataY = matrix_workspace->readY(groupNum);
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


    QString pyString;
    if (plotType.compare("Counts") == 0)
    {
      pyString = pyS;
    }
    else if (plotType.compare("Asymmetry") == 0)
    {
      pyString = "RemoveExpDecay(\"" + cropWS + "\",\"" 
        + cropWS + "\"," + gNum + ")\n" + pyS
        + "l.setAxisTitle(Layer.Left, \"Asymmetry\")\n";
    }
    else if (plotType.compare("Logorithm") == 0)
    {
      pyString += "Logarithm(\"" + cropWS + "\",\"" 
        + cropWS + "\","
        + gNum + ")\n" + pyS
        + "l.setAxisTitle(Layer.Left, \"Logorithm\")\n";
    }
    else
    {
      g_log.error("Unknown group table plot function");
      return;
    }

    // run python script
    QString pyOutput = runPythonCode( pyString ).trimmed();
  }  
}

/**
 * Plot pair
 */
void MuonAnalysis::plotPair(const std::string& plotType)
{
  int pairNum = getPairNumberFromRow(m_pairTableRowInFocus);
  if ( pairNum >= 0 )
  {
    QTableWidgetItem *item = m_uiForm.pairTable->item(m_pairTableRowInFocus,3);
    QTableWidgetItem *itemName = m_uiForm.pairTable->item(m_pairTableRowInFocus,0);
    QString pairName = itemName->text();

    // Decide on name for workspaceGroup
    Poco::File l_path( m_previousFilename.toStdString() );
    std::string workspaceGroupName = Poco::Path(l_path.path()).getFileName();
    std::size_t extPos = workspaceGroupName.find(".");
    if ( extPos!=std::string::npos)
      workspaceGroupName = workspaceGroupName.substr(0,extPos);

    // decide on name for workspace to be plotted
    QString cropWS;
    QString cropWSfirstPart = QString(workspaceGroupName.c_str()) + "; Group="
      + pairName + "";

    // check if this workspace already exist to avoid replotting an existing workspace
    int plotNum = 1;
    while (1==1)
    {
      cropWS = cropWSfirstPart + "; #" + boost::lexical_cast<std::string>(plotNum).c_str();
      if ( AnalysisDataService::Instance().doesExist(cropWS.toStdString()) ) 
        plotNum++;
      else
        break;
    }

    // create the plot workspace
    createPlotWS(workspaceGroupName,cropWS.toStdString());

    // curve plot label
    QString titleLabel = cropWS;


    // create first part of plotting Python string
    QString gNum = QString::number(pairNum);
    QString pyS;
    if ( m_uiForm.showErrorBars->isChecked() )
      pyS = "gs = plotSpectrum(\"" + cropWS + "\"," + gNum + ",true)\n";
    else
      pyS = "gs = plotSpectrum(\"" + cropWS + "\"," + gNum + ")\n";
    pyS += "l = gs.activeLayer()\n"
      "l.setCurveTitle(0, \"" + titleLabel + "\")\n"
      "l.setTitle(\"" + m_title.c_str() + "\")\n"
      "l.setAxisTitle(Layer.Bottom, \"Time / microsecond\")\n";

    if ( !m_uiForm.yAxisAutoscale->isChecked() )
    {
      Workspace_sptr ws_ptr = AnalysisDataService::Instance().retrieve(cropWS.toStdString());
      MatrixWorkspace_sptr matrix_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(ws_ptr);
      const Mantid::MantidVec& dataY = matrix_workspace->readY(pairNum);
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


    QString pyString;
    if (plotType.compare("Asymmetry") == 0)
    {
      QComboBox* qw1 = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(m_pairTableRowInFocus,1));
      QComboBox* qw2 = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(m_pairTableRowInFocus,2));

      QString pairName;
      QTableWidgetItem *itemName = m_uiForm.pairTable->item(m_pairTableRowInFocus,0);
      if (itemName)
        pairName = itemName->text();

      //QString outputWS_Name = m_workspace_name.c_str() + QString("_") + pairName + periodStr;

      pyString = "AsymmetryCalc(\"" + cropWS + "\",\"" 
        + cropWS + "\","
        + QString::number(qw1->currentIndex()) + "," 
        + QString::number(qw2->currentIndex()) + "," 
        + item->text() + ")\n" + pyS
        + "l.setAxisTitle(Layer.Left, \"Asymmetry\")\n";
    }
    else
    {
      g_log.error("Unknown pair table plot function");
      return;
    }

    // run python script
    QString pyOutput = runPythonCode( pyString ).trimmed();
  }
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
 * @param filename :: Name of grouping file
 */
bool MuonAnalysis::applyGroupingToWS( const std::string& inputWS,  const std::string& outputWS, 
   const std::string& filename)
{
  if ( AnalysisDataService::Instance().doesExist(inputWS) )
  {

    AnalysisDataService::Instance().remove(outputWS);

    QString pyString = 
      "from mantidsimple import *\n"
      "import sys\n"
      "try:\n"
      "  GroupDetectors('" + QString(inputWS.c_str()) + "','" + outputWS.c_str() + "','" + filename.c_str() + "')\n"
      "except SystemExit, message:\n"
      "  print str(message)";

    // run python script
    QString pyOutput = runPythonCode( pyString ).trimmed();

    // if output is none empty something has gone wrong
    if ( !pyOutput.toStdString().empty() )
    {
      noDataAvailable();
      QMessageBox::warning(this, "MantidPlot - MuonAnalysis", "Can't group data file according to group-table. Plotting disabled.");
      return false;
      //m_uiForm.frontWarningMessage->setText("Can't group data file according to group-table. Plotting disabled.");
    }
    else
    {
      nowDataAvailable();
      return true;
    }
  }
}

/**
 * Apply whatever grouping is specified in GUI tables to workspace. 
 */
bool MuonAnalysis::applyGroupingToWS( const std::string& inputWS,  const std::string& outputWS)
{
  if ( isGroupingSet() && AnalysisDataService::Instance().doesExist(inputWS) )
  {

    std::string complaint = isGroupingAndDataConsistent();
    if ( complaint.empty() )
    {
      nowDataAvailable();
      m_uiForm.frontWarningMessage->setText("");
    }
    else
    {
      if (m_uiForm.frontPlotButton->isEnabled() )
        QMessageBox::warning(this, "MantidPlot - MuonAnalysis", complaint.c_str());
      noDataAvailable();
      //m_uiForm.frontWarningMessage->setText(complaint.c_str());
      return false;
    }

    saveGroupingTabletoXML(m_uiForm, m_groupingTempFilename);
    return applyGroupingToWS(inputWS, outputWS, m_groupingTempFilename);
  }
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
  //int retVal = 0;
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
 * When no data loaded set various buttons etc to inactive
 */
void MuonAnalysis::noDataAvailable()
{
  m_uiForm.frontPlotButton->setEnabled(false);
  m_uiForm.groupTablePlotButton->setEnabled(false);
  m_uiForm.pairTablePlotButton->setEnabled(false);

  m_uiForm.guessAlphaButton->setEnabled(false);
}

/**
 * When data loaded set various buttons etc to active
 */
void MuonAnalysis::nowDataAvailable()
{
  m_uiForm.frontPlotButton->setEnabled(true);
  m_uiForm.groupTablePlotButton->setEnabled(true);
  m_uiForm.pairTablePlotButton->setEnabled(true);

  m_uiForm.guessAlphaButton->setEnabled(true);
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
  
  // Set initial front assuming to alpha specified etc...
  m_uiForm.frontAlphaLabel->setVisible(false);
  m_uiForm.frontAlphaNumber->setVisible(false);
  m_uiForm.frontAlphaNumber->setEnabled(false);
  m_uiForm.homePeriodBox2->setEditable(false);
  m_uiForm.homePeriodBox2->setEnabled(false);

  // set various properties of the group table
  m_uiForm.groupTable->setColumnWidth(1, 2*m_uiForm.groupTable->columnWidth(1));
  m_uiForm.groupTable->setColumnWidth(3, 0.5*m_uiForm.groupTable->columnWidth(3));
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

  // Load nexus file with grouping
  QString pyString = "LoadMuonNexus('";
  pyString.append(nexusFile);
  pyString.append("','");
  pyString.append( groupedWS.c_str());
  pyString.append("', AutoGroup=\"1\");");
  runPythonCode( pyString ).trimmed();

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
  int numOfHist = matrix_workspace->getNumberHistograms();
  for (int wsIndex = 0; wsIndex < numOfHist; wsIndex++)
  {
    IDetector_sptr det = matrix_workspace->getDetector(wsIndex);

    if( boost::dynamic_pointer_cast<DetectorGroup>(det) )
    {
      // prepare IDs string

      boost::shared_ptr<DetectorGroup> detG = boost::dynamic_pointer_cast<DetectorGroup>(det);
      std::vector<int> detIDs = detG->getDetectorIDs();
      if (detIDs.size() > 1)
      {
        thereIsGrouping = true;
        break;
      }
    }
  }

  // if no grouping in nexus then set dummy grouping and display warning to user
  if ( thereIsGrouping == false )
  {
    std::stringstream idstr;
    idstr << "1-" << matrix_workspace->getNumberHistograms();
    m_uiForm.groupTable->setItem(0, 0, new QTableWidgetItem("NoGroupingDetected"));
    m_uiForm.groupTable->setItem(0, 1, new QTableWidgetItem(idstr.str().c_str()));

    updateFrontAndCombo();

    QMessageBox::warning(this, "MantidPlot - MuonAnalysis", "No grouping detected in Nexus.");

    return;
  }


  // Add info about grouping from Nexus file to group table
  for (int wsIndex = 0; wsIndex < matrix_workspace->getNumberHistograms(); wsIndex++)
  {
    IDetector_sptr det = matrix_workspace->getDetector(wsIndex);

    if( boost::dynamic_pointer_cast<DetectorGroup>(det) )
    {
      // prepare IDs string

      boost::shared_ptr<DetectorGroup> detG = boost::dynamic_pointer_cast<DetectorGroup>(det);
      std::vector<int> detIDs = detG->getDetectorIDs();
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
  
  updatePairTable();
  updateFrontAndCombo();
}


 /**
 * Time zero returend in ms
 */
QString MuonAnalysis::timeZero()
{
  std::stringstream str(m_uiForm.timeZeroFront->text().toStdString()); 
  double tz;
  str >> tz;
  tz /= 1000.0;  // convert from ns to ms

  return QString((boost::lexical_cast<std::string>(tz)).c_str());
}

 /**
 * first good bin returend in ms
 * returned as the absolute value of first-good-bin minus time zero
 */
QString MuonAnalysis::firstGoodBin()
{
  std::stringstream str(m_uiForm.firstGoodBinFront->text().toStdString()); 
  double fgb;
  str >> fgb;
  fgb /= 1000.0;  // convert from ns to ms

  return QString((boost::lexical_cast<std::string>(fgb)).c_str());
}

 /**
 * first good bin returend in ms
 * returned as the absolute value of first-good-bin minus time zero
 */
double MuonAnalysis::plotFromTime()
{
  double retVal;
  try 
  {
    retVal = boost::lexical_cast<double>(m_uiForm.timeAxisStartAtInput->text().toStdString());
    retVal /= 1000.0;  // convert from ns to ms
  }
  catch (...)
  {
    retVal = 0.0;
    QMessageBox::warning(this,"Mantid - MuonAnalysis", "Number not recognised in Plot Option 'Start at (ns)' input box. Plot from time zero.");
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

  int nDet = matrix_workspace->getNumberHistograms();

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
        if ( boost::lexical_cast<int>(values[i].c_str()) > nDet )
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
}
