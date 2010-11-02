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
#include <QDesktopServices>
#include <QUrl>


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
	//Tree on LHS of the investigation display	
	populateInvestigationTreeWidget();
	//an item selected from the LHS treewidget
	connect(m_uiForm.invsttreeWidget,SIGNAL(itemClicked (QTreeWidgetItem*, int )),this,SLOT(investigationClicked(QTreeWidgetItem*, int)));
	/// cancel clicked
	connect(m_uiForm.cancelButton,SIGNAL(clicked()),this,SLOT(onCancel()));

	connect(m_uiForm.invsttreeWidget,SIGNAL(itemExpanded(QTreeWidgetItem* )),this,SLOT(investigationWidgetItemExpanded(QTreeWidgetItem* )));
	connect(m_uiForm.invsttableWidget,SIGNAL(itemClicked (QTableWidgetItem* )),this,SLOT(tableItemSelected(QTableWidgetItem* )));
	
	//download button clciked
	connect(m_uiForm.downloadButton,SIGNAL(clicked()),this,SLOT(onDownload()));
	//load button clicked
	connect(m_uiForm.LoadButton,SIGNAL(clicked()),this,SLOT(onLoad()));
	/// send error mesages to logwindow
 	connect(this,SIGNAL(error(const QString&,int) ),parent()->parent(),SLOT(writeErrorLogWindow(const QString&)));
	//execute loadraw asynchronously
	connect(this,SIGNAL(loadRawAsynch(const QString&,const QString&)),parent()->parent(),SLOT(executeLoadRawAsynch(const QString&,const QString& )));
	//execute loadnexus asynchronously
	connect(this,SIGNAL(loadNexusAsynch(const QString&,const QString&)),parent()->parent(),SLOT(executeLoadNexusAsynch(const QString&,const QString& )));
	//select all file button clicked
	connect(m_uiForm.selectallButton,SIGNAL(clicked()),this,SLOT(onSelectAllFiles()));
	//download button clicked
	connect(this,SIGNAL(download(const std::vector<std::string>&,const std::vector<long long>&)),
		parent()->parent(),SLOT(executeDownloadDataFiles(const std::vector<std::string>&,const std::vector<long long>&)));
	///
	connect(this,SIGNAL(executeLoadAlgorithm(const QString&, const QString&, const QString&)),parent()->parent(),
		SLOT(executeloadAlgorithm(const QString&, const QString&, const QString&)));
	//helpbutton clicked
	connect(m_uiForm.helpButton,SIGNAL(clicked()),this,SLOT(helpButtonClicked()));
}

/// Set up the dialog layout
void ICatInvestigation::initLayout()
{
  m_uiForm.setupUi(this);
}

