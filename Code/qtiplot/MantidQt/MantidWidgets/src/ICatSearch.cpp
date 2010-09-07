
//----------------------
// Includes
//----------------------
#include "MantidQtMantidWidgets/ICatSearch.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/TableRow.h" 


#include<QStringList>
#include<QFont>
#include <QTableWidgetItem>
#include <QSettings>
#include <QMdiSubWindow>
#include <QDesktopServices>
#include <QUrl>


using namespace Mantid::Kernel;
using namespace Mantid::API;

using namespace MantidQt::MantidWidgets;

//----------------------
// Public member functions
//----------------------
///Constructor
ICatSearch::ICatSearch(QWidget *par) :
QWidget(par),m_sender(NULL),m_invstWidget(NULL),
m_utils_sptr( new ICatUtils)
{
	initLayout();
//	 getting the application window pointer and setting it 
	//this is bcoz parent()->parent() is not working in some slots as I expected 
	QObject* qobj = parent();
	QWidget* parent = qobject_cast<QWidget*>(qobj->parent());
	if(parent)
	{
		setparentWidget(parent);
	}

	
}
ICatSearch::~ICatSearch()
{
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
 	m_uiForm.setupUi(this);
	// as the instrument combo box popup down and up arrow disappeared when the light blue back ground is set in ICat search dailog 
	// I'm setting this style sheet to bring the combo box arrows back.
	QString str="QComboBox#instrumentBox QListView{background-color: white;background-image: url(ICatCombobackground.png);background-attachment: scroll;}"
		"QComboBox#instrumentBox QListView QScrollBar:vertical{background-image: url(:/images/ICatComboVScrollbar.png); background-repeat: repeat-y; width: 17px; height:20px;} ";
	m_uiForm.instrumentBox->setStyleSheet(str);

	QValidator * val= new QIntValidator(0,100000000,m_uiForm.startRunEdit);
	m_uiForm.startRunEdit->setValidator(val);
	m_uiForm.endRunEdit->setValidator(val);
    	
	populateInstrumentBox();
		
	//getting last saved input data from registry
	readSettings();

	connect(m_uiForm.searchButton,SIGNAL(clicked()),this,SLOT(onSearch()));
	connect(m_uiForm.closeButton,SIGNAL(clicked()),this,SLOT(onClose()));
	connect(m_uiForm.searchtableWidget,SIGNAL(itemDoubleClicked(QTableWidgetItem* )),
		this,SLOT(investigationSelected(QTableWidgetItem* )));
	connect(this,SIGNAL(error(const QString&)),parent()->parent(),SLOT(writetoLogWindow(const QString& )));
	connect(m_uiForm.startdatetoolButton,SIGNAL(clicked()),this,SLOT(popupCalendar()));
	connect(m_uiForm.enddatetoolButton,SIGNAL(clicked()),this,SLOT(popupCalendar()));
	connect(m_uiForm.helpButton,SIGNAL(clicked()),this,SLOT(helpButtonClicked()));

	m_uiForm.startRunEdit->installEventFilter(this);
	m_uiForm.endRunEdit->installEventFilter(this);
	m_uiForm.keywordslineEdit->installEventFilter(this);
	m_uiForm.searchframeWidget->installEventFilter(this);
		
}
/// This method gets called  when the widget is closed
void ICatSearch::closeEvent(QCloseEvent*)
{
	saveSettings();
}

/// This method is the handler for search button
void ICatSearch::onSearch()
{	 
	//clear the workspace pointer
	m_ws_sptr.reset();
	// execute the search by run number algorithm
	executeSearchByRunNumber(m_ws_sptr);
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
	//ICatUtils utils;
	if(!m_utils_sptr)
		return;
	m_utils_sptr->resetSearchResultsWidget(m_uiForm.searchtableWidget);
	m_utils_sptr->updatesearchResults(ws_sptr,m_uiForm.searchtableWidget);
	m_utils_sptr->updateSearchLabel(ws_sptr,m_uiForm.searchlabel);

}

