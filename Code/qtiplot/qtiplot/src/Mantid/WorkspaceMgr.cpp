#include <vector>
#include <string>
#include <QMessageBox>
#include <QListWidgetItem>

#include "WorkspaceMgr.h"
#include "../ApplicationWindow.h"
#include "../Matrix.h"
#include "LoadRawDlg.h"
#include "ImportWorkspaceDlg.h"
#include "ExecuteAlgorithm.h"

WorkspaceMgr::WorkspaceMgr(QWidget *parent) : QDialog(parent)
{
	m_parent = parent;
	
	setupUi(this);
	setupActions();
	
	interface = new Mantid::PythonAPI::PythonInterface;
	interface->InitialiseFrameworkManager();
	
	std::vector<std::string> names = interface->GetWorkspaceNames();

	for(unsigned int i = 0; i < names.size(); ++i) 
	{
		listWorkspaces->insertItem(0, QString::fromStdString(names[i]));
	}
	
	std::vector<std::string> algs = interface->GetAlgorithmNames();
	
	for(unsigned int i = 0; i < algs.size(); ++i) 
	{
		listAlgorithms->insertItem(0, QString::fromStdString(algs[i]));
	}
	
}

WorkspaceMgr::~WorkspaceMgr()
{
	
}

void WorkspaceMgr::setupActions()
{
	connect(pushExit, SIGNAL(clicked()), this, SLOT(close()));
	connect(pushAddWorkspace, SIGNAL(clicked()), this, SLOT(addWorkspaceClicked()));
	connect(listWorkspaces, SIGNAL(itemSelectionChanged()), this, SLOT(selectedWorkspaceChanged()));
	connect(pushImportWorkspace, SIGNAL(clicked()), this, SLOT(importWorkspace()));
	connect(pushExecuteAlg, SIGNAL(clicked()), this, SLOT(importWorkspace()));
}

void WorkspaceMgr::addWorkspaceClicked()
{
	loadRawDlg* dlg = new loadRawDlg(this);
	dlg->setModal(true);	
	dlg->exec();
	
	if (!dlg->getFilename().isEmpty())
	{	
		
		Mantid::API::Workspace_sptr ws = interface->LoadIsisRawFile(dlg->getFilename().toStdString(), dlg->getWorkspaceName().toStdString());
		if (ws.use_count() == 0)
		{
			QMessageBox::warning(this, tr("Mantid"),
                   		tr("A workspace with this name already exists.\n")
                    		, QMessageBox::Ok, QMessageBox::Ok);
			return;
		}
		
		listWorkspaces->insertItem(0, dlg->getWorkspaceName() );
	}
}

void WorkspaceMgr::selectedWorkspaceChanged()
{
	if (listWorkspaces->currentRow() != -1)
	{
		QListWidgetItem *selected = listWorkspaces->item(listWorkspaces->currentRow());
		QString wsName = selected->text();
		
		Mantid::API::Workspace_sptr ws = interface->RetrieveWorkspace(wsName.toStdString());
		
		int numHists = ws->getHistogramNumber();
		int numBins = ws->blocksize();
		
		textWorkspaceInfo->setPlainText("Number of histograms: " + QString::number(numHists) + "\nNumber of bins: " + QString::number(numBins));
		
	}
}

void WorkspaceMgr::importWorkspace()
{
	if (listWorkspaces->currentRow() != -1)
	{
		QListWidgetItem *selected = listWorkspaces->item(listWorkspaces->currentRow());
		QString wsName = selected->text();
		
		Mantid::API::Workspace_sptr ws = interface->RetrieveWorkspace(wsName.toStdString());
		
		int numHists = ws->getHistogramNumber();
		int numBins = ws->blocksize();
		
		ImportWorkspaceDlg* dlg = new ImportWorkspaceDlg(this, numHists);
		dlg->setModal(true);	
		if (dlg->exec() == QDialog::Accepted)
		{
			int start = dlg->getLowerLimit();
			int end = dlg->getUpperLimit();
			int diff = end - start;
			
			ApplicationWindow* temp = dynamic_cast<ApplicationWindow*>(m_parent);
			Matrix *mat = temp->newMatrix(wsName, numBins, end-start +1);

			int histCount = 0;
			
			for (int row = 0; row < numHists; ++row)
			{
				if (row >= start && row <= end)
				{
					std::vector<double>* Y = interface->GetYData(wsName.toStdString(), row);
					for (int col = 0; col < numBins; ++col)
					{
						mat->setCell(col, histCount, Y->at(col));
					}
					++histCount;
				}
			}	
		}
	}
}

