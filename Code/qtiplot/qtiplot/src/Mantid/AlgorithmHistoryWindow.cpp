#include "AlgorithmHistoryWindow.h"
#include "QMessageBox"

AlgExecSummaryGrpBox::AlgExecSummaryGrpBox(QString title,QWidget*w):QGroupBox(title,w),
m_execDurationlabel(NULL),m_execDurationEdit(NULL),m_Datelabel(NULL),m_execDateTimeEdit(NULL),
m_algexecDuration(NULL)
{
	m_execDurationEdit=new QLineEdit("",this);
	if(m_execDurationEdit){
		m_execDurationEdit->setReadOnly(1);
		int charWidth = m_execDurationEdit->fontMetrics().maxWidth();
		m_execDurationEdit->setMaximumWidth(charWidth*4);
		m_execDurationEdit->setMaximumHeight(100);
	}
	m_execDurationlabel=new QLabel("&Duration",this,0);
	if(m_execDurationlabel)m_execDurationlabel->setBuddy(m_execDurationEdit);
		
	QDateTime datetime(QDate(0,0,0), QTime(0,0,0),Qt::LocalTime );
	m_execDateTimeEdit=new QLineEdit("",this);
	if(m_execDateTimeEdit){
		m_execDateTimeEdit->setReadOnly(1);
		int charWidth = m_execDateTimeEdit->fontMetrics().maxWidth();
		m_execDateTimeEdit->setMaximumWidth(charWidth*6);
		m_execDateTimeEdit->setGeometry(5,310,100,100);
	}
	m_Datelabel=new QLabel("&Date",this,0);
	if(m_Datelabel)m_Datelabel->setBuddy(m_execDateTimeEdit);
		
	QVBoxLayout *vbox = new QVBoxLayout;
	if(vbox)
	{	vbox->addWidget(m_execDurationlabel);
		vbox->addWidget(m_execDurationEdit);
		vbox->addWidget(m_Datelabel);
		vbox->addWidget(m_execDateTimeEdit);
		vbox->addStretch(1);
		setLayout(vbox);
	}
	setGeometry (5,210,210,200);
}
AlgExecSummaryGrpBox::~AlgExecSummaryGrpBox()
{
	if(m_execDurationlabel){
		delete m_execDurationlabel;
		m_execDurationlabel=NULL;
	}
	if(m_execDurationEdit){
		delete m_execDurationEdit;
		m_execDurationEdit=NULL;
	}
	if(m_Datelabel){
		delete m_Datelabel;
		m_Datelabel=NULL;
	}
	if(m_Datelabel){
		delete m_Datelabel;
		m_Datelabel=NULL;
	}
	if(m_execDateTimeEdit){
		delete m_execDateTimeEdit;
		m_execDateTimeEdit=NULL;
	}
}
void AlgExecSummaryGrpBox::setData(const double execDuration,const Mantid::Kernel::dateAndTime execDate)
{
	QString dur("");
	dur.setNum(execDuration,'g',6);
	dur+=" seconds";
	QLineEdit* execDurationEdit=getAlgExecDurationCtrl();
	if(execDurationEdit)execDurationEdit->setText(dur);
	
	std::tm*  t;
	t=localtime(&execDate);
	QTime qt(t->tm_hour,t->tm_min,t->tm_sec);
	QDate qd(t->tm_year+1900,t->tm_mon+1,t->tm_mday);
	QDateTime datetime(qd,qt,Qt::LocalTime );
	
	QString str("");
	str=datetime.toString("dd/MM/yyyy hh:mm:ss");
	
	QLineEdit* datetimeEdit=getAlgExecDateCtrl();
	if(datetimeEdit)datetimeEdit->setText(str);

}
AlgEnvHistoryGrpBox::AlgEnvHistoryGrpBox(QString title,QWidget*w):QGroupBox(title,w),
m_osNameLabel(NULL),m_osNameEdit(NULL),m_osVersionLabel(NULL),m_osVersionEdit(NULL),
m_frmworkVersionLabel(NULL),m_frmwkVersnEdit(NULL)
{
	//OS Name Label & Edit Box
	int charWidth=0;
	m_osNameEdit=new QLineEdit("",this);
	if(m_osNameEdit)
	{
		m_osNameEdit->setReadOnly(1);
		charWidth = m_osNameEdit->fontMetrics().maxWidth();
		m_osNameEdit->setMaximumWidth(charWidth*4);
		m_osNameEdit->setMaximumHeight(100);
	}
	m_osNameLabel=new QLabel("&OSName",this,0);
	if(m_osNameLabel)m_osNameLabel->setBuddy(m_osNameEdit);

	//OS Version Label & Edit Box
	m_osVersionEdit=new QLineEdit("",this);
	if(m_osVersionEdit)
	{
		m_osVersionEdit->setReadOnly(1);
		charWidth = m_osVersionEdit->fontMetrics().maxWidth();
		m_osVersionEdit->setMaximumWidth(charWidth*10);
		m_osVersionEdit->setMaximumHeight(100);
		m_osVersionLabel=new QLabel("&OSVersion",this,0);
	}
	if(m_osVersionLabel)
		m_osVersionLabel->setBuddy(m_osVersionEdit);

	//Mantid FRamework Version Label & Edit Box
	m_frmwkVersnEdit=new QLineEdit("",this);
	if(m_frmwkVersnEdit)
	{
		m_frmwkVersnEdit->setReadOnly(1);
		charWidth = m_frmwkVersnEdit->fontMetrics().maxWidth();
		m_frmwkVersnEdit->setMaximumWidth(charWidth*4);
		m_frmwkVersnEdit->setMaximumHeight(100);
	}
	m_frmworkVersionLabel=new QLabel("&FrameWorkVersion",this,0);
	if(m_frmworkVersionLabel)
	m_frmworkVersionLabel->setBuddy(m_frmwkVersnEdit);
	
	QVBoxLayout *vbox = new QVBoxLayout;
	if(vbox)
	{
		vbox->addWidget(m_osNameLabel);
		vbox->addWidget(m_osNameEdit);
		vbox->addWidget(m_osVersionLabel);
		vbox->addWidget(m_osVersionEdit);
		vbox->addWidget(m_frmworkVersionLabel);
		vbox->addWidget(m_frmwkVersnEdit);
		vbox->addStretch(1);
		setLayout(vbox);
	}
	setGeometry (220,210,375,200);
}
AlgEnvHistoryGrpBox::~AlgEnvHistoryGrpBox()
{
	if(m_osNameLabel){delete m_osNameLabel;m_osNameLabel=NULL;}
	if(m_osNameEdit){delete m_osNameEdit;m_osNameEdit=NULL;}
	if(m_osNameEdit){delete m_osNameEdit;m_osNameEdit=NULL;}
	if(m_osVersionLabel){delete m_osVersionLabel;m_osVersionLabel=NULL;}
	if(m_osVersionEdit){delete m_osVersionEdit;m_osVersionEdit=NULL;}
	if(m_frmworkVersionLabel){delete m_frmworkVersionLabel;m_frmworkVersionLabel=NULL;}
	if(m_frmwkVersnEdit){ delete m_frmwkVersnEdit;m_frmwkVersnEdit=NULL;}
}
AlgHistScriptButton::AlgHistScriptButton(QString title,QWidget* w):QPushButton(title,w)
{
	setGeometry (500,420,100,30);
	connect(this,SIGNAL(clicked()),w,SLOT(generateScript()));
}
AlgHistScriptButton::~AlgHistScriptButton()
{
}

