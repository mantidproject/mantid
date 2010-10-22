
#include "MantidQtMantidWidgets/ICatUtils.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidQtAPI/InterfaceManager.h"
#include "MantidQtAPI/AlgorithmDialog.h"

#include <QMdiSubWindow>
#include <QStringList>
#include <QFont>
#include <QHeaderView>
#include <QDialog>
#include <QPalette>
#include <QColor>

using namespace MantidQt::MantidWidgets;

using namespace Mantid::API;


ICatUtils::ICatUtils():m_calendarWidget(NULL),m_applicationWindow(NULL)
{
	
	
}

/**This method updates the search result to search tree
 *@param ws_sptr workspace shared pointer
 *@param tablewidget pointer to table widget
*/ 
void ICatUtils::updatesearchResults(Mantid::API::ITableWorkspace_sptr& ws_sptr,QTableWidget* tablewidget )
{
	if(!ws_sptr || ws_sptr->rowCount()==0)
	{
		return ;
	}

	//now set alternating color flag
	tablewidget->setAlternatingRowColors(true);
	//stylesheet for alternating background color
	tablewidget->setStyleSheet("alternate-background-color: rgb(216, 225, 255)");
	//disable  sorting as per QT documentation.otherwise  setitem will give undesired results
	tablewidget->setSortingEnabled(false);

	tablewidget->verticalHeader()->setVisible(false);
	tablewidget->setRowCount(ws_sptr->rowCount());	
	tablewidget->setColumnCount(ws_sptr->columnCount());	

	for (int i=0;i<ws_sptr->rowCount();++i)
	{
		//setting the row height of tableWidget 
		tablewidget->setRowHeight(i,20);
	}
	
	QStringList qlabelList;
	for(int i=0;i<ws_sptr->columnCount();i++)
	{
		Column_sptr col_sptr = ws_sptr->getColumn(i);
		//get the column name to display as the header of table widget
		QString colTitle = QString::fromStdString(col_sptr->name());
		qlabelList.push_back(colTitle);
	
		for(int j=0;j<ws_sptr->rowCount();++j)
		{
		    std::ostringstream ostr;
		     col_sptr->print(ostr,j);
						 
			 QTableWidgetItem *newItem  = new QTableWidgetItem(QString::fromStdString(ostr.str()));
			 newItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
			 tablewidget->setItem(j,i, newItem);
			 newItem->setToolTip(QString::fromStdString(ostr.str()));
		}
	}
	QFont font;
	font.setBold(true);
	//setting table widget header labels from table workspace
	tablewidget->setHorizontalHeaderLabels(qlabelList);
	for (int i=0;i<tablewidget->columnCount();++i)
	{
		tablewidget->horizontalHeaderItem(i)->setFont(font);
	}
	//sorting by title
	tablewidget->sortByColumn(2,Qt::AscendingOrder);
	//enable sorting
	tablewidget->setSortingEnabled(true);

}

//for clearing the table widget
void ICatUtils::resetSearchResultsWidget(QTableWidget* tablewidget )
{
	//below for loop is for clearing the table widget on search button click.Bcoz Each click on search button to load data,rows were getting appended.
	// table widget clear() method is clearing only the tablewidgetitem text,not removing the rows,columns
	// so i'm using removeRow().When I removed the row from top of the table it was not working.so the for loop starts from bottom to top
	for (int i=tablewidget->rowCount()-1;i>=0;--i)
	{
		tablewidget->removeRow(i);
	}
	for (int j=tablewidget->columnCount()-1;j>=0;--j)
	{
		tablewidget->removeColumn(j);
	}
	//now set alternating color flag
	tablewidget->setAlternatingRowColors(false);
	// reset the background color to white
	// if it's not reset to white alternating colour is not working.
	tablewidget->setStyleSheet("background-color: rgb(216, 225, 255)");
	
	//disable  sorting as per QT documentation.otherwise  setitem will give undesired results
	tablewidget->setSortingEnabled(false);

	tablewidget->verticalHeader()->setVisible(false);
}


//This method clears the data associated to the previous search
void ICatUtils::clearSearch(QTableWidget* tablewidget,const std::string & wsName )
{
	//before starting new search investigations clear the old one.

	if(AnalysisDataService::Instance().doesExist(wsName))
	{
		AnalysisDataService::Instance().remove(wsName);

	}

	resetSearchResultsWidget(tablewidget);

}
/**This method is called when an investigation is selected  from investigations list
 *@param item  table widget item
 */
void ICatUtils::investigationSelected(QTableWidget* tablewidget,QTableWidgetItem* item,
									   QWidget* parent,Mantid::API::ITableWorkspace_sptr ws_sptr )
{
	if(!item) return ;
	int row=item->row();

	// column zero is investigation id
	QTableWidgetItem* invstItem = tablewidget->item(row,0);
	QString qinvstId = invstItem->text();
	long long invstId = qinvstId.toLongLong();
    
	//column one is RbNumber
	QTableWidgetItem* rbNumberItem = tablewidget->item(row,1);
	if(!rbNumberItem) return;
    QString qRbNumber = rbNumberItem->text();
	///column two is Title
	QTableWidgetItem* titleItem = tablewidget->item(row,2);
	if(!titleItem)return ;
	QString qTitle = titleItem->text();
    //column 4 is Instrument
	QTableWidgetItem* instrumentItem = tablewidget->item(row,3);
	if(!instrumentItem)return;
	QString qInstrument = instrumentItem->text();
		
	//parent of user_win is application window;
	QMdiSubWindow* usr_win = new QMdiSubWindow(parent);
	if(!usr_win) return;
	usr_win->setAttribute(Qt::WA_DeleteOnClose, false);

	m_invstWidget= new ICatInvestigation(invstId,qRbNumber,qTitle,qInstrument,ws_sptr,usr_win);
	if( m_invstWidget )
	{ 
		QRect frame = QRect(usr_win->frameGeometry().topLeft() - usr_win->geometry().topLeft(), 
			usr_win->geometry().bottomRight() - usr_win->geometry().bottomRight());
		usr_win->setWidget(m_invstWidget);
		QRect iface_geom = QRect(frame.topLeft() + m_invstWidget->geometry().topLeft(), 
			frame.bottomRight() + m_invstWidget->geometry().bottomRight()+QPoint(15,35));
		usr_win->setGeometry(iface_geom);
		usr_win->move(QPoint(600, 400));
		usr_win->show();
	}
}
/** This method executes the ListInstruments algorithm
  * and fills the instrument box with instrument lists returned by ICat API
  * @return shared pointer to workspace which contains instrument names
*/
std::vector<std::string> ICatUtils::executeListInstruments()
{
	QString algName("ListInstruments");
	const int version=-1;
	Mantid::API::IAlgorithm_sptr alg;
	try
	{
		alg = Mantid::API::AlgorithmManager::Instance().create(algName.toStdString(),version);
	}
	catch(...)
	{
		throw std::runtime_error("Error when Populating the instrument list box"); 
			
	}
		
	Poco::ActiveResult<bool> result(alg->executeAsync());
	while( !result.available() )
	{
		QCoreApplication::processEvents();
	}
	if(!alg->isExecuted())
	{		
		//if the algorithm failed check the session id passed is valid
		if(!isSessionValid(alg))
		{			
			//at this point session is invalid, popup loginbox to login
			if(login())
			{
			//now populate instrument box
			 std::vector<std::string> instruments =executeListInstruments();
			 return instruments;
			}
		}
		else
		{			
			return std::vector<std::string>();
		}
	}
	std::vector<std::string>instrlist;
	try
	{
		
	  instrlist= alg->getProperty("InstrumentList");
	}
	catch (Mantid::Kernel::Exception::NotFoundError& e)
	{
		throw e;
	}
	return instrlist;

}
bool ICatUtils::isSessionValid(const Mantid::API::IAlgorithm_sptr& alg)
{
	try
	{
	return  alg->getProperty("isValid");
	}
	catch (Mantid::Kernel::Exception::NotFoundError&e)
	{
		throw e;
	}
   
}
bool ICatUtils::login()
{
	QString algName("Login");
	const int version =-1;
	Mantid::API::IAlgorithm_sptr alg;
	try
	{
		alg = Mantid::API::AlgorithmManager::Instance().create(algName.toStdString(),version);
	}
	catch(...)
	{		
		throw std::runtime_error("Error when Populating the instrument list box"); 
	}
	if(!m_applicationWindow)
	{return false;}

	MantidQt::API::AlgorithmDialog *dlg =MantidQt::API::InterfaceManager::Instance().createDialog(alg.get(), m_applicationWindow, false, "", "", "");
	if(!dlg) return false;
	if(dlg->exec()==QDialog::Accepted)
	{
		delete dlg;
		Poco::ActiveResult<bool> result(alg->executeAsync());
		while( !result.available() )
		{
			QCoreApplication::processEvents();
		}
	
		if(!alg->isExecuted())
		{
			return false;
		}
		return true;
	}
}

/** This method populates the instrument box
*/
void ICatUtils::populateInstrumentBox(QComboBox* instrumentBox)
{
	
	/// execute the algorithm ListInstruments
	std::vector<std::string> instrlist = executeListInstruments();

	if(instrlist.empty())
	{
		throw std::runtime_error("Instrument list is empty");
	}

	/// loop through values
	std::vector<std::string>::const_iterator citr;
	for (citr=instrlist.begin();citr!=instrlist.end();++citr)
	{
		//populate the instrument box  
		instrumentBox->addItem(QString::fromStdString(*citr));
	}

	//sorting the combo by instrument name;
	instrumentBox->model()->sort(0);
	instrumentBox->insertItem(-1,"");
}


/// for displaying the investigatiosn count 
void ICatUtils::updateSearchLabel(const Mantid::API::ITableWorkspace_sptr& ws_sptr,QLabel* label)
{
	std::stringstream rowcount;
	QString results("Investigations Search Results :");
	if(!ws_sptr)
	{
		results+=" No investigations to dispaly as an error occured during investigations search";
	}
	else{
		rowcount<<ws_sptr->rowCount();
		results+=QString::fromStdString(rowcount.str()) + " Investigations Found";
	}
    setLabelText(label,results);
}
void ICatUtils::setLabelText(QLabel* plabel,const QString& text)
{
	//setting the label string
	QFont font;
	font.setBold(true);
	plabel->setText(text);
	plabel->setAlignment(Qt::AlignHCenter);
	plabel->setFont(font);
}
void  ICatUtils::popupCalendar(QWidget* parent)
{
	m_calendarWidget = new SearchCalendar(parent);
	connect(m_calendarWidget,SIGNAL(clicked(const QDate&)) ,parent,SLOT(getDate(const QDate&)));
    m_calendarWidget->setObjectName(QString::fromUtf8("calendarWidget"));
    m_calendarWidget->setGeometry(QRect(386, 64, 211, 148));
    m_calendarWidget->setGridVisible(true);
	m_calendarWidget->show();
	
}

/// close calendarwidget
void ICatUtils::closeCalendarWidget()
{
	if(m_calendarWidget)
	{
		m_calendarWidget->hide();
	}
}
/// This method returns the calendar widget
QCalendarWidget* ICatUtils::calendarWidget()
{
	return m_calendarWidget;
}

/// This method sets the parent widget of search widgets.
void ICatUtils::setParent(QWidget*parent)
{
	m_applicationWindow=parent;

}

SearchCalendar::SearchCalendar(QWidget* par):QCalendarWidget(par)
{
}









