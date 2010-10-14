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
#include "MantidAPI/IInstrument.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/V3D.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/XMLlogfile.h"
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

  // populate group plot functions. Get these from relevant combobox 
  for (int i = 0; i < m_uiForm.groupTablePlotChoice->count(); i++)
    m_groupPlotFunc.append(m_uiForm.groupTablePlotChoice->itemText(i));

  // pair plot functions. Get these from relevant combobox
  for (int i = 0; i < m_uiForm.pairTablePlotChoice->count(); i++)
    m_pairPlotFunc.append(m_uiForm.pairTablePlotChoice->itemText(i));

  //
  m_uiForm.frontAlphaLabel->setVisible(false);
  m_uiForm.frontAlphaNumber->setVisible(false);
  m_uiForm.frontAlphaNumber->setEnabled(false);

  m_uiForm.homePeriodBox2->setEditable(false);
  m_uiForm.homePeriodBox2->setEnabled(false);


  // connect exit button
  connect(m_uiForm.exitButton, SIGNAL(clicked()), this, SLOT(exitClicked())); 

  // connect guess alpha 
  connect(m_uiForm.guessAlphaButton, SIGNAL(clicked()), this, SLOT(guessAlphaClicked())); 

	// signal/slot connections to respond to changes in instrument selection combo boxes
	connect(m_uiForm.instrSelector, SIGNAL(instrumentSelectionChanged(const QString&)), this, SLOT(userSelectInstrument(const QString&)));

  // If group table change
  connect(m_uiForm.groupTable, SIGNAL(cellChanged(int, int)), this, SLOT(groupTableChanged(int, int))); 
  connect(m_uiForm.groupTable, SIGNAL(cellClicked(int, int)), this, SLOT(groupTableClicked(int, int))); 
  m_uiForm.groupTable->setColumnWidth(1, 2*m_uiForm.groupTable->columnWidth(1));
  m_uiForm.groupTable->setColumnWidth(3, 0.5*m_uiForm.groupTable->columnWidth(3));

  // group table plot button and choice
  connect(m_uiForm.groupTablePlotButton, SIGNAL(clicked()), this, SLOT(runGroupTablePlotButton())); 
  connect(m_uiForm.groupTablePlotChoice, SIGNAL(currentIndexChanged(const QString)), this, 
    SLOT(runGroupTablePlotChoice(const QString))); 

  // pair table
  connect(m_uiForm.pairTablePlotButton, SIGNAL(clicked()), this, SLOT(runPairTablePlotButton())); 
  connect(m_uiForm.pairTable, SIGNAL(cellClicked(int, int)), this, SLOT(pairTableClicked(int, int))); 

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
* This function: 1. loads the instrument and gets the value of deltaE-mode parameter
*				 2. Based on this value, makes the necessary changes to the form setup (direct or indirect).
* @param name name of the instrument from the QComboBox
*/
//void MuonAnalysis::instrumentSelectChanged(const QString& name)
//{
//}


/**
 * Save grouping button (slot)
 */
