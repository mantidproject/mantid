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
#include "MantidGeometry/IInstrument.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/V3D.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/Instrument/XMLlogfile.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include <Poco/StringTokenizer.h>

#include "Poco/File.h"
#include "Poco/Path.h"

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
}
}

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
  UserSubWindow(parent), m_last_dir(), m_workspace_name("MuonAnalysis"), m_period(0), m_groupTableRowInFocus(0), m_pairTableRowInFocus(0),
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
  connect(m_uiForm.exitButton, SIGNAL(clicked()), this, SLOT(exitClicked())); 

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

  // front select 1st period combobox
  connect(m_uiForm.homePeriodBox1, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(runHomePeriodBox1(const QString&)));

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


  connect(m_uiForm.mwRunFiles, SIGNAL(fileEditingFinished()), this, SLOT(inputFileChanged()));
}


/**
* Muon Analysis help (slot)
*/
void MuonAnalysis::runHomePeriodBox1(const QString& text)
{
  std::stringstream str(text.toStdString());
  str >> m_period;
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
* @param prefix instrument name from QComboBox object
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
    m_period = 1;
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
  int nDet = matrix_workspace->getInstrument()->getDetectors().size();
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
 * @param row 
 * @param column
 */
void MuonAnalysis::groupTableChanged(int row, int column)
{
  if ( column == 2 )
    return;

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
        checkIf_ID_dublicatesInTable(row);
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
 *
 * @param row 
 * @param column
 */
void MuonAnalysis::pairTableChanged(int row, int column)
{
  // alpha been modified
  if ( column == 3 )
  {
    QTableWidgetItem* itemAlpha = m_uiForm.pairTable->item(row,3);

    if ( itemAlpha->text().toStdString().empty() )
    {
    }
    else
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


  // Load nexus file with no grouping
  AnalysisDataService::Instance().remove(m_workspace_name);
  QString pyString = "alg = LoadMuonNexus('";
  pyString.append(m_previousFilename);
  pyString.append("','");
  pyString.append(m_workspace_name.c_str());
  pyString.append("', AutoGroup=\"0\")\n");
  pyString.append("print alg.getPropertyValue('MainFieldDirection'), alg.getPropertyValue('TimeZero'), alg.getPropertyValue('FirstGoodData')");
  QString outputParams = runPythonCode( pyString ).trimmed();

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
    m_period = 1;
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
  int nDet = matrix_workspace->getInstrument()->getDetectors().size();
  str << nDet;
  str << " detector spectrometer, main field ";
  str << QString(mainFieldDirection.c_str()).toLower().toStdString(); 
  str << " to muon polarisation";
  m_uiForm.instrumentDescription->setText(str.str().c_str());

  m_uiForm.timeZeroFront->setText((boost::lexical_cast<std::string>(static_cast<int>(timeZero))).c_str());
  m_uiForm.firstGoodBinFront->setText((boost::lexical_cast<std::string>(static_cast<int>(firstGoodData))).c_str());


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
 * Exit the interface (slot)
 */
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
}

/**
 * Guess Alpha (slot)
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

    QString periodStr = "";
    if (m_period > 0)  
      periodStr += QString("_") + iToString(m_period).c_str();

    QString inputWS = m_workspace_name.c_str() + periodStr;

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
 * @param row A row in the group table
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
 * @param row A row in the pair table
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
 * convert int to string
 *
 * return string representation of integer
 */
std::string MuonAnalysis::iToString(int i)
{
  std::stringstream str;
  str << i;
  return str.str();
}


/**
 * Create WS contained the data for a plot
 * Take the MuonAnalysisGrouped WS and reduce(crop) histograms according to e.g. first-good-bin.
 * If period data then the resulting cropped WS is on for the period, or sum/difference of, selected 
 * by the user on the front panel
 */
void MuonAnalysis::createPlotWS(const std::string& wsname)
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

    // create output workspace title
    Poco::File l_path( m_previousFilename.toStdString() );
    std::string filenamePart = Poco::Path(l_path.path()).getFileName();

    QString title = QString(filenamePart.c_str()) + " " + plotType.c_str() +"; Group='"
      + groupName + "'";

    // create workspace which starts at first-good-bin 
    QString cropWS = "MuonAnalysis_" + title;
    createPlotWS(cropWS.toStdString());

    // create plotting Python string
    QString gNum = QString(iToString(groupNum).c_str());

    QString pyS = "gs = plotSpectrum(\"" + cropWS + "\"," + gNum + ")\n"
      "l = gs.activeLayer()\n"
      "l.setCurveTitle(0, \"" + title + "\")\n";

    QString pyString;
    if (plotType.compare("Counts") == 0)
    {
      pyString = pyS;
    }
    else if (plotType.compare("Asymmetry") == 0)
    {
      pyString = "RemoveExpDecay(\"" + cropWS + "\",\"" 
        + cropWS + "\"," + gNum + ")\n" + pyS;
    }
    else if (plotType.compare("Logorithm") == 0)
    {
      pyString += "Logarithm(\"" + cropWS + "\",\"" 
        + cropWS + "\","
        + gNum + ")\n" + pyS;
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
  if ( getPairNumberFromRow(m_pairTableRowInFocus) >= 0 )
  {
    QTableWidgetItem *item = m_uiForm.pairTable->item(m_pairTableRowInFocus,3);
    QTableWidgetItem *itemName = m_uiForm.groupTable->item(m_groupTableRowInFocus,0);
    QString pairName = itemName->text();

    // create output workspace title
    Poco::File l_path( m_previousFilename.toStdString() );
    std::string filenamePart = Poco::Path(l_path.path()).getFileName();

    QString title = QString(filenamePart.c_str()) + " " + plotType.c_str() +"; Pair='"
      + pairName + "'";


    // create workspace which starts at first-good-bin 
    QString cropWS = "MuonAnalysis_" + title;
    createPlotWS(cropWS.toStdString());


    // create plotting Python string 

    QString pyS = "gs = plotSpectrum(\"" + cropWS + "\",0)\n"
      "l = gs.activeLayer()\n"
      "l.setCurveTitle(0, \"" + title + "\")\n";

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
        + iToString(qw1->currentIndex()).c_str() + "," 
        + iToString(qw2->currentIndex()).c_str() + "," 
        + item->text() + ")\n" + pyS;
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
  QTableWidgetItem *item = m_uiForm.groupTable->item(0,1);
  if (item)
    if ( !item->text().isEmpty() )
      return true;
  
  return false;
}

/**
 * Apply grouping specified in xml file to workspace
 *
 * @param filename Name of grouping file
 */
bool MuonAnalysis::applyGroupingToWS( const std::string& inputWS,  const std::string& outputWS, 
   const std::string& filename)
{
  if ( isGroupingSet() && m_uiForm.frontPlotButton->isEnabled() )
  {

    std::string complaint = isGroupingAndDataConsistent();
    if ( complaint.empty() )
    {
      nowDataAvailable();
      m_uiForm.frontWarningMessage->setText("");
    }
    else
    {
      noDataAvailable();
      QMessageBox::warning(this, "MantidPlot - MuonAnalysis", complaint.c_str());
      //m_uiForm.frontWarningMessage->setText(complaint.c_str());
      return false;
    }

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
      return true;
  }
}

/**
 * Apply whatever grouping is specified in GUI tables to workspace. 
 */
bool MuonAnalysis::applyGroupingToWS( const std::string& inputWS,  const std::string& outputWS)
{
  if ( isGroupingSet() && m_uiForm.frontPlotButton->isEnabled() )
  {
    saveGroupingTabletoXML(m_uiForm, m_groupingTempFilename);
    return applyGroupingToWS(inputWS, outputWS, m_groupingTempFilename);
  }
}

/**
 * Calculate number of detectors from string of type 1-3, 5, 10-15
 *
 * @param str String of type "1-3, 5, 10-15"
 * @return Number of detectors. Return 0 if not recognised
 */
int MuonAnalysis::numOfDetectors(const std::string& str) const
{
  return static_cast<int>(spectrumIDs(str).size());
}


/**
 * Return a vector of IDs for row number from string of type 1-3, 5, 10-15
 *
 * @param str String of type "1-3, 5, 10-15"
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
 *  @param s The input string
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
 * Return a none empty string if the data and group detector info are inconsistent
 */
 QString MuonAnalysis::dataAndTablesConsistent()
 {
   return QString();
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
 * first good bin returend in ms
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
          return complaint;
        }
      }
      else
      {
        if ( boost::lexical_cast<int>(values[i].c_str()) > nDet )
        {
          complaint += " Group-table row " + boost::lexical_cast<std::string>(m_groupToRow[iG]+1) + " refers to spectrum "
            + values[i] + ".";
          return complaint;
        }
      }
    }
  }

  return std::string("");
}


/**
* Boevs
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