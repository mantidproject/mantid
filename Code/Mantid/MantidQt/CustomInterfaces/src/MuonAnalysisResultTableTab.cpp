//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidQtCustomInterfaces/MuonAnalysisResultTableTab.h"
#include "MantidKernel/ConfigService.h"

#include "MantidQtAPI/UserSubWindow.h"

#include <boost/shared_ptr.hpp>
#include <fstream>  

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

//-----------------------------------------------------------------------------


namespace MantidQt
{
namespace CustomInterfaces
{
namespace Muon
{
  using namespace MantidQt::API;
  using namespace Mantid::Kernel;
  using namespace MantidQt::MantidWidgets;

/**
* Init the layout.
*/
void MuonAnalysisResultTableTab::initLayout()
{
  // Connect the help button to the wiki page.
  connect(m_uiForm.muonAnalysisHelpResults, SIGNAL(clicked()), this, SLOT(helpResultsClicked()));
  
  // add check boxes for the include columns on log table and fitting table.
  for (int i = 0; i < m_uiForm.valueTable->rowCount(); i++)
  {
    m_uiForm.valueTable->setCellWidget(i,3, new QCheckBox);
  }
  for (int i = 0; i < m_uiForm.fittingResultsTable->rowCount(); i++)
  {
    m_uiForm.fittingResultsTable->setCellWidget(i,1, new QCheckBox);
  }

  //Set the default name
  m_uiForm.tableName->setText("ResultsTable");

  // Connect the select/deselect all buttons.
  connect(m_uiForm.selectAllLogValues, SIGNAL(clicked()), this, SLOT(selectAllLogs()));
  connect(m_uiForm.selectAllFittingResults, SIGNAL(clicked()), this, SLOT(selectAllFittings()));

  // Connect the create table button
  connect(m_uiForm.createTableBtn, SIGNAL(clicked()), this, SLOT(createTable()));
}


/**
* Muon Analysis Results Table Help (slot)
*/
void MuonAnalysisResultTableTab::helpResultsClicked()
{
  QDesktopServices::openUrl(QUrl(QString("http://www.mantidproject.org/") +
            "MuonAnalysisResultsTable"));
}


/**
* Select/Deselect all log values to be included in the table
*/
void MuonAnalysisResultTableTab::selectAllLogs()
{
  if (m_uiForm.selectAllLogValues->isChecked())
  {
    for (int i = 0; i < m_uiForm.valueTable->rowCount(); i++)
    {
      QTableWidgetItem* temp = static_cast<QTableWidgetItem*>(m_uiForm.valueTable->item(i,0));
      // If there is an item there then check the box
      if (temp != NULL)
      {
        QCheckBox* includeCell = static_cast<QCheckBox*>(m_uiForm.valueTable->cellWidget(i,3));
        includeCell->setChecked(true);
      }
    }
  }
  else
  {
    for (int i = 0; i < m_uiForm.valueTable->rowCount(); i++)
    {
      QCheckBox* includeCell = static_cast<QCheckBox*>(m_uiForm.valueTable->cellWidget(i,3));
      includeCell->setChecked(false);
    }
  }
}


/**
* Select/Deselect all fitting results to be included in the table
*/
void MuonAnalysisResultTableTab::selectAllFittings()
{
  if (m_uiForm.selectAllFittingResults->isChecked())
  {
    for (int i = 0; i < m_uiForm.fittingResultsTable->rowCount(); i++)
    {
      QTableWidgetItem* temp = static_cast<QTableWidgetItem*>(m_uiForm.fittingResultsTable->item(i,0));
      // If there is an item there then check the box
      if (temp != NULL)
      {
        QCheckBox* includeCell = static_cast<QCheckBox*>(m_uiForm.fittingResultsTable->cellWidget(i,1));
        includeCell->setChecked(true);
      }
    }
  }
  else
  {
    for (int i = 0; i < m_uiForm.fittingResultsTable->rowCount(); i++)
    {
      QCheckBox* includeCell = static_cast<QCheckBox*>(m_uiForm.fittingResultsTable->cellWidget(i,1));
      includeCell->setChecked(false);
    }
  }
}

/**
* Populates the tables with all the correct log values and fitting results. It takes the
* given workspace list and checks to see if a fit has been done to the data set and if so
* adds it to a new workspace list.
*
* @params wsList :: A list containing all the data set workspaces that have been loaded
*                   by muon analysis.
*/
void MuonAnalysisResultTableTab::populateTables(const QStringList& wsList)
{
  m_wsList.clear();
  
  // Get all the workspaces from the fitPropertyBrowser and find out whether they have had fitting done to them.
  for (int i(0); i<wsList.size(); i++)
  {
    if((Mantid::API::AnalysisDataService::Instance().doesExist(wsList[i].toStdString() + "_parameters"))&&(Mantid::API::AnalysisDataService::Instance().doesExist(wsList[i].toStdString()))) 
      m_wsList.append(wsList[i]);
  }

  // Error message if one of more data sets loaded by muon analysis don't have parameter information associated with it.
  //if (wsList.size() > m_wsList.size())
  //{
  //  QMessageBox::information(this, "Mantid - Muon Analysis", "One or more of the data sets loaded has not got fitting information\n"
  //    "attached to it and will therefore not be displayed on the list.");
  //}

  // populate the individual log values and fittings into their respective tables.
  populateLogValues();
  populateFittings();
}

/**
* Populates the items (log values) into their table.
*
* @params wsList :: a workspace list containing ONLY the workspaces that have parameter
*                   tables associated with it.
*/
void MuonAnalysisResultTableTab::populateLogValues()
{
  // Clear the logs if not empty and then repopulate.
  m_logs.clear();
  
  for (int i=0; i<m_wsList.size(); i++)
  {
    // Get log information
    Mantid::API::ExperimentInfo_sptr ws = boost::dynamic_pointer_cast<Mantid::API::ExperimentInfo>(Mantid::API::AnalysisDataService::Instance().retrieve(m_wsList[i].toStdString()));
    if (!ws)
    {
      throw std::runtime_error("Wrong type of Workspace");
    }
    
    const std::vector< Mantid::Kernel::Property * > & logData = ws->run().getLogData();
    std::vector< Mantid::Kernel::Property * >::const_iterator pEnd = logData.end();
    for( std::vector< Mantid::Kernel::Property * >::const_iterator pItr = logData.begin();
          pItr != pEnd; ++pItr )
    {
      QString logFile(QFileInfo((**pItr).name().c_str()).fileName());
      // Just get the num.series log values
      Mantid::Kernel::TimeSeriesProperty<double> *tspd = dynamic_cast<Mantid::Kernel::TimeSeriesProperty<double> *>(*pItr);
      Mantid::Kernel::TimeSeriesProperty<int>    *tspi = dynamic_cast<Mantid::Kernel::TimeSeriesProperty<int> *>(*pItr);
      Mantid::Kernel::TimeSeriesProperty<bool>   *tspb = dynamic_cast<Mantid::Kernel::TimeSeriesProperty<bool> *>(*pItr);

      if( tspd || tspi || tspb ) //If it is then it must be num.series
      {
        // Check to see if already registered log.
        bool isAlreadyLog(false);
        for (int j=0; j<m_logs.size(); ++j)
        {
          if (m_logs[j] == logFile)
            isAlreadyLog = true;
        }
        if (!isAlreadyLog)
          m_logs.push_back(logFile);

        //std::ostringstream msg;
        if (tspd)
          std::cout << "\n\n" << tspd->nthValue(0) << "\n\n";
        else if (tspi)
          std::cout << "\n\n" << tspi->nthValue(0) << "\n\n";
        else if (tspb)
          std::cout << "\n\n" << tspb->nthValue(0) << "\n\n";
      }
    }
  } // End loop over all workspace's log information

  if(m_logs.size() > m_uiForm.valueTable->rowCount())
  {
    QMessageBox::information(this, "Mantid - Muon Analysis", "There is not enough room in the table to populate all fitting parameter results");
  }
  else
  {
    // Populate table with all log values available without repeating any.
    for (int row = 0; row < m_uiForm.valueTable->rowCount(); row++)
    {
      if (row < m_logs.size())
        m_uiForm.valueTable->setItem(row,0, new QTableWidgetItem(m_logs[row]));
      else
        m_uiForm.valueTable->setItem(row,0, NULL);
    }
  }
}


/**
* Populates the items (fitted workspaces) into their table.
*
* @params wsList :: a workspace list containing ONLY the workspaces that have parameter
*                   tables associated with it.
*/
void MuonAnalysisResultTableTab::populateFittings()
{
  if(m_wsList.size() > m_uiForm.fittingResultsTable->rowCount())
  {
    QMessageBox::information(this, "Mantid - Muon Analysis", "There is not enough room in the table to populate all fitting parameter results");
  }
  else
  {
    for (int row = 0; row < m_uiForm.fittingResultsTable->rowCount(); row++)
    {
      // Fill values and delete previous old ones.
      if (row < m_wsList.size())
        m_uiForm.fittingResultsTable->setItem(row,0, new QTableWidgetItem(m_wsList[row]));
      else
        m_uiForm.fittingResultsTable->setItem(row,0, NULL);
    }
  }
}

/**
* Creates the table using the information selected by the user in the tables
*/
void MuonAnalysisResultTableTab::createTable()
{
  //std::string fileName(getFileName());
  
  QVector<QString> wsSelected;
  //std::vector<std::string> logValuesAndParams;
  
  //Get the user selected workspaces with _parameters files associated with it
  for (int i = 0; i < m_wsList.size(); i++)
  {
    QCheckBox* includeCell = static_cast<QCheckBox*>(m_uiForm.fittingResultsTable->cellWidget(i,1));
    if (includeCell->isChecked())
    {
      QTableWidgetItem* wsName = static_cast<QTableWidgetItem*>(m_uiForm.fittingResultsTable->item(i,0));
      wsSelected.push_back(wsName->text());
    }
  }

  // Extract information from the _parameters files.
  for (int i = 0; i<wsSelected.size(); i++)
  {
    Mantid::API::ITableWorkspace_sptr paramTable = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(wsSelected[i].toStdString() + "_parameters"));
    Mantid::API::TableRowHelper row = paramTable->getRow(0);
  }

  //Mantid::API::ITableWorkspace_sptr resultsTable = Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
  //
}


/**
* Checks that the file name isn't been used, displays the appropriate message and
* then returns the name in which to save.
*
* @return name :: The name the results table should be created with.
*/
std::string MuonAnalysisResultTableTab::getFileName()
{
  std::string fileName(m_uiForm.tableName->text());
  
  if (Mantid::API::AnalysisDataService::Instance().doesExist(fileName))
  {
    int versionNum(2); 
    fileName += " #";
    while(Mantid::API::AnalysisDataService::Instance().doesExist(fileName + boost::lexical_cast<std::string>(versionNum)))
    {
      versionNum += 1;
    }
    return (fileName + boost::lexical_cast<std::string>(versionNum));
  }
  else
    return fileName;
}


}
}
}
