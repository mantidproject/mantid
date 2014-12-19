#include "MantidTable.h"
#include "../ApplicationWindow.h"
#include "../Mantid/MantidUI.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Algorithm.h"

#include <QApplication>
#include <QMessageBox>
#include <QHash>

#include <iostream>
#include <qfontmetrics.h>

using namespace MantidQt::API;

//------------------------------------------------------------------------------------------------
/** Create MantidTable out of a ITableWorkspace
 *
 * @param env :: scripting environment (?)
 * @param ws :: ITableWorkspace to reproduce
 * @param label ::
 * @param parent :: parent window
 * @param transpose ::
 * @return the MantidTable created
 */
MantidTable::MantidTable(ScriptingEnv *env, Mantid::API::ITableWorkspace_sptr ws, const QString &label, 
    ApplicationWindow* parent, bool transpose):
Table(env,
  transpose? static_cast<int>(ws->columnCount()) : static_cast<int>(ws->rowCount()),
  transpose? static_cast<int>(ws->rowCount() + 1) : static_cast<int>(ws->columnCount()),
  label,parent,"",0),
m_ws(ws),
m_wsName(ws->getName()),
m_transposed(transpose)
{
  d_table->blockResizing(true);

  // Filling can take a while, so process any pending events and set appropriate cursor
  QApplication::processEvents();
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  // Set name and stuff
  parent->initTable(this, parent->generateUniqueName("Table-"));

  // Fill up the view
  this->fillTable();

  QApplication::restoreOverrideCursor();

  connect(this,SIGNAL(needToClose()),this,SLOT(closeTable()));
  connect(this,SIGNAL(needToUpdate()),this,SLOT(updateTable()));
  connect(d_table,SIGNAL(unwantedResize()),this,SLOT(dealWithUnwantedResize()));
  observePreDelete();
  observeAfterReplace();
}

//------------------------------------------------------------------------------------------------

/**
 * Updates the table when ws used is changed
 */
void MantidTable::updateTable()
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  // Fill with the new data
  fillTable();

  QApplication::restoreOverrideCursor();
}

/**
 * Respond to cellsAdded signal from d_table. 
 */
void MantidTable::dealWithUnwantedResize()
{
  if (static_cast<int>(m_ws->rowCount()) != d_table->numRows() || static_cast<int>(m_ws->columnCount()) != d_table->numCols())
  {
    updateTable();
  }
}

/** Refresh the table by filling it */
void MantidTable::fillTable()
{
  if (m_transposed)
  {
    fillTableTransposed();
    return;
  }

  // temporarily allow resizing
  d_table->blockResizing(false);

  setNumRows(0);
  setNumCols(0);

  // Resize to fit the new workspace
  setNumRows(static_cast<int>(m_ws->rowCount()));
  setNumCols(static_cast<int>(m_ws->columnCount()));

  // Add all columns
  for(int i=0; i < static_cast<int>(m_ws->columnCount());i++)
  {
    Mantid::API::Column_sptr c = m_ws->getColumn(static_cast<int>(i));
    QString colName = QString::fromStdString(c->name());
    setColName(i,colName);
    // Make columns of ITableWorkspaces read only, if specified
    setReadOnlyColumn(i, c->getReadOnly() );

    // If plot type is set in ws then update Table with that information
    int plotType = m_ws->getColumn(i)->getPlotType();

    if ( plotType != -1000 )
    {
      setColPlotDesignation(i, plotType);
    }

    // Special for errors?
    if (colName.endsWith("_err",Qt::CaseInsensitive) ||
        colName.endsWith("_error",Qt::CaseInsensitive))
    {
      setColPlotDesignation(i,Table::yErr);
    }

    setHeaderColType();

    // Track the column width. All text should fit in.
    int maxWidth = 60;
    QFontMetrics fm( this->getTextFont() );
    int thisWidth = fm.width(colName);
    if (thisWidth > maxWidth) maxWidth = thisWidth;

    // Print out the data in each row of this column
    for(int j=0; j < static_cast<int>(m_ws->rowCount()); j++)
    {
      std::ostringstream ostr;
      // This is the method on the Column object to convert to a string.
      c->print(static_cast<size_t>(j),ostr);
      QString qstr = QString::fromStdString(ostr.str());
      setText(j,i,qstr);

      // Measure the width
      thisWidth = fm.width(qstr);
      if (thisWidth > maxWidth) maxWidth = thisWidth;
    }

    // A bit of padding
    maxWidth += 10;
    // Avoid crazy widths
    if (maxWidth > 300) maxWidth = 300;
    setColumnWidth(i, maxWidth);
  }

  // Set all the row labels
  if (m_ws->rowCount() < 1000)
  {
    // Note: This is very slow for some reason so it is only done for smallish tables.
    for(int j=0; j < static_cast<int>(m_ws->rowCount()); j++)
      d_table->verticalHeader()->setLabel(j,QString::number(j));
  }

  // block resizing
  d_table->blockResizing(true);

}