/// Poulate the tree widget on LHS
void ICatInvestigation::populateInvestigationTreeWidget(){
	

  m_uiForm.invsttreeWidget->setHeaderLabel("");
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
	item4->setToolTip(0,"View investigations datasets");

	qlist.clear();
	qlist.push_back("Default");
	QTreeWidgetItem *item5 = new QTreeWidgetItem(qlist);
	item4->addChild(item5);
	item5->setToolTip(0,"View default's files");

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
	QString algName("GetDataFiles");
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
		alg->setProperty("FilterLogFiles",isDataFilesChecked());
		alg->setPropertyValue("OutputWorkspace","datafiles");
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
	if(AnalysisDataService::Instance().doesExist("datafiles"))
	{
	ws_sptr = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>
		(AnalysisDataService::Instance().retrieve("datafiles"));
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

/** this method gets called when an item on the investigation tree widget is  clicked
  *@param item - selected item
  *@param int 
*/
void ICatInvestigation::investigationClicked(QTreeWidgetItem* item, int)
{
	if(!item)return;
	if(!item->text(0).compare("Default"))
	{			
		if(isDataFilesChecked())
		{
			if(!m_filteredws_sptr)
			{
        updateLabel("Loading data files...");
				m_filteredws_sptr=executeGetdataFiles();
			}
      
			populateinvestigationWidget(m_filteredws_sptr,"DataFiles",true);
		}
		else
		{
			if(!m_datafilesws_sptr)
			{
        updateLabel("Loading files...");
				m_datafilesws_sptr = executeGetdataFiles();
			}
			if(!m_datafilesws_sptr) return;
			populateinvestigationWidget(m_datafilesws_sptr,"DataFiles",true);
		}
	}
	else if(!item->text(0).compare("DataSets"))
	{
		if(!m_datasetsws_sptr){
      updateLabel("Loading datasets...");
			m_datasetsws_sptr= executeGetdataSets();

		}
		if(!m_datasetsws_sptr)return;
		populateinvestigationWidget(m_datasetsws_sptr,"DataSets",false);
	}
	
}
/**This method populates the investigation table widget
  *@param ws_sptr - shared pointer to workspace
  *@param  type - string used to identify datasets or data files
  *@param bEnable - flag to enable sorting
*/
void ICatInvestigation::populateinvestigationWidget(Mantid::API::ITableWorkspace_sptr dataws_sptr,const QString& type,bool bEnable)
{	
	if(!dataws_sptr){return;}

	Mantid::API::ITableWorkspace_sptr ws_sptr(dataws_sptr);
  if(!ws_sptr){return;}

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
  updateLabel(labelText);
			
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
/**update label on labled widget with the given text
*/
void ICatInvestigation::updateLabel(const QString& labelText)
{
  QFont font;
	font.setBold(true);
 	m_uiForm.invstlabel->clear();
	m_uiForm.invstlabel->setText(labelText);
	m_uiForm.invstlabel->setAlignment(Qt::AlignHCenter);
	m_uiForm.invstlabel->setFont(font);
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

/** This method returns the fileids of the given files.
@param fileNames - list of filenames
@param fileIds - reference to a vector of fileIds
*/
void ICatInvestigation::getFileIds(const std::vector<std::string> &fileNames, std::vector<long long >&fileIds)
{
	ITableWorkspace_sptr ws_sptr;

	if(AnalysisDataService::Instance().doesExist("datafiles"))
	{
		ws_sptr = boost::dynamic_pointer_cast<Mantid::API::ITableWorkspace>
			(AnalysisDataService::Instance().retrieve("datafiles"));
	}
	long long fileId=0;
	const int col=0;
	int row=0;
	std::vector<std::string>::const_iterator citr;
	for(citr=fileNames.begin();citr!=fileNames.end();++citr)
	{
		try
		{
		ws_sptr->find(*citr,row,col);
		fileId=ws_sptr->cell<int64_t>(row,col+2);//3rd column is the file id.
		}
		catch(std::range_error&)
		{
			throw std::runtime_error("selected file "+*citr+" not exists in the ICat search results workspace");
		}
		catch(std::out_of_range&)
		{
			throw std::runtime_error("selected file "+*citr+" not exists in the ithe ICat search results workspace");;
		}
		catch(std::runtime_error&)
		{
			throw std::runtime_error("selected file "+*citr+" not exists in the the ICat search results workspace");;
		}

		fileIds.push_back(fileId);
		++row;
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
		QString msg="No files are selected to download.Click on any file to select it" 
			"\n or Use the 'Select All Files' button provided or mouse left button shift/Ctrl key to select the files.";
		emit error(msg);
		return;
	}

	std::vector<long long >fileIds;
	//get the file ids for the given file names.
	getFileIds(fileNames,fileIds);
	
	emit download(fileNames,fileIds);
	
}
/** This slot sets the file locations returned by download algorithm to downloaded filelist
*/
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
	///get selected filename (raw,nexus,log) from table widget 
	std::vector<std::string> sfileNames;
	getSelectedFileNames(sfileNames);
	if(sfileNames.empty())
	{
		QString msg="Files must be downloaded before trying to load.";
		emit error( msg);
	}

	// before loading check the file is downloaded
	std::vector<std::string>::const_iterator citr;
	for(citr=sfileNames.begin();citr!=sfileNames.end();++citr)
	{		
		std::string loadPath;
		if(isFileExistsInDownloadedList(*citr,loadPath ))
		{
			//if the file is downloaded ,then load to mantid and create workspace
			loadData(QString::fromStdString(loadPath));
		}
		else
		{
			emit error("The file "+ QString::fromStdString(*citr)+ " is not downloaded. Use the downlaod button provided to down load the file and then load." );
		}
		
	}

		
}

bool ICatInvestigation::isFileExistsInDownloadedList(const std::string& selectedFile,std::string& loadPath )
{
	std::basic_string <char>::size_type npos = -1;
	std::basic_string <char>::size_type index;
	std::string filenamePart;

	std::vector<std::string>::const_iterator cditr;
	for(cditr=m_downloadedFileList.begin();cditr!=m_downloadedFileList.end();++cditr)
	{
		// the selected file name from UI contains only file names,but the downloaded filelist returned
		// by downlaod algorithm contains filename with full path
		// below code extarcts the file name part and checks file exists in the downloaded list
		index=(*cditr).find_last_of("/");
		if(index!=npos)
		{
			filenamePart=(*cditr).substr(index+1,(*cditr).length()-index);
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
*/
void ICatInvestigation::loadData( const QString& filePath)
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
			executeLoadRaw(filePath,wsName);
		
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
			executeLoadNexus(filePath,wsName);
		}
		else
		{			
			emit loadNexusAsynch(filePath,wsName);
			
		}
	}
	else
	{
		emit error("ICat interface is not supporting the loading of log files.",1);
		
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
  *@return  true if it's nexus file
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
	
}

/** This method executes loadRaw algorithm
  * @param fileName name of the raw file
  * @param wsName name of the workspace to store the data
*/
void ICatInvestigation::executeLoadRaw(const QString& fileName,const QString& wsName)
{	
	emit executeLoadAlgorithm("LoadRaw",fileName,wsName);
}

/** This method executes loadNexus algorithm
  * @param fileName name of the nexus file
  * @param wsName name of the workspace to store the data
*/
void ICatInvestigation::executeLoadNexus(const QString& fileName,const QString& wsName)
{
	emit executeLoadAlgorithm("LoadNexus",fileName,wsName);
}
/**This method executes loadraw/loadnexus algorithm
  *@param algName - algoritm name
  *@param version -algorithm version
  *@param fileName - name of the file to load
  *@param wsName name of the workspace to store the data
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

//handler for helpbutton
void ICatInvestigation::helpButtonClicked()
{
	QDesktopServices::openUrl(QUrl("http://www.mantidproject.org/Investigation"));

}
