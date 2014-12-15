#ifndef MANTIDTABLE_H
#define MANTIDTABLE_H

#include "../Table.h"
#include "MantidQtAPI/WorkspaceObserver.h"
#include "MantidAPI/ITableWorkspace.h"


/** A MantidTable appears to be a MantidPlot "Table" object
 * that shows the data from an ITableWorkspace.
 *
 */
class MantidTable: public Table, public MantidQt::API::WorkspaceObserver
{
  Q_OBJECT
public:
  MantidTable(ScriptingEnv *env, Mantid::API::ITableWorkspace_sptr ws, const QString &label, 
    ApplicationWindow* parent, bool transpose = false);

  /// returns the workspace name
  const std::string & getWorkspaceName() {return m_wsName; }

  //! is this table editable
  virtual bool isEditable();
  //! is this table sortable
  virtual bool isSortable();
  //! are the columns fixed - not editable by the GUI
  virtual bool isFixedColumns() {return true;}

  virtual void sortTableDialog();

signals:
  void needToClose();
  void needToUpdate();

public slots:
  void deleteRows(int startRow, int endRow);
  void cellEdited(int,int col);

protected slots:
  void closeTable();
  void fillTable();
  void fillTableTransposed();
  void updateTable();
  void dealWithUnwantedResize();

protected:
  void preDeleteHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws);
  void afterReplaceHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws);

  // Reimplemented methods for custom sorting of TableWorkspaces
  virtual void sortColumn(int col, int order);
  virtual void sortColumns(const QStringList& cols, int type = 0, int order = 0, const QString& leadCol = QString());

private:
  /// ITableWorkspace being displayed
  Mantid::API::ITableWorkspace_sptr m_ws;
  /// Name of the TableWorkspace being displayed
  const std::string m_wsName;
  /// Show the table workspace transposed
  bool m_transposed;

};

#endif /* MANTIDTABLE_H */
