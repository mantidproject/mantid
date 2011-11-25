//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidQtCustomInterfaces/MuonAnalysisResultTableTab.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/TimeSeriesProperty.h"

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

#include <algorithm>

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
  // Clear the previous table values
  m_tableValues.clear();
  QVector<QString> fittedWsList;

  // Get all the workspaces from the fitPropertyBrowser and find out whether they have had fitting done to them.
  for (int i(0); i<wsList.size(); i++)
  {
    if((Mantid::API::AnalysisDataService::Instance().doesExist(wsList[i].toStdString() + "_parameters"))&&(Mantid::API::AnalysisDataService::Instance().doesExist(wsList[i].toStdString()))) 
      fittedWsList.append(wsList[i]);
  }

  // populate the individual log values and fittings into their respective tables.
  populateFittings(fittedWsList);
  populateLogAndParamValues(fittedWsList);
}

/**
* Populates the items (log values) into their table.
*
* @params wsList :: a workspace list containing ONLY the workspaces that have parameter
*                   tables associated with it.
*/
void MuonAnalysisResultTableTab::populateLogAndParamValues(const QVector<QString>& fittedWsList)
{
  // Clear the logs if not empty and then repopulate.
  QVector<QString> logsAndParamsToDisplay;
  
  for (int i=0; i<fittedWsList.size(); i++)
  { 
    QMap<QString, double> allLogsAndParams;
    
    // Get param information

    Mantid::API::ITableWorkspace_sptr paramWs = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(fittedWsList[i].toStdString() + "_parameters") );

    Mantid::API::TableRow row = paramWs->getFirstRow();
    
    // Loop over all rows and get values and errors.
    do
    {  
      std::string key;
      double value;
      double error;
      row >> key >> value >> error;
      if(i == 0)
      {
        logsAndParamsToDisplay.push_back(QString::fromStdString(key));
        logsAndParamsToDisplay.push_back(QString::fromStdString(key) + "Error");
      }
      allLogsAndParams[QString::fromStdString(key)] = value;
      allLogsAndParams[QString::fromStdString(key) + "Error"] = error;
    }
    while(row.next());

    // Get log information
    Mantid::API::ExperimentInfo_sptr ws = boost::dynamic_pointer_cast<Mantid::API::ExperimentInfo>(Mantid::API::AnalysisDataService::Instance().retrieve(fittedWsList[i].toStdString()));
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

      if( tspd )//If it is then it must be num.series
      {
        allLogsAndParams[logFile] = tspd->nthValue(0);
        if (i == 0)
          logsAndParamsToDisplay.push_back(logFile);
        else
        {
          bool reg(true);
          for(int j=0; j<logsAndParamsToDisplay.size(); ++j)
          {
            //if log file already registered then don't register it again.
            if (logsAndParamsToDisplay[j] == logFile)
            {
              reg = false;
              break;
            }
          }
          if (reg==true)
          logsAndParamsToDisplay.push_back(logFile);
        }
      }
    }

    // Add all data collected from one workspace to another map. Will be used when creating table.
    m_tableValues[fittedWsList[i]] = allLogsAndParams;

  } // End loop over all workspace's log information and param information

  QVector<int> toRemove;
  for(int i=0; i<logsAndParamsToDisplay.size(); ++i)
  {
    QMap<QString,QMap<QString, double> >::Iterator itr; 
    for (itr = m_tableValues.begin(); itr != m_tableValues.end(); itr++)
    { 
      QMap<QString, double> logParamsAndValues = itr.value();

      if (!(logParamsAndValues.contains(logsAndParamsToDisplay[i])))
      {      
        toRemove.push_back(i);
        break;
      }
    }      
  }

  for(int i=0; i<toRemove.size(); ++i)
  {
    logsAndParamsToDisplay.remove(toRemove[i]-i);
  }

  if(logsAndParamsToDisplay.size() > m_uiForm.valueTable->rowCount())
  {
    QMessageBox::information(this, "Mantid - Muon Analysis", "There is not enough room in the table to populate all fitting parameter results");
  }
  else
  {
    // Populate table with all log values available without repeating any.
    for (int row = 0; row < m_uiForm.valueTable->rowCount(); row++)
    {
      if (row < logsAndParamsToDisplay.size())
        m_uiForm.valueTable->setItem(row,0, new QTableWidgetItem(logsAndParamsToDisplay[row]));
      else
        m_uiForm.valueTable->setItem(row,0, NULL);
    }
  }

  m_numLogsAndParamsDisplayed = logsAndParamsToDisplay.size();
}


