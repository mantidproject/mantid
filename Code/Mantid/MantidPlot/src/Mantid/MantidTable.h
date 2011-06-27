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
    ApplicationWindow* parent, const QString& name = QString(), Qt::WFlags f=0);
signals:
  void needToClose();
  void needToUpdate();
public slots:
  void deleteRows(int startRow, int endRow);
  void cellEdited(int,int col);
protected slots:
  void closeTable();
  void fillTable();
protected:
  void deleteHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws);
  void afterReplaceHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws);
private:
  Mantid::API::ITableWorkspace_sptr m_ws;
  const std::string m_wsName;

};

#endif /* MANTIDTABLE_H */
