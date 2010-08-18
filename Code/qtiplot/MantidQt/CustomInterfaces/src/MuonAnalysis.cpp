//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/MuonAnalysis.h"
#include "MantidQtCustomInterfaces/IO_MuonGrouping.h"

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
  m_groupTablePlotChoice("Counts"), m_pairTablePlotChoice("Asymmetry"), m_groupNames(), m_groupingTempFilename("c:/tempMuonAnalysisGrouping.xml")
{
}

/// Set up the dialog layout
void MuonAnalysis::initLayout()
{
  m_uiForm.setupUi(this);

  // connect exit button
  connect(m_uiForm.runButton, SIGNAL(clicked()), this, SLOT(runClicked())); 

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

  // make as selected   
  connect(m_uiForm.selectGroupButton, SIGNAL(clicked()), this, SLOT(runSelectGroupButton())); 



  // add combo boxes to pairTable
  for (int i = 0; i < m_uiForm.pairTable->rowCount(); i++)
  {
    m_uiForm.pairTable->setCellWidget(i,1, new QComboBox);
    m_uiForm.pairTable->setCellWidget(i,2, new QComboBox);
  }

  QString filter;
  filter.append("Files (*.NXS *.nxs)");
  filter.append(";;All Files (*.*)");

  //m_uiForm.mwRunFiles = new MWRunFile; 
  //m_uiForm.mwRunFiles->allowMultipleFiles(false);
  //m_uiForm.fileInputLayout->insertWidget(0, m_uiForm.mwRunFiles);
  connect(m_uiForm.mwRunFiles, SIGNAL(fileChanged()), this, SLOT(inputFileChanged()));
}


/**
 * Make selected group
 */
void MuonAnalysis::runSelectGroupButton()
{
  QList<QTableWidgetItem *> items = m_uiForm.groupTable->selectedItems();

  if ( items.size() > 0 )
  {
    int row = items.at(0)->row();
  }
}


/**
 * Update front "group / group-pair" combo-box and pair-table combo-boxes
 * according to changes in group-table
 */
void MuonAnalysis::updateFrontGroupComboBox()
{
  m_uiForm.frontGroupGroupPairComboBox->clear();


  int numRows = m_uiForm.groupTable->rowCount();
  for (int i = 0; i < numRows; i++)
  {
    QTableWidgetItem *item = m_uiForm.groupTable->item(i,0);
    if (!item)
      break;
    if ( item->text().isEmpty() )
      break;

    m_uiForm.frontGroupGroupPairComboBox->addItem(item->text());
  }

  int rowNum = m_uiForm.pairTable->rowCount();
  for (int i = 0; i < rowNum; i++)
  {
    QComboBox* qw1 = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(i,1));
    qw1->clear();
    QComboBox* qw2 = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(i,2));
    qw2->clear();

    for (int ii = 0; ii < m_uiForm.frontGroupGroupPairComboBox->count(); ii++)
    {
      qw1->addItem( m_uiForm.frontGroupGroupPairComboBox->itemText(ii));
      qw2->addItem( m_uiForm.frontGroupGroupPairComboBox->itemText(ii));
    }
    
    if ( qw2->count() > 1 )
      qw2->setCurrentIndex(1);

  }


}


/**
 * Save grouping button
 */
void MuonAnalysis::runSaveGroupButton()
{
  saveGroupingTabletoXML(m_uiForm.groupTable, m_groupingTempFilename);
}


/**
 * Load grouping button
 */
void MuonAnalysis::runLoadGroupButton()
{
  // Get grouping file

  QString filter;
  filter.append("Files (*.XML *.xml)");
  filter += ";;AllFiles (*.*)";
  QString groupingFile = QFileDialog::getOpenFileName(this, "blah", "",filter);    
  if( groupingFile.isEmpty() || QFileInfo(groupingFile).isDir() ) 
    return;

  std::cout << groupingFile.toStdString() << std::endl;

  saveGroupingTabletoXML(m_uiForm.groupTable, m_groupingTempFilename);
  try
  {
    loadGroupingXMLtoTable(m_uiForm.groupTable, groupingFile.toStdString());
  }
  catch (Exception::FileError& e)
  {
    g_log.error(e.what());
    g_log.error("Revert to previous grouping");
    loadGroupingXMLtoTable(m_uiForm.groupTable, m_groupingTempFilename);
  }

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

  updateFrontGroupComboBox();
}

