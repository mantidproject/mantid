//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidQtCustomInterfaces/MuonAnalysisResultTableTab.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/TableRow.h"

#include "MantidQtAPI/UserSubWindow.h"

#include <boost/shared_ptr.hpp>

#include <QLineEdit>
#include <QFileDialog>
#include <QHash>
#include <QMessageBox>
#include <QDesktopServices>

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

  const std::string MuonAnalysisResultTableTab::RUN_NO_LOG = "run_number";
  const std::string MuonAnalysisResultTableTab::RUN_NO_TITLE = "Run Number";
/**
* Constructor
*/
MuonAnalysisResultTableTab::MuonAnalysisResultTableTab(Ui::MuonAnalysis& uiForm)
  : m_uiForm(uiForm), m_numLogsdisplayed(0), m_selectedLogs(), m_unselectedFittings()
{  
  // Connect the help button to the wiki page.
  connect(m_uiForm.muonAnalysisHelpResults, SIGNAL(clicked()), this, SLOT(helpResultsClicked()));

  //Set the default name
  m_uiForm.tableName->setText("ResultsTable");

  // Connect the select/deselect all buttons.
  connect(m_uiForm.selectAllLogValues, SIGNAL(toggled(bool)), this, SLOT(selectAllLogs(bool)));
  connect(m_uiForm.selectAllFittingResults, SIGNAL(toggled(bool)), this, SLOT(selectAllFittings(bool)));

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
void MuonAnalysisResultTableTab::selectAllLogs(bool state)
{
  if (state)
  {
    for (int i = 0; i < m_uiForm.valueTable->rowCount(); i++)
    {
      QTableWidgetItem* temp = static_cast<QTableWidgetItem*>(m_uiForm.valueTable->item(i,0));
      // If there is an item there then check the box
      if (temp != NULL)
      {
        QCheckBox* includeCell = static_cast<QCheckBox*>(m_uiForm.valueTable->cellWidget(i,1));
        includeCell->setChecked(true);
      }
    }
  }
  else
  {
    for (int i = 0; i < m_uiForm.valueTable->rowCount(); i++)
    {
      QCheckBox* includeCell = static_cast<QCheckBox*>(m_uiForm.valueTable->cellWidget(i,1));
      includeCell->setChecked(false);
    }
  }
}


/**
* Select/Deselect all fitting results to be included in the table
*/
void MuonAnalysisResultTableTab::selectAllFittings(bool state)
{
  if (state)
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
 * Remembers which fittings and logs have been selected/deselected by the user. Used in combination with
 * applyUserSettings() so that we dont lose what the user has chosen when switching tabs.
 */
void MuonAnalysisResultTableTab::storeUserSettings()
{
  // Find which logs have been selected by the user.
  for (int row = 0; row < m_uiForm.valueTable->rowCount(); ++row)
  {
    QTableWidgetItem * temp = m_uiForm.valueTable->item(row,0);
    if( temp )
    {
      QCheckBox* logChoice = static_cast<QCheckBox*>(m_uiForm.valueTable->cellWidget(row,1));
      if( logChoice->isChecked() )
        m_selectedLogs += temp->text();
    }
  }

  // Find which fittings have been deselected by the user.
  for (int row = 0; row < m_uiForm.fittingResultsTable->rowCount(); ++row)
  {
    QTableWidgetItem * temp = m_uiForm.fittingResultsTable->item(row,0);
    if( temp )
    {
      QCheckBox* fittingChoice = static_cast<QCheckBox*>(m_uiForm.fittingResultsTable->cellWidget(row,1));
      if( ! fittingChoice->isChecked() )
        m_unselectedFittings += temp->text();
    }
  }
}

/**
 * Applies the stored lists of which fittings and logs have been selected/deselected by the user.
 */
void MuonAnalysisResultTableTab::applyUserSettings()
{
  // If we're just starting the tab for the first time (and there are no user choices),
  // then don't bother.
  if( m_selectedLogs.isEmpty() && m_unselectedFittings.isEmpty() )
    return;
  
  // If any of the logs have previously been selected by the user, select them again.
  for (int row = 0; row < m_uiForm.valueTable->rowCount(); ++row)
  {
    QTableWidgetItem * temp = m_uiForm.valueTable->item(row,0);
    if( temp )
    {
      if( m_selectedLogs.contains(temp->text()) )
      {
        QCheckBox* logChoice = static_cast<QCheckBox*>(m_uiForm.valueTable->cellWidget(row,1));
        logChoice->setChecked(true);
      }
    }
  }

  // If any of the fittings have previously been deselected by the user, deselect them again.
  for (int row = 0; row < m_uiForm.fittingResultsTable->rowCount(); ++row)
  {
    QTableWidgetItem * temp = m_uiForm.fittingResultsTable->item(row,0);
    if( temp )
    {
      if( m_unselectedFittings.contains(temp->text()) )
      {
        QCheckBox* fittingChoice = static_cast<QCheckBox*>(m_uiForm.fittingResultsTable->cellWidget(row,1));
        fittingChoice->setChecked(false);
      }
    }
  }
}

/**
* Populates the tables with all the correct log values and fitting results. It takes the
* given workspace list and checks to see if a fit has been done to the data set and if so
* adds it to a new workspace list.
*
* @param wsList :: A list containing all the data set workspaces that have been loaded
*                   by muon analysis.
*/
void MuonAnalysisResultTableTab::populateTables(const QStringList& wsList)
{
  storeUserSettings();
  
  // Clear the previous table values
  m_logValues.clear();
  QVector<QString> fittedWsList;
  // Get all the workspaces from the fitPropertyBrowser and find out whether they have had fitting done to them.
  for (int i(0); i<wsList.size(); ++i)
  {
    if((Mantid::API::AnalysisDataService::Instance().doesExist(wsList[i].toStdString() + "_Parameters"))&&(Mantid::API::AnalysisDataService::Instance().doesExist(wsList[i].toStdString()))) 
      fittedWsList.append(wsList[i]);
  }

  if(fittedWsList.size() > 0)
  { 
    // Make sure all params match.
    QVector<QString> sameFittedWsList(fittedWsList);

    // Clear the previous tables.
    const int fittingRowCount(m_uiForm.fittingResultsTable->rowCount());
    for (int i=0; i < fittingRowCount; ++i)
	    m_uiForm.fittingResultsTable->removeRow(0);
    const int logRowCount(m_uiForm.valueTable->rowCount());
    for (int i=0; i < logRowCount; ++i)
      m_uiForm.valueTable->removeRow(0);

    // Add number of rows  for the amount of fittings.
    for(int i=0; i < sameFittedWsList.size(); ++i)
      m_uiForm.fittingResultsTable->insertRow(m_uiForm.fittingResultsTable->rowCount() );

    // Add check boxes for the include column on fitting table, and make text uneditable.
    for (int i = 0; i < m_uiForm.fittingResultsTable->rowCount(); i++)
    {
      m_uiForm.fittingResultsTable->setCellWidget(i,1, new QCheckBox);
      QTableWidgetItem * textItem = m_uiForm.fittingResultsTable->item(i, 0);
      if(textItem)
        textItem->setFlags(textItem->flags() & (~Qt::ItemIsEditable));
    }

    // Populate the individual log values and fittings into their respective tables.
    populateFittings(sameFittedWsList);
    populateLogsAndValues(sameFittedWsList);    
    
    // Add check boxes for the include column on log table, and make text uneditable.
    for (int i = 0; i < m_uiForm.valueTable->rowCount(); i++)
    {
      m_uiForm.valueTable->setCellWidget(i,1, new QCheckBox);
      QTableWidgetItem * textItem = m_uiForm.valueTable->item(i, 0);
      if(textItem)
        textItem->setFlags(textItem->flags() & (~Qt::ItemIsEditable));
    }

    QTableWidgetItem* temp = static_cast<QTableWidgetItem*>(m_uiForm.valueTable->item(0,0));
    // If there is no item in the first row then there must be no log files found between the two data sets.
    if (temp == NULL)
    {
      QMessageBox::information(this, "Mantid - Muon Analysis", "There were no common log files found.");
    }
    else
    {
      // Make sure all fittings are selected by default.
      selectAllFittings(true);
    }

    // If we have Run Number log value, we want to select it by default.
    auto found = m_uiForm.valueTable->findItems(RUN_NO_TITLE.c_str(), Qt::MatchFixedString);
    if(!found.empty())
    {
      int r = found[0]->row();

      if(QCheckBox* cb = dynamic_cast<QCheckBox*>(m_uiForm.valueTable->cellWidget(r, 1)))
        cb->setCheckState(Qt::Checked); 
    }

    applyUserSettings();
  }
  else
  {
    QMessageBox::information(this, "Mantid - Muon Analysis", "A fitting must be made on the Data Analysis tab before producing a Results Table.");
  }
}


/**
* Populates the items (log values) into their table.
*
* @param fittedWsList :: a workspace list containing ONLY the workspaces that have parameter
*                   tables associated with it.
*/
void MuonAnalysisResultTableTab::populateLogsAndValues(const QVector<QString>& fittedWsList)
{
  // Clear the logs if not empty and then repopulate.
  QVector<QString> logsToDisplay;
  
  // Add run number explicitly as it is the only non-timeseries log value we are using 
  logsToDisplay.push_back(RUN_NO_TITLE.c_str());

  for (int i=0; i<fittedWsList.size(); i++)
  { 
    QMap<QString, QVariant> allLogs;

    // Get log information
    Mantid::API::ExperimentInfo_sptr ws = boost::dynamic_pointer_cast<Mantid::API::ExperimentInfo>(Mantid::API::AnalysisDataService::Instance().retrieve(fittedWsList[i].toStdString()));
    if (!ws)
    {
      throw std::runtime_error("Wrong type of Workspace");
    }

    const std::vector< Mantid::Kernel::Property * > & logData = ws->run().getLogData();
    std::vector< Mantid::Kernel::Property * >::const_iterator pEnd = logData.end();

    // Try to get a run number for the workspace
    if (ws->run().hasProperty(RUN_NO_LOG))
    {
      // Set run number as a string, as we don't want it to be formatted like double.
      allLogs[RUN_NO_TITLE.c_str()] = QString(ws->run().getLogData(RUN_NO_LOG)->value().c_str());
    }

    Mantid::Kernel::DateAndTime start = ws->run().startTime();
    Mantid::Kernel::DateAndTime end = ws->run().endTime();

    for( std::vector< Mantid::Kernel::Property * >::const_iterator pItr = logData.begin();
          pItr != pEnd; ++pItr )
    {  

      QString logFile(QFileInfo((**pItr).name().c_str()).fileName());
      // Just get the num.series log values
      Mantid::Kernel::TimeSeriesProperty<double> *tspd = dynamic_cast<Mantid::Kernel::TimeSeriesProperty<double> *>(*pItr);

      if( tspd )//If it is then it must be num.series
      {
        bool logFound(false);
        double value(0.0);
        double count(0.0);

        Mantid::Kernel::DateAndTime logTime;

        //iterate through all logs entries of a specific log
        for (int k(0); k<tspd->size(); k++)
        {
          // Get the log time for the specific entry
          logTime = tspd->nthTime(k);

          // If the entry was made during the run times
          if ((logTime >= start) && (logTime <= end))
          {
            // add it to a total and increment the count (will be used to make average entry value during a run)
            value += tspd->nthValue(k);
            count++;
            logFound = true;
          }
        }

        if (logFound == true)
        {
          //Find average
          allLogs[logFile] = value/count;
          if (i == 0)
            logsToDisplay.push_back(logFile);
          else
          {
            bool reg(true);
            for(int j=0; j<logsToDisplay.size(); ++j)
            {
              //if log file already registered then don't register it again.
              if (logsToDisplay[j] == logFile)
              {
                reg = false;
                break;
              }
            }
            if (reg==true)
              logsToDisplay.push_back(logFile);
          }
        }
      }
    }

    // Add all data collected from one workspace to another map. Will be used when creating table.
    m_logValues[fittedWsList[i]] = allLogs;

  } // End loop over all workspace's log information and param information

  // Remove the logs that don't appear in all workspaces
  QVector<int> toRemove;
  for(int i=0; i<logsToDisplay.size(); ++i)
  {
    for (auto itr = m_logValues.begin(); itr != m_logValues.end(); itr++)
    { 
      auto wsLogValues = itr.value();
      if (!wsLogValues.contains(logsToDisplay[i]))
      {      
        toRemove.push_back(i);
        break;
      }
    }      
  }

  for(int i=0; i<toRemove.size(); ++i)
  {
    logsToDisplay.remove(toRemove[i]-i);
  }
  
  // Add number of rows to the table based on number of logs to display.
  for (int i=0; i < logsToDisplay.size(); ++i)
    m_uiForm.valueTable->insertRow(m_uiForm.valueTable->rowCount() );

  // If there isn't enough rows in the table to populate all logs then display error message
  if(logsToDisplay.size() > m_uiForm.valueTable->rowCount())
  {
    QMessageBox::information(this, "Mantid - Muon Analysis", "There is not enough room in the table to populate all fitting parameter results");
  }
  else
  {
    // Populate table with all log values available without repeating any.
    for (int row = 0; row < m_uiForm.valueTable->rowCount(); row++)
    {
      if (row < logsToDisplay.size())
        m_uiForm.valueTable->setItem(row,0, new QTableWidgetItem(logsToDisplay[row]));
      else
        m_uiForm.valueTable->setItem(row,0, NULL);
    }
  }

  // Save the number of logs displayed so don't have to search through all cells.
  m_numLogsdisplayed = logsToDisplay.size();
}


/**
* Populates the items (fitted workspaces) into their table.
*
* @param fittedWsList :: a workspace list containing ONLY the workspaces that have parameter
*                        tables associated with it.
*/
void MuonAnalysisResultTableTab::populateFittings(const QVector<QString>& fittedWsList)
{
  if(fittedWsList.size() > m_uiForm.fittingResultsTable->rowCount())
  {
    QMessageBox::information(this, "Mantid - Muon Analysis", "There is not enough room in the table to populate all fitting parameter results");
  }
  else
  {
    // Get colors, 0=Black, 1=Red, 2=Green, 3=Blue, 4=Orange, 5=Purple. (If there are more than this then use black as default.)
    QMap<int, int> colors = getWorkspaceColors(fittedWsList);
    for (int row = 0; row < m_uiForm.fittingResultsTable->rowCount(); row++)
    {
      // Fill values and delete previous old ones.
      if (row < fittedWsList.size())
      {
        QTableWidgetItem *item = new QTableWidgetItem(fittedWsList[row]);
        int color(colors.find(row).data());
        switch (color)
        {
          case(1):
            item->setTextColor("red");
            break;
          case(2):
            item->setTextColor("green");
            break;
          case(3):
            item->setTextColor("blue");
            break;
          case(4):
            item->setTextColor("orange");
            break;
          case(5):
            item->setTextColor("purple");
            break;
          default:
            item->setTextColor("black");
        }
        m_uiForm.fittingResultsTable->setItem(row, 0, item);
      }
      else
        m_uiForm.fittingResultsTable->setItem(row,0, NULL);
    }
  }
}


/**
* Get the colors corresponding to their position in the workspace list.
*
* @param wsList :: List of all workspaces with fitted parameters.
* @return colors :: List of colors (as numbers) with the key being position in wsList.
*/
QMap<int, int> MuonAnalysisResultTableTab::getWorkspaceColors(const QVector<QString>& wsList)
{
  QMap<int,int> colors; //position, color
  int posCount(0);
  int colorCount(0);

  while (wsList.size() != posCount)
  {
    // If a color has already been chosen for the current workspace then skip
    if (!colors.contains(posCount))
    {
      std::vector<std::string> firstParams;
      // Find the first parameter table and use this as a comparison for all the other tables.
      Mantid::API::ITableWorkspace_sptr paramWs = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(wsList[posCount].toStdString() + "_Parameters") );

      Mantid::API::TableRow paramRow = paramWs->getFirstRow();
      do
      {  
        std::string key;
        paramRow >> key;
        firstParams.push_back(key);
      }
      while(paramRow.next());

      colors.insert(posCount, colorCount);

      // Compare to all the other parameters. +1 don't compare with self.
      for (int i=(posCount + 1); i<wsList.size(); ++i)
      {
        if (!colors.contains(i))
        {
          std::vector<std::string> nextParams;
          Mantid::API::ITableWorkspace_sptr paramWs = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(wsList[i].toStdString() + "_Parameters") );

          Mantid::API::TableRow paramRow = paramWs->getFirstRow();
          do
          {  
            std::string key;
            paramRow >> key;
            nextParams.push_back(key);
          }
          while(paramRow.next());

          if (firstParams == nextParams)
          {
            colors.insert(i, colorCount);
          }
        }
      }
      colorCount++;
    }
    posCount++;
  }
  return colors;
}


