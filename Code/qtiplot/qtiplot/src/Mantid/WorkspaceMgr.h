
#ifndef WORKSPACEMGR_H
#define WORKSPACEMGR_H

#include <QDialog>

#include "ui_WorkspaceMgr.h"

#include "MantidPythonAPI/PythonInterface.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/IAlgorithm.h"

class WorkspaceMgr : public QDialog, private Ui::Dialog
{
	Q_OBJECT
	
public:
	WorkspaceMgr(QWidget *parent = 0);
	~WorkspaceMgr();

protected:
	
private slots:
	void addWorkspaceClicked();
	void selectedWorkspaceChanged();
	void importWorkspace();
	void executeAlgorithm();

private:
	void setupActions();
	void getWorkspaces();
	void getAlgorithms();
	Mantid::PythonAPI::PythonInterface* m_interface;

	QWidget* m_parent;

};

#endif /* WORKSPACEMGR_H */
