#include "MantidQtMantidWidgets/ICatMyDataSearch.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"


#include<QStringList>
#include<QTreeWidget>
#include<QTreeWidgetItem>
#include<QFont>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

ICatMyDataSearch::ICatMyDataSearch(QWidget*par):QWidget(par),m_utils_sptr()
{
	m_uiForm.setupUi(this);

	connect(this,SIGNAL(error(const QString&)),parent()->parent(),SLOT(writetoLogWindow(const QString& )));
	connect(m_uiForm.myDatatableWidget,SIGNAL(itemDoubleClicked(QTableWidgetItem* )),
		this,SLOT(investigationSelected(QTableWidgetItem* )));

	QObject* qobj=parent();
	QWidget* parent=qobject_cast<QWidget*>(qobj->parent());
	if(parent)
	{
		setparentWidget(parent);
	}
	
	Mantid::API::ITableWorkspace_sptr  ws_sptr ;
	if(executeMyDataSearch(ws_sptr))
	{
		if(!ws_sptr)
		{
			emit error("MyData search completed,No results to display.");
			return;
		}
		//boost::shared_ptr<ICatUtils> utils(new ICatUtils(parent));
		
		m_utils_sptr->setParent(parent);
		m_utils_sptr->updatesearchResults(ws_sptr,m_uiForm.myDatatableWidget);

		//setting the label string
		QFont font;
		font.setBold(true);

		QString labelText;
		std::stringstream totalCount;
		totalCount<<ws_sptr->rowCount();
		labelText="Data: "+QString::fromStdString(totalCount.str())+" Investigations "+" found";

		m_uiForm.mydatalabel->setText(labelText);
		m_uiForm.mydatalabel->setAlignment(Qt::AlignHCenter);
		m_uiForm.mydatalabel->setFont(font);
	}
	
	
}

/* this method sets the parent widget as application window
*/
void ICatMyDataSearch::setparentWidget(QWidget* par)
{
	m_applicationWindow= par;
}
bool ICatMyDataSearch::executeMyDataSearch(ITableWorkspace_sptr& ws_sptr)
{
	Mantid::API::IAlgorithm_sptr alg;
	//Mantid::API::ITableWorkspace_sptr ws_sptr;
	try
	{
		alg = Mantid::API::AlgorithmManager::Instance().create("MyDataSearch",1);
	}
	catch(...)
	{
		throw std::runtime_error("Error when loading Mydata search results."); 
	}
	try
	{
		alg->setPropertyValue("OutputWorkspace","MyInvestigations");
		
	}
	catch(std::invalid_argument& e)
	{		
		emit error(e.what());
		return false;
	}
	catch (Mantid::Kernel::Exception::NotFoundError& e)
	{
		emit error(e.what());
		return false;
	}
	
	
	Poco::ActiveResult<bool> result(alg->executeAsync());
	while( !result.available() )
	{
		QCoreApplication::processEvents();
	}
		//return (!result.failed());
    if(!alg->isExecuted())
	{		
		return false;
	}
	
	if(AnalysisDataService::Instance().doesExist("MyInvestigations"))
	{
		ws_sptr = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>
			(AnalysisDataService::Instance().retrieve("MyInvestigations"));

	}
	
	return true;

}
void ICatMyDataSearch::investigationSelected(QTableWidgetItem* item)
{
	ICatUtils utils;
	utils.investigationSelected(m_uiForm.myDatatableWidget,item,m_applicationWindow,m_ws2_sptr);

}