/** This method populates the instrument box
*/
void ICatSearch::populateInstrumentBox(){

	try{

		//ICatUtils utils;
		if(!m_utils_sptr)
			return;
		m_utils_sptr->populateInstrumentBox(m_uiForm.instrumentBox);
	}
	catch(std::invalid_argument& e)
	{
		emit error(e.what());
		return;
	}
	catch (Mantid::Kernel::Exception::NotFoundError& e)
	{
		emit error(e.what());
		return;

	}
	catch(std::runtime_error & e)
	{
		emit error(e.what());
		return;
	}
	catch(...)
	{
		emit error("Error when Populating the instruments box");
	}
	return;
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
	catch (Mantid::Kernel::Exception::NotFoundError& e)
	{
		emit error(e.what());
		return ws_sptr;
	}

	Poco::ActiveResult<bool> result(alg->executeAsync());
	while( !result.available() )
	{
		QCoreApplication::processEvents();
	}
	
	if(!alg->isExecuted())
	{
		return ws_sptr;
	}
	if(AnalysisDataService::Instance().doesExist("instruments"))
	{
		ws_sptr = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>
			(AnalysisDataService::Instance().retrieve("instruments"));
	}
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

/**This method gets start and end dates from the start and end date boxes.
  *@param startDate - start date  
  *@param endDate - end date
*/
void ICatSearch::getDates(QString& startDate,QString& endDate)
{
	startDate = m_uiForm.startdateLineEdit->text();
	endDate =m_uiForm.enddateLineEdit->text();

	if(!startDate.compare("//",Qt::CaseInsensitive))
	{
		startDate="";
	}
	if(!endDate.compare("//",Qt::CaseInsensitive))
	{
		endDate="";
	}

}
///popup DateTime calender to select date
void ICatSearch:: popupCalendar()
{
	if(!m_utils_sptr)
		return;

	m_utils_sptr->popupCalendar(this);
	
	QObject * qsender= sender();
	if(!qsender) return;
	 m_sender=qsender;
	
}
///date changed
void ICatSearch::getDate(const QDate& date  )
{
	//calendarWidget()->close();
	m_utils_sptr->closeCalendarWidget();//close the calendar widget
	if(!m_sender) return;

	if(!m_sender->objectName().compare("startdatetoolButton"))
	{
	m_uiForm.startdateLineEdit->setText(date.toString("dd/MM/yyyy"));
	}
	if(!m_sender->objectName().compare("enddatetoolButton"))
	{
	m_uiForm.enddateLineEdit->setText(date.toString("dd/MM/yyyy"));
	}
}

/**This method gets the selected instrument
  *@param instrName name of the selected instrument
*/
void ICatSearch::getSelectedInstrument(QString& instrName)
{
	instrName=m_uiForm.instrumentBox->currentText();
}

/**This method executes the search by run number algorithm
 *@param ws_sptr shared pointer to outputworkspace
*/
bool  ICatSearch::executeSearchByRunNumber(ITableWorkspace_sptr& ws_sptr)
{
	//before starting new search for investigations object clear the old one.

	if(AnalysisDataService::Instance().doesExist("investigations"))
	{
		AnalysisDataService::Instance().remove("investigations");

	}
	
	QString algName("SearchByRunNumber");
	const int version=-1;

	QString startDate,endDate;
	getDates(startDate,endDate);

	double startRun=0,endRun=0;
	//get start and end run values 
	getRunValues(startRun,endRun);
	
	//try to validate the UI level.
	if(startRun> endRun)
	{
		emit error("Run end number cannot be lower than run start number.");
		return false;
	}

	// get the selected instrument
	QString instr ;;
	getSelectedInstrument(instr);
	
	bool bCase(isCaseSensitiveSearch());
	
	QString keywords= m_uiForm.keywordslineEdit->text();

	//ITableWorkspace_sptr  ws_sptr;
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
		alg->setProperty("Instrument",instr.toStdString());
		alg->setProperty("StartDate",startDate.toStdString());
		alg->setProperty("EndDate",endDate.toStdString());
		alg->setProperty("Case Sensitive",bCase);
		alg->setProperty("Keywords",keywords.toStdString());
		alg->setProperty("OutputWorkspace","investigations");
	
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
	while(!result.available() )
	{
		QCoreApplication::processEvents();
		
	}
	if(result.available())
	{
		if(!alg->isExecuted())
		{
			ws_sptr.reset();
			return false;
		}
		if(AnalysisDataService::Instance().doesExist("investigations"))
		{
			ws_sptr = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>
				(AnalysisDataService::Instance().retrieve("investigations"));
		}
			
		updatesearchResults(ws_sptr);
	}
	return true;

}

/** This method closes the search widget.
*/
void ICatSearch::onClose()
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
	//ICatUtils utils;
	if(!m_utils_sptr)
		return;
	m_utils_sptr->investigationSelected(m_uiForm.searchtableWidget,item,m_applicationWindow,m_ws_sptr);
}

