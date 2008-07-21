
#include "WorkspaceMatrix.h"
#include "WorkspaceMgr.h"
#include "../ApplicationWindow.h"
#include "../Matrix.h"
#include "LoadRawDlg.h"
#include "ImportWorkspaceDlg.h"
#include "ExecuteAlgorithm.h"

#include "MantidKernel/Property.h"
#include "MantidLog.h"
#include "MantidUI.h"

#include <vector>
#include <string>
#include <stdexcept>
#include <QMessageBox>
#include <QListWidgetItem>
#include <QStringList>

WorkspaceMgr::WorkspaceMgr(QWidget *parent) : QDialog(parent)
{
	m_parent = parent;
	
	setupUi(this);
	setupActions();
	
	//m_interface = new Mantid::PythonAPI::PythonInterface;
	
	getWorkspaces();
	
	getAlgorithms();	

    /// Make connection to Mantid's SignalChannel
    if (parent->isA("ApplicationWindow"))
    {
        MantidLog::connect(static_cast<ApplicationWindow*>(parent));
    }

}

WorkspaceMgr::~WorkspaceMgr()
{
    //delete m_interface;
}

void WorkspaceMgr::setupActions()
{
	connect(pushExit, SIGNAL(clicked()), this, SLOT(close()));
	connect(pushAddWorkspace, SIGNAL(clicked()), this, SLOT(addWorkspaceClicked()));
	connect(removeWorkspaceButton, SIGNAL(clicked()), this, SLOT(deleteWorkspaceClicked()));
	connect(listWorkspaces, SIGNAL(itemSelectionChanged()), this, SLOT(selectedWorkspaceChanged()));
	connect(pushImportWorkspace, SIGNAL(clicked()), this, SLOT(importWorkspaceMatrix()));
	connect(pushExecuteAlg, SIGNAL(clicked()), this, SLOT(executeAlgorithm()));
}

void WorkspaceMgr::getWorkspaces()
{
	listWorkspaces->clear();
	
	QStringList names = static_cast<ApplicationWindow*>(m_parent)->mantidUI->getWorkspaceNames();

	for(unsigned int i = 0; i < names.size(); ++i) 
	{
		listWorkspaces->insertItem(0, names[i]);
	}
}

void WorkspaceMgr::getAlgorithms()
{
	listAlgorithms->clear();
	
	QStringList algs = static_cast<ApplicationWindow*>(m_parent)->mantidUI->getAlgorithmNames();
	
	for(unsigned int i = 0; i < algs.size(); ++i) 
	{
		listAlgorithms->insertItem(0, algs[i]);
	}
}

void WorkspaceMgr::addWorkspaceClicked()
{
	loadRawDlg* dlg = new loadRawDlg(this);
	dlg->setModal(true);	
	dlg->exec();
	
	if (!dlg->getFilename().isEmpty())
	{	
		
		Mantid::API::Workspace_sptr ws = 
            static_cast<ApplicationWindow*>(m_parent)->mantidUI->LoadIsisRawFile(dlg->getFilename(), 
                                                                                 dlg->getWorkspaceName(),
                                                                                 dlg->getSpectrumMin(),
                                                                                 dlg->getSpectrumMax());
		if (ws.use_count() == 0)
		{
			QMessageBox::warning(this, tr("Mantid"),
                   		tr("A workspace with this name already exists.\n")
                    		, QMessageBox::Ok, QMessageBox::Ok);
			return;
		}
		
		getWorkspaces();

	}
}

void WorkspaceMgr::deleteWorkspaceClicked()
{
	if (listWorkspaces->currentRow() != -1)
	{
		QListWidgetItem *selected = listWorkspaces->item(listWorkspaces->currentRow());
		QString wsName = selected->text();
		
		static_cast<ApplicationWindow*>(m_parent)->mantidUI->deleteWorkspace(wsName);
		
		listWorkspaces->setCurrentRow(-1);
		
		getWorkspaces();
	}
}


void WorkspaceMgr::selectedWorkspaceChanged()
{
	if (listWorkspaces->currentRow() != -1)
	{
		QListWidgetItem *selected = listWorkspaces->item(listWorkspaces->currentRow());
		QString wsName = selected->text();
		
        Mantid::API::Workspace_sptr ws = Mantid::API::AnalysisDataService::Instance().retrieve(wsName.toStdString());
		
		int numHists = ws->getNumberHistograms();
		int numBins = ws->blocksize();
		
		textWorkspaceInfo->setPlainText("Number of histograms: " + QString::number(numHists) + "\nNumber of bins: " + QString::number(numBins));
	}
    else
        textWorkspaceInfo->setPlainText("");
}

void WorkspaceMgr::importWorkspace()
{
	if (listWorkspaces->currentRow() != -1)
	{
		QListWidgetItem *selected = listWorkspaces->item(listWorkspaces->currentRow());
		QString wsName = selected->text();
		
		Mantid::API::Workspace_sptr ws = Mantid::API::AnalysisDataService::Instance().retrieve(wsName.toStdString());
		
		int numHists = ws->getNumberHistograms();
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
					std::vector<double>* Y = &ws->dataY(row);
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

void WorkspaceMgr::importWorkspaceMatrix()
{
	if (listWorkspaces->currentRow() != -1)
	{
		QListWidgetItem *selected = listWorkspaces->item(listWorkspaces->currentRow());
		QString wsName = selected->text();
		
		Mantid::API::Workspace_sptr ws = Mantid::API::AnalysisDataService::Instance().retrieve(wsName.toStdString());
		
		int numHists = ws->getNumberHistograms();
		int numBins = ws->blocksize();
		
		ImportWorkspaceDlg* dlg = new ImportWorkspaceDlg(this, numHists);
		dlg->setModal(true);	
		if (dlg->exec() == QDialog::Accepted) 
		{
			int start = dlg->getLowerLimit();
			int end = dlg->getUpperLimit();
			int diff = end - start;
			
			ApplicationWindow* temp = dynamic_cast<ApplicationWindow*>(m_parent);
			WorkspaceMatrix *mat = dynamic_cast<WorkspaceMatrix*>(temp->newWMatrix(wsName,ws,start,end,dlg->isFiltered(),dlg->getMaxValue()));

		}
	}
}

void WorkspaceMgr:: executeAlgorithm()
{
	if (listAlgorithms->currentRow() != -1)
	{
		QListWidgetItem *selected = listAlgorithms->item(listAlgorithms->currentRow());
		QString algName = (selected->text()).split("|")[0];
		
		Mantid::API::Algorithm* alg = dynamic_cast<Mantid::API::Algorithm*>(Mantid::API::FrameworkManager::Instance().createAlgorithm(algName.toStdString()));
		
		if (alg)
		{		
			ExecuteAlgorithm* dlg = new ExecuteAlgorithm(this);
			dlg->CreateLayout(alg);
			dlg->setModal(true);
		
			dlg->exec();	
				
			getWorkspaces();
		}
	}
}