AlgorithmHistoryWindow::AlgorithmHistoryWindow(ApplicationWindow *w,const std::vector<AlgorithmHistory> &algHist,const EnvironmentHistory& envHist):
MantidQtDialog(w),m_algHist(algHist),m_histPropWindow(NULL),m_execSumGrpBox(NULL),m_envHistGrpBox(NULL),m_algName(""),m_nVersion(0)
{
    setWindowTitle(tr("Algorithm History"));
	setMinimumHeight(470);
    setMinimumWidth(610);
	setGeometry(50,150,560,420); 
	
	//Create a tree widget to display the algorithm names in the workspacehistory
	m_Historytree = new AlgHistoryTreeWidget(this);
	if(m_Historytree)
	{	m_Historytree->setHeaderLabel("Algorithms");
		m_Historytree->setGeometry (5,5,210,200);
	}
	//Populate the History Tree widget
	populateAlgHistoryTreeWidget();
   //Create a GroupBox to display exec date,duration
	if(!m_execSumGrpBox)m_execSumGrpBox=createExecSummaryGrpBox();
	//Create a Groupbox to display environment details
	if(!m_envHistGrpBox)m_envHistGrpBox=createEnvHistGrpBox(envHist);

	std::string algrithmName=(*m_algHist.rbegin( )).name();
	int version=(*m_algHist.rbegin( )).version();
	//create a tree widget to dispaly history properties
	if(!m_histPropWindow)
	{m_histPropWindow=createAlgHistoryPropWindow(algrithmName.c_str(),version);
	}
	connect(m_Historytree,SIGNAL(updateAlgorithmHistoryWindow(QString, int)),this,SLOT(updateAll(QString,int)));
	m_scriptButton = CreateScriptButton();
}
AlgorithmHistoryWindow::~AlgorithmHistoryWindow()
{	
	if(m_Historytree){delete m_Historytree;m_Historytree=NULL;}
	if(m_histPropWindow){ delete m_histPropWindow;m_histPropWindow=NULL;}
	if(m_execSumGrpBox){delete m_execSumGrpBox;m_execSumGrpBox=NULL;}
	if(m_envHistGrpBox){delete m_envHistGrpBox;m_envHistGrpBox=NULL;}
	if(m_scriptButton){delete m_scriptButton;m_scriptButton=NULL;}
}

