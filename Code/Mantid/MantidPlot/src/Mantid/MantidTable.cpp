#include "MantidTable.h"
#include "../ApplicationWindow.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Algorithm.h"

#include <QMessageBox>
#include <iostream>


//------------------------------------------------------------------------------------------------
/** Create MantidTable out of a ITableWorkspace
 *
 * @param env :: scripting environment (?)
 * @param ws :: ITableWorkspace to reproduce
 * @param label
 * @param parent :: parent window
 * @param name
 * @param f :: flags (?)
 * @return the MantidTable created
 */
MantidTable::MantidTable(ScriptingEnv *env, Mantid::API::ITableWorkspace_sptr ws, const QString &label, 
    ApplicationWindow* parent, const QString& name, Qt::WFlags f):
Table(env,ws->rowCount(),ws->columnCount(),label,parent,"",f),
m_ws(ws)
{
  // Set name and stuff
  parent->initTable(this, parent->generateUniqueName(name+"-"));
  //  askOnCloseEvent(false);
  // Fill up the view
  this->fillTable();

  connect(this,SIGNAL(needToClose()),this,SLOT(closeTable()));
  observeDelete();
  observeAfterReplace();
}

//------------------------------------------------------------------------------------------------
/** Refresh the table by filling it */
void MantidTable::fillTable()
{
  setNumCols(m_ws->columnCount());
  setNumRows(m_ws->rowCount());

  // Add all columns
  for(int i=0;i<m_ws->columnCount();i++)
  {
    Mantid::API::Column_sptr c = m_ws->getColumn(i);
    QString colName = QString::fromStdString(c->name());
    setColName(i,colName);
    // Make columns of ITableWorkspaces read only, if specified
    setReadOnlyColumn(i, c->getReadOnly() );
    // Special for errors?
    if (colName.endsWith("_err",Qt::CaseInsensitive) ||
        colName.endsWith("_error",Qt::CaseInsensitive))
    {
      setColPlotDesignation(i,Table::yErr);
    }
    // Print out the data in each row of this column
    for(int j=0; j<m_ws->rowCount(); j++)
    {
      std::ostringstream ostr;
      // This is the method on the Column object to convert to a string.
      c->print(ostr,j);
      setText(j,i,QString::fromStdString(ostr.str()));
    }
  }

}

//------------------------------------------------------------------------------------------------
void MantidTable::closeTable()
{
  askOnCloseEvent(false);
  close();
}

//------------------------------------------------------------------------------------------------
void MantidTable::deleteHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws)
{
  UNUSED_ARG(wsName)
  if (dynamic_cast<Mantid::API::ITableWorkspace*>(ws.get()) == m_ws.get())
  {
    emit needToClose();
  }
}

//------------------------------------------------------------------------------------------------
void MantidTable::afterReplaceHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws)
{
  UNUSED_ARG(wsName)
  if (dynamic_cast<Mantid::API::ITableWorkspace*>(ws.get()) == m_ws.get())
  {
    fillTable();
  }
}


//------------------------------------------------------------------------------------------------
/** Call an algorithm in order to delete table rows
 *
 * @param startRow :: row index (starts at 1) to start to remove
 * @param endRow :: end row index, inclusive, to remove
 */
void MantidTable::deleteRows(int startRow, int endRow)
{
  Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("DeleteTableRow");
  alg->setPropertyValue("TableWorkspace",m_ws->getName());
  QStringList rows;
  rows << QString::number(startRow - 1) << QString::number(endRow - 1);
  alg->setPropertyValue("Rows",rows.join("-").toStdString());
  try
  {
    alg->execute();
  }
  catch(...)
  {
    QMessageBox::critical(this,"MantidPlot - Error", "DeleteTableRow algorithm failed");
  }
}