/**
* Creates the table using the information selected by the user in the tables
*/
void MuonAnalysisResultTableTab::createTable()
{  
  if (m_logValues.size() == 0)
  {
    QMessageBox::information(this, "Mantid - Muon Analysis", "No workspace found with suitable fitting.");
    return;
  }

  // Get the user selection
  QVector<QString> wsSelected = getSelectedWs();
  QVector<QString> logsSelected = getSelectedLogs();

  if ((wsSelected.size() == 0) || logsSelected.size() == 0)
  {
    QMessageBox::information(this, "Mantid - Muon Analysis", "Please select options from both tables.");
    return;
  }

  // Check workspaces have same parameters
  if (haveSameParameters(wsSelected))
  {
    // Create the results table
    Mantid::API::ITableWorkspace_sptr table = Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");

    // Add columns for log values 
    foreach(QString log, logsSelected)
    {
      std::string columnTypeName;
      int columnPlotType;

      // We use values of the first workspace to determine the type of the column to add. It seems reasonable to assume
      // that log values with the same name will have same types.
      QString typeName = m_logValues[wsSelected[0]][log].typeName();
      if (typeName == "double")
      {
        columnTypeName = "double";
        columnPlotType = 1;
      }
      else if(typeName == "QString")
      {
        columnTypeName = "str";
        columnPlotType = 6;
      }
      else
        throw std::runtime_error("Couldn't find appropriate column type for value with type " + typeName.toStdString());
        
      Mantid::API::Column_sptr newColumn = table->addColumn(columnTypeName, log.toStdString());
      newColumn->setPlotType(columnPlotType);
    }

    // Get param information
    QMap<QString, QMap<QString, double> > wsParamsList;
    QVector<QString> paramsToDisplay;
    for(int i=0; i<wsSelected.size(); ++i)
    {
      QMap<QString, double> paramsList;
      Mantid::API::ITableWorkspace_sptr paramWs = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(wsSelected[i].toStdString() + "_Parameters") );

      Mantid::API::TableRow paramRow = paramWs->getFirstRow();
    
      // Loop over all rows and get values and errors.
      do
      {  
        std::string key;
        double value;
        double error;
        paramRow >> key >> value >> error;
        if (i == 0)
        {
          table->addColumn("double", key);
          table->getColumn(table->columnCount()-1)->setPlotType(2);
          table->addColumn("double", key + "Error");
          table->getColumn(table->columnCount()-1)->setPlotType(5);
          paramsToDisplay.append(QString::fromStdString(key));
          paramsToDisplay.append(QString::fromStdString(key) + "Error");
        }
        paramsList[QString::fromStdString(key)] = value;
        paramsList[QString::fromStdString(key) + "Error"] = error;
      }
      while(paramRow.next());

      wsParamsList[wsSelected[i]] = paramsList;
    }

    // Add data to table
    for (auto itr = m_logValues.begin(); itr != m_logValues.end(); itr++)
    { 
      for(int i=0; i<wsSelected.size(); ++i)
      {
        if (wsSelected[i] == itr.key())
        {
          // Add new row
          Mantid::API::TableRow row = table->appendRow();

          // Add log values to the row
          QMap<QString, QVariant>& logValues = itr.value();

          for(int j = 0; j < logsSelected.size(); j++)
          {
            Mantid::API::Column_sptr c = table->getColumn(j);
            QVariant& v = logValues[logsSelected[j]];
            
            if(c->isType<double>())
              row << v.toDouble();
            else if(c->isType<std::string>())
              row << v.toString().toStdString();
            else
              throw std::runtime_error("Log value with name '" + logsSelected[j].toStdString() + "' in '" 
                + wsSelected[i].toStdString() + "' has unexpected type.");
          }

          // Add param values (presume params the same for all workspaces)
          QMap<QString, double> paramsList = wsParamsList.find(itr.key()).value();
          for(int j=0; j<paramsToDisplay.size(); ++j)
          {
            row << paramsList.find(paramsToDisplay[j]).value();
          }
        }
      }
    }  

    std::string tableName = getFileName();

    // Save the table to the ADS
    Mantid::API::AnalysisDataService::Instance().addOrReplace(tableName,table);

    // Python code to show a table on the screen
    std::stringstream code;
    code << "found = False" << std::endl
         << "for w in windows():" << std::endl
         << "  if w.windowLabel() == '" << tableName << "':" << std::endl
         << "    found = True; w.show(); w.setFocus()" << std::endl
         << "if not found:" << std::endl
         << "  importTableWorkspace('" << tableName << "', True)" << std::endl;

    emit runPythonCode(QString::fromStdString(code.str()), false);
  }
  else
  {
    QMessageBox::information(this, "Mantid - Muon Analysis", "Please pick workspaces with the same fitted parameters");
  }
}


