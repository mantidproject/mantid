#include "MantidQtMantidWidgets/ICatMyDataSearch.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include<QStringList>
#include<QTreeWidget>
#include<QTreeWidgetItem>
#include<QFont>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

ICatMyDataSearch::ICatMyDataSearch(QWidget*par):QWidget(par)
{
	 m_uiForm.setupUi(this);

	 connect(this,SIGNAL(error(const QString&)),parent()->parent(),SLOT(writetoLogWindow(const QString& )));
	 
	Mantid::API::ITableWorkspace_sptr  ws_sptr = executeMyDataSearch();
	if(!ws_sptr)
	{
		emit error("MyData search completed,No results to display.");
		return;
	}

	for (int i=m_uiForm.myDatatableWidget->rowCount()-1;i>=0;--i)
	{
		m_uiForm.myDatatableWidget->removeRow(i);
	}
	m_uiForm.myDatatableWidget->setRowCount(ws_sptr->rowCount());
	for (int i=0;i<ws_sptr->rowCount();++i)
	{
		//setting the row height of tableWidget 
		m_uiForm.myDatatableWidget->setRowHeight(i,20);
	}
	
	QStringList qlabelList;
	for(int i=0;i<ws_sptr->columnCount();i++)
	{
		Column_sptr col_sptr = ws_sptr->getColumn(i);
		//get the column name to display as the header of table widget
		qlabelList.push_back(QString::fromStdString(col_sptr->name()));

		for(int j=0;j<ws_sptr->rowCount();++j)
		{
		    std::ostringstream ostr;
		     col_sptr->print(ostr,j);
			 QTableWidgetItem *newItem  = new QTableWidgetItem(QString::fromStdString(ostr.str()));
			 newItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
			 m_uiForm.myDatatableWidget->setItem(j,i,newItem);
			 newItem->setToolTip(QString::fromStdString(ostr.str()));
			// newItem->setBackground(QBrush(QColor(Qt::blue))); 
		}
		m_uiForm.myDatatableWidget->resizeColumnToContents(i);
	}
	//setting table widget header labels from table workspace
	m_uiForm.myDatatableWidget->setHorizontalHeaderLabels(qlabelList);
	QFont font;
	font.setBold(true);
	for (int i=0;i<m_uiForm.myDatatableWidget->columnCount();++i)
	{
		m_uiForm.myDatatableWidget->horizontalHeaderItem(i)->setFont(font);;
	}
	//resizing the coluns based on data size
	m_uiForm.myDatatableWidget->resizeColumnsToContents();
		
	QString labelText;
	std::stringstream totalCount;
	totalCount<<ws_sptr->rowCount();
	labelText="Data: "+QString::fromStdString(totalCount.str())+" Investigations "+" found";
	
	m_uiForm.mydatalabel->clear();
	m_uiForm.mydatalabel->setText(labelText);
	m_uiForm.mydatalabel->setAlignment(Qt::AlignHCenter);
	m_uiForm.mydatalabel->setFont(font);

	m_uiForm.myDatatableWidget->setSortingEnabled(true);
	m_uiForm.myDatatableWidget->sortByColumn(2,Qt::AscendingOrder);
	

}
Mantid::API::ITableWorkspace_sptr ICatMyDataSearch::executeMyDataSearch()
{
	Mantid::API::IAlgorithm_sptr alg;
	Mantid::API::ITableWorkspace_sptr ws_sptr;
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
		return ws_sptr;
	}
	
	try
	{
		Poco::ActiveResult<bool> result(alg->executeAsync());
		while( !result.available() )
		{
			QCoreApplication::processEvents();
		}
		//return (!result.failed());
	}
	catch(...)
    {   
	  return ws_sptr;
    }
	if(AnalysisDataService::Instance().doesExist("MyInvestigations"))
	{
		ws_sptr = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>
			(AnalysisDataService::Instance().retrieve("MyInvestigations"));
	}
	return ws_sptr;

}

//void MyTreeWidget::drawRow(QPainter* p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const
//{ QTreeWidget::drawRow(p, opt, idx);
//for (int col = 0; col < columnCount(); ++col)
//{  QModelIndex s = idx.sibling(idx.row(), col);
//if (s.isValid()) 
//{
//	QRect rect = visualRect(s);
//	p->setPen(Qt::DotLine);
//	p->drawRect(rect); }
//}
//}