
//----------------------
// Includes
//----------------------
#include "MantidQtMantidWidgets/ICatSearch.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/TableRow.h" 
#include "MantidQtAPI/InterfaceManager.h"

#include<QStringList>
#include<QFont>
#include <QTableWidgetItem>
#include <QSettings>
#include <QMdiSubWindow>
#include <QDesktopServices>
#include <QUrl>
#include <QLayoutItem>

using namespace Mantid::Kernel;
using namespace Mantid::API;

using namespace MantidQt::MantidWidgets;

//----------------------
// Public member functions
//----------------------
///Constructor
ICatSearch::ICatSearch(QWidget *par) :
QWidget(par),m_sender(NULL),m_invstWidget(NULL),
	m_utils_sptr(new ICatUtils),m_applicationWindow(NULL)
{
	
	// getting the application window pointer and setting it as the parent 
	QObject* qobj = parent();
	QWidget* parent = qobject_cast<QWidget*>(qobj->parent());
	if(parent)
	{
		setparentWidget(parent);
	}
	m_utils_sptr->setParent(parent);
	initLayout();
    m_alg=createAlgorithm();
	addtoPropertyLabelsHash();
	
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
	// when light blue back ground is set in ICat search dailog ,
	//the instrument combo box popup down and up arrow disappears
	// I'm setting this style sheet to bring the combobox's popup listview's arrows back.
	QString str="QComboBox#Instrument QListView{background-color: white;background-image:"
		"url(ICatCombobackground.png);background-attachment: scroll;}"
		"QComboBox#Instrument QListView QScrollBar:vertical{background-image:"
		"url(:/images/ICatComboVScrollbar.png); "
		"background-repeat: repeat-y; width: 17px; height:20px;} ";
	m_uiForm.Instrument->setStyleSheet(str);
	
  connect(this,SIGNAL(error(const QString&)),parent()->parent(),SLOT(writeErrorToLogWindow(const QString&)));
	try
	{
	populateInstrumentBox();
	}
	catch(Mantid::Kernel::Exception::NotFoundError&)
	{
		emit error("Error when Populating instruments box");

	}
	catch(std::runtime_error & e)
	{
		emit error(e.what());
	}
	
	//validator for start and end run numbers
	QValidator * runval= new QIntValidator(0,100000000,m_uiForm.StartRun);
	m_uiForm.StartRun->setValidator(runval);
	m_uiForm.EndRun->setValidator(runval);
			
	//getting last saved input data from registry
	readSettings();

	connect(m_uiForm.searchButton,SIGNAL(clicked()),this,SLOT(onSearch()));
	connect(m_uiForm.closeButton,SIGNAL(clicked()),this,SLOT(onClose()));
	connect(m_uiForm.searchtableWidget,SIGNAL(itemDoubleClicked(QTableWidgetItem* )),
		this,SLOT(investigationSelected(QTableWidgetItem* )));
	
	connect(m_uiForm.startdatetoolButton,SIGNAL(clicked()),this,SLOT(popupCalendar()));
	connect(m_uiForm.enddatetoolButton,SIGNAL(clicked()),this,SLOT(popupCalendar()));
	connect(m_uiForm.helpButton,SIGNAL(clicked()),this,SLOT(helpButtonClicked()));

	//this is for hiding the calendar widget if it's visible
	m_uiForm.StartRun->installEventFilter(this);
	m_uiForm.EndRun->installEventFilter(this);
	m_uiForm.Keywords->installEventFilter(this);
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
	executeSearch(m_ws_sptr);
}

/** Is case sensitive search
*/
bool ICatSearch::isCaseSensitiveSearch()
{
	return m_uiForm.CaseSensitive->isChecked();
}



/* This method updates the search result to search tree
 * @param ws_sptr :: workspace shared pointer
*/ 
void ICatSearch::updatesearchResults(ITableWorkspace_sptr& ws_sptr )
{	
	if(!m_utils_sptr)
  {
		return;
  }
	m_utils_sptr->resetSearchResultsWidget(m_uiForm.searchtableWidget);
	m_utils_sptr->updatesearchResults(ws_sptr,m_uiForm.searchtableWidget);
	m_utils_sptr->updateSearchLabel(ws_sptr,m_uiForm.searchlabel);

}

/** This method populates the instrument box
*/
void ICatSearch::populateInstrumentBox(){
  				
		if(!m_utils_sptr)
    {
			return;
    }
		m_utils_sptr->populateInstrumentBox(m_uiForm.Instrument);
	
}

/**This method gets run numbers from the start and end run boxes.
  *@param startRun :: start run number
  *@param endRun :: end run number
*/
void ICatSearch::getRunValues(double& startRun,double& endRun)
{
	endRun = m_uiForm.EndRun->text().toDouble();
	startRun = m_uiForm.StartRun->text().toDouble();
}

/**This method gets start and end dates from the start and end date boxes.
  *@param startDate :: start date  
  *@param endDate :: end date
*/
void ICatSearch::getDates(QString& startDate,QString& endDate)
{
	startDate = m_uiForm.StartDate->text();
	endDate =m_uiForm.EndDate->text();

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
	m_uiForm.StartDate->setText(date.toString("dd/MM/yyyy"));
	}
	if(!m_sender->objectName().compare("enddatetoolButton"))
	{
	m_uiForm.EndDate->setText(date.toString("dd/MM/yyyy"));
	}
}

/**This method gets the selected instrument
  *@param instrName :: name of the selected instrument
*/
void ICatSearch::getSelectedInstrument(QString& instrName)
{
	instrName=m_uiForm.Instrument->currentText();
}

Mantid::API::IAlgorithm_sptr ICatSearch::createAlgorithm()
{
	QString algName("CatalogSearch");
	Mantid::API::IAlgorithm_sptr alg;
	const int version=-1;
	try
	{
	alg = Mantid::API::AlgorithmManager::Instance().create(algName.toStdString(),version);
	}
	catch(...)
	{
		throw std::runtime_error("Error when creating search algorithm"); 
	}
	return alg;

}
/**This method executes the search by run number algorithm
 *@param ws_sptr :: shared pointer to outputworkspace
*/
bool  ICatSearch::executeSearch(ITableWorkspace_sptr& ws_sptr)
{
	//before starting new search investigations clear the old one.
	m_utils_sptr->clearSearch(m_uiForm.searchtableWidget,"investigations");
	
	//update the label status
	m_utils_sptr->setLabelText(m_uiForm.searchlabel,"Searching investigations...");
	
	//now get the input values for search
	QString startDate,endDate;
	getDates(startDate,endDate);

	double startRun=0,endRun=0;
	//get start and end run values 
	getRunValues(startRun,endRun);
	
	// get the selected instrument
	QString instr ;
	getSelectedInstrument(instr);
	
	bool bCase(isCaseSensitiveSearch());
	
	QString keywords= m_uiForm.Keywords->text();

		
	if(!setProperty("StartRun",startRun))
	{
		updatesearchResults(ws_sptr);
		return false;
	}
	
	if(!setProperty("EndRun",endRun)){
		
		updatesearchResults(ws_sptr);
		return false;
	}
	if(!setProperty("Instrument",instr.toStdString())){
		updatesearchResults(ws_sptr);
		return false;
	}
	if(!setProperty("StartDate",startDate.toStdString())){
		updatesearchResults(ws_sptr);
		return false;
	}
	if(!setProperty("EndDate",endDate.toStdString())){
		updatesearchResults(ws_sptr);
		return false;
	}
	if(!setProperty("Case Sensitive",bCase)){
		updatesearchResults(ws_sptr);
		return false;
	}
	if(!setProperty("Keywords",keywords.toStdString())){
		updatesearchResults(ws_sptr);
		return false;
	}
	if(!setProperty("OutputWorkspace","investigations")){
		updatesearchResults(ws_sptr);
		return false;
	}
	
	// execute the algorithm asynchrnously	
	Poco::ActiveResult<bool> result(m_alg->executeAsync());
	while(!result.available() )
	{
		QCoreApplication::processEvents();
		
	}
	if(result.available())
	{
		if(result.failed())
		{
			ws_sptr.reset();
			updatesearchResults(ws_sptr);
			emit error(QString::fromStdString(result.exception()->message()));
			return false;
		}
		if(AnalysisDataService::Instance().doesExist("investigations"))
		{
			ws_sptr = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>
				(AnalysisDataService::Instance().retrieve("investigations"));
			updatesearchResults(ws_sptr);
		}
			
		
	}
	return true;

}

struct Contains
{ 
Contains(std::string name):m_name(name){}
bool operator()(Mantid::Kernel::Property* prop)
{
std::string name=prop->name();
std::string::iterator pos = std::remove_if(name.begin(),name.end(), isspace);
name.erase(pos,name.end());//removing the sapce

return (!name.compare(m_name));
}
std::string m_name;

};

/** This method adds prpoerty name and validator label for the property to a hash table
  * This method iterates through each widget in the search grid layout and gets the label widget
  * and if it's validator label adds this to a hash table.
  * validator label objectname  for each property is "propertyname_2" eg;"StartDate_2"
  * from the label object name remove the "_2" to get propertyname
  * some property has space between words.remove the space(as space not allowed in QT object names)
  * and compare with propertyname to know the label is a validator label
*/
void ICatSearch::addtoPropertyLabelsHash()
{		
	//get total row and column count in the gridlayout where search controls are placed
	int totalcol= m_uiForm.gridLayout->columnCount();
	int totalrow =m_uiForm.gridLayout->rowCount();
	//loop through each widget in the gridlayout
	for (int row=0;row<totalrow;++row)
	{
		for (int col=0;col<totalcol;++col)
		{
			QLayoutItem  *item= m_uiForm.gridLayout->itemAtPosition(row,col); 
			if(!item) continue;
			QWidget* widget=item->widget();
			if(!widget) continue;
			QLabel* label=qobject_cast<QLabel*>(widget);
			if(!label)
			{
				continue;
			}
			
			//for each property,the validator label is named as "propertyname_2",
			//now remove "_2" from label name to get property name
			int index=label->objectName().indexOf("_");
			if(index==-1)
			{
				continue;
			}
				QString name;
				name=label->objectName().left(index);
				std::string propName=name.toStdString();
				
				std::vector<Mantid::Kernel::Property*> props=m_alg->getProperties();
				//if this name exists in algorithm properties vector it's a property validator label
				std::vector<Mantid::Kernel::Property*>::iterator prop;
				prop=std::find_if(props.begin(),props.end(),Contains(propName));
				if(prop!=props.end())
				{				
					//at this point the label is a validator label
					//add the lable to a hash 
					m_propLabelHash[QString::fromStdString((*prop)->name())]=label;
					label->hide();//initially hide the label 

				}
		
			
		}
	}
	
}
/// This method shows the invalid marker label for a property
void ICatSearch::showInvalidMarkerLabel(const QString& name)
{
	if(m_propLabelHash.contains(name))
	{	
		std::string documentation;
		std::vector<Mantid::Kernel::Property*> props=m_alg->getProperties();
		//if this name exists in algorithm properties vector it's a proprty validator label
		std::vector<Mantid::Kernel::Property*>::iterator result;
		result=std::find_if(props.begin(),props.end(),Contains(name.toStdString()));
		if(result!=props.end())
		{
			documentation=(*result)->documentation();

		}
		m_propLabelHash.value(name)->setToolTip(QString::fromStdString(documentation));
		m_propLabelHash.value(name)->show();
	}
}
/// hide invalid marker labels
void ICatSearch::hideInvalidMarkerLabel(const QString& name)
{
	if(m_propLabelHash.contains(name))
	{
		if(m_propLabelHash.value(name)->isVisible())
		{
			m_propLabelHash.value(name)->hide();
		}
	}
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
  *@param item ::  item in the table
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
	searchsettings.setValue("StartRun",m_uiForm.StartRun->text());
    searchsettings.setValue("EndRun",m_uiForm.EndRun->text());
	searchsettings.setValue("Instrument",m_uiForm.Instrument->currentText());
	searchsettings.setValue("Start Date",m_uiForm.StartDate->text());
	searchsettings.setValue("End Date",m_uiForm.EndDate->text());
	searchsettings.setValue("Keywords",m_uiForm.Keywords->text());
	searchsettings.setValue("Case Sensitive",m_uiForm.CaseSensitive->isChecked());
	
	searchsettings.endGroup();

}
/// read settings from registry
void ICatSearch::readSettings()
{
	QSettings searchsettings;
	searchsettings.beginGroup("ICatSettings/Search");
	m_uiForm.StartRun->setText(searchsettings.value("startRun").toString());
	m_uiForm.EndRun->setText(searchsettings.value("endRun").toString());
	//m_uiForm.Instrument->setItemText (0,searchsettings.value("instrument").toString());
	int index=m_uiForm.Instrument->findText(searchsettings.value("instrument").toString());
	if(index!=-1)
	{
		m_uiForm.Instrument->setCurrentIndex(index);
	}
	m_uiForm.StartDate->setText(searchsettings.value("Start Date").toString());
	m_uiForm.EndDate->setText(searchsettings.value("End Date").toString());
	m_uiForm.CaseSensitive->setChecked(searchsettings.value("Case Sensitive").toBool());

	searchsettings.endGroup();
}

//handler for helpbutton
void ICatSearch::helpButtonClicked()
{
	QDesktopServices::openUrl(QUrl("http://www.mantidproject.org/ISIS_Search"));

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