/**
* See if the workspaces selected have the same parameters.
*
* @param wsList :: A list of workspaces with fitted parameters.
* @return bool :: Whether or not the wsList given share the same fitting parameters.
*/
bool MuonAnalysisResultTableTab::haveSameParameters(const QVector<QString>& wsList)
{
  std::vector<std::string> firstParams;

  // Find the first parameter table and use this as a comparison for all the other tables.
  Mantid::API::ITableWorkspace_sptr paramWs = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(wsList[0].toStdString() + "_Parameters") );

  Mantid::API::TableRow paramRow = paramWs->getFirstRow();
  do
  {  
    std::string key;
    paramRow >> key;
    firstParams.push_back(key);
  }
  while(paramRow.next());

  // Compare to all the other parameters.
  for (int i=1; i<wsList.size(); ++i)
  {
    std::vector<std::string> nextParams;
    Mantid::API::ITableWorkspace_sptr paramWs = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(Mantid::API::AnalysisDataService::Instance().retrieve(wsList[i].toStdString() + "_Parameters") );

    Mantid::API::TableRow paramRow = paramWs->getFirstRow();
    do
    {  
      std::string key;
      paramRow >> key;
      nextParams.push_back(key);
    }
    while(paramRow.next());

    if (!(firstParams == nextParams))
      return false;
  }
  return true;
}