/**
* Populates the items (fitted workspaces) into their table.
*
* @params wsList :: a workspace list containing ONLY the workspaces that have parameter
*                   tables associated with it.
*/
void MuonAnalysisResultTableTab::populateFittings(const QVector<QString>& fittedWsList)
{
  if(fittedWsList.size() > m_uiForm.fittingResultsTable->rowCount())
  {
    QMessageBox::information(this, "Mantid - Muon Analysis", "There is not enough room in the table to populate all fitting parameter results");
  }
  else
  {
    for (int row = 0; row < m_uiForm.fittingResultsTable->rowCount(); row++)
    {
      // Fill values and delete previous old ones.
      if (row < fittedWsList.size())
        m_uiForm.fittingResultsTable->setItem(row,0, new QTableWidgetItem(fittedWsList[row]));
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
  if (m_tableValues.size() == 0)
  {
    QMessageBox::information(this, "Mantid - Muon Analysis", "No workspace found with suitable fitting.");
    return;
  }

  QVector<QString> wsSelected;
  QVector<QString> logParamSelected;

  //Get the user selected workspaces with _parameters files associated with it
  for (int i = 0; i < m_tableValues.size(); i++)
  {
    QCheckBox* includeCell = static_cast<QCheckBox*>(m_uiForm.fittingResultsTable->cellWidget(i,1));
    if (includeCell->isChecked())
    {
      QTableWidgetItem* wsName = static_cast<QTableWidgetItem*>(m_uiForm.fittingResultsTable->item(i,0));
      wsSelected.push_back(wsName->text());
    }
  }

  // Get the user selected log file and parameters
  for (int i = 0; i < m_numLogsAndParamsDisplayed; i++)
  {
    QCheckBox* includeCell = static_cast<QCheckBox*>(m_uiForm.valueTable->cellWidget(i,3));
    if (includeCell->isChecked())
    {
      QTableWidgetItem* logParam = static_cast<QTableWidgetItem*>(m_uiForm.valueTable->item(i,0));
      logParamSelected.push_back(logParam->text());
    }
  }

  if ((wsSelected.size() == 0) || logParamSelected.size() == 0)
  {
    QMessageBox::information(this, "Mantid - Muon Analysis", "Please select options from both tables.");
    return;
  }

  // Create the results table

  Mantid::API::ITableWorkspace_sptr table = Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
  table->addColumn("str","Run Number");
  for(int i=0; i<logParamSelected.size(); ++i)
  {
    table->addColumn("double", logParamSelected[i].toStdString());
  }

  // Add data to table

  QMap<QString,QMap<QString, double> >::Iterator itr; 
  for (itr = m_tableValues.begin(); itr != m_tableValues.end(); itr++)
  { 
    for(int i=0; i<wsSelected.size(); ++i)
    {
      if (wsSelected[i] == itr.key())
      {
        Mantid::API::TableRow row = table->appendRow();
        QString runNumber(itr.key().left(itr.key().find(';')));
        row << runNumber.toStdString();
      
        QMap<QString, double> logParamsAndValues = itr.value();

        // Can't have blank data values
        for(int j=0; j<logParamSelected.size(); ++j)
        {
          row << logParamsAndValues.find(logParamSelected[j]).value();
        }
      }
    }  
  }

  // Save the table to the ADS

  Mantid::API::AnalysisDataService::Instance().addOrReplace(getFileName(),table);
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
    int choice = QMessageBox::question(this, tr("MantidPlot - Overwrite Warning"), QString::fromStdString(fileName) +
          tr("already exists. Do you want to replace it?"),
          QMessageBox::Yes|QMessageBox::Default, QMessageBox::No|QMessageBox::Escape);
    if (choice == QMessageBox::No)
    {
      int versionNum(2); 
      fileName += " #";
      while(Mantid::API::AnalysisDataService::Instance().doesExist(fileName + boost::lexical_cast<std::string>(versionNum)))
      {
        versionNum += 1;
      }
      return (fileName + boost::lexical_cast<std::string>(versionNum));
    }
  }
  return fileName;
}


}
}
}