/**
 * Make the transposed table.
 */
void MantidTable::fillTableTransposed()
{

  int ncols = static_cast<int>(m_ws->rowCount() + 1);
  int nrows = static_cast<int>(m_ws->columnCount());

  // temporarily allow resizing
  d_table->blockResizing(false);

  setNumRows(0);
  setNumCols(0);

  setNumCols(ncols);
  setNumRows(nrows);

  // Track the column width. All text should fit in.
  std::vector<int> maxWidth(numCols(), 6);
  QFontMetrics fm( this->getTextFont() );
  // Add all columns
  for(int i = 0; i < static_cast<int>(m_ws->columnCount()); ++i)
  {
    Mantid::API::Column_sptr c = m_ws->getColumn(static_cast<size_t>(i));

    QString colName = QString::fromStdString(c->name());

    int row = i;
    setText(row, 0, colName);

    int thisWidth = fm.width(colName) + 20;
    if (thisWidth > maxWidth[0]) maxWidth[0] = thisWidth;

    // Print out the data in each row of this column
    for(int j = 0; j < static_cast<int>(m_ws->rowCount()); ++j)
    {
      std::ostringstream ostr;
      // This is the method on the Column object to convert to a string.
      c->print(static_cast<size_t>(j),ostr);
      QString qstr = QString::fromStdString(ostr.str());

      int col = j + 1;
      setText(row, col, qstr);

      // Measure the width
      thisWidth = fm.width(qstr) + 20;
      if (thisWidth > maxWidth[col]) maxWidth[col] = thisWidth;
      // Avoid crazy widths
      if (maxWidth[col] > 300)
      {
        maxWidth[col] = 300;
      }
    }

  }
  // Make columns of ITableWorkspaces read only, and of the right width
  for(int j = 0; j < numCols(); ++j)
  {
    setReadOnlyColumn(j);
    setColumnWidth(j,maxWidth[j]);
    // make columns unplottable
    setColPlotDesignation(j,Table::None);
    if (j == 0)
    {
      setColName(j,"Name");
    }
    else
    {
      // give names to columns as corresponding row numbers in the TableWorkspace
      setColName(j,QString::number(j-1));
    }
  }

  d_table->blockResizing(true);

}

//------------------------------------------------------------------------------------------------
void MantidTable::closeTable()
{
  confirmClose(false);
  close();
}

//------------------------------------------------------------------------------------------------
void MantidTable::preDeleteHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws)
{
  Mantid::API::ITableWorkspace* ws_ptr = dynamic_cast<Mantid::API::ITableWorkspace*>(ws.get());
  if (!ws_ptr) return;
  if (ws_ptr == m_ws.get() || wsName == m_wsName)
  {
    emit needToClose();
  }
}

//------------------------------------------------------------------------------------------------
void MantidTable::afterReplaceHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws)
{
  Mantid::API::ITableWorkspace_sptr new_ws = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>(ws);
  if (!new_ws) return;
  if (new_ws.get() == m_ws.get() || wsName == m_wsName)
  {
    m_ws = new_ws;
    emit needToUpdate();
  }
}