void MuonAnalysis::runSaveGroupButton()
{
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
void MuonAnalysis::groupTableClicked(int row, int column)
{
  (void) column;
  if ( m_uiForm.groupTable->item(row,2) != NULL )
  {
    m_groupTableRowInFocus = row;
    m_uiForm.frontGroupGroupPairComboBox->setCurrentIndex(row);
    updateFront();
  }
  else
    m_groupTableRowInFocus = -1;
}

/**
 * Group table clicked (slot)
 */
void MuonAnalysis::pairTableClicked(int row, int column)
{
  (void) column;
  if ( m_uiForm.pairTable->item(row,3) != NULL )
  {
    m_pairTableRowInFocus = row;
    m_uiForm.frontGroupGroupPairComboBox->setCurrentIndex(row+numGroups());
    updateFront();
  }
  else
    m_pairTableRowInFocus = -1;
}

/**
 * Group table changed (slot)
 */
void MuonAnalysis::groupTableChanged(int row, int column)
{
  if ( column == 1 )
  {
    if ( m_uiForm.groupTable->item(row,2) == NULL )
      return;


    QTableWidgetItem *item = m_uiForm.groupTable->item(row,1);

    std::stringstream detNumRead;
    try
    {
      detNumRead << numOfDetectors(item->text().toStdString());
      m_uiForm.groupTable->item(row, 2)->setText(detNumRead.str().c_str());
    }
    catch (...)
    {
      m_uiForm.groupTable->item(row, 2)->setText("Invalid");
    }
  }   

  if ( column == 0 )
  {
    if ( m_uiForm.groupTable->item(row,2) == NULL )
      return;


    QTableWidgetItem *item = m_uiForm.groupTable->item(row,0);

    for (int i = 0; i < m_uiForm.pairTable->rowCount(); i++)
    {
      QComboBox* qw1 = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(i,1));
      QComboBox* qw2 = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(i,2));

      qw1->setItemText(row, item->text());
      qw2->setItemText(row, item->text());
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

  QString m_previousFsdafilename = m_uiForm.mwRunFiles->getFirstFilename();

  if ( m_previousFilename.compare(m_uiForm.mwRunFiles->getFirstFilename()) == 0 )
    return;

  m_previousFilename = m_uiForm.mwRunFiles->getFirstFilename();

  // create Python string to load muon Nexus file
  QString pyString = "LoadMuonNexus('";
  pyString.append(m_uiForm.mwRunFiles->getFirstFilename());
  pyString.append("','");
  pyString.append(m_workspace_name.c_str());

  if ( isGroupingSet() )
    pyString.append("');");  // don't less use grouping in nexus
  else
    pyString.append("', AutoGroup=\"1\");");
  
  // run python script
  runPythonCode( pyString ).trimmed();

  // Get hold of a pointer to a matrix workspace and apply grouping if applicatable
  //AnalysisDataService::Instance().doExist(m_workspace_name);
  Workspace_sptr workspace_ptr = AnalysisDataService::Instance().retrieve(m_workspace_name);
  WorkspaceGroup_sptr wsPeriods = boost::dynamic_pointer_cast<WorkspaceGroup>(workspace_ptr);
  MatrixWorkspace_sptr matrix_workspace;
  int numPeriods = 1;   // 1 may mean either a group with one period or simply just 1 normal matrix workspace
  if (wsPeriods)
  {
    numPeriods = wsPeriods->getNumberOfEntries() - 1;  // note getNumberOfEntries returns one more # of periods 
    if ( isGroupingSet() )
    {
      for ( int i = 1; i <= numPeriods; i++)
      {
        // apply grouping if specified in group table
        applyGroupingToWS( m_workspace_name + "_" + iToString(i));
      }
    }
    Workspace_sptr workspace_ptr1 = AnalysisDataService::Instance().retrieve(m_workspace_name + "_1");
    matrix_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(workspace_ptr1);
    m_period = 1;
  }
  else
  {
    // apply grouping if specified in group table
    if ( isGroupingSet() )
      applyGroupingToWS(m_workspace_name);

    matrix_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(workspace_ptr);
  }


  // Populate instrument description field

  IInstrument_sptr instrument = matrix_workspace->getInstrument();
  std::stringstream str;
  str << "Description: ";
  unsigned int nDet = instrument->getDetectors().size();
  str << nDet;
  str << " detector spectrometer, main field ";
  str << "longitudinal";
  str << " to muon polarisation";
  m_uiForm.instrumentDescription->setText(str.str().c_str());


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

  // Populate grouping table and front combobox

  if ( !isGroupingSet() )
  {
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

        m_uiForm.frontGroupGroupPairComboBox->addItems(QStringList(gName.str().c_str()));
        m_uiForm.groupTable->setItem(wsIndex, 0, new QTableWidgetItem(gName.str().c_str()));
        m_uiForm.groupTable->setItem(wsIndex, 1, new QTableWidgetItem(idstr.str().c_str()));

        std::stringstream detNumRead;
        try
        {
          detNumRead << numOfDetectors(idstr.str());
          m_uiForm.groupTable->setItem(wsIndex, 2, new QTableWidgetItem(detNumRead.str().c_str()));
        }
        catch (...)
        {
          m_uiForm.groupTable->setItem(wsIndex, 2, new QTableWidgetItem("Invalid"));
        }
        m_uiForm.groupTable->item(wsIndex,2)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);

       // populate pair table combo boxes
       int rowNum = m_uiForm.pairTable->rowCount();
       for (int i = 0; i < rowNum; i++)
       {
         QComboBox* qw1 = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(i,1));
         QComboBox* qw2 = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(i,2));

          qw1->addItem( gName.str().c_str() );
          qw2->addItem( gName.str().c_str() );
       
   
        if ( qw2->count() > 1 )
          qw2->setCurrentIndex(1);
       }
      }
    }
  


    m_groupTableRowInFocus = 0;
    updateFront();
    nowDataAvailable();
  } // end isGroupingSet()
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
  if ( m_pairTableRowInFocus >= 0 )
  {

    QComboBox* qwF = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(m_pairTableRowInFocus,1));
    QComboBox* qwB = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(m_pairTableRowInFocus,2));

    if (qwF || qwB)
      return;

    QTableWidgetItem *idsF = m_uiForm.groupTable->item(qwF->currentIndex(),1);
    QTableWidgetItem *idsB = m_uiForm.groupTable->item(qwB->currentIndex(),1);

    if (idsF || idsB)
      return;

    QString periodStr = "";
    if (m_period > 0)  
      periodStr += QString("_") + iToString(m_period).c_str();

    QString inputWS = m_workspace_name.c_str() + periodStr;

    QString pyString;

    pyString += "AlphaCalc(\"" + inputWS + "\",\"" 
        + idsF->text() + "\",\""
        + idsB->text() + "\",\"" 
        + m_uiForm.firstGoodBinFront->text() + "\);";

    std::cout << pyString.toStdString() << std::endl;

    // run python script
    QString pyOutput = runPythonCode( pyString ).trimmed();
  }
}

