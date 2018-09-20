#ifndef MANTIDTABLE_H
#define MANTIDTABLE_H

#include "../Table.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidQtWidgets/Common/WorkspaceObserver.h"

/** A MantidTable appears to be a MantidPlot "Table" object
 * that shows the data from an ITableWorkspace.
 *
 */
class MantidTable : public Table, public MantidQt::API::WorkspaceObserver {
  Q_OBJECT
public:
  MantidTable(ScriptingEnv *env, Mantid::API::ITableWorkspace_sptr ws,
              const QString &label, ApplicationWindow *parent,
              bool transpose = false);

  /// returns the workspace name
  const std::string &getWorkspaceName() { return m_wsName; }

  //! is this table editable
  bool isEditable() override;
  //! is this table sortable
  bool isSortable() override;
  //! are the columns fixed - not editable by the GUI
  bool isFixedColumns() override { return true; }

  void sortTableDialog() override;

signals:
  void needToClose();
  void needToUpdate();

public slots:
  void deleteRows(int startRow, int endRow) override;
  void cellEdited(int, int col) override;
  void setPlotDesignation(PlotDesignation pd,
                          bool rightColumns = false) override;

protected slots:
  void closeTable();
  void fillTable();
  void fillTableTransposed();
  void updateTable();
  void dealWithUnwantedResize();

protected:
  void
  preDeleteHandle(const std::string &wsName,
                  const boost::shared_ptr<Mantid::API::Workspace> ws) override;
  void afterReplaceHandle(
      const std::string &wsName,
      const boost::shared_ptr<Mantid::API::Workspace> ws) override;

  // Reimplemented methods for custom sorting of TableWorkspaces
  void sortColumn(int col, int order) override;
  void sortColumns(const QStringList &cols, int type = 0, int order = 0,
                   const QString &leadCol = QString()) override;

private:
  /// Set the plot type on the workspace for each selected column
  void setPlotTypeForSelectedColumns(int plotType);

  /// ITableWorkspace being displayed
  Mantid::API::ITableWorkspace_sptr m_ws;
  /// Name of the TableWorkspace being displayed
  const std::string m_wsName;
  /// Show the table workspace transposed
  bool m_transposed;
};

#endif /* MANTIDTABLE_H */