/**
 * Clear grouping button
 */
void MuonAnalysis::runClearGroupingButton()
{
  m_uiForm.groupTable->clearContents();
  m_uiForm.pairTable->clearContents();
  m_uiForm.frontGroupGroupPairComboBox->clear();

  for (int i = 0; i < m_uiForm.pairTable->rowCount(); i++)
  {
    m_uiForm.pairTable->setCellWidget(i,1, new QComboBox);
    m_uiForm.pairTable->setCellWidget(i,2, new QComboBox);
  }
}


/**
 * convert int to string
 */
std::string MuonAnalysis::iToString(int i)
{
  std::stringstream str;
  str << i;
  return str.str();
}

/**
 * change group table plotting choice
 */
void MuonAnalysis::runGroupTablePlotChoice(const QString str)
{
  m_groupTablePlotChoice = str.toStdString();
}

/**
 * change pair table plotting choice
 */
void MuonAnalysis::runPairTablePlotChoice(const QString str)
{
  m_pairTablePlotChoice = str.toStdString();
}


/**
 * Group table plot button
 */
void MuonAnalysis::runGroupTablePlotButton()
{
  if ( m_groupTableRowInFocus >= 0 )
  {

    // create Python string 
    QString pyString;
    if (m_groupTablePlotChoice.compare("Counts") == 0)
    {
      pyString.append("plotSpectrum(\"");
      pyString.append(m_workspace_name.c_str());
      if (m_period > 0)
      {
        pyString.append("_");
        pyString.append(iToString(m_period).c_str());
      }
      pyString.append("\",");
      pyString.append(iToString(m_groupTableRowInFocus).c_str());
      pyString.append(")");
    }
    else if (m_groupTablePlotChoice.compare("Asymmetry") == 0)
    {
      pyString.append("RemoveExpDecay(\"");
      pyString.append(m_workspace_name.c_str());
      if (m_period > 0)
      {
        pyString.append("_");
        pyString.append(iToString(m_period).c_str());
      }
      pyString.append("\",\"");
      pyString.append(m_workspace_name.c_str());
      pyString.append("_asym\",");
      pyString.append(iToString(m_groupTableRowInFocus).c_str());
      pyString.append("); plotSpectrum(\"");
      pyString.append(m_workspace_name.c_str());
      pyString.append("_asym");
      pyString.append("\",");
      pyString.append(iToString(m_groupTableRowInFocus).c_str());
      pyString.append(")");
    }
    else if (m_groupTablePlotChoice.compare("Logorithm") == 0)
    {
      pyString.append("Logarithm(\"");
      pyString.append(m_workspace_name.c_str());
      if (m_period > 0)
      {
        pyString.append("_");
        pyString.append(iToString(m_period).c_str());
      }
      pyString.append("\",\"");
      pyString.append(m_workspace_name.c_str());
      pyString.append("_log\",");
      pyString.append(iToString(m_groupTableRowInFocus).c_str());
      pyString.append("); plotSpectrum(\"");
      pyString.append(m_workspace_name.c_str());
      pyString.append("_log");
      pyString.append("\",");
      pyString.append(iToString(m_groupTableRowInFocus).c_str());
      pyString.append(")");
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
 * Pair table plot button
 */
void MuonAnalysis::runPairTablePlotButton()
{
  if ( m_pairTableRowInFocus >= 0 )
  {
    // only plot if alpha defined in table
    QTableWidgetItem *item = m_uiForm.pairTable->item(m_pairTableRowInFocus,3);
    if (!item)
      return;

    // create Python string 
    QString pyString;
    if (m_pairTablePlotChoice.compare("Asymmetry") == 0)
    {
      QComboBox* qw1 = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(m_pairTableRowInFocus,1));
      QComboBox* qw2 = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(m_pairTableRowInFocus,2));

      pyString.append("AsymmetryCalc(\"");
      pyString.append(m_workspace_name.c_str());
      if (m_period > 0)
      {
        pyString.append("_");
        pyString.append(iToString(m_period).c_str());
      }
      pyString.append("\",\"");
      pyString.append(m_workspace_name.c_str());
      pyString.append("_pair_asym\",");
      pyString.append(iToString(qw1->currentIndex()).c_str());
      pyString.append(", ");
      pyString.append(iToString(qw2->currentIndex()).c_str());
      pyString.append("); plotSpectrum(\"");
      pyString.append(m_workspace_name.c_str());
      pyString.append("_pair_asym");
      pyString.append("\",");
      pyString.append(iToString(0).c_str());
      pyString.append(")");
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
 * Group table clicked
 */
void MuonAnalysis::groupTableClicked(int row, int column)
{
  if ( m_uiForm.groupTable->item(row,2) != NULL )
    m_groupTableRowInFocus = row;
  else
    m_groupTableRowInFocus = -1;
}

/**
 * Group table clicked
 */
void MuonAnalysis::pairTableClicked(int row, int column)
{
  if ( m_uiForm.pairTable->item(row,3) != NULL )
    m_pairTableRowInFocus = row;
  else
    m_pairTableRowInFocus = -1;
}

/**
 * Group table changed
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
 * Is Grouping set.
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
 * Apply grouping to workspace. If no 2nd argument try to use whatever grouping
 * is currently specified
 */
void MuonAnalysis::applyGroupingToWS( const std::string& wsName, std::string filename)
{
  if ( filename.compare("") == 0 )
  {
    if ( isGroupingSet() )
    {
      saveGrouping();
      filename = m_groupingTempFilename;
    }
    else
      return;
  }

  QString pyString = "GroupDetectors('";
  pyString.append(wsName.c_str());
  pyString.append("','");
  pyString.append(wsName.c_str());
  pyString.append("','");
  pyString.append(filename.c_str());
  pyString.append("');");
  
}

/**
 * Save grouping to file. If filename is empty string save
 * to temporary grouping file
 *
 * @param filename Filename to save grouping to
 */
void MuonAnalysis::saveGrouping( std::string filename )
{
  if ( isGroupingSet() ) 
  {
    if ( filename.compare("") == 0 )
    {
      filename = m_groupingTempFilename;
    }
  }

  // save grouping to xml grouping file

}

/**
 * Input file changed. Update information accordingly
 */
void MuonAnalysis::inputFileChanged()
{
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
  Workspace_sptr workspace_ptr = AnalysisDataService::Instance().retrieve(m_workspace_name);
  WorkspaceGroup_sptr wsPeriods = boost::dynamic_pointer_cast<WorkspaceGroup>(workspace_ptr);
  MatrixWorkspace_sptr matrix_workspace;
  int numPeriods = 1;   // 1 may mean either a group with one period or simply just 1 normal matrix workspace
  if (wsPeriods)
  {
    numPeriods = wsPeriods->getNumberOfEntries() - 1;  // note getNumberOfEntries returns one more # of periods 
    for ( int i = 1; i <= numPeriods; i++)
    {
      // apply grouping if specified in group table
      applyGroupingToWS( m_workspace_name + iToString(i));
    }
    Workspace_sptr workspace_ptr1 = AnalysisDataService::Instance().retrieve(m_workspace_name + "_1");
    matrix_workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(workspace_ptr1);
    m_period = 1;
  }
  else
  {
    // apply grouping if specified in group table
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
  while ( m_uiForm.homePeriodBox2->count() != 0 )
    m_uiForm.homePeriodBox2->removeItem(0);

  for ( int i = 1; i <= numPeriods; i++ )
  {
    std::stringstream strInt;
    strInt << i;
    m_uiForm.homePeriodBox1->addItem(strInt.str().c_str());
  }

  if ( numPeriods == 1 )
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
  }

  // Populate grouping table

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
    }
  }
  updateFrontGroupComboBox();
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

  for (int i = 0; i < values.count(); i++)
  {
    int found= values[0].find("-");
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
 * Test
 */
void MuonAnalysis::runClicked()
{

  QMessageBox::information(this, "MantidPlot", "Run clicked!!!!!");
}
