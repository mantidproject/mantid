#include "MantidTable.h"
#include "../ApplicationWindow.h"
#include "MantidAPI/Column.h"

#include <iostream>

MantidTable::MantidTable(ScriptingEnv *env, Mantid::API::ITableWorkspace_sptr ws, const QString &label, 
    ApplicationWindow* parent, const QString& name, Qt::WFlags f):
Table(env,ws->rowCount(),ws->columnCount(),label,parent,"",f),
m_ws(ws)
{
  parent->initTable(this, parent->generateUniqueName(name+"-"));
  //  askOnCloseEvent(false);

  for(int i=0;i<ws->columnCount();i++)
  {
    Mantid::API::Column_sptr c = ws->getColumn(i);
    QString colName = QString::fromStdString(c->name());
    setColName(i,colName);
    setReadOnlyColumn(i);
    if (colName.endsWith("_err",Qt::CaseInsensitive) ||
        colName.endsWith("_error",Qt::CaseInsensitive))
    {
      setColPlotDesignation(i,Table::yErr);
    }
    for(int j=0;j<ws->rowCount();j++)
    {
      std::ostringstream ostr;
      c->print(ostr,j);
      setText(j,i,QString::fromStdString(ostr.str()));
    }
  }
  std::cerr << "Mantid\n";

  connect(this,SIGNAL(needToClose()),this,SLOT(closeTable()));
  observeDelete();
  observeAfterReplace();
}

void MantidTable::fillTable()
{
  setNumCols(m_ws->columnCount());
  setNumRows(m_ws->rowCount());
  for(int i=0;i<m_ws->columnCount();i++)
  {
    Mantid::API::Column_sptr c = m_ws->getColumn(i);
    QString colName = QString::fromStdString(c->name());
    setColName(i,colName);
    setReadOnlyColumn(i);
    if (colName.endsWith("_err",Qt::CaseInsensitive) ||
        colName.endsWith("_error",Qt::CaseInsensitive))
    {
      setColPlotDesignation(i,Table::yErr);
    }
    for(int j=0;j<m_ws->rowCount();j++)
    {
      std::ostringstream ostr;
      c->print(ostr,j);
      setText(j,i,QString::fromStdString(ostr.str()));
    }
  }
}

void MantidTable::closeTable()
{
  askOnCloseEvent(false);
  close();
}

void MantidTable::deleteHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws)
{
  if (dynamic_cast<Mantid::API::ITableWorkspace*>(ws.get()) == m_ws.get())
  {
    emit needToClose();
  }
}

void MantidTable::afterReplaceHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws)
{
  if (dynamic_cast<Mantid::API::ITableWorkspace*>(ws.get()) == m_ws.get())
  {
    fillTable();
  }
}
