#include "AlgorithmHistoryWindow.h"

AlgExecSummaryGrpBox::AlgExecSummaryGrpBox(QString title,QWidget*w):QGroupBox(title,w),
m_execDurationlabel(NULL),m_execDurationEdit(NULL),m_Datelabel(NULL),m_execDateTimeEdit(NULL),
m_algexecDuration(NULL)
{
	m_execDurationlabel=new QLabel("&Duration",this,0);
	m_execDurationEdit=new QLineEdit("",this);
	m_execDurationEdit->setReadOnly(1);
	int charWidth = m_execDurationEdit->fontMetrics().maxWidth();
	m_execDurationEdit->setMaximumWidth(charWidth*4);
	m_execDurationEdit->setMaximumHeight(100);
	m_execDurationlabel->setBuddy(m_execDurationEdit);

	m_Datelabel=new QLabel("&Date",this,0);
	QDateTime datetime(QDate(0,0,0), QTime(0,0,0),Qt::LocalTime );
	//m_execDateTimeEdit=new QDateTimeEdit(datetime,this);
	m_execDateTimeEdit=new QLineEdit("",this);
	m_execDateTimeEdit->setReadOnly(1);
	charWidth = m_execDateTimeEdit->fontMetrics().maxWidth();
	m_execDateTimeEdit->setMaximumWidth(charWidth*6);
	m_execDateTimeEdit->setGeometry(5,310,100,100); 
	m_Datelabel->setBuddy(m_execDateTimeEdit);
		
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
void AlgExecSummaryGrpBox::setData(const double execDuration,const dateAndTime execDate)
{
	QString dur;
	dur.setNum(execDuration,'g',6);
	dur+=" seconds";
	QLineEdit* execDurationEdit=getAlgExecDuration();
	execDurationEdit->setText(dur);

	struct tm  t;errno_t err;
	err = _localtime64_s( &t, &execDate );
	QTime qt(t.tm_hour,t.tm_min,t.tm_sec);
	QDate qd(t.tm_year+1900,t.tm_mon+1,t.tm_mday);
	QDateTime datetime(qd,qt,Qt::LocalTime );
	QString str;
	str=datetime.toString("dd/MM/yyyy hh:mm:ss");
	/*QMessageBox msgbox;
	msgbox.setText(str);
	msgbox.exec();*/
	QLineEdit* datetimeEdit=getAlgExecDate();
	datetimeEdit->setText(str);

}
AlgEnvHistoryGrpBox::AlgEnvHistoryGrpBox(QString title,QWidget*w):QGroupBox(title,w),
m_osNameLabel(NULL),m_osNameEdit(NULL),m_osVersionLabel(NULL),m_osVersionEdit(NULL),
m_frmworkVersionLabel(NULL),m_frmworkVersionEdit(NULL)
{
	//OS Name Label & Edit Box
	m_osNameLabel=new QLabel("&OSName",this,0);
	m_osNameEdit=new QLineEdit("",this);
	m_osNameEdit->setReadOnly(1);
	int charWidth = m_osNameEdit->fontMetrics().maxWidth();
	m_osNameEdit->setMaximumWidth(charWidth*4);
	m_osNameEdit->setMaximumHeight(100);
	m_osNameLabel->setBuddy(m_osNameEdit);

	//OS Version Label & Edit Box
	m_osVersionLabel=new QLabel("&OSVersion",this,0);
	m_osVersionEdit=new QLineEdit("",this);
	m_osVersionEdit->setReadOnly(1);
	charWidth = m_osVersionEdit->fontMetrics().maxWidth();
	m_osVersionEdit->setMaximumWidth(charWidth*10);
	m_osVersionEdit->setMaximumHeight(100);
	m_osVersionLabel->setBuddy(m_osVersionEdit);

	//Mantid FRamework Version Label & Edit Box
	m_frmworkVersionLabel=new QLabel("&FrameWorkVersion",this,0);
	m_frmworkVersionEdit=new QLineEdit("",this);
	m_frmworkVersionEdit->setReadOnly(1);
	charWidth = m_frmworkVersionEdit->fontMetrics().maxWidth();
	m_frmworkVersionEdit->setMaximumWidth(charWidth*4);
	m_frmworkVersionEdit->setMaximumHeight(100);
	m_frmworkVersionLabel->setBuddy(m_frmworkVersionEdit);
	
	QVBoxLayout *vbox = new QVBoxLayout;
	if(vbox)
	{
		vbox->addWidget(m_osNameLabel);
		vbox->addWidget(m_osNameEdit);
		vbox->addWidget(m_osVersionLabel);
		vbox->addWidget(m_osVersionEdit);
		vbox->addWidget(m_frmworkVersionLabel);
		vbox->addWidget(m_frmworkVersionEdit);
		vbox->addStretch(1);
		setLayout(vbox);
	}
	setGeometry (220,210,370,200);
}

AlgorithmHistoryWindow::AlgorithmHistoryWindow(ApplicationWindow *w,const std::vector<AlgorithmHistory> &algHist,const EnvironmentHistory& envHist):
QDialog(w),m_algHist(algHist),m_histPropWindow(NULL),m_execSumGrpBox(NULL),m_envHistGrpBox(NULL)
{
    setWindowTitle(tr("Algorithm History"));
	setMinimumHeight(550);
    setMinimumWidth(600);
	setGeometry(50,150,600,550); 
	
	//QFrame *f = new QFrame(this);
   // setCentralWidget(f);
	//m_tree = new AlgHistoryTreeWidget(f);
	m_Historytree = new AlgHistoryTreeWidget(this);
	if(m_Historytree)
	{	m_Historytree->setHeaderLabel("Algorithms");
		m_Historytree->show();
		m_Historytree->setGeometry (5,5,210,200);
	}
	
	populateAlgHistoryTreeWidget();

	if(!m_execSumGrpBox)m_execSumGrpBox=CreateExecSummaryGroupBox();
	  fillExecutionSummaryGroupBox();
	if(!m_envHistGrpBox)m_envHistGrpBox=CreateEnvHistoryGroupBox();
	fillEnvHistoryGroupBox(envHist);

	std::string algrithmName=(*m_algHist.rbegin( )).name();
	if(!m_histPropWindow)
	{m_histPropWindow=CreateAlgHistoryPropertiesWindow(algrithmName.c_str());
	 m_histPropWindow->displayAlgHistoryProperties(algrithmName.c_str());
	}
	connect(m_Historytree,SIGNAL(updateAlgorithmHistoryWindow(QString)),this,SLOT(updateData(QString)));
}
AlgorithmHistoryWindow::~AlgorithmHistoryWindow()
{	if(m_Historytree){delete m_Historytree;m_Historytree=NULL;}
	if(m_histPropWindow){ delete m_histPropWindow;m_histPropWindow=NULL;}
	if(m_execSumGrpBox){delete m_execSumGrpBox;m_execSumGrpBox=NULL;}
	if(m_envHistGrpBox){delete m_envHistGrpBox;m_envHistGrpBox=NULL;}

}
AlgExecSummaryGrpBox* AlgorithmHistoryWindow::CreateExecSummaryGroupBox()
{	return (new AlgExecSummaryGrpBox("Execution Summary",this));
}
AlgEnvHistoryGrpBox* AlgorithmHistoryWindow::CreateEnvHistoryGroupBox()
{	return(new  AlgEnvHistoryGrpBox("Environment History",this) );
}
AlgHistoryProperties* AlgorithmHistoryWindow::CreateAlgHistoryPropertiesWindow(const QString& algrithmName)
{	return(new AlgHistoryProperties(this,m_algHist,algrithmName));
}
void AlgorithmHistoryWindow::populateAlgHistoryTreeWidget()
{
	std::vector <AlgorithmHistory>::reverse_iterator ralgHistory_Iter=m_algHist.rbegin( );
	std::string algrithmName;
	algrithmName=(*ralgHistory_Iter).name();
	int nAlgVersion=(*ralgHistory_Iter).version();
	getAlgNamewithVersion(algrithmName,nAlgVersion);
	
	QTreeWidgetItem * item= new	QTreeWidgetItem(QStringList(algrithmName.c_str()),QTreeWidgetItem::Type);
	if(m_Historytree)m_Historytree->addTopLevelItem(item);
	
	ralgHistory_Iter++;
	for ( ; ralgHistory_Iter != m_algHist.rend( ) ; ralgHistory_Iter++ )
	{
		algrithmName=(*ralgHistory_Iter).name();
		nAlgVersion=(*ralgHistory_Iter).version();
		getAlgNamewithVersion(algrithmName,nAlgVersion);
		QTreeWidgetItem * subitem= new	QTreeWidgetItem(QStringList(QStringList(algrithmName.c_str())));
		if(item)item->addChild(subitem);
	}
	
}
void AlgorithmHistoryWindow::getAlgNamewithVersion(std::string& algName,const int version)
{
	algName+=" v.";
	QString algVersion;
	algVersion.setNum(version,10);
	algName+=algVersion;
}

void AlgorithmHistoryWindow::fillExecutionSummaryGroupBox()
{
	std::vector <AlgorithmHistory>::reverse_iterator ralgHistory_Iter;
	for (ralgHistory_Iter = m_algHist.rbegin( ); ralgHistory_Iter != m_algHist.rend( ) ; ralgHistory_Iter++ )
	{
		double duration=0;
		duration=(*ralgHistory_Iter).executionDuration();
		dateAndTime date=(*ralgHistory_Iter).executionDate();
		if(m_execSumGrpBox)m_execSumGrpBox->setData(duration,date);
	}
}
void AlgorithmHistoryWindow::fillEnvHistoryGroupBox(const EnvironmentHistory& envHistory)
{
	std::string osname=envHistory.osName();
	std::string osversion=envHistory.osVersion();
	std:string userName=envHistory.userName();
	std::string frameworkVersion=envHistory.frameworkVersion();
	
	QLineEdit* osNameEdit= m_envHistGrpBox->getosNameEdit();
	osNameEdit->setText(osname.c_str());
	
	QLineEdit* osVersionEdit=m_envHistGrpBox->getosVersionEdit();
	osVersionEdit->setText(osversion.c_str());

	QLineEdit* frmworkVersionEdit=m_envHistGrpBox->getfrmworkVersionEdit();
	frmworkVersionEdit->setText(frameworkVersion.c_str());

}
void AlgorithmHistoryWindow::updateData(QString algName)
{	
	/* 
	if(!m_histPropWindow) updateAlgorithmHistoryWindow
	{
		m_histPropWindow=new AlgHistoryProperties(this,m_algHist,algName);
	}*/
	if(m_histPropWindow)
	{   m_histPropWindow->clearData();
		m_histPropWindow->displayAlgHistoryProperties(algName);
	}
	updateExecutionSummaryGroupBox(algName);
	
}
void AlgorithmHistoryWindow::updateExecutionSummaryGroupBox(const QString& algName)
{
	std::vector <AlgorithmHistory>::reverse_iterator ralgHistory_Iter;
	for (ralgHistory_Iter=m_algHist.rbegin( );
		ralgHistory_Iter!=m_algHist.rend();ralgHistory_Iter++)
	{
		std::string name=(*ralgHistory_Iter).name();
		if(algName==name.c_str())
			break;
	}//end of algorithm history for loop
	double duration=0;
	duration=(*ralgHistory_Iter).executionDuration();
	dateAndTime date=(*ralgHistory_Iter).executionDate();
	if(m_execSumGrpBox)m_execSumGrpBox->setData(duration,date);
}

AlgHistoryProperties::AlgHistoryProperties(AlgorithmHistoryWindow *w,const std::vector<AlgorithmHistory> &algHist,const QString& algName):
m_algHist(algHist)
{
	QStringList hList;
	hList<<"Name"<<"Value"<<"Default?:"<<"Direction"<<"";
	m_histpropTree = new  QTreeWidget(w);
	if(m_histpropTree)
	{	m_histpropTree->setColumnCount(5);
		m_histpropTree->setSelectionMode(QAbstractItemView::NoSelection);
		m_histpropTree->show();	
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
void AlgHistoryProperties::displayAlgHistoryProperties(const QString& algName)
{
	QStringList propList;
	std::string sProperty;
	bool bisDefault;
	int nDirection=0;
	std::vector<Mantid::Kernel::PropertyHistory> m_properties;
	for (std::vector <AlgorithmHistory>::reverse_iterator ralgHistory_Iter=m_algHist.rbegin( );
		ralgHistory_Iter!=m_algHist.rend();ralgHistory_Iter++)
	{
		m_properties=(*ralgHistory_Iter).getProperties();
		std::string name=(*ralgHistory_Iter).name();
		if(algName==name.c_str())
			break;
		       
	}//end of algorithm history for loop
	
	for ( std::vector<Mantid::Kernel::PropertyHistory>::const_iterator pIter = m_properties.begin();
		pIter != m_properties.end(); ++pIter )
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
	QString algName;
	int nversion;
	getAlgorithmName(algName,nversion);
	emit updateAlgorithmHistoryWindow(algName);
}

void AlgHistoryTreeWidget::getAlgorithmName(QString& algName, int& version)
{	
	QList<QTreeWidgetItem*> items = selectedItems();
	QString str;
	if( !items.empty() )
    {		
      QTreeWidgetItem *item = items[0];
	  if(item)
	  {	  str = item->text(0);
		  int nIndex=str.indexOf(".",0,Qt::CaseSensitive );
		  algName=str.left(nIndex-2);
	  }
	}
 }
void AlgHistoryTreeWidget::mouseDoubleClickEvent(QMouseEvent *e)
{		
	QString algName;
	int nversion;
	getAlgorithmName(algName,nversion);
	emit updateAlgorithmHistoryWindow(algName);
	QTreeWidget::mouseDoubleClickEvent(e);
}
