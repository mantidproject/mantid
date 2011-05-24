#ifndef MANTIDTABLE_H
#define MANTIDTABLE_H

#include "../Table.h"
#include "WorkspaceObserver.h"
#include "MantidAPI/ITableWorkspace.h"

class MantidTable: public Table, public WorkspaceObserver
{
  Q_OBJECT
public:
  MantidTable(ScriptingEnv *env, Mantid::API::ITableWorkspace_sptr ws, const QString &label, 
    ApplicationWindow* parent, const QString& name = QString(), Qt::WFlags f=0);
signals:
  void needToClose();
public slots:
  void deleteRows(int startRow, int endRow);
protected slots:
  void closeTable();
protected:
  void fillTable();
  void deleteHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws);
  void afterReplaceHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws);
private:
  Mantid::API::ITableWorkspace_sptr m_ws;

};

#endif /* MANTIDTABLE_H */