/**
* Get the user selected workspaces with _parameters files associated with it
*
* @return wsSelected :: A vector of QString's containing the workspace that are selected.
*/
QVector<QString> MuonAnalysisResultTableTab::getSelectedWs()
{
  QVector<QString> wsSelected;
  for (int i = 0; i < m_logValues.size(); i++)
  {
    QCheckBox* includeCell = static_cast<QCheckBox*>(m_uiForm.fittingResultsTable->cellWidget(i,1));
    if (includeCell->isChecked())
    {
      QTableWidgetItem* wsName = static_cast<QTableWidgetItem*>(m_uiForm.fittingResultsTable->item(i,0));
      wsSelected.push_back(wsName->text());
    }
  }
  return wsSelected;
}


/**
* Get the user selected log file
*
* @return logsSelected :: A vector of QString's containing the logs that are selected.
*/
QVector<QString> MuonAnalysisResultTableTab::getSelectedLogs()
{
  QVector<QString> logsSelected;
  for (int i = 0; i < m_numLogsdisplayed; i++)
  {
    QCheckBox* includeCell = static_cast<QCheckBox*>(m_uiForm.valueTable->cellWidget(i,1));
    if (includeCell->isChecked())
    {
      QTableWidgetItem* logParam = static_cast<QTableWidgetItem*>(m_uiForm.valueTable->item(i,0));
      logsSelected.push_back(logParam->text());
    }
  }
  return logsSelected;
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
          tr(" already exists. Do you want to replace it?"),
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
