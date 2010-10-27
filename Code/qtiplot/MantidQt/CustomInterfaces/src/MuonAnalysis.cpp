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
  UserSubWindow(parent), m_last_dir(), m_workspace_name("MuonAnalysis"), m_period(0), m_groupTableRowInFocus(-1), m_pairTableRowInFocus(-1),
  m_groupTablePlotChoice("Counts"), m_pairTablePlotChoice("Asymmetry"), m_groupNames(), m_groupingTempFilename("tempMuonAnalysisGrouping.xml"),
  m_dataLoaded(false)
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

  // If group table change
  connect(m_uiForm.groupTable, SIGNAL(cellChanged(int, int)), this, SLOT(groupTableChanged(int, int))); 
  connect(m_uiForm.groupTable, SIGNAL(cellClicked(int, int)), this, SLOT(groupTableClicked(int, int))); 
  // group table plot button
  connect(m_uiForm.groupTablePlotButton, SIGNAL(clicked()), this, SLOT(runGroupTablePlotButton())); 
  // Store selected group-plot-choice in local variable
  connect(m_uiForm.groupTablePlotChoice, SIGNAL(currentIndexChanged(const QString)), this, 
    SLOT(runGroupTablePlotChoice(const QString))); 

  // If pair table change
  connect(m_uiForm.pairTable, SIGNAL(cellChanged(int, int)), this, SLOT(pairTableChanged(int, int))); 
  connect(m_uiForm.pairTable, SIGNAL(cellClicked(int, int)), this, SLOT(pairTableClicked(int, int)));
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

    std::string str = m_uiForm.frontPlotFuncs->currentText().toStdString();
    plotPair(str);
  }
  else
  {
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
 * change group table plotting choice (slot)
 */
void MuonAnalysis::runGroupTablePlotChoice(const QString str)
{
  m_groupTablePlotChoice = str.toStdString();
}

/**
 * change pair table plotting choice (slot)
 */
void MuonAnalysis::runPairTablePlotChoice(const QString str)
{
  m_pairTablePlotChoice = str.toStdString();
}

/**
 * Group table plot button (slot)
 */
void MuonAnalysis::runGroupTablePlotButton()
{
  plotGroup(m_groupTablePlotChoice);
}

/**
 * Pair table plot button (slot)
 */
void MuonAnalysis::runPairTablePlotButton()
{
  plotPair(m_pairTablePlotChoice);
}

/**
 * Group table clicked (slot)
 */
void MuonAnalysis::pairTableClicked(int row, int column)
{
  (void) column;

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
 * Group table clicked (slot)
 */
void MuonAnalysis::groupTableClicked(int row, int column)
{
  (void) column;

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
      if (numDet >= 0 )
      {
        detNumRead << numDet;
        if (itemNdet == NULL)
          m_uiForm.groupTable->setItem(row,2, new QTableWidgetItem(detNumRead.str().c_str()));
        else
          m_uiForm.groupTable->item(row, 2)->setText(detNumRead.str().c_str());
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
      return;

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
    return;

  if ( m_previousFilename.compare(m_uiForm.mwRunFiles->getFirstFilename()) == 0 )
    return;

  m_previousFilename = m_uiForm.mwRunFiles->getFirstFilename();

  // Load nexus file with no grouping
  QString pyString = "alg = LoadMuonNexus('";
  pyString.append(m_previousFilename);
  pyString.append("','");
  pyString.append(m_workspace_name.c_str());
  pyString.append("', AutoGroup=\"0\")\n");
  pyString.append("print alg.getPropertyValue('MainFieldDirection'), alg.getPropertyValue('TimeZero'), alg.getPropertyValue('FirstGoodData')");
  QString outputParams = runPythonCode( pyString ).trimmed();

  if ( !isGroupingSet() )
    setGroupingFromNexus(m_previousFilename);

  // Get hold of a pointer to a matrix workspace and apply grouping if applicatable

  Workspace_sptr workspace_ptr = AnalysisDataService::Instance().retrieve(m_workspace_name);
  WorkspaceGroup_sptr wsPeriods = boost::dynamic_pointer_cast<WorkspaceGroup>(workspace_ptr);
  MatrixWorkspace_sptr matrix_workspace;
  int numPeriods = 1;   // 1 may mean either a group with one period or simply just 1 normal matrix workspace
  if (wsPeriods)
  {
    numPeriods = wsPeriods->getNumberOfEntries() - 1;  // note getNumberOfEntries returns one more # of periods 

//    for ( int i = 1; i <= numPeriods; i++)
//      applyGroupingToWS( m_workspace_name + "_" + iToString(i));

    Workspace_sptr workspace_ptr1 = AnalysisDataService::Instance().retrieve(m_workspace_name + "_1");
    matrix_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(workspace_ptr1);
    m_period = 1;
  }
  else
  {

    matrix_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(workspace_ptr);
  }

  applyGroupingToWS(m_workspace_name, m_workspace_name+"Grouping");


  // get hold of output parameters
  std::stringstream strParam(outputParams.toStdString());
  std::string mainFieldDirection;
  double timeZero;
  double firstGoodData;
  strParam >> mainFieldDirection >> timeZero >> firstGoodData;
  
  timeZero *= 1000.0;      // convert to ns
  firstGoodData *= 1000.0;


  // Populate instrument fields

  IInstrument_sptr instrument = matrix_workspace->getInstrument();
  std::stringstream str;
  str << "Description: ";
  unsigned int nDet = instrument->getDetectors().size();
  str << nDet;
  str << " detector spectrometer, main field ";
  str << QString(mainFieldDirection.c_str()).toLower().toStdString(); 
  str << " to muon polarisation";
  m_uiForm.instrumentDescription->setText(str.str().c_str());

  //m_uiForm.timeZeroFront->setText((boost::lexical_cast<std::string>(timeZero)).c_str());
  //m_uiForm.firstGoodBinFront->setText((boost::lexical_cast<std::string>(firstGoodData)).c_str());


  // Populate run information text field

  std::string infoStr = "Title: "; 
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
  //while ( m_uiForm.homePeriodBox2->count() != 0 )
  //  m_uiForm.homePeriodBox2->removeItem(0);

  for ( int i = 1; i <= numPeriods; i++ )
  {
    std::stringstream strInt;
    strInt << i;
    m_uiForm.homePeriodBox1->addItem(strInt.str().c_str());
  }

  /* if ( numPeriods == 1 )
  {
    m_uiForm.homePeriodBox2->setEnabled(false);
  }
  else
  {
    m_uiForm.homePeriodBox2->setEnabled(true);

    m_uiForm.homePeriodBox2->addItem("");
    for ( int i = 1; i <= numPeriods; i++ )
    {
      std::stringstream strInt;
      strInt << i;
      m_uiForm.homePeriodBox2->addItem(strInt.str().c_str());
    }
  }*/

  nowDataAvailable();
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
      m_pairTableRowInFocus = index-numG;
    }
    else
    {
      // i.e. index points to a group
      m_uiForm.frontPlotFuncs->addItems(m_groupPlotFunc);

      m_uiForm.frontAlphaLabel->setVisible(false);
      m_uiForm.frontAlphaNumber->setVisible(false);
      m_groupTableRowInFocus = index;
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
 * Return the group which is in focus and -1 if none
 */
int MuonAnalysis::groupInFocus()
{
  if ( getGroupNumberFromRow(m_groupTableRowInFocus) >= 0 )
    return m_groupTableRowInFocus;
  else 
    return -1;
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
 * Take the MuonAnalysisGrouped WS and reduce(crop) histograms according to e.g. first-good-bin.
 * If period data then the resulting cropped WS is on for the period selected by the user
 * on the front panel. The outputted WS is named MuonAnalysisCropped.
 */
void MuonAnalysis::createCropWS()
{
  QString periodStr = "";  // used to pick out the WS representing the period selected by user
  if (m_period > 0)
  {    
    periodStr += QString("_") + iToString(m_period).c_str();
  }

  QString outputWS = "MuonAnalysisCropped";
  QString cropStr = "CropWorkspace(\"";
  cropStr += m_workspace_name.c_str() + periodStr;
  cropStr += "\",\"";
  cropStr += outputWS;
  cropStr += "\"," + firstGoodBin() + ");";
  runPythonCode( cropStr ).trimmed();
}


/**
 * Plot group
 */
void MuonAnalysis::plotGroup(std::string& plotType)
{
  if ( m_groupTableRowInFocus >= 0 )
  {
    // only plot if group name available
    QTableWidgetItem *itemName = m_uiForm.groupTable->item(m_groupTableRowInFocus,0);
    QString groupName;
    if (!itemName)
      return;
    else
      groupName = itemName->text();


    // create workspace which starts at first-good-bin 

    QString periodStr = "";
    if (m_period > 0)
    {    
      periodStr += QString("_") + iToString(m_period).c_str();
    }
    QString cropWS = m_workspace_name.c_str() + QString("_") + groupName + periodStr;
    QString cropStr = "CropWorkspace(\"";
    cropStr += m_workspace_name.c_str() + periodStr;
    cropStr += "\",\"";
    cropStr += cropWS;
    cropStr += "\"," + firstGoodBin() + ");";
    runPythonCode( cropStr ).trimmed();


    // create plotting Python string 

    QString rowNum = QString(iToString(m_groupTableRowInFocus).c_str());
    QString pyString;
    if (plotType.compare("Counts") == 0)
    {
      pyString += "plotSpectrum(\"" + cropWS + "\"," 
        + rowNum + ");";
    }
    else if (plotType.compare("Asymmetry") == 0)
    {
      QString outputName = cropWS + "_asym";
      pyString += "RemoveExpDecay(\"" + cropWS + "\",\"" 
        + outputName + "\","
        + rowNum + "); plotSpectrum(\"" + outputName
        + "\"," + rowNum + ");";
    }
    else if (plotType.compare("Logorithm") == 0)
    {
      QString outputName = cropWS + "_log";
      pyString += "Logarithm(\"" + cropWS + "\",\"" 
        + outputName + "\","
        + rowNum + "); plotSpectrum(\"" + outputName
        + "\"," + rowNum + ");";
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
void MuonAnalysis::plotPair(std::string& plotType)
{
  if ( m_pairTableRowInFocus >= 0 )
  {
    // only plot if alpha defined in table
    QTableWidgetItem *item = m_uiForm.pairTable->item(m_pairTableRowInFocus,3);
    if (!item)
      return;

    // create cropped workspace of relevant workspace

    QString periodStr = "";
    if (m_period > 0)
    {    
      periodStr += QString("_") + iToString(m_period).c_str();
    }
    QString cropWS = m_workspace_name.c_str() + QString("_crop");
    QString cropStr = "CropWorkspace(\"";
    cropStr += m_workspace_name.c_str() + periodStr;
    cropStr += "\",\"";
    cropStr += cropWS;
    cropStr += "\"," + firstGoodBin() + ");";
    runPythonCode( cropStr ).trimmed();


    // create plotting Python string 

    QString pyString;
    if (plotType.compare("Asymmetry") == 0)
    {
      QComboBox* qw1 = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(m_pairTableRowInFocus,1));
      QComboBox* qw2 = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(m_pairTableRowInFocus,2));

      QString pairName;
      QTableWidgetItem *itemName = m_uiForm.pairTable->item(m_pairTableRowInFocus,0);
      if (itemName)
        pairName = itemName->text();

      QString outputWS_Name = m_workspace_name.c_str() + QString("_") + pairName + periodStr;

      pyString += "AsymmetryCalc(\"" + cropWS + "\",\"" 
        + outputWS_Name + "\","
        + iToString(qw1->currentIndex()).c_str() + "," + iToString(qw2->currentIndex()).c_str()
        + "," + item->text() + "); plotSpectrum(\"" + outputWS_Name
        + "\",0);";
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
void MuonAnalysis::applyGroupingToWS( const std::string& inputWS,  const std::string& outputWS, 
   const std::string& filename)
{
  QString pyString = "GroupDetectors('";
  pyString.append(inputWS.c_str());
  pyString.append("','");
  pyString.append(outputWS.c_str());
  pyString.append("','");
  pyString.append(filename.c_str());
  pyString.append("');");
  
  // run python script
  QString pyOutput = runPythonCode( pyString ).trimmed();
}

/**
 * Apply whatever grouping is specified in GUI tables to workspace. 
 */
void MuonAnalysis::applyGroupingToWS( const std::string& inputWS,  const std::string& outputWS)
{
  if ( isGroupingSet() )
  {
    saveGroupingTabletoXML(m_uiForm, m_groupingTempFilename);
    applyGroupingToWS(inputWS, outputWS, m_groupingTempFilename);
  }
}

/**
 * Calculate number of detectors from string of type 1-3, 5, 10-15
 *
 * @param str String of type "1-3, 5, 10-15"
 * @return Number of detectors. Return -1 if not recognised
 */
int MuonAnalysis::numOfDetectors(std::string str) const
{
  int retVal = 0;

  if (str.empty())
    return 0;

  typedef Poco::StringTokenizer tokenizer;
  tokenizer values(str, ",", tokenizer::TOK_TRIM);

  for (int i = 0; i < static_cast<int>(values.count()); i++)
  {
    std::size_t found= values[i].find("-");
    if (found!=std::string::npos)
    {
      tokenizer aPart(values[i], "-", tokenizer::TOK_TRIM);

      if ( aPart.count() != 2 )
        return -1;
      else
      {
        if ( !(isNumber(aPart[0]) && isNumber(aPart[1])) )
          return -1;
      }

      int leftInt;
      std::stringstream leftRead(aPart[0]);
      leftRead >> leftInt;
      int rightInt;
      std::stringstream rightRead(aPart[1]);
      rightRead >> rightInt;

      if (leftInt > rightInt)
      {
        throw;
      }
      retVal += rightInt-leftInt+1;
    }
    else
    {

      if (isNumber(values[i]))
        retVal++;
      else
        return -1;
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
  m_dataLoaded = false;

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
  m_dataLoaded = true;

  m_uiForm.guessAlphaButton->setEnabled(true);
  m_uiForm.frontPlotButton->setEnabled(true);
  m_uiForm.groupTablePlotButton->setEnabled(true);
  m_uiForm.pairTablePlotButton->setEnabled(true);
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

  std::string groupedWS = m_workspace_name+"Grouping";

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

  // check if there is any grouping in nexus file
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


    //m_uiForm.frontGroupGroupPairComboBox->addItems(QStringList("NoGroupingDetected"));


    m_uiForm.groupTable->setItem(0, 0, new QTableWidgetItem("NoGroupingDetected"));
    m_uiForm.groupTable->setItem(0, 1, new QTableWidgetItem(idstr.str().c_str()));

    m_groupTableRowInFocus = 0;
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
  

  m_groupTableRowInFocus = 0;
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