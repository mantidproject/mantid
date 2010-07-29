
//----------------------
// Includes
//----------------------
#include "MantidQtMantidWidgets/ICatSearch.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/TableRow.h" 

#include<QStringList>
#include<QTreeWidget>
#include<QTreeWidgetItem>
#include<QFont>
#include<QPainter>
#include<QHeaderView>
#include<QPen>
#include <QTableWidgetItem>
#include <QSettings>
#include <QBrush>
#include <QMdiSubWindow>

using namespace Mantid::Kernel;
using namespace Mantid::API;

using namespace MantidQt::MantidWidgets;

//----------------------
// Public member functions
//----------------------
///Constructor
ICatSearch::ICatSearch(QWidget *par) :
QWidget(par)
{
	initLayout();
//	 getting the application window pointer and setting it 
	//this is bcoz parent()->parent() is not working in some slots as I expected 
	QObject* qobj=parent();
	QWidget* parent=qobject_cast<QWidget*>(qobj->parent());
	if(parent)
	{
		setparentWidget(parent);
	}

}
QWidget* ICatSearch::getParentWidget()
{
	return m_applicationWindow;
}
/* this method sets the parent widget as application window
*/
void ICatSearch::setparentWidget(QWidget* par)
{
	m_applicationWindow= par;
}
/// Set up the dialog layout
void ICatSearch::initLayout()
{
 // 
	m_uiForm.setupUi(this);

	m_invstWidget=NULL;
	
	setToolTips();
	//disable the table widget's vertical header
	m_uiForm.searchtableWidget->verticalHeader()->setVisible(false);
	
	populateInstrumentBox();
	
	//getting last saved input data from registry
	readSettings();

	connect(m_uiForm.searchButton,SIGNAL(clicked()),this,SLOT(onSearch()));
	connect(m_uiForm.cancelButton,SIGNAL(clicked()),this,SLOT(onCancel()));
	connect(m_uiForm.searchtableWidget,SIGNAL(itemDoubleClicked(QTableWidgetItem * )),
		this,SLOT(investigationSelected(QTableWidgetItem* )));
	connect(this,SIGNAL(error(const QString&)),parent()->parent(),SLOT(writetoLogWindow(const QString& )));
}
/// This method gets called  when the widget is closed
void ICatSearch::closeEvent(QCloseEvent*)
{
	saveSettings();
}
/// This method sets the tool tips search input controls
void ICatSearch::setToolTips()
{
	m_uiForm.startRunEdit->setToolTip("The start run number for the range of investigations to be searched");
	m_uiForm.endRunEdit->setToolTip("The end run number for the range of investigations to be searched");
}
/// This method is the handler for search button
void ICatSearch::onSearch()
{
	double startRun=0,endRun=0;
	//get start and end run values 
	getRunValues(startRun,endRun);

	if(startRun==0 || endRun==0)
	{
		emit error("Run number cannot be zero.Enter valid strat and end run numbers to do Search.");
		return;
	}
	//try to validate the UI level.
	if(startRun> endRun)
	{
		emit error("Run end number cannot be lower than run start number.");
		return;
	}
	QString instrName;
	// get the selected instrument
	getSelectedInstrument(instrName);

	std::string instr(instrName.toStdString());

	// execute the search by run number algorithm
	ITableWorkspace_sptr ws_sptr = executeSearchByRunNumber(startRun,endRun,isCaseSensitiveSearch(),instr);
	if(!ws_sptr)
	{
		return;
	}
	updatesearchResults(ws_sptr);
	//setting the label string
	QFont font;
	font.setBold(true);
	m_uiForm.searchlabel->setText("Investigations Search Results");
	m_uiForm.searchlabel->setAlignment(Qt::AlignHCenter);
	m_uiForm.searchlabel->setFont(font);
}
/** Is case sensitive search
*/
bool ICatSearch::isCaseSensitiveSearch()
{
	return m_uiForm.casesensitiveBox->isChecked();
}
/* This method updates the search result to search tree
 * @param ws_sptr workspace shared pointer
*/ 
void ICatSearch::updatesearchResults(ITableWorkspace_sptr& ws_sptr )
{
	
	//int rows = ws_sptr->rowCount();
	//int columns = ws_sptr->columnCount();

	//long  long invstId=0;
	//std::string stringValue;
	//QStringList qlist;

	//QFont font;
	//font.setBold(true);

	////loop through table workspace rows
	//for (int i=0;i<rows;++i)
	//{
	//	TableRow row =ws_sptr->getRow(i);
	//	row>>invstId;
	//	QString qinvstId=QString::number(invstId);
	//	m_uiForm.searchtableWidget->insertRow(i);

	//	QTableWidgetItem *newItem  = new QTableWidgetItem(qinvstId);
	//	m_uiForm.searchtableWidget->setItem(i, 0, newItem);
	//	newItem->setToolTip(qinvstId);
	//
	//	// loop through columns
	//	for (int j=1;j<columns;++j)
	//	{
	//		row>>stringValue;
	//		QTableWidgetItem *newItem1  = new QTableWidgetItem(QString::fromStdString(stringValue));
	//		//below if loop not working,look into it again later sometime.
	//		if(j==2)
	//		{
	//			QSize qsize;
	//			qsize.setWidth(400);
	//			newItem1->setSizeHint(qsize);
	//		}
	//		
	//        m_uiForm.searchtableWidget->setItem(i, j, newItem1);
	//		newItem1->setToolTip(QString::fromStdString(stringValue));
	//	}
 //     
	//}
	////setting the row height of tableWidget 
	//rows=m_uiForm.searchtableWidget->rowCount();
	//for (int i=0;i<rows;++i)
	//{
	//	m_uiForm.searchtableWidget->setRowHeight(i,20);
	//}
	//

	//below for loop is for clearing the table widget on search button click.Bcoz Each click on search button to load data,rows were getting appended.
	// table widget clear() method is clearing only the tablewidgetitem text,not removing the rows,columns
	// so i'm using removeRow().When I removed the row from top of the table it was not working.so the for loop starts from bottom to top

	for (int i=m_uiForm.searchtableWidget->rowCount()-1;i>=0;--i)
	{
		m_uiForm.searchtableWidget->removeRow(i);
	}
	
	for (int i=0;i<ws_sptr->rowCount();++i)
	{
		m_uiForm.searchtableWidget->insertRow(i);
		//setting the row height of tableWidget 
		m_uiForm.searchtableWidget->setRowHeight(i,20);
	}

	QStringList qlabelList;//QBrush brush;
	//QColor color("red");
	//brush.setColor(color);
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
			 m_uiForm.searchtableWidget->setItem(j,i, newItem);
			 newItem->setToolTip(QString::fromStdString(ostr.str()));
			  //newItem->setBackground(brush);
		}
	}
	//setting table widget header labels from table workspace
	m_uiForm.searchtableWidget->setHorizontalHeaderLabels(qlabelList);
	QFont font;
	font.setBold(true);
	for (int i=0;i<m_uiForm.searchtableWidget->columnCount();++i)
	{
		m_uiForm.searchtableWidget->horizontalHeaderItem(i)->setFont(font);;
	}
	//sorting by title
	m_uiForm.searchtableWidget->sortByColumn(2,Qt::AscendingOrder);
	// resizing the coulmn based on data size
	m_uiForm.searchtableWidget->resizeColumnsToContents ();
}
/** This method populates the instrument box
*/
void ICatSearch::populateInstrumentBox(){
	
	// execute the algorithm ListInstruments
	ITableWorkspace_sptr ws_sptr=executeListInstruments();
	if(!ws_sptr)
		return ;
	
	// loop through values
	for(int row=0;row<ws_sptr->rowCount();++row)
	{
		//retrieving the  instrument name from table workspace
		std::string instName(ws_sptr->String(row,0));
		//populate the instrument box  
		m_uiForm.instrmentBox->insertItem(row,QString::fromStdString(instName));

	}
	//sorting the combo by instrument name;
	m_uiForm.instrmentBox->model()->sort(0);
}
/** This method executes the ListInstruments algorithm
  * and fills the instrument box with instrument lists returned by ICat API
  * @return shared pointer to workspace which contains instrument names
*/
ITableWorkspace_sptr  ICatSearch::executeListInstruments()
{
	QString algName("ListInstruments");
	const int version=1;
	ITableWorkspace_sptr  ws_sptr;
	Mantid::API::IAlgorithm_sptr alg;
	try
	{
		alg = Mantid::API::AlgorithmManager::Instance().create(algName.toStdString(),version);
	}
	catch(...)
	{
		throw std::runtime_error("Error when Populating the instrument list box"); 
	}
	try
	{
	alg->setPropertyValue("OutputWorkspace","instruments");
	}
	catch(std::invalid_argument& e)
	{		
		emit error(e.what());
		return ws_sptr;
	}
	alg->execute();
	
	if(!alg->isExecuted())
	{
		return ws_sptr;
	}
	
	ws_sptr = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>
		(AnalysisDataService::Instance().retrieve("instruments"));

	return ws_sptr;
}
/**This method gets run numbers from the start and end run boxes.
  *@param startRun - start run number
  *@param endRun - end run number
*/
void ICatSearch::getRunValues(double& startRun,double& endRun)
{
	endRun = m_uiForm.endRunEdit->text().toDouble();
	startRun = m_uiForm.startRunEdit->text().toDouble();
}
/**This method gets the selected instrument
  *@param instrName name of the selected instrument
*/
void ICatSearch::getSelectedInstrument(QString& instrName)
{
	instrName=m_uiForm.instrmentBox->currentText();
}
/**This method executes the search by run number algorithm
  *@param startRun start run number 
  *@param endRun end run number 
*/
ITableWorkspace_sptr  ICatSearch::executeSearchByRunNumber(const double &startRun,const double &endRun,bool bCase,const std::string& instrName)
{
	QString algName("SearchByRunNumber");
	const int version=1;
	ITableWorkspace_sptr  ws_sptr;
	Mantid::API::IAlgorithm_sptr alg;
	try
	{
		alg = Mantid::API::AlgorithmManager::Instance().create(algName.toStdString(),version);
	}
	catch(...)
	{
		throw std::runtime_error("Error when Populating the instrument list box"); 
	}
	try
	{
		alg->setProperty("StartRun",startRun);
		alg->setProperty("EndRun",endRun);
		alg->setProperty("Instrument",instrName);
		alg->setProperty("Case Sensitive",bCase);
		alg->setProperty("OutputWorkspace","investigations");
	}
	catch(std::invalid_argument& e)
	{		
		emit error(e.what());
		return ws_sptr;
	}
	alg->execute();
	
	if(!alg->isExecuted())
	{
		return ws_sptr;
	}

	ws_sptr = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>
		(AnalysisDataService::Instance().retrieve("investigations"));
	return ws_sptr;

}
/** This method cancels the search widget.
*/
void ICatSearch::onCancel()
{
	this->close();
	QObject* qobj=parent();
	QWidget* parent=qobject_cast<QWidget*>(qobj);
	if(parent)
	{
		parent->close();
	}
}
/** This method is called when an investigation is selected  from investigations list
  *@param item  item in the table
*/
void ICatSearch::investigationSelected(QTableWidgetItem * item )
{
	int row=item->row();

	// column zero is investigation id
	QTableWidgetItem* invstItem = m_uiForm.searchtableWidget->item(row,0);
	QString qinvstId = invstItem->text();
	long long invstId = qinvstId.toLongLong();
    
	//column one is RbNumber
	QTableWidgetItem* rbNumberItem = m_uiForm.searchtableWidget->item(row,1);
    QString qRbNumber = rbNumberItem->text();
	///column two is Title
	QTableWidgetItem* titleItem = m_uiForm.searchtableWidget->item(row,2);
	QString qTitle = titleItem->text();
    //column 4 is Instrument
	QTableWidgetItem* instrumentItem = m_uiForm.searchtableWidget->item(row,3);
	QString qInstrument = instrumentItem->text();
		
	//parent of user_win is application window;
	QMdiSubWindow* usr_win = new QMdiSubWindow(m_applicationWindow);
	usr_win->setAttribute(Qt::WA_DeleteOnClose, false);
	m_invstWidget= new MantidQt::MantidWidgets::ICatInvestigation(invstId,qRbNumber,qTitle,qInstrument,usr_win);
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
	//// create investigation widget
	//m_invstWidget= new MantidQt::MantidWidgets::ICatInvestigation(invstId,qRbNumber,qTitle,qInstrument,NULL);
	//m_invstWidget->show();

}

/** This method saves search settings
*/
void ICatSearch::saveSettings()
{
	QSettings searchsettings;
	searchsettings.beginGroup("ICatSettings/Search");
	searchsettings.setValue("startRun",m_uiForm.startRunEdit->text());
    searchsettings.setValue("endRun",m_uiForm.endRunEdit->text());
	searchsettings.setValue("instrument",m_uiForm.instrmentBox->currentText());
	searchsettings.endGroup();

}
/// read settings from registry
void ICatSearch::readSettings()
{
	QSettings searchsettings;
	searchsettings.beginGroup("ICatSettings/Search");
	m_uiForm.startRunEdit->setText(searchsettings.value("startRun").toString());
	m_uiForm.endRunEdit->setText(searchsettings.value("endRun").toString());
	//m_uiForm.instrmentBox->setItemText (0,searchsettings.value("instrument").toString());
	int index=m_uiForm.instrmentBox->findText(searchsettings.value("instrument").toString());
	if(index!=-1)
	{
		m_uiForm.instrmentBox->setCurrentIndex(index);
	}

	searchsettings.endGroup();
}





