//----------------------
// Includes
//----------------------
#include "MantidQtMantidWidgets/ICatInvestigation.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/TableRow.h" 
#include "MantidKernel/ConfigService.h"

#include<QHeaderView>


using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

//----------------------
// Public member functions
//----------------------
///Constructor
ICatInvestigation::ICatInvestigation(long long investId,const QString &RbNumber,
									 const QString &Title,const QString &Instrument,ITableWorkspace_sptr& ws2_sptr,QWidget *par) :
                                     QWidget(par),m_invstId(investId),m_RbNumber(RbNumber),m_Title(Title),
									 m_Instrument(Instrument),m_downloadedFileList()
{
	initLayout();
	m_uiForm.invsttableWidget->verticalHeader()->setVisible(false);
	m_investws_sptr=ws2_sptr;
		
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
	connect(m_uiForm.selectallButton,SIGNAL(clicked()),this,SLOT(onSelectAllFiles()));
	connect(this,SIGNAL(executeDownload(std::vector<std::string>&)),
		parent()->parent(),SLOT(executeDownloadDataFiles(std::vector<std::string>&)));
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
		alg->setProperty("DataFiles",isDataFilesChecked());
		alg->setPropertyValue("OutputWorkspace","insvestigation");
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
	
	try
	{
		Poco::ActiveResult<bool> result(alg->executeAsync());
		while( !result.available() )
		{
			QCoreApplication::processEvents();
		}
	}
	catch(...)
    {     
		return ws_sptr;
    }
	if(AnalysisDataService::Instance().doesExist("insvestigation"))
	{
	ws_sptr = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>
		(AnalysisDataService::Instance().retrieve("insvestigation"));
	}
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
	catch (Mantid::Kernel::Exception::NotFoundError& e)
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
	}
	catch(...)
	{     
		return ws_sptr;
	}
	if(AnalysisDataService::Instance().doesExist("datasets"))
	{
		ws_sptr = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>
			(AnalysisDataService::Instance().retrieve("datasets"));
	}

	return ws_sptr;
}
/**This method executes the GetdatFile algorithm used for getting the data file location or downloading the data file
  *@param fileName -name of the file to download
  *@param fileLocs -archive location of the file
*/
bool ICatInvestigation::executeDownloadDataFiles(const std::vector<std::string>& fileNames,std::vector<std::string>& fileLocs)
{
	//
	QString algName("GetDataFile");
	const int version=-1;
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
		
		alg->setProperty("Filenames",fileNames);
		alg->setPropertyValue("InputWorkspace","insvestigation");
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
	
	try
	{
		Poco::ActiveResult<bool> result(alg->executeAsync());
		while( !result.available() )
		{
			QCoreApplication::processEvents();
		}
	}
	catch(...)
	{     
		return false;
	}
	try
	{
		fileLocs = alg->getProperty("FileLocations");
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
	if(!item)return;
	//QFont font;
	//font.setBold(true);
	//QString labelText;
	//std::stringstream totalCount;
	if(!item->text(0).compare("Default"))
	{			
		if(isDataFilesChecked())
		{
			if(!m_filteredws_sptr)
			{
				m_filteredws_sptr=executeGetdataFiles();
			}
			populateinvestigationWidget(m_filteredws_sptr,"DataFiles",true);
		}
		else
		{
			if(!m_datafilesws_sptr)
			{
				m_datafilesws_sptr = executeGetdataFiles();
			}
			if(!m_datafilesws_sptr) return;
			populateinvestigationWidget(m_datafilesws_sptr,"DataFiles",true);
		}
	}
	else if(!item->text(0).compare("DataSets"))
	{
		if(!m_datasetsws_sptr){
			m_datasetsws_sptr= executeGetdataSets();

		}
		if(!m_datasetsws_sptr)return;
		populateinvestigationWidget(m_datasetsws_sptr,"DataSets",false);
	}
	
}
/**This method populates the investigation table widget
  *@param bdataFiles -shared pointer to data files
  *@param ws_sptr - shared pointer to workspace
  *@param bEnable - flag to enable sorting
*/
void ICatInvestigation::populateinvestigationWidget(Mantid::API::ITableWorkspace_sptr dataws_sptr,const QString& type,bool bEnable)
{	
	if(!dataws_sptr){return;}

	Mantid::API::ITableWorkspace_sptr ws_sptr(dataws_sptr);

	//turn off sorting as per QT documentation
	m_uiForm.invsttableWidget->setSortingEnabled(false);
	
	//below for loop for clearing the table widget on each mouse click,otherwise rows will be appended.
	// table widget clear() method is clearing only the item text,not removing the rows,columns
	// so i'm using removeRow().When I removed the row from top of the table it was not working.

	for (int i=m_uiForm.invsttableWidget->rowCount()-1;i>=0;--i)
	{
		m_uiForm.invsttableWidget->removeRow(i);
	}
	m_uiForm.invsttableWidget->setRowCount(ws_sptr->rowCount());
	m_uiForm.invsttableWidget->setColumnCount(ws_sptr->columnCount());
	 
	for (int i=0;i<ws_sptr->rowCount();++i)
	{
		//m_uiForm.invsttableWidget->insertRow(i);
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
	m_uiForm.invsttableWidget->resizeColumnsToContents();
		
	QString labelText;
	std::stringstream totalCount;
	totalCount<<ws_sptr->rowCount();
	labelText="Data: "+QString::fromStdString(totalCount.str())+" "+type+" found";
	
	m_uiForm.invstlabel->clear();
	m_uiForm.invstlabel->setText(labelText);
	m_uiForm.invstlabel->setAlignment(Qt::AlignHCenter);
	m_uiForm.invstlabel->setFont(font);

	
	//if flag is set sort by first column
	if(bEnable)
	{   
		m_uiForm.invsttableWidget->setSortingEnabled(true);
		m_uiForm.invsttableWidget->sortByColumn(0,Qt::AscendingOrder);
	}
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
void ICatInvestigation::onSelectAllFiles()
{
	QItemSelectionModel* selectionModel = m_uiForm.invsttableWidget->selectionModel(); 
	QAbstractItemModel *model = m_uiForm.invsttableWidget->model();
	int rowCount = model->rowCount();
	int colCount = model->columnCount();
	
	QModelIndex topLeft = model->index(0,0,QModelIndex()); 
	QModelIndex bottomRight = model->index(rowCount-1,colCount-1,QModelIndex());
	
	QItemSelection selection(topLeft, bottomRight);
	selectionModel->select(selection, QItemSelectionModel::Select);

		
}
/// if data file checkbox selected
bool ICatInvestigation::isDataFilesChecked()
{
	return m_uiForm.dataFilescheckBox->isChecked();
}
/** This method gets the selected file name from the selected row in the table
  * @param fileNames - table widget item selected.
*/
void ICatInvestigation::getSelectedFileNames(std::vector<std::string>& fileNames)
{
	
	QItemSelectionModel *selmodel = m_uiForm.invsttableWidget->selectionModel();
	QModelIndexList  indexes=selmodel->selectedRows();
	QModelIndex index;
	QAbstractItemModel *model = m_uiForm.invsttableWidget->model();
	foreach(index, indexes) 
		{			
			QString text = model->data(index, Qt::DisplayRole).toString();
			fileNames.push_back(text.toStdString());

		}
}
/** download button clicked
*/
void  ICatInvestigation::onDownload()
{
	//get selected file name to download
    std::vector<std::string> fileNames;
	std::vector<std::string> fileLocs;
	//get selected file from the Mantid-ICat interface to download
	getSelectedFileNames(fileNames);
	if(fileNames.empty())
	{
		QString msg="No files are selected to download.Use 'Select All Files' button provided"
			"\n or mouse left button and shift/Ctrl key to select the files.";
		emit error(msg);
		return;
	}
	emit executeDownload(fileNames);
	
}
void ICatInvestigation::setfileLocations(const std::vector<std::string>& fileLocs)
{	
	m_downloadedFileList.assign(fileLocs.begin(),fileLocs.end());
	
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
	///get selected filename (raw,nexus,log) from table widget to load to mantid
	
    std::vector<std::string> sfileNames;
	getSelectedFileNames(sfileNames);
	if(sfileNames.empty())
	{
		QString msg="Select the files  to load using 'Select All files' button provided or "
			 "\n mouse left button and Shift/Ctrl Key and download the files using Download button.";
		emit error( msg);
	}

	// for loop for checking the selected file names is there in the downloaded files list.
	//users are suopposed to download first and then load
		
	std::vector<std::string>::const_iterator citr;
	for(citr=sfileNames.begin();citr!=sfileNames.end();++citr)
	{		
		std::string loadPath;
		if(isFileExistsInDownlodedList(*citr,loadPath ))
		{
			loadData(QString::fromStdString(loadPath));
		}
		else
		{
			emit error("The file "+ QString::fromStdString(*citr)+ " is not downloaded. Use the downlaod button provided to down load the file and then load." );
		}
		
	}

		
}

bool ICatInvestigation::isFileExistsInDownlodedList(const std::string& selectedFile,std::string& loadPath )
{
	std::basic_string <char>::size_type npos = -1;
	std::basic_string <char>::size_type index;
	std::string filenamePart;

	std::vector<std::string>::const_iterator cditr;
	for(cditr=m_downloadedFileList.begin();cditr!=m_downloadedFileList.end();++cditr)
	{
		// the selected file name UI contains only file names,but the downloaded filelist returned
		// by downlaod algorithm contains filename with full path
		// so below code extarcts the file name part and checks file exists in the downloaded list
		index=(*cditr).find_last_of("/");
		if(index!=npos)
		{
			filenamePart=(*cditr).substr(index+1,(*cditr).length()-index);
			//emit error("fileNamepart is "+QString::fromStdString(filenamePart) );
			QString temp=QString::fromStdString(filenamePart);
			QString temp1=QString::fromStdString(selectedFile);
			if(!temp.compare(temp1,Qt::CaseInsensitive))
			{	
				loadPath=(*cditr);
				return true;
			}
			
		}
		
	}
	return false;

}
/**This method loads the data file
  *@param filePath name of the file
  *@return  boolean
*/
bool ICatInvestigation::loadData( const QString& filePath)
{

	QString wsName;
	int index = filePath.lastIndexOf(".");
	int index1 = filePath.lastIndexOf("/");
	if(index!=-1 && index1!=-1)
	{
		wsName=filePath.mid(index1+1,index-index1-1);
	}

	if (isRawFile(filePath))
	{		
		if(!isLoadingControlled())
		{
			if(!executeLoadRaw(filePath,wsName))
			{
				return false;
			}

		}
		else
		{		
			emit loadRawAsynch(filePath,wsName);
		}
	}
	else if (isNexusFile(filePath))
	{

		if(!isLoadingControlled())
		{
			if(!executeLoadNexus(filePath,wsName))
			{
				return false;
			}
		}
		else
		{			
			emit loadNexusAsynch(filePath,wsName);
			
		}
	}
	else
	{
		emit error("ICat interface is not currently supporting the loading of log files ");
		return false;
	}
   return true;
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
	//return fileName.endsWith(".raw",Qt::CaseInsensitive);
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
	(!extn.compare("nxs",Qt::CaseInsensitive) )? bnxs=true : bnxs=false;
	
	return bnxs;
	//return fileName.endsWith(".nxs",Qt::CaseInsensitive);
}

/** This method executes loadRaw algorithm
  * @param fileName name of the raw file
*/
bool ICatInvestigation::executeLoadRaw(const QString& fileName,const QString& wsName)
{	
	return execute("LoadRaw",-1,fileName,wsName);
}

/** This method executes loadNexus algorithm
  * @param fileName name of the nexus file
*/
bool ICatInvestigation::executeLoadNexus(const QString& fileName,const QString& wsName)
{
	return execute("LoadNexus",-1,fileName,wsName);

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
		throw std::runtime_error("Error when loading the file"+ fileName.toStdString()); 
	}
	try
	{
		alg->setProperty("Filename",fileName.toStdString());
		alg->setPropertyValue("OutputWorkspace",wsName.toStdString());
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
		
	try
	{
		Poco::ActiveResult<bool> result(alg->executeAsync());
		while( !result.available() )
		{
			QCoreApplication::processEvents();
		}
		return (!result.failed());
	}
	catch(...)
    {   
	  return false;
    }
	

}
