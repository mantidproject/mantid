
#include "MantidQtMantidWidgets/ICatAdvancedSearch.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/TableRow.h" 

#include <QDesktopServices>
#include <QSettings>
#include <QUrl>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

ICatAdvancedSearch::ICatAdvancedSearch(QWidget* par):
QWidget(par),m_utils_sptr( new ICatUtils)
{
	initLayout();
	readSettings();
	connect(m_uiForm.searchButton,SIGNAL(clicked()),this,SLOT(onSearch()));
	connect(m_uiForm.closeButton,SIGNAL(clicked()),this,SLOT(onClose()));
	connect(m_uiForm.advSearchtableWidget,SIGNAL(itemDoubleClicked(QTableWidgetItem* )),
		this,SLOT(investigationSelected(QTableWidgetItem* )));
	connect(this,SIGNAL(error(const QString&)),parent()->parent(),SLOT(writetoLogWindow(const QString& )));
	connect(m_uiForm.startdatetoolButton,SIGNAL(clicked()),this,SLOT(popupCalendar()));
	connect(m_uiForm.enddatetoolButton,SIGNAL(clicked()),this,SLOT(popupCalendar()));
	connect(m_uiForm.helpButton,SIGNAL(clicked()),this,SLOT(helpButtonClicked()));

	//	 getting the application window pointer and setting it 
	//this is bcoz parent()->parent() is not working in some slots as I expected 
	QObject* qobj = parent();
	QWidget* parent = qobject_cast<QWidget*>(qobj->parent());
	if(parent)
	{
		setparentWidget(parent);
	}

	m_uiForm.startRunEdit->installEventFilter(this);
	m_uiForm.endRunEdit->installEventFilter(this);
	m_uiForm.keywordsEdit->installEventFilter(this);
	m_uiForm.advframeWidget->installEventFilter(this);
	m_uiForm.investigatonNameEdit->installEventFilter(this);
	m_uiForm.invstAbstractEdit->installEventFilter(this);
	m_uiForm.sampleEdit->installEventFilter(this);
	m_uiForm.invstsurnameEdit->installEventFilter(this);
	m_uiForm.datafilenameEdit->installEventFilter(this);


}
ICatAdvancedSearch::~ICatAdvancedSearch()
{
}
/* this method sets the parent widget as application window
*/
void ICatAdvancedSearch::setparentWidget(QWidget* par)
{
	m_applicationWindow= par;
}
void ICatAdvancedSearch::initLayout()
{
	m_uiForm.setupUi(this);

	// as the instrument/investigation type combo box's popup down and up arrow disappeared when the light blue back ground is set in ICat search dailog 
	// I'm setting this style sheet to bring the combo box arrows back.

	QString str="QComboBox#instrumentBox QListView{background-color: white;background-image: url(ICatCombobackground.png);background-attachment: scroll;}"
		"QComboBox#instrumentBox QListView QScrollBar:vertical{background-image: url(:/images/ICatComboVScrollbar.png); background-repeat: repeat-y; width: 17px; height:20px;} ";
	m_uiForm.instrumentBox->setStyleSheet(str);

    str="QComboBox#invstTypeBox QListView{background-color: white;background-image: url(ICatCombobackground.png);background-attachment: scroll;}"
		"QComboBox#invstTypeBox QListView QScrollBar:vertical{background-image: url(:/images/ICatComboVScrollbar.png); background-repeat: repeat-y; width: 17px; height:20px;} ";
	m_uiForm.invstTypeBox->setStyleSheet(str);
	

	QValidator * val= new QIntValidator(0,100000000,m_uiForm.startRunEdit);
	m_uiForm.startRunEdit->setValidator(val);
	m_uiForm.endRunEdit->setValidator(val);

	populateInstrumentBox();
	populateInvestigationType();
}

void ICatAdvancedSearch::populateInstrumentBox()
{	
	try{
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

void ICatAdvancedSearch::populateInvestigationType()
{

	ITableWorkspace_sptr ws_sptr=executeListInvestigationTypes();
	if(!ws_sptr)
	{
		emit error("Error when Populating the investigation types box");
		return;
	}
		
	// loop through values
	for(int row=0;row<ws_sptr->rowCount();++row)
	{
		//retrieving the  instrument name from table workspace
		std::string instName(ws_sptr->String(row,0));
		//populate the instrument box  
		m_uiForm.invstTypeBox->insertItem(row,QString::fromStdString(instName));

	}
	//sorting the combo by instrument name;
	m_uiForm.invstTypeBox->model()->sort(0);
	m_uiForm.invstTypeBox->insertItem(-1,"");

}

ITableWorkspace_sptr ICatAdvancedSearch:: executeListInvestigationTypes()
{
	
	QString algName("ListInvestigationTypes");
	const int version=1;
	ITableWorkspace_sptr  ws_sptr;
	Mantid::API::IAlgorithm_sptr alg;
	try
	{
		alg = Mantid::API::AlgorithmManager::Instance().create(algName.toStdString(),version);
	}
	catch(...)
	{
		//throw std::runtime_error("Error when Populating the instrument list box"); 
		throw;
	}
	try
	{
	alg->setPropertyValue("OutputWorkspace","investigationTypes");
	}
	catch(std::invalid_argument&)
	{			
		throw;
	}
	catch (Mantid::Kernel::Exception::NotFoundError&)
	{		
		throw;
	}
	catch(std::runtime_error & e)
	{
		emit error(e.what());
		throw;
	}
	
	Poco::ActiveResult<bool> result(alg->executeAsync());
	while( !result.available() )
	{
		QCoreApplication::processEvents();
	}
	if(!alg->isExecuted())
	{
		ws_sptr.reset();
		return ws_sptr;
	}
	
	if(AnalysisDataService::Instance().doesExist("investigationTypes"))
	{
		ws_sptr = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>
			(AnalysisDataService::Instance().retrieve("investigationTypes"));
	}
	return ws_sptr;

}

void ICatAdvancedSearch	::onSearch()
{
	QString invstName;
	QString invstAbstract;
	QString sampleName;
	QString invstSurName;
	QString dataFileName;
	QString instrName;
	QString invstType;
	QString keywords;

	bool bCase;
	double startRun=0,endRun=0;
	QString startDate,endDate;

	getInvestigationName(invstName);
	getInvestigationAbstract(invstAbstract);
	getSampleName(sampleName);
	getInvestigatorSurName(invstSurName);
	getDatafileName(dataFileName);
	getCaseSensitive(bCase);
	getInvestigationType(invstType);
	//get start and end run values 
	getRunNumbers(startRun,endRun);
	getDates(startDate,endDate);
	getInstrument(instrName);
	getKeyWords(keywords);

	//before starting new search for investigations object clear the old one.

	if(AnalysisDataService::Instance().doesExist("advanced_investigations"))
	{
		AnalysisDataService::Instance().remove("advanced_investigations");

	}
	
	QString algName("AdvancedSearch");
	const int version=-1;
	
	//try to validate the UI level.
	if(startRun> endRun)
	{
		emit error("Run end number cannot be lower than run start number.");
		return ;
	}

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
		alg->setProperty("Instrument",instrName.toStdString());
		alg->setProperty("StartDate",startDate.toStdString());
		alg->setProperty("EndDate",endDate.toStdString());
		//alg->setProperty("Keywords",m_uiForm.enddateLineEdit->setText);
		alg->setProperty("Case Sensitive",bCase);
		alg->setProperty("Keywords",keywords.toStdString());

		alg->setProperty("Investigation Name",invstName.toStdString());
		alg->setProperty("Investigation Abstract",invstAbstract.toStdString());
		alg->setProperty("Investigation Type",invstType.toStdString());
		alg->setProperty("Sample Name",sampleName.toStdString());
		alg->setProperty("Investigator SurName",invstSurName.toStdString());
		alg->setProperty("DataFile Name",dataFileName.toStdString());
		alg->setProperty("OutputWorkspace","advanced_investigations");
		
	
	}
	
	//get start and end run values 
	catch(std::invalid_argument& e)
	{			
		emit error(e.what());
		return ;
	}
	catch (Mantid::Kernel::Exception::NotFoundError& e)
	{
		emit error(e.what());
		return ;
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
			m_ws_sptr.reset();
			return ;
		}
		if(AnalysisDataService::Instance().doesExist("advanced_investigations"))
		{
			m_ws_sptr = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>
				(AnalysisDataService::Instance().retrieve("advanced_investigations"));
		}

		updatesearchResults(m_ws_sptr);
	}
	return ;

	
}

void ICatAdvancedSearch::getInvestigationName(QString& invstName)
{
	invstName = m_uiForm.investigatonNameEdit->text();
}
void ICatAdvancedSearch::getInvestigationAbstract(QString& invstAbstract)
{
	invstAbstract = m_uiForm.invstAbstractEdit->text();
}
void ICatAdvancedSearch::getInvestigatorSurName(QString& invstSurName)
{
	invstSurName = m_uiForm.invstsurnameEdit->text();
}

void ICatAdvancedSearch::getSampleName(QString& sampleName)
{
	sampleName = m_uiForm.sampleEdit->text();
}
			
void ICatAdvancedSearch::getDatafileName(QString& dataFileName)
{
	dataFileName = m_uiForm.datafilenameEdit->text();
}

void ICatAdvancedSearch::getInvestigationType(QString& invstType)
{

	invstType=m_uiForm.invstTypeBox->currentText();
}

void ICatAdvancedSearch::getRunNumbers(double& startRun,double& endRun)
{
	endRun = m_uiForm.endRunEdit->text().toDouble();
	startRun = m_uiForm.startRunEdit->text().toDouble();
}
void ICatAdvancedSearch::getDates(QString& startDate,QString& endDate)
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
void ICatAdvancedSearch::getInstrument(QString& instrName)
{
	instrName=m_uiForm.instrumentBox->currentText();
}
void ICatAdvancedSearch::getCaseSensitive(bool & bCase )
{
	bCase= m_uiForm.casesensitiveBox->isChecked();
}
void ICatAdvancedSearch::getKeyWords(QString& keywords)
{
	keywords=m_uiForm.keywordsEdit->text();
}

/* This method updates the search result to search tree
 * @param ws_sptr workspace shared pointer
*/ 
void ICatAdvancedSearch::updatesearchResults(ITableWorkspace_sptr& ws_sptr )
{	
	//ICatUtils utils;
	if(!m_utils_sptr)
		return;
	m_utils_sptr->resetSearchResultsWidget(m_uiForm.advSearchtableWidget);
	m_utils_sptr->updatesearchResults(ws_sptr,m_uiForm.advSearchtableWidget);
	m_utils_sptr->updateSearchLabel(ws_sptr,m_uiForm.advsearchLabel);

}

/** This method closes the search widget.
*/
void ICatAdvancedSearch::onClose()
{
	saveSettings();
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
void ICatAdvancedSearch::investigationSelected(QTableWidgetItem * item )
{
	//ICatUtils utils;
	if(!m_utils_sptr)
		return;
	m_utils_sptr->investigationSelected(m_uiForm.advSearchtableWidget,item,m_applicationWindow,m_ws_sptr);
}

///popup DateTime calender to select date
void ICatAdvancedSearch:: popupCalendar()
{
	if(!m_utils_sptr)
		return;

	m_utils_sptr->popupCalendar(this);
	
	QObject * qsender= sender();
	if(!qsender) return;
	 m_sender=qsender;
	
}

///This signal is selects a date from the calendarwidget and 
//set the selected to start and end date boxes.
void ICatAdvancedSearch::getDate(const QDate& date  )
{
	m_utils_sptr->closeCalendarWidget();//close the calendar widget
	if(!m_sender) return;

	//set the date to start and end line edit
	if(!m_sender->objectName().compare("startdatetoolButton"))
	{
	m_uiForm.startdateLineEdit->setText(date.toString("dd/MM/yyyy"));
	}
	if(!m_sender->objectName().compare("enddatetoolButton"))
	{
	m_uiForm.enddateLineEdit->setText(date.toString("dd/MM/yyyy"));
	}
}

//handler for helpbutton
void ICatAdvancedSearch::helpButtonClicked()
{
	QDesktopServices::openUrl(QUrl("http://www.mantidproject.org/Advanced_Search"));

}

/** This method saves search settings
*/
void ICatAdvancedSearch::saveSettings()
{
	QSettings searchsettings;
	searchsettings.beginGroup("ICatSettings/AdvancedSearch");
	searchsettings.setValue("Start Run",m_uiForm.startRunEdit->text());
    searchsettings.setValue("End Run",m_uiForm.endRunEdit->text());
	searchsettings.setValue("Instrument",m_uiForm.instrumentBox->currentText());
	searchsettings.setValue("Start Date",m_uiForm.startdateLineEdit->text());
	searchsettings.setValue("End Date",m_uiForm.enddateLineEdit->text());
	searchsettings.setValue("Keywords",m_uiForm.keywordsEdit->text());
	searchsettings.setValue("Case Sensitive",m_uiForm.casesensitiveBox->isChecked());

	searchsettings.setValue("Investigation Name",m_uiForm.investigatonNameEdit->text());
	searchsettings.setValue("Investigation Abstract",m_uiForm.invstAbstractEdit->text());
	searchsettings.setValue("Sample Name",m_uiForm.sampleEdit->text());
	searchsettings.setValue("Investigators Surname",m_uiForm.invstsurnameEdit->text());
	searchsettings.setValue("Datafile Name",m_uiForm.datafilenameEdit->text());
	searchsettings.setValue("Investigation Type",m_uiForm.invstTypeBox->currentText());
	searchsettings.endGroup();

}

/// read settings from registry
void ICatAdvancedSearch::readSettings()
{
	QSettings searchsettings;
	searchsettings.beginGroup("ICatSettings/AdvancedSearch");
	m_uiForm.startRunEdit->setText(searchsettings.value("Start Run").toString());
	m_uiForm.endRunEdit->setText(searchsettings.value("End Run").toString());
	//m_uiForm.instrumentBox->setItemText (0,searchsettings.value("instrument").toString());
	int index=m_uiForm.instrumentBox->findText(searchsettings.value("Instrument").toString());
	if(index!=-1)
	{
		m_uiForm.instrumentBox->setCurrentIndex(index);
	}

	m_uiForm.startdateLineEdit->setText(searchsettings.value("Start Date").toString());
	m_uiForm.enddateLineEdit->setText(searchsettings.value("End Date").toString());
	m_uiForm.casesensitiveBox->setChecked(searchsettings.value("Case Sensitive").toBool());
	m_uiForm.investigatonNameEdit->setText(searchsettings.value("Investigation Name").toString());
	m_uiForm.invstAbstractEdit->setText(searchsettings.value("Investigation Abstract").toString());
	m_uiForm.sampleEdit->setText(searchsettings.value("Sample Name").toString());
	m_uiForm.invstsurnameEdit->setText(searchsettings.value("Investigators Surname").toString());
	m_uiForm.datafilenameEdit->setText(searchsettings.value("Datafile Name").toString());
	index=m_uiForm.invstTypeBox->findText(searchsettings.value("Investigation Type").toString());
	if(index!=-1)
	{
		m_uiForm.invstTypeBox->setCurrentIndex(index);
	}


	searchsettings.endGroup();
}
bool ICatAdvancedSearch::eventFilter(QObject *obj, QEvent *event)
{
	if (event->type() ==QEvent::FocusIn && obj==m_uiForm.advframeWidget)
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





