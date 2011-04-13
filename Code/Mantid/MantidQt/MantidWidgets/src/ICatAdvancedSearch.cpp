
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

namespace MantidQt
{
  namespace MantidWidgets
  {

    ICatAdvancedSearch::ICatAdvancedSearch(QWidget* par):
    QWidget(par),m_utils_sptr(new ICatUtils),m_applicationWindow(NULL)
    {

      //	 getting the application window pointer and setting it
      //this is bcoz parent()->parent() is not working in some slots as I expected
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

      QString str="QComboBox#Instrument QListView{background-color: white;background-image: url(ICatCombobackground.png);background-attachment: scroll;}"
          "QComboBox#Instrument QListView QScrollBar:vertical{background-image: url(:/images/ICatComboVScrollbar.png); background-repeat: repeat-y; width: 17px; height:20px;} ";
      m_uiForm.Instrument->setStyleSheet(str);

      str="QComboBox#InvestigationType QListView{background-color: white;background-image: url(ICatCombobackground.png);background-attachment: scroll;}"
          "QComboBox#InvestigationType QListView QScrollBar:vertical{background-image: url(:/images/ICatComboVScrollbar.png); background-repeat: repeat-y; width: 17px; height:20px;} ";
      m_uiForm.InvestigationType->setStyleSheet(str);

      populateInstrumentBox();
      populateInvestigationType();


      QValidator * val= new QIntValidator(0,100000000,m_uiForm.StartRun);
      m_uiForm.StartRun->setValidator(val);
      m_uiForm.EndRun->setValidator(val);

      readSettings();

      connect(m_uiForm.searchButton,SIGNAL(clicked()),this,SLOT(onSearch()));
      connect(m_uiForm.closeButton,SIGNAL(clicked()),this,SLOT(onClose()));
      connect(m_uiForm.advSearchtableWidget,SIGNAL(itemDoubleClicked(QTableWidgetItem* )),
          this,SLOT(investigationSelected(QTableWidgetItem* )));
      connect(this,SIGNAL(error(const QString&)),parent()->parent(),SLOT(writeErrorToLogWindow(const QString& )));
      connect(m_uiForm.startdatetoolButton,SIGNAL(clicked()),this,SLOT(popupCalendar()));
      connect(m_uiForm.enddatetoolButton,SIGNAL(clicked()),this,SLOT(popupCalendar()));
      connect(m_uiForm.helpButton,SIGNAL(clicked()),this,SLOT(helpButtonClicked()));

      m_uiForm.StartRun->installEventFilter(this);
      m_uiForm.EndRun->installEventFilter(this);
      m_uiForm.Keywords->installEventFilter(this);
      m_uiForm.advframeWidget->installEventFilter(this);
      m_uiForm.InvestigationName->installEventFilter(this);
      m_uiForm.InvestigationAbstract->installEventFilter(this);
      m_uiForm.SampleName->installEventFilter(this);
      m_uiForm.InvestigatorSurName->installEventFilter(this);
      m_uiForm.DataFileName->installEventFilter(this);
    }

    void ICatAdvancedSearch::populateInstrumentBox()
    {
      try{
        if(!m_utils_sptr)
          return;
        m_utils_sptr->populateInstrumentBox(m_uiForm.Instrument);
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
      std::vector<std::string> invstList;
      try
      {
        invstList = executeListInvestigationTypes();
      }
      catch(std::runtime_error & e)
      {
        emit error(e.what());
      }
      if(invstList.empty())
      {
        emit error("Investigation Types list is empty");

      }
      std::vector<std::string>::const_iterator citr;
      // loop through values
      for(citr=invstList.begin();citr!=invstList.end();++citr)
      {
        //populate the instrument box
        m_uiForm.InvestigationType->addItem(QString::fromStdString(*citr));

      }
      //sorting the combo by instrument name;
      m_uiForm.InvestigationType->model()->sort(0);
      m_uiForm.InvestigationType->insertItem(-1,"");

    }

    std::vector<std::string> ICatAdvancedSearch:: executeListInvestigationTypes()
    {
      QString algName("CatalogListInvestigationTypes");
      const int version=-1;
      Mantid::API::IAlgorithm_sptr alg;
      try
      {
        alg = Mantid::API::AlgorithmManager::Instance().create(algName.toStdString(),version);
      }
      catch(...)
      {
        throw std::runtime_error("Error when Populating the Investigations types list box");

      }

      Poco::ActiveResult<bool> result(alg->executeAsync());
      while( !result.available() )
      {
        QCoreApplication::processEvents();
      }
      if(!alg->isExecuted())
      {

        //if the algorithm failed check the session id passed is valid
        if(! m_utils_sptr->isSessionValid(alg))
        {
          //at this point session is invalid, popup loginbox to login
          if(m_utils_sptr->login())
          {
            //now populate instrument box
            std::vector<std::string> invstTypes =executeListInvestigationTypes();
            return invstTypes;
          }
          else
          {
            throw std::runtime_error("Please Login to the information catalog using the login menu provided to do the investigation search.");
          }
        }
        else
        {
          return std::vector<std::string>();
        }

      }

      std::vector<std::string> invstlist ;
      try
      {

        invstlist= alg->getProperty("InvestigationTypes");
      }
      catch (Mantid::Kernel::Exception::NotFoundError&)
      {
        throw  std::runtime_error(" Error when retrieving the Investigation Types from the selected  catalog");
      }
      return invstlist;


    }

    Mantid::API::IAlgorithm_sptr ICatAdvancedSearch::createAlgorithm()
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

    void ICatAdvancedSearch	::onSearch()
    {
      m_ws_sptr.reset();
      //before starting new search investigations clear the old one.
      m_utils_sptr->clearSearch(m_uiForm.advSearchtableWidget,"advanced_investigations");

      //update the label status
      m_utils_sptr->setLabelText(m_uiForm.advsearchLabel,"Searching investigations...");

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
      getRunNumbers(startRun,endRun);
      getDates(startDate,endDate);
      getInstrument(instrName);
      getKeyWords(keywords);

      if(!setProperty("StartRun",startRun))
      {
        updatesearchResults(m_ws_sptr);
        return ;
      }
      if(!setProperty("EndRun",endRun))
      {
        updatesearchResults(m_ws_sptr);
        return ;
      }
      if(!setProperty("Instrument",instrName.toStdString()))
      {
        updatesearchResults(m_ws_sptr);
        return ;
      }
      if(!setProperty("StartDate",startDate.toStdString()))
      {
        updatesearchResults(m_ws_sptr);
        return ;
      }
      if(!setProperty("EndDate",endDate.toStdString()))
      {
        updatesearchResults(m_ws_sptr);
        return ;
      }

      if(!setProperty("Case Sensitive",bCase))
      {
        updatesearchResults(m_ws_sptr);
        return ;
      }
      if(!setProperty("Keywords",keywords.toStdString()))
      {
        updatesearchResults(m_ws_sptr);
        return ;
      }

      if(!setProperty("Investigation Name",invstName.toStdString()))
      {
        updatesearchResults(m_ws_sptr);
        return ;
      }
      if(!setProperty("Investigation Abstract",invstAbstract.toStdString()))
      {
        updatesearchResults(m_ws_sptr);
        return ;
      }
      if(!setProperty("Investigation Type",invstType.toStdString()))
      {
        updatesearchResults(m_ws_sptr);
        return ;
      }
      if(!setProperty("Sample Name",sampleName.toStdString()))
      {
        updatesearchResults(m_ws_sptr);
        return ;
      }
      if(!setProperty("Investigator SurName",invstSurName.toStdString()))
      {
        updatesearchResults(m_ws_sptr);
        return ;
      }
      if(!setProperty("DataFile Name",dataFileName.toStdString()))
      {
        updatesearchResults(m_ws_sptr);
        return ;
      }
      if(!setProperty("OutputWorkspace","advanced_investigations"))
      {
        updatesearchResults(m_ws_sptr);
        return ;
      }
      Poco::ActiveResult<bool> result(m_alg->executeAsync());
      while(!result.available() )
      {
        QCoreApplication::processEvents();

      }
      if(result.available())
      {
        //if(!m_alg->isExecuted())
        if(result.failed())
        {
          m_ws_sptr.reset();
          updatesearchResults(m_ws_sptr);
          return ;
        }
        if(AnalysisDataService::Instance().doesExist("advanced_investigations"))
        {
          m_ws_sptr = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>
          (AnalysisDataService::Instance().retrieve("advanced_investigations"));
          updatesearchResults(m_ws_sptr);
        }


      }
      return ;


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
    void ICatAdvancedSearch::addtoPropertyLabelsHash()
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
            continue;

          //validator labels is named as "propertyname_2",
          // remove "_2" from label name to get property name
          int index=label->objectName().indexOf("_");
          if(index!=-1)
          {
            QString name;
            name=label->objectName().left(index);
            std::string propName=name.toStdString();

            std::vector<Mantid::Kernel::Property*> props=m_alg->getProperties();
            //if this name exists in algorithm properties vector it's a property validator label
            std::vector<Mantid::Kernel::Property*>::iterator result;
            result=std::find_if(props.begin(),props.end(),Contains(propName));
            if(result!=props.end())
            {
              //at this point the label is a validator label
              m_propLabelHash[QString::fromStdString((*result)->name())]=label;
              label->hide();//initially hide the label

            }
          }

        }
      }

    }

    /// show invalid marker labels
    void ICatAdvancedSearch::showInvalidMarkerLabel(const QString& name)
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
    void ICatAdvancedSearch::hideInvalidMarkerLabel(const QString& name)
    {
      if(m_propLabelHash.contains(name))
      {
        if(m_propLabelHash.value(name)->isVisible())
        {
          m_propLabelHash.value(name)->hide();
        }
      }
    }

    void ICatAdvancedSearch::getInvestigationName(QString& invstName)
    {
      invstName = m_uiForm.InvestigationName->text();
    }
    void ICatAdvancedSearch::getInvestigationAbstract(QString& invstAbstract)
    {
      invstAbstract = m_uiForm.InvestigationAbstract->text();
    }
    void ICatAdvancedSearch::getInvestigatorSurName(QString& invstSurName)
    {
      invstSurName = m_uiForm.InvestigatorSurName->text();
    }

    void ICatAdvancedSearch::getSampleName(QString& sampleName)
    {
      sampleName = m_uiForm.SampleName->text();
    }

    void ICatAdvancedSearch::getDatafileName(QString& dataFileName)
    {
      dataFileName = m_uiForm.DataFileName->text();
    }

    void ICatAdvancedSearch::getInvestigationType(QString& invstType)
    {

      invstType=m_uiForm.InvestigationType->currentText();
    }

    void ICatAdvancedSearch::getRunNumbers(double& startRun,double& endRun)
    {
      endRun = m_uiForm.EndRun->text().toDouble();
      startRun = m_uiForm.StartRun->text().toDouble();
    }
    void ICatAdvancedSearch::getDates(QString& startDate,QString& endDate)
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
    void ICatAdvancedSearch::getInstrument(QString& instrName)
    {
      instrName=m_uiForm.Instrument->currentText();
    }
    void ICatAdvancedSearch::getCaseSensitive(bool & bCase )
    {
      bCase= m_uiForm.casesensitiveBox->isChecked();
    }
    void ICatAdvancedSearch::getKeyWords(QString& keywords)
    {
      keywords=m_uiForm.Keywords->text();
    }

    /* This method updates the search result to search tree
     * @param ws_sptr :: workspace shared pointer
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
     *@param item ::  item in the table
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
        m_uiForm.StartDate->setText(date.toString("dd/MM/yyyy"));
      }
      if(!m_sender->objectName().compare("enddatetoolButton"))
      {
        m_uiForm.EndDate->setText(date.toString("dd/MM/yyyy"));
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
      searchsettings.setValue("Start Run",m_uiForm.StartRun->text());
      searchsettings.setValue("End Run",m_uiForm.EndRun->text());
      searchsettings.setValue("Instrument",m_uiForm.Instrument->currentText());
      searchsettings.setValue("Start Date",m_uiForm.StartDate->text());
      searchsettings.setValue("End Date",m_uiForm.EndDate->text());
      searchsettings.setValue("Keywords",m_uiForm.Keywords->text());
      searchsettings.setValue("Case Sensitive",m_uiForm.casesensitiveBox->isChecked());

      searchsettings.setValue("Investigation Name",m_uiForm.InvestigationName->text());
      searchsettings.setValue("Investigation Abstract",m_uiForm.InvestigationAbstract->text());
      searchsettings.setValue("Sample Name",m_uiForm.SampleName->text());
      searchsettings.setValue("Investigators Surname",m_uiForm.InvestigatorSurName->text());
      searchsettings.setValue("Datafile Name",m_uiForm.DataFileName->text());
      searchsettings.setValue("Investigation Type",m_uiForm.InvestigationType->currentText());
      searchsettings.endGroup();

    }

    /// read settings from registry
    void ICatAdvancedSearch::readSettings()
    {
      QSettings searchsettings;
      searchsettings.beginGroup("ICatSettings/AdvancedSearch");
      m_uiForm.StartRun->setText(searchsettings.value("Start Run").toString());
      m_uiForm.EndRun->setText(searchsettings.value("End Run").toString());
      //m_uiForm.Instrument->setItemText (0,searchsettings.value("instrument").toString());
      int index=m_uiForm.Instrument->findText(searchsettings.value("Instrument").toString());
      if(index!=-1)
      {
        m_uiForm.Instrument->setCurrentIndex(index);
      }

      m_uiForm.StartDate->setText(searchsettings.value("Start Date").toString());
      m_uiForm.EndDate->setText(searchsettings.value("End Date").toString());
      m_uiForm.casesensitiveBox->setChecked(searchsettings.value("Case Sensitive").toBool());
      m_uiForm.InvestigationName->setText(searchsettings.value("Investigation Name").toString());
      m_uiForm.InvestigationAbstract->setText(searchsettings.value("Investigation Abstract").toString());
      m_uiForm.SampleName->setText(searchsettings.value("Sample Name").toString());
      m_uiForm.InvestigatorSurName->setText(searchsettings.value("Investigators Surname").toString());
      m_uiForm.DataFileName->setText(searchsettings.value("Datafile Name").toString());
      index=m_uiForm.InvestigationType->findText(searchsettings.value("Investigation Type").toString());
      if(index!=-1)
      {
        m_uiForm.InvestigationType->setCurrentIndex(index);
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

  }
}