/** This method saves search settings
*/
void ICatSearch::saveSettings()
{
	QSettings searchsettings;
	searchsettings.beginGroup("ICatSettings/Search");
	searchsettings.setValue("StartRun",m_uiForm.startRunEdit->text());
    searchsettings.setValue("EndRun",m_uiForm.endRunEdit->text());
	searchsettings.setValue("Instrument",m_uiForm.instrumentBox->currentText());
	searchsettings.setValue("Start Date",m_uiForm.startdateLineEdit->text());
	searchsettings.setValue("End Date",m_uiForm.enddateLineEdit->text());
	searchsettings.setValue("Keywords",m_uiForm.keywordslineEdit->text());
	searchsettings.setValue("Case Sensitive",m_uiForm.casesensitiveBox->isChecked());
	
	searchsettings.endGroup();

}
/// read settings from registry
void ICatSearch::readSettings()
{
	QSettings searchsettings;
	searchsettings.beginGroup("ICatSettings/Search");
	m_uiForm.startRunEdit->setText(searchsettings.value("startRun").toString());
	m_uiForm.endRunEdit->setText(searchsettings.value("endRun").toString());
	//m_uiForm.instrumentBox->setItemText (0,searchsettings.value("instrument").toString());
	int index=m_uiForm.instrumentBox->findText(searchsettings.value("instrument").toString());
	if(index!=-1)
	{
		m_uiForm.instrumentBox->setCurrentIndex(index);
	}
	m_uiForm.startdateLineEdit->setText(searchsettings.value("Start Date").toString());
	m_uiForm.enddateLineEdit->setText(searchsettings.value("End Date").toString());
	m_uiForm.casesensitiveBox->setChecked(searchsettings.value("Case Sensitive").toBool());

	searchsettings.endGroup();
}

//handler for helpbutton
void ICatSearch::helpButtonClicked()
{
	QDesktopServices::openUrl(QUrl("http://www.mantidproject.org/ISIS_Search"));

}

void ICatSearch::mousePressEvent (QMouseEvent* )
{
	/*if(m_uiForm.startRunEdit->underMouse())
	{
		emit error("mousePressEvent:undermouse");
		if(event->button()==Qt::LeftButton )
		{
			if(m_utils_sptr->calendarWidget())
			{		
				m_utils_sptr->calendarWidget()->hide();
			}
		}
	}
	else
	{
		emit error("mousePressEvent:not undermouse");
	}*/

}
bool ICatSearch::eventFilter(QObject *obj, QEvent *event)
{
	if (event->type() ==QEvent::FocusIn && obj==m_uiForm.searchframeWidget)
	{		
		if(m_utils_sptr->calendarWidget())
		{
			m_utils_sptr->calendarWidget()->hide();
		}
	
	}
	else if (event->type()==QEvent::MouseButtonPress)
	{
		if(m_utils_sptr->calendarWidget())
		{
			m_utils_sptr->calendarWidget()->hide();
		}

	}
	else
	{
		// standard event processing
		return QWidget::eventFilter(obj, event);
	}
	return true;
}