//------------------------------------------------------------------------------------------------
/** Called when a cell is edited */
void MantidTable::cellEdited(int row,int col)
{
  if (m_transposed)
  {
    return;
  }

  std::string text = d_table->text(row,col).remove(QRegExp("\\s")).toStdString();
  Mantid::API::Column_sptr c = m_ws->getColumn(col);

  // Have the column convert the text to a value internally
  int index = row;
  c->read(index, text);

  // Set the table view to be the same text after editing.
  // That way, if the string was stupid, it will be reset to the old value.
  std::ostringstream s;
  c->print(index, s);
  d_table->setText(row, col, QString(s.str().c_str()));
}

//------------------------------------------------------------------------------------------------
/** Call an algorithm in order to delete table rows
 *
 * @param startRow :: row index (starts at 1) to start to remove
 * @param endRow :: end row index, inclusive, to remove
 */
void MantidTable::deleteRows(int startRow, int endRow)
{
  if (m_transposed)
  {
    QMessageBox::warning(this,"MantidPlot - Warning","Cannot delete rows in a transposed table");
    return;
  }

  Mantid::API::IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().create("DeleteTableRows");
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

//------------------------------------------------------------------------------------------------
/**\brief Returns true if the selected column is editable
 * \returns true if the table is editable
 */
bool MantidTable::isEditable() 
{
  bool retval = true;
  if ((this->selectedColumn() == -1) || this->table()->isColumnReadOnly(this->selectedColumn()))
  {
    retval = false;
  }
  return retval;
}

//------------------------------------------------------------------------------------------------
/**\brief Returns true if the table is sortable
 * \returns true if the table is sortable
 */
bool MantidTable::isSortable() 
{
  bool retval = false;
  if (!m_ws) return retval;
  if (m_ws->customSort())
  {
    // Currently only table workspaces that have a custom sort are sortable
    retval = true;
  }
  return retval;
}

//------------------------------------------------------------------------------------------------
/**\brief Sort the specified column.
 * @param col :: the column to be sorted
 * @param order :: 0 means ascending, anything else means descending
 */
void MantidTable::sortColumn(int col, int order)
{
  if (!m_ws) return;
  if (m_ws->customSort())
  {
    // Customized sorting routine for this TableWorkspace
    std::vector< std::pair<std::string, bool> > criteria;
    // Only one criterion in sorting
    criteria.push_back( std::pair<std::string, bool> (m_ws->getColumn(col)->name(), (order == 0)) );
    m_ws->sort(criteria);
    // Refresh the table
    this->fillTable();
  }
  else
  {
    // Fall-back to the default sorting of the table
    Table::sortColumn(col, order);
  }
}

//------------------------------------------------------------------------------------------------
/**\brief Sort the specified columns.
 * @param s :: the columns to be sorted
 * @param type :: 0 means sort individually (as in sortColumn()), anything else means together
 * @param order :: 0 means ascending, anything else means descending
 * @param leadCol :: for sorting together, the column which determines the permutation
 */
void MantidTable::sortColumns(const QStringList&s, int type, int order, const QString& leadCol)
{
  if (!m_ws) return;
  if (m_ws->customSort())
  {
    // Customized sorting routine for this TableWorkspace
    std::vector< std::pair<std::string, bool> > criteria;

    // Unmangle the column name, which comes as "TableName_ColName"
    std::string col = leadCol.toStdString();
    size_t n = col.rfind('_');
    if (n != std::string::npos && n < col.size()-1)
      col = col.substr(n+1, col.size()-n-1);
    // Only one criterion in sorting
    criteria.push_back( std::pair<std::string, bool> (col, (order == 0)) );
    m_ws->sort(criteria);
    // Refresh the table
    this->fillTable();
  }
  else
  {
    // Fall-back to the default sorting of the table
    Table::sortColumns(s, type, order, leadCol);
  }
}

void MantidTable::sortTableDialog()
{
  QHash<QString, QString> paramList;
  paramList["InputWorkspace"] = QString::fromStdString(m_wsName);
  applicationWindow()->mantidUI->showAlgorithmDialog("SortTableWorkspace",paramList);
}