/**
 * Return number of groups defined (not including pairs)
 *
 * @return number of groups
 */
int MuonAnalysis::numGroups()
{
  int numRows = m_uiForm.groupTable->rowCount();
  int retVal = 0;
  for (int i = 0; i < numRows; i++)
  {
    QTableWidgetItem *item = m_uiForm.groupTable->item(i,0);
    if (!item)
      break;
    if ( item->text().isEmpty() )
      break;

    retVal++;
  }
  return retVal;
}

/**
 * Return number of pairs
 *
 * @return number of pairs
 */
int MuonAnalysis::numPairs()
{
  int numRows = m_uiForm.pairTable->rowCount();
  int retVal = 0;
  for (int i = 0; i < numRows; i++)
  {
    QTableWidgetItem *item = m_uiForm.pairTable->item(i,0);
    if (!item)
      break;
    if ( item->text().isEmpty() )
      break;
    item = m_uiForm.pairTable->item(i,3);
    if (!item)
      break;
    if ( item->text().isEmpty() )
      break;

    retVal++;
  }
  return retVal;
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
 * Clear tables and front combo box
 */
void MuonAnalysis::clearTablesAndCombo()
{
  m_uiForm.groupTable->clearContents();
  m_uiForm.pairTable->clearContents();
  m_uiForm.frontGroupGroupPairComboBox->clear();
  m_uiForm.frontPlotFuncs->clear();

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

    // create cropped workspace of relevant workspace

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
    cropStr += "\"," + m_uiForm.firstGoodBinFront->text() + ");";
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
    cropStr += "\"," + m_uiForm.firstGoodBinFront->text() + ");";
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
void MuonAnalysis::applyGroupingToWS( const std::string& wsName, std::string filename)
{
  QString pyString = "GroupDetectors('";
  pyString.append(wsName.c_str());
  pyString.append("','");
  pyString.append(wsName.c_str());
  pyString.append("','");
  pyString.append(filename.c_str());
  pyString.append("');");
  
  // run python script
  QString pyOutput = runPythonCode( pyString ).trimmed();
}

/**
 * Apply whatever grouping is specified in GUI tables to workspace. 
 */
void MuonAnalysis::applyGroupingToWS( const std::string& wsName)
{
  if ( isGroupingSet() )
  {
    saveGroupingTabletoXML(m_uiForm, m_groupingTempFilename);
    applyGroupingToWS(wsName, m_groupingTempFilename);
  }
}

/**
 * Calculate number of detectors from string of type 1-3, 5, 10-15
 *
 * @param str String of type "1-3, 5, 10-15"
 * @return Number of detectors
 */
int MuonAnalysis::numOfDetectors(std::string str)
{
  int retVal = 0;

    typedef Poco::StringTokenizer tokenizer;
    tokenizer values(str, ",", tokenizer::TOK_TRIM);

  for (int i = 0; i < static_cast<int>(values.count()); i++)
  {
    std::size_t found= values[0].find("-");
    if (found!=std::string::npos)
    {
      tokenizer aPart(values[0], "-", tokenizer::TOK_TRIM);

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
      retVal++;
    }
  }
  return retVal;
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
}

/**
 * Return a none empty string if the data and group detector info are inconsistent
 */
 QString MuonAnalysis::dataAndTablesConsistent()
 {
   return QString();
 }