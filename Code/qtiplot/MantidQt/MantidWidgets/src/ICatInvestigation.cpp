//----------------------
// Includes
//----------------------
#include "MantidQtMantidWidgets/ICatInvestigation.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/TableRow.h" 
#include "MantidKernel/ConfigService.h"

#include<QStringList>
#include<QHeaderView>
#include<QFont>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

//----------------------
// Public member functions
//----------------------
///Constructor
ICatInvestigation::ICatInvestigation(long long investId,const QString &RbNumber,const QString &Title,
				    const QString &Instrument,QWidget *par) :QWidget(par),m_invstId(investId),
					m_RbNumber(RbNumber),m_Title(Title),m_Instrument(Instrument),m_archiveLoc("")
{
	initLayout();
	m_uiForm.invsttableWidget->verticalHeader()->setVisible(false);
	m_uiForm.LoadButton->setEnabled(false);
	m_uiForm.downloadButton->setEnabled(false);
	
	populateInvestigationTreeWidget();//Tree on LHS of the display

	connect(m_uiForm.invsttreeWidget,SIGNAL(itemClicked (QTreeWidgetItem*, int )),this,SLOT(investigationClicked(QTreeWidgetItem*, int)));
	connect(m_uiForm.cancelButton,SIGNAL(clicked()),this,SLOT(onCancel()));
	connect(m_uiForm.invsttreeWidget,SIGNAL(itemExpanded(QTreeWidgetItem* )),this,SLOT(investigationWidgetItemExpanded(QTreeWidgetItem* )));
	connect(m_uiForm.invsttableWidget,SIGNAL(itemClicked (QTableWidgetItem* )),this,SLOT(tableItemSelected(QTableWidgetItem* )));
	
	//download button clciked
	connect(m_uiForm.downloadButton,SIGNAL(clicked()),this,SLOT(onDownload()));
	//load button clikced
	connect(m_uiForm.LoadButton,SIGNAL(clicked()),this,SLOT(onLoad()));
    
	connect(this,SIGNAL(error(const QString&)),parent()->parent(),SLOT(writetoLogWindow(const QString& )));
	connect(this,SIGNAL(loadRawAsynch(const QString&,const QString&)),parent()->parent(),SLOT(executeLoadRawAsynch(const QString&,const QString& )));
	connect(this,SIGNAL(loadNexusAsynch(const QString&,const QString&)),parent()->parent(),SLOT(executeLoadNexusAsynch(const QString&,const QString& )));

}

/// Set up the dialog layout
void ICatInvestigation::initLayout()
{
  m_uiForm.setupUi(this);
}

/// Poulate the tree widget on LHS
void ICatInvestigation::populateInvestigationTreeWidget(){
	
	QStringList qlist;
	qlist.push_back(m_Title);
	QTreeWidgetItem *item1 = new QTreeWidgetItem(qlist);
	item1->setToolTip(0,m_Title);

	qlist.clear();
	QString rbNumber("Rb number: ");
	rbNumber+=m_RbNumber;
	qlist.push_back(rbNumber);
	QTreeWidgetItem *item2 = new QTreeWidgetItem(qlist);
	item2->setToolTip(0,rbNumber);
	item1->addChild(item2);


	qlist.clear();
	QString instrument("Instrument: ");
	instrument+=m_Instrument;
	qlist.push_back(instrument);
	QTreeWidgetItem *item3 = new QTreeWidgetItem(qlist);
	item1->addChild(item3);

	m_uiForm.invsttreeWidget->insertTopLevelItem(0,item1);

	qlist.clear();
	qlist.push_back("DataSets");
	QTreeWidgetItem *item4 = new QTreeWidgetItem(qlist);
	item1->addChild(item4);

	qlist.clear();
	qlist.push_back("Default");
	QTreeWidgetItem *item5 = new QTreeWidgetItem(qlist);
	item4->addChild(item5);

	qlist.clear();
	qlist.push_back("Status:");
	QTreeWidgetItem *sitem = new QTreeWidgetItem(qlist);
	item5->addChild(sitem);
	
	qlist.clear();
	qlist.push_back("Type:");
	QTreeWidgetItem *titem = new QTreeWidgetItem(qlist);
	item5->addChild(titem);
	qlist.clear();
	qlist.push_back("Description:");
	QTreeWidgetItem* ditem = new QTreeWidgetItem(qlist);
	item5->addChild(ditem);
	
}

///
void ICatInvestigation::tableItemSelected(QTableWidgetItem*)
{
	m_uiForm.downloadButton->setEnabled(true);
}
/// execute getdatafIles algorithm
ITableWorkspace_sptr ICatInvestigation::executeGetdataFiles()
{
	QString algName("GetInvestigation");
	const int version=1;
	Mantid::API::ITableWorkspace_sptr  ws_sptr;
	Mantid::API::IAlgorithm_sptr alg;
	try
	{
		alg = Mantid::API::AlgorithmManager::Instance().create(algName.toStdString(),version);

	}
	catch(...)
	{
		throw std::runtime_error("Error when loading the data files associated to the selected investigation "); 
	}
	try
	{
		alg->setProperty("InvestigationId",m_invstId);
		alg->setPropertyValue("OutputWorkspace","insvestigation");
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
		(AnalysisDataService::Instance().retrieve("insvestigation"));

	return ws_sptr;
	
}
/// This  executes the GetDataSets algorithm and creates workspace
ITableWorkspace_sptr ICatInvestigation::executeGetdataSets()
{
	QString algName("GetDataSets");
	const int version=1;
	ITableWorkspace_sptr ws_sptr;
	Mantid::API::IAlgorithm_sptr alg;
	try
	{
		alg = Mantid::API::AlgorithmManager::Instance().create(algName.toStdString(),version);
	}
	catch(...)
	{
		throw std::runtime_error("Error when displaying the data sets associated to the selected investigation"); 
	}
	try
	{
		alg->setProperty("InvestigationId",m_invstId);
		alg->setPropertyValue("OutputWorkspace","datasets");
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
		(AnalysisDataService::Instance().retrieve("datasets"));

	return ws_sptr;
}
/**This method executes the GetdatFile algorithm used for getting the data file location or downloading the data file
  *@param fileName -name of the file to download
  *@param fileLoc -archive location of the file
*/
bool ICatInvestigation::executeDownloadDataFile(const QString& fileName,QString& fileLoc)
{
	//
	QString algName("GetDataFile");
	const int version=1;

	if(!AnalysisDataService::Instance().doesExist("insvestigation"))
	{
		return false;
	}

	Mantid::API::IAlgorithm_sptr alg;
	try
	{
		alg = Mantid::API::AlgorithmManager::Instance().create(algName.toStdString(),version);
	}
	catch(...)
	{
		throw std::runtime_error("Error when getting/downloading data file from isis server "); 
	}
	try
	{
		alg->setProperty("Filename",fileName.toStdString());
		//alg->setProperty("InputWorkspace",m_datafilesws_sptr);
		alg->setPropertyValue("InputWorkspace","insvestigation");
	}
	catch(std::invalid_argument& e)
	{		
		emit error(e.what());
		return false;
	}
	alg->execute();
	if(!alg->isExecuted())
	{
		return false;
	}
	try
	{
		fileLoc = QString::fromStdString(alg->getPropertyValue("FileLocation"));
	}
	catch (Mantid::Kernel::Exception::NotFoundError&e)
	{
		emit error(e.what());
		return false;
	}
	return true;
	
	
}
/** this method gets called when an item on the investigation tree widget is  clicked
  *@param item - selected item
  *@param int 
*/
void ICatInvestigation::investigationClicked(QTreeWidgetItem* item, int)
{
	QFont font;
	font.setBold(true);
	if(!item->text(0).compare("Default"))
	{
		if(!m_datafilesws_sptr){
		m_datafilesws_sptr = executeGetdataFiles();
				
        m_uiForm.invstlabel->clear();
		m_uiForm.invstlabel->setText("ICat Datafiles");
		m_uiForm.invstlabel->setAlignment(Qt::AlignHCenter);
		m_uiForm.invstlabel->setFont(font);
		}
		populateinvestigationWidget(m_datafilesws_sptr);
	}
	if(!item->text(0).compare("DataSets"))
	{
		if(!m_datasetsws_sptr){
			m_datasetsws_sptr= executeGetdataSets();
			m_uiForm.invstlabel->clear();
			m_uiForm.invstlabel->setText("ICat Datasets");
			m_uiForm.invstlabel->setAlignment(Qt::AlignHCenter);
			m_uiForm.invstlabel->setFont(font);
		}
	    populateinvestigationWidget(m_datasetsws_sptr);
	}
}
/**This method populates the investigation table widget
  *@param ws_sptr - shared pointer to workspace
*/
void ICatInvestigation::populateinvestigationWidget(Mantid::API::ITableWorkspace_sptr & ws_sptr)
{	
	if(!ws_sptr){return;}
	
	//below for loop for clearing the table widget on each mouse click,otherwise rows will be appended.
	// table widget clear() method is clearing only the item text,not removing the rows,columns
	// so i'm using removeRow().When I removed the row from top of the table it was not working.

	for (int i=m_uiForm.invsttableWidget->rowCount()-1;i>=0;--i)
	{
		m_uiForm.invsttableWidget->removeRow(i);
	}

	for (int i=0;i<ws_sptr->rowCount();++i)
	{
		m_uiForm.invsttableWidget->insertRow(i);
		//setting the row height of tableWidget 
		m_uiForm.invsttableWidget->setRowHeight(i,20);
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
			 m_uiForm.invsttableWidget->setItem(j,i, newItem);
			 newItem->setToolTip(QString::fromStdString(ostr.str()));
		}
	}
	//setting table widget header labels from table workspace
	m_uiForm.invsttableWidget->setHorizontalHeaderLabels(qlabelList);
	QFont font;
	font.setBold(true);
	for (int i=0;i<m_uiForm.invsttableWidget->columnCount();++i)
	{
		m_uiForm.invsttableWidget->horizontalHeaderItem(i)->setFont(font);;
	}
	//resizing the coluns based on data size
	m_uiForm.invsttableWidget->resizeColumnsToContents ();
 }
/** Cancel button clicked
*/
void  ICatInvestigation::onCancel()
{
	this->close();
	QObject* qobj=parent();
	QWidget* parent=qobject_cast<QWidget*>(qobj);
	if(parent)
	{
		parent->close();
	}
}
/** This method gets the selected file name from the selected row in the table
  *@param fileName - table widget item selected.
*/
void ICatInvestigation::getSelctedFileName(QString& fileName )
{
	QList<QTableWidgetItem *> items = m_uiForm.invsttableWidget->selectedItems();
	if(!items.empty())
	{
		fileName=items[0]->text();
	}
	
}
/** download button clicked
*/
void  ICatInvestigation::onDownload()
{
	//get selected file name to download
    QString fileName,fileLoc;
	//get selected file from the Mantid-ICat interface to download
	getSelctedFileName(fileName);

	if(executeDownloadDataFile(fileName,fileLoc))
	{
		//set archive file location.
		setisisarchiveFileLocation(fileLoc);
		//if download completed succesfully enable load button
		m_uiForm.LoadButton->setEnabled(true);
	}

}
/**This method sets the isis archive location
  *@fileLoc - isis archive location 
 */ 
void ICatInvestigation::setisisarchiveFileLocation(const QString& fileLoc)
{
	m_archiveLoc=fileLoc;

}
/**This method gets called when Treewidget item defaults expanded
  *@param item - treewidget item 
*/
void ICatInvestigation::investigationWidgetItemExpanded(QTreeWidgetItem* item )
{
	if(item->text(0)=="Default")
	{
		if(!m_datasetsws_sptr){
			m_datasetsws_sptr = executeGetdataSets();
		}

		QStringList qlist;
		std::string status = m_datasetsws_sptr->getRef<std::string>("Status",0);
		QString qval="Status: "+QString::fromStdString(status);
		QTreeWidgetItem* child = item->child(0);
		child->setText(0,qval);
		child->setToolTip(0,qval);
		
		std::string type = m_datasetsws_sptr->getRef<std::string>("Type",0);
		qlist.clear();
		qval="Type: "+QString::fromStdString(type);
		
	    child = item->child(1);
		child->setText(0,qval);
		child->setToolTip(0,qval);

		std::string description = m_datasetsws_sptr->getRef<std::string>("Description",0);
		qval="Description: "+QString::fromStdString(description);
		child=item->child(2);
		child->setText(0,qval);
		child->setToolTip(0,qval);
	
	}
		
}
/// Load button clicked
void ICatInvestigation::onLoad()
{
	///get filename (raw,nexus,log) from table widget to load to mantid
	QString fileName;
	getSelctedFileName(fileName);
	QString wsName;
	int index = fileName.lastIndexOf(".");
	if(index!=-1)
	{
		wsName=fileName.left(index);
	}
	QString filepath;
	if(m_archiveLoc.isEmpty())
	{				
		filepath = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("icatDownload.directory"));
		filepath += fileName;
	}
	else
	{
		filepath=m_archiveLoc;
	}

	if (isRawFile(fileName))
	{		
		if(!isLoadingControlled())
		{
			if(!executeLoadRaw(filepath,wsName))
			{
				return;
			}

		}
		else
		{		
			emit loadRawAsynch(filepath,wsName);
		}
	}
	else if (isNexusFile(fileName))
	{
		/*QString filepath;
		if(m_archiveLoc.isEmpty())
		{				
			filepath = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("icatDownload.directory"));
			filepath += fileName;
		}
		else
		{
			filepath=m_archiveLoc;
		}*/
		if(!isLoadingControlled())
		{
			if(!executeLoadNexus(filepath,wsName))
			{
				return;
			}
		}
		else
		{			
			emit loadNexusAsynch(filepath,wsName);
		}
	}
	else
	{
		emit error("ICat interface is not currently supporting the loading of files with extension .log");
	}
}
/// If user selected controlled loading of data check box
bool ICatInvestigation::isLoadingControlled()
{
	return m_uiForm.loadcheckBox->isChecked();
}

/**This method checks the file name extension and returns true if it's raw file 
  *@param fileName name of the file
  *@return  boolean
*/
bool ICatInvestigation::isRawFile(const QString& fileName)
{
	int index = fileName.lastIndexOf(".");
	bool braw;
	QString extn;
	if(index!=-1)
	{
		extn=fileName.right(fileName.length()-index-1);
	}
	(!extn.compare("raw",Qt::CaseInsensitive))? braw=true : braw=false;
	return braw;
}

/**This method checks the file name extension and returns true if it's nexus file 
  *@param fileName name of the file
  *@return  boolean
*/
bool ICatInvestigation::isNexusFile(const QString& fileName)
{
	bool bnxs;
	QString extn;
	int index = fileName.lastIndexOf(".");
	if(index!=-1)
	{
		extn=fileName.right(fileName.length()-index-1);
	}
	(!extn.compare("nxs")|| !extn.compare("NXS") )? bnxs=true : bnxs=false;
	
	return bnxs;
}

/** This method executes loadRaw algorithm
  * @param fileName name of the raw file
*/
bool ICatInvestigation::executeLoadRaw(const QString& fileName,const QString& wsName)
{	
	return execute("LoadRaw",3,fileName,wsName);
}

/** This method executes loadNexus algorithm
  * @param fileName name of the nexus file
*/
bool ICatInvestigation::executeLoadNexus(const QString& fileName,const QString& wsName)
{
	return execute("LoadNexus",1,fileName,wsName);

}
/**This method executes loadraw/loadnexus algorithm
  *@param algName - algoritm name
  *@param version -algorithm version
  *@param fileName - name of the file to load
*/
bool ICatInvestigation::execute(const QString& algName,const int& version,const QString& fileName,const QString& wsName)
{
	Mantid::API::IAlgorithm_sptr alg;
	try
	{
		alg = Mantid::API::AlgorithmManager::Instance().create(algName.toStdString(),version);
	}
	catch(...)
	{
		throw std::runtime_error("Error when loading the raw file"+ fileName.toStdString()); 
	}
	try
	{
		alg->setProperty("Filename",fileName.toStdString());
	}
	catch(std::invalid_argument&ex)
	{	
		emit error(QString::fromStdString(ex.what()));
		return false;
	}
	try{
		alg->setPropertyValue("OutputWorkspace",wsName.toStdString());
	}
	catch(std::invalid_argument& e)
	{		
		emit error(e.what());
		return false;
	}
	alg->execute();
	return (alg->isExecuted());

}
