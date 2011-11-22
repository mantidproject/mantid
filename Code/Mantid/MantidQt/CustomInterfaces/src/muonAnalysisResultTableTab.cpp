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
  QStringList paramWsList;
  
  // Get all the workspaces from the fitPropertyBrowser and find out whether they have had fitting done to them.
  for (int i(0); i<wsList.size(); i++)
  {
    if(Mantid::API::AnalysisDataService::Instance().doesExist(wsList[i].toStdString() + "_parameters"))
      paramWsList.append(wsList[i]);
  }

  if (wsList.size() > paramWsList.size())
  {
    QMessageBox::information(this, "Mantid - Muon Analysis", "One or more of the data sets loaded has not got fitting information\n"
      "attached to it and will therefore not be displayed on the list.");
  }
  
  // populate the individual log values and fittings into their respective tables.
  populateLogValues(paramWsList);
  populateFittings(paramWsList);
}

/**
* Populates the items (log values) into their table.
*
* @params wsList :: a workspace list containing ONLY the workspaces that have parameter
*                   tables associated with it.
*/
void MuonAnalysisResultTableTab::populateLogValues(const QStringList& wsList)
{

}


/**
* Populates the items (fitted workspaces) into their table.
*
* @params wsList :: a workspace list containing ONLY the workspaces that have parameter
*                   tables associated with it.
*/
void MuonAnalysisResultTableTab::populateFittings(const QStringList& wsList)
{
  if(wsList.size() > m_uiForm.fittingResultsTable->rowCount())
  {
    QMessageBox::information(this, "Mantid - Muon Analysis", "There is not enough room in the table to populate all fitting parameter results");
  }
  else
  {
    for (int row = 0; row < wsList.size(); row++)
    {
      m_uiForm.fittingResultsTable->setItem(row,0, new QTableWidgetItem(wsList[row]));
    }
  }
}

/**
* Creates the table using the information selected by the user in the tables
*/
void MuonAnalysisResultTableTab::createTable()
{
  //std::string fileName(getFileName());

  //Mantid::API::ITableWorkspace_sptr resultsTable = Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
  //std::vector<std::string> logValuesAndParams;
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