QPushButton * AlgorithmHistoryWindow::CreateScriptButton()
{	return  (new AlgHistScriptButton("Generate Script",this));//QPushButton("Script",this);
}
AlgExecSummaryGrpBox* AlgorithmHistoryWindow::createExecSummaryGrpBox()
{	
	AlgExecSummaryGrpBox *pgrpBox=new AlgExecSummaryGrpBox("Execution Summary",this);
	if(pgrpBox)
	{
		//iterating through algorithm history to display exec duration,date
		//last executed algorithm exec duration,date will be displayed in gruopbox
		for (vector <AlgorithmHistory>::const_iterator algIter= m_algHist.begin( );
			algIter != m_algHist.end( ); algIter++ )
		{
			double duration=0;
			duration=(*algIter).executionDuration();
			Mantid::Kernel::dateAndTime date=(*algIter).executionDate();
			pgrpBox->setData(duration,date);
		}
		return pgrpBox;
	}
	else{
		QMessageBox::critical(this,"Mantid","Invalid Pointer");
		return 0;
	}
	
}
AlgEnvHistoryGrpBox* AlgorithmHistoryWindow::createEnvHistGrpBox(const EnvironmentHistory& envHist)
{	
	AlgEnvHistoryGrpBox * pEnvGrpBox=new  AlgEnvHistoryGrpBox("Environment History",this);
	if(pEnvGrpBox){
		pEnvGrpBox->fillEnvHistoryGroupBox(envHist);
		return pEnvGrpBox;
	}
	else{
		QMessageBox::critical(this,"Mantid","Invalid Pointer");
		return 0;
	}
	
}
AlgHistoryProperties* AlgorithmHistoryWindow::createAlgHistoryPropWindow(const QString& algName,int version)
{	
	vector<PropertyHistory> histProp;
	for (vector <AlgorithmHistory>::reverse_iterator ralgHistory_Iter=m_algHist.rbegin( );
		ralgHistory_Iter!=m_algHist.rend();ralgHistory_Iter++)
	{
		histProp=(*ralgHistory_Iter).getProperties();
		std::string name=(*ralgHistory_Iter).name();
		int nVer=(*ralgHistory_Iter).version();
		if((algName==name.c_str())&& (nVer==version))
			break;
		       
	}//end of algorithm history for loop
	//AlgHistoryProperties * phistPropWindow=new AlgHistoryProperties(this,m_algHist);
	if(histProp.empty()){
		QMessageBox::critical(this,"Mantid","Properties not set");
		return 0;
	}
	AlgHistoryProperties * phistPropWindow=new AlgHistoryProperties(this,histProp);
	if(phistPropWindow){
		phistPropWindow->displayAlgHistoryProperties();
		return phistPropWindow;
	}
	else{QMessageBox::critical(this,"Mantid","Invalid Pointer");
		return 0;
	}
}
void AlgorithmHistoryWindow::handleException( const exception& e )
{
	QMessageBox::critical(0,"Mantid-Error",QString::fromStdString(e.what()));
}
void AlgorithmHistoryWindow::generateScript()
{	
	QString algName("");
	string algParam("");
	QString script("");
	int nVersion=-1;
	vector<Property*> algPropUnmngd;
	vector<Property*>::const_iterator itUmnngd;
	vector<PropertyHistory>algHistProp;
	if(m_Historytree)
	{
		algName=getAlgorithmName();
		nVersion=getAlgorithmVersion();
	}
	//creating an unmanaged instance of the selected algorithm
	//this is bcoz algorith history is giving dynamically generated workspaces for some 
	//algorithms like LoadRaw.But python script for LoadRaw has only one output workspace parameter
	//To eliminate the dynamically generated parameters unmanged instances created and compared with it.
	IAlgorithm_sptr  ialg_Sptr= AlgorithmManager::Instance().createUnmanaged(algName.toStdString(),nVersion);
	if(ialg_Sptr)
	{	ialg_Sptr->initialize();
		algPropUnmngd= ialg_Sptr->getProperties();
		itUmnngd=algPropUnmngd.begin();
	}
	//getting the properties for the selected algorithm from the algorithhistory
	for (vector <AlgorithmHistory>::reverse_iterator ralgHistIter=m_algHist.rbegin( );
		ralgHistIter!=m_algHist.rend();ralgHistIter++)
	{
		algHistProp=(*ralgHistIter).getProperties();
		string name=(*ralgHistIter).name();
		if(name==algName.toStdString()&&nVersion==(*ralgHistIter).version())
			break;  
	}//end of algorithm history for loop

	//iterating through the properties
	for (vector<PropertyHistory>::const_iterator propIter = algHistProp.begin();
		propIter != algHistProp.end(); ++propIter )
	{
		string name= (*propIter).name();
		string value=(*propIter).value();
		bool bdefault=(*propIter).isDefault();
		//if it's not a default property  add it to 
		//algorithm parameters to form the script
		if(!bdefault)
		{
			//if the property name obtained by unmanaged instance of the algorithm
			//is same as the algorithm history property add it to script string
			if (name==(*itUmnngd)->name())
			{
				string sanitisedname=sanitizePropertyName(name);
				algParam+=sanitisedname;
				algParam+="=\"";
				algParam+=value;
				algParam+="\",";
			}
		}
		//int direction=(*propIter).direction();
		itUmnngd++;
		if(itUmnngd==algPropUnmngd.end())
			break;
	}
	//erasing the last "," from the parameter list
	//as concatenation is done in loop last "," has to be erased
	int nIndex=algParam.find_last_of(",");
	if(string::npos!=nIndex)
		algParam=algParam.erase(nIndex);
	//script string
	script=algName+"("+algParam.c_str()+")";
	writeToScriptFile(script);
}
void AlgorithmHistoryWindow::writeToScriptFile(const QString& script)
{
	QString scriptPath("");
	QString prevdir=MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();
	//default script directory
	if(prevdir.isEmpty())
	{scriptPath="C\\Mantid\\Code\\Mantid\\PythonAPI\\Scripts";
	}
	else{scriptPath=prevdir;}//last opened path
	//setting the script filename in dialog to 
	//selected algorithm name&version  by default
	QString algName=getAlgorithmName();
	int nVer=getAlgorithmVersion();
	algName+="v";
	algName+=QString::number(nVer,10);
	algName=algName+".py";
	scriptPath+="\\";
	scriptPath+=algName;

	QString filePath=QFileDialog::getSaveFileName(this,tr("Save Script As "),scriptPath,tr("Script files (*.py)"));
	QFile scriptfile(filePath);
	if (!scriptfile.open(QIODevice::WriteOnly | QIODevice::Text))
         return;
	QTextStream out(&scriptfile);
	out<<"######################################################################\n";
	out<<"#Python Script Generated by Algorithm History Display \n";
	out<<"######################################################################\n";
	out<<script<<"\n";
	scriptfile.close();
	//save the file path
	MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(QFileInfo(filePath).absoluteDir().path());
}
string AlgorithmHistoryWindow::sanitizePropertyName(const string & name)
{
	string arg;
	string::const_iterator sIter = name.begin();
	string::const_iterator sEnd = name.end();
	for( ; sIter != sEnd; ++sIter )
	{
		int letter = (int)(*sIter);
		if( (letter >= 48 && letter <= 57) || (letter >= 97 && letter <= 122) ||
			(letter >= 65 && letter <= 90) )
		{
			arg.push_back(*sIter);
		}
	}
	return arg;
}
void AlgorithmHistoryWindow::setAlgorithmName(const QString& algName)
{
	m_algName=algName;
}
const QString &AlgorithmHistoryWindow::getAlgorithmName() const
{	return m_algName;
}
void AlgorithmHistoryWindow::setAlgorithmVersion(const int& version) 
{
	m_nVersion=version;
}
const int& AlgorithmHistoryWindow::getAlgorithmVersion()const
{
	return m_nVersion;
}
void AlgorithmHistoryWindow::populateAlgHistoryTreeWidget()
{	
	vector <AlgorithmHistory>::reverse_iterator ralgHistory_Iter=m_algHist.rbegin( );
	string algrithmName;
	algrithmName=(*ralgHistory_Iter).name();
	QString algName=algrithmName.c_str();
	int nAlgVersion=(*ralgHistory_Iter).version();
	concatVersionwithName(algName,nAlgVersion);
	
	QTreeWidgetItem * item= new	QTreeWidgetItem(QStringList(algName),QTreeWidgetItem::Type);
	if(m_Historytree)m_Historytree->addTopLevelItem(item);
	
	ralgHistory_Iter++;
	for ( ; ralgHistory_Iter != m_algHist.rend( ) ; ralgHistory_Iter++ )
	{
		algrithmName=(*ralgHistory_Iter).name();
		nAlgVersion=(*ralgHistory_Iter).version();
		algName=algrithmName.c_str();
		concatVersionwithName(algName,nAlgVersion);
		QTreeWidgetItem * subitem= new	QTreeWidgetItem(QStringList(algName));
		if(item)item->addChild(subitem);
	}
}
void AlgorithmHistoryWindow::concatVersionwithName( QString& algName,const int version)
{
	algName= algName+" v.";
	QString algVersion=QString::number(version,10);
	algName+=algVersion;
}

void AlgEnvHistoryGrpBox::fillEnvHistoryGroupBox(const EnvironmentHistory& envHistory)
{
	string osname=envHistory.osName();
	string osversion=envHistory.osVersion();
	string userName=envHistory.userName();
	string frwkversn=envHistory.frameworkVersion();
	
	QLineEdit* osNameEdit=getosNameEdit();
	if(osNameEdit)osNameEdit->setText(osname.c_str());
	
	QLineEdit* osVersionEdit=getosVersionEdit();
	if(osVersionEdit)osVersionEdit->setText(osversion.c_str());

	QLineEdit* frmwkVersnEdit=getfrmworkVersionEdit();
	if(frmwkVersnEdit)frmwkVersnEdit->setText(frwkversn.c_str());

}
void AlgorithmHistoryWindow::updateAll(QString algName,int version)
{	
	updateAlgHistoryProperties(algName,version);
	updateExecSummaryGrpBox(algName,version);
	setAlgorithmName(algName);
	setAlgorithmVersion(version);
	
}
void AlgorithmHistoryWindow::updateAlgHistoryProperties(QString algName,int version)
{
	vector<PropertyHistory> histProp;
	for (vector <AlgorithmHistory>::reverse_iterator ralgHistory_Iter=m_algHist.rbegin( );
		ralgHistory_Iter!=m_algHist.rend();ralgHistory_Iter++)
	{
		histProp=(*ralgHistory_Iter).getProperties();
		std::string name=(*ralgHistory_Iter).name();
		int nVer=(*ralgHistory_Iter).version();
		if((algName==name.c_str())&& (nVer==version))
			break;
		       
	}//end of algorithm history for loop
	if(m_histPropWindow)
	{  m_histPropWindow->setAlgProperties(histProp);
		m_histPropWindow->clearData();
		m_histPropWindow->displayAlgHistoryProperties();
	}
}
void AlgorithmHistoryWindow::updateExecSummaryGrpBox(const QString& algName,const int & version)
{
	vector <AlgorithmHistory>::reverse_iterator ralgHistory_Iter;
	for (ralgHistory_Iter=m_algHist.rbegin( );
		ralgHistory_Iter!=m_algHist.rend();ralgHistory_Iter++)
	{
		std::string name=(*ralgHistory_Iter).name();
		if(algName==name.c_str()&& version==(*ralgHistory_Iter).version())
			break;
	}//end of algorithm history for loop
	double duration=0;
	duration=(*ralgHistory_Iter).executionDuration();
	Mantid::Kernel::dateAndTime date=(*ralgHistory_Iter).executionDate();
	if(m_execSumGrpBox)m_execSumGrpBox->setData(duration,date);
}

AlgHistoryProperties::AlgHistoryProperties(QWidget*w,const vector<PropertyHistory>& propHist):
m_Histprop(propHist)
{
	QStringList hList;
	hList<<"Name"<<"Value"<<"Default?:"<<"Direction"<<"";
	m_histpropTree = new  QTreeWidget(w);
	if(m_histpropTree)
	{	m_histpropTree->setColumnCount(5);
		m_histpropTree->setSelectionMode(QAbstractItemView::NoSelection);
		m_histpropTree->setHeaderLabels(hList);
		m_histpropTree->setGeometry (213,5,385,200);
	}
}
void AlgHistoryProperties::clearData()
{
	if(m_histpropTree)
	{   m_histpropTree->clear();
		int ntopcount=m_histpropTree->topLevelItemCount() ;
		while(ntopcount--)
		{m_histpropTree->topLevelItem(ntopcount);
		}
	}
}
void AlgHistoryProperties::setAlgProperties( const vector<PropertyHistory>& histProp)
{
	m_Histprop.assign(histProp.begin(),histProp.end());
}
const vector<PropertyHistory>& AlgHistoryProperties:: getAlgProperties()
{
	return m_Histprop;
}
void AlgHistoryProperties::displayAlgHistoryProperties()
{
	QStringList propList;
	string sProperty;
	bool bisDefault;
	int nDirection=0;
	for ( vector<Mantid::Kernel::PropertyHistory>::const_iterator pIter = m_Histprop.begin();
		pIter != m_Histprop.end(); ++pIter )
	{
		sProperty=(*pIter).name();
		propList.append(sProperty.c_str());
		sProperty=(*pIter).value();
		propList.append(sProperty.c_str());
		
		bisDefault=(*pIter).isDefault();
		bisDefault? (sProperty="Yes"):(sProperty="No");
		
		propList.append(sProperty.c_str());
		nDirection=(*pIter).direction();
		switch(nDirection)
		{
		case 0:{sProperty="Input";break;}
		case 1:{sProperty="Output";break;}
		case 2:{sProperty="InOut";break;}
		default:sProperty="N/A";
		}
		propList.append(sProperty.c_str());
		QTreeWidgetItem * item= new	QTreeWidgetItem(propList);
		if(m_histpropTree)m_histpropTree->addTopLevelItem(item);
		propList.clear();

	}// end of properties for loop
    
}
void AlgHistoryTreeWidget::treeSelectionChanged()
{	
	QString algName("");int nVersion=-1;
	getSelectedAlgorithmName(algName,nVersion);
	emit updateAlgorithmHistoryWindow(algName,nVersion);
}
void AlgHistoryTreeWidget::setAlgorithmName(const QString& algName)
{
	m_algName=algName;
}
const QString AlgHistoryTreeWidget::getAlgorithmName()
{	return m_algName;
}
void AlgHistoryTreeWidget::setAlgorithmVersion(const int& version)
{
	m_nVersion=version;
}
const int& AlgHistoryTreeWidget::getAlgorithmVersion()
{
	return m_nVersion;
}
void AlgHistoryTreeWidget::getSelectedAlgorithmName(QString& algName,int & version)
{	
	QList<QTreeWidgetItem*> items = selectedItems();
	if( !items.empty() )
    {QTreeWidgetItem *item = items[0];
	  if(item)
	  {	QString  str=item->text(0);
		int nIndex=str.indexOf(".",0,Qt::CaseSensitive );
		algName=str.left(nIndex-2);
		version=str.right(str.length()-nIndex-1).toInt();
	  }
	}
 }
void AlgHistoryTreeWidget::mouseDoubleClickEvent(QMouseEvent *e)
{		
	QString algName;int nVersion=-1;
	getSelectedAlgorithmName(algName,nVersion);
	emit updateAlgorithmHistoryWindow(algName,nVersion);
	QTreeWidget::mouseDoubleClickEvent(e);
}

