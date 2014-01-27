#include "AlgorithmHistoryWindow.h"
#include "MantidAPI/AlgorithmManager.h"

#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidQtAPI/FileDialogHandler.h"

#include <QLineEdit>
#include <QLabel>
#include <QFileDialog>
#include <QDateTime>
#include <QFormLayout>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QTextStream>
#include <QTemporaryFile>
#include <QDir>

#include <iostream>
#include <fstream>
#include <stdio.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;

// Get a reference to the logger
Mantid::Kernel::Logger& AlgorithmHistoryWindow::g_log = Mantid::Kernel::Logger::get("AlgorithmHistoryWindow");
Mantid::Kernel::Logger& AlgHistoryTreeWidget::g_log = Mantid::Kernel::Logger::get("AlgHistoryTreeWidget");

AlgExecSummaryGrpBox::AlgExecSummaryGrpBox(QWidget *w) : QGroupBox(w),
  m_execDurationlabel(NULL),m_execDurationEdit(NULL),
  m_Datelabel(NULL),m_execDateTimeEdit(NULL), m_algexecDuration()
{
}

AlgExecSummaryGrpBox::AlgExecSummaryGrpBox(QString title,QWidget*w)
  : QGroupBox(title,w), m_execDurationlabel(NULL),m_execDurationEdit(NULL),
    m_Datelabel(NULL),m_execDateTimeEdit(NULL), m_algexecDuration()
{

  m_execDurationEdit=new QLineEdit("",this);
  if(m_execDurationEdit)
    m_execDurationEdit->setReadOnly(1);
  m_execDurationlabel=new QLabel("Duration:",this,0);
  if(m_execDurationlabel)m_execDurationlabel->setBuddy(m_execDurationEdit);

  QDateTime datetime(QDate(0,0,0), QTime(0,0,0),Qt::LocalTime );
  m_execDateTimeEdit=new QLineEdit("",this);
  if(m_execDateTimeEdit)
    m_execDateTimeEdit->setReadOnly(1);
  m_Datelabel=new QLabel("Date:",this,0);
  if(m_Datelabel)m_Datelabel->setBuddy(m_execDateTimeEdit);
		
  QFormLayout *formLayout = new QFormLayout;
  if(formLayout){
    formLayout->addRow(m_execDurationlabel,m_execDurationEdit);
    formLayout->addRow(m_Datelabel,m_execDateTimeEdit);
    setLayout(formLayout);
  }
  setGeometry (5,210,205,130);

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
void AlgExecSummaryGrpBox::setData(const double execDuration,const Mantid::Kernel::DateAndTime execDate)
{
  QString dur("");
  dur.setNum(execDuration,'g',6);
  dur+=" seconds";
  QLineEdit* execDurationEdit=getAlgExecDurationCtrl();
  if(execDurationEdit)execDurationEdit->setText(dur);
	
  //Get the timeinfo structure, but converting from UTC to local time
  std::tm t = execDate.to_localtime_tm() ;
  QTime qt(t.tm_hour,t.tm_min,t.tm_sec);
  QDate qd(t.tm_year+1900,t.tm_mon+1,t.tm_mday);
  QDateTime datetime(qd,qt,Qt::LocalTime );
	
  QString str("");
  str=datetime.toString("dd/MM/yyyy hh:mm:ss");
	
  QLineEdit* datetimeEdit=getAlgExecDateCtrl();
  if(datetimeEdit)datetimeEdit->setText(str);

}

AlgEnvHistoryGrpBox::AlgEnvHistoryGrpBox(QWidget *w) : QGroupBox(w),
  m_osNameLabel(NULL),m_osNameEdit(NULL),m_osVersionLabel(NULL),m_osVersionEdit(NULL),
  m_frmworkVersionLabel(NULL),m_frmwkVersnEdit(NULL)
{
}

AlgEnvHistoryGrpBox::AlgEnvHistoryGrpBox(QString title,QWidget*w):QGroupBox(title,w),
								  m_osNameLabel(NULL),m_osNameEdit(NULL),m_osVersionLabel(NULL),m_osVersionEdit(NULL),
								  m_frmworkVersionLabel(NULL),m_frmwkVersnEdit(NULL)
{
  //OS Name Label & Edit Box
  m_osNameEdit=new QLineEdit("",this);
  if(m_osNameEdit)
  {
    m_osNameEdit->setReadOnly(1);
  }
  m_osNameLabel=new QLabel("OS Name:",this,0);
  if(m_osNameLabel)m_osNameLabel->setBuddy(m_osNameEdit);

  //OS Version Label & Edit Box
  m_osVersionEdit=new QLineEdit("",this);
  if(m_osVersionEdit)
  {
    m_osVersionEdit->setReadOnly(1);
    m_osVersionLabel=new QLabel("OS Version:",this,0);
  }
  if(m_osVersionLabel)
    m_osVersionLabel->setBuddy(m_osVersionEdit);

  //Mantid Framework Version Label & Edit Box
  m_frmwkVersnEdit=new QLineEdit("",this);
  if(m_frmwkVersnEdit)
    m_frmwkVersnEdit->setReadOnly(1);
  m_frmworkVersionLabel=new QLabel("Framework Version:",this,0);
  if(m_frmworkVersionLabel)
    m_frmworkVersionLabel->setBuddy(m_frmwkVersnEdit);
	
  QFormLayout * formLayout=new QFormLayout();
  if(formLayout)
  {
    formLayout->addRow(m_osNameLabel,m_osNameEdit);
    formLayout->addRow(m_osVersionLabel,m_osVersionEdit);
    formLayout->addRow(m_frmworkVersionLabel,m_frmwkVersnEdit);
    setLayout(formLayout);
  }
  setGeometry (214,210,347,130);
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


AlgorithmHistoryWindow::AlgorithmHistoryWindow(QWidget *parent,const boost::shared_ptr<const Workspace> wsptr):
MantidDialog(parent),m_algHist(wsptr->getHistory()),m_histPropWindow(NULL),m_execSumGrpBox(NULL),m_envHistGrpBox(NULL),m_wsName(wsptr->getName().c_str())
{
  setWindowTitle(tr("Algorithm History"));
  setMinimumHeight(400);
  setMinimumWidth(570);
  setGeometry(50,150,540,380); 

  //Create a tree widget to display the algorithm names in the workspacehistory
  m_Historytree = new AlgHistoryTreeWidget(this);
  if(m_Historytree)
  {
    m_Historytree->setHeaderLabel("Algorithms");
    m_Historytree->setGeometry (5,5,205,200);   
  }
  //Populate the History Tree widget
  populateAlgHistoryTreeWidget();

  //create a tree widget to dispaly history properties
  if(!m_histPropWindow)
    m_histPropWindow=createAlgHistoryPropWindow();
  connect(m_Historytree,SIGNAL(updateAlgorithmHistoryWindow(QString, int,int)),this,SLOT(updateAll(QString,int,int)));

  // The tree and the history details layout
  QHBoxLayout *treeLayout = new QHBoxLayout;
  treeLayout->addWidget(m_Historytree,1); // History stretches 1
  treeLayout->addWidget(m_histPropWindow->m_histpropTree,2); // Properties gets more space

  //Create a GroupBox to display exec date,duration
  if(!m_execSumGrpBox)m_execSumGrpBox=createExecSummaryGrpBox();
  //Create a Groupbox to display environment details
  if(!m_envHistGrpBox)m_envHistGrpBox=createEnvHistGrpBox(wsptr->getHistory().getEnvironmentHistory());

  QHBoxLayout *environmentLayout = new QHBoxLayout;
  environmentLayout->addWidget(m_execSumGrpBox, 1);
  environmentLayout->addWidget(m_envHistGrpBox, 2);

  // The button at the bottom
  m_scriptButtonFile = new QPushButton("Script to File",this);
  m_scriptButtonClipboard = new QPushButton("Script to Clipboard",this);
  connect(m_scriptButtonFile,SIGNAL(clicked()), this, SLOT(writeToScriptFile()));
  connect(m_scriptButtonClipboard,SIGNAL(clicked()), this, SLOT(copytoClipboard()));

  QHBoxLayout *buttonLayout = new QHBoxLayout;
  buttonLayout->addStretch(1); // Align the button to the right
  buttonLayout->addWidget(m_scriptButtonFile);
  buttonLayout->addWidget(m_scriptButtonClipboard);

  //Main layout
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->addLayout(treeLayout);
  mainLayout->addLayout(environmentLayout);
  mainLayout->addLayout(buttonLayout);
}

AlgorithmHistoryWindow::~AlgorithmHistoryWindow()
{	
  if(m_Historytree){delete m_Historytree;m_Historytree=NULL;}
  if(m_histPropWindow){ delete m_histPropWindow;m_histPropWindow=NULL;}
  if(m_execSumGrpBox){delete m_execSumGrpBox;m_execSumGrpBox=NULL;}
  if(m_envHistGrpBox){delete m_envHistGrpBox;m_envHistGrpBox=NULL;}
}

AlgExecSummaryGrpBox* AlgorithmHistoryWindow::createExecSummaryGrpBox()
{	
  AlgExecSummaryGrpBox *pgrpBox=new AlgExecSummaryGrpBox("Execution Summary",this);
  if(pgrpBox)
  {
    //iterating through algorithm history to display exec duration,date
    //last executed algorithm exec duration,date will be displayed in gruopbox
    const size_t noEntries = m_algHist.size();
    for( size_t i = 0; i < noEntries; ++i)
    {
      const AlgorithmHistory & entry = m_algHist.getAlgorithmHistory(i);
      double duration=0;
      duration = entry.executionDuration();
      Mantid::Kernel::DateAndTime date = entry.executionDate();
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
AlgHistoryProperties* AlgorithmHistoryWindow::createAlgHistoryPropWindow()
{	
  std::vector<PropertyHistory> histProp;
  const WorkspaceHistory::AlgorithmHistories & entries = m_algHist.getAlgorithmHistories();
  auto rIter = entries.rbegin();
  histProp=(*rIter).getProperties();

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

void AlgorithmHistoryWindow::writeToScriptFile()
{
  QString prevDir = MantidQt::API::AlgorithmInputHistory::Instance().getPreviousDirectory();
  QString scriptDir("");
  // Default script directory
  if(prevDir.isEmpty())
  {
    scriptDir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("pythonscripts.directory"));
  }
  else
  {
    scriptDir = prevDir;
  }
  QString filePath = MantidQt::API::FileDialogHandler::getSaveFileName(this,tr("Save Script As "),scriptDir,tr("Script files (*.py)"));
  // An empty string indicates they clicked cancel
  if( filePath.isEmpty() ) return;
  
  IAlgorithm_sptr genPyScript = AlgorithmManager::Instance().createUnmanaged("GeneratePythonScript");
  genPyScript->initialize();
  genPyScript->setChild(true); // Use as utility
  genPyScript->setRethrows(true); // Make it throw to catch errors messages and display them in a more obvious place for this window
  genPyScript->setPropertyValue("InputWorkspace",m_wsName.toStdString());
  genPyScript->setPropertyValue("Filename",filePath.toStdString());
  try
  {
    genPyScript->execute();
  }
  catch(std::exception &)
  {
    QMessageBox::information(this, "Generate Python Script", "Unable to generate script, see log window for details.");
  }

  MantidQt::API::AlgorithmInputHistory::Instance().setPreviousDirectory(QFileInfo(filePath).absoluteDir().path());
}

void AlgorithmHistoryWindow::populateAlgHistoryTreeWidget()
{
  const WorkspaceHistory::AlgorithmHistories & entries = m_algHist.getAlgorithmHistories();
  auto ralgHistory_Iter = entries.rbegin();
  std::string algrithmName;
  algrithmName=(*ralgHistory_Iter).name();
  QString algName=algrithmName.c_str();
  int nAlgVersion=(*ralgHistory_Iter).version();
  concatVersionwithName(algName,nAlgVersion);
	
  QTreeWidgetItem * item= new	QTreeWidgetItem(QStringList(algName),QTreeWidgetItem::Type);
  if(m_Historytree)m_Historytree->addTopLevelItem(item);
  ++ralgHistory_Iter;
  for ( ; ralgHistory_Iter != entries.rend( ) ; ++ralgHistory_Iter )
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
  std::string osname=envHistory.osName();
  std::string osversion=envHistory.osVersion();
  std::string frwkversn=envHistory.frameworkVersion();
	
  QLineEdit* osNameEdit=getosNameEdit();
  if(osNameEdit)osNameEdit->setText(osname.c_str());
	
  QLineEdit* osVersionEdit=getosVersionEdit();
  if(osVersionEdit)osVersionEdit->setText(osversion.c_str());

  QLineEdit* frmwkVersnEdit=getfrmworkVersionEdit();
  if(frmwkVersnEdit)frmwkVersnEdit->setText(frwkversn.c_str());

}
void AlgorithmHistoryWindow::updateAll(QString algName,int version,int index)
{	
  int nSize=static_cast<int>(m_algHist.size());
  //indicates the position of the ALgorithmHistory Object in the history vector
  int pos=0;
  //index =-1 for the parent item(root item)
  if (index==-1)
  {	//parent in the algorithHistoryTree widget comes here
    pos=nSize-1;
    g_log.debug()<< "selected algorithm is at position "<<pos<<"  in the History vector "<<std::endl;
  }
  else
  {	pos=nSize-2-index;
    g_log.debug ()<< "selected algorithm is at position  "<<pos <<"  in the History vector "<<std::endl;
  }
  updateAlgHistoryProperties(algName,version,pos);
  updateExecSummaryGrpBox(algName,version,pos);
	
}
void AlgorithmHistoryWindow::updateAlgHistoryProperties(QString algName,int version,int pos)
{
  std::vector<PropertyHistory> histProp;
  //getting the selcted algorithm at pos from History vector
  const AlgorithmHistory & algHist = m_algHist.getAlgorithmHistory(pos);
  std::string name=algHist.name();
  int nVer=algHist.version();
  //if name and version in the history is same as selected item
  //get the properties and display it.
  if((algName==name.c_str())&& (nVer==version))
  {
    histProp=algHist.getProperties();
    if(m_histPropWindow)
    {  m_histPropWindow->setAlgProperties(histProp);
      m_histPropWindow->clearData();
      m_histPropWindow->displayAlgHistoryProperties();
    }
  }
}
void AlgorithmHistoryWindow::updateExecSummaryGrpBox(const QString& algName,const int & version,int pos)
{
  //getting the selcted algorithm at pos from History vector
  const AlgorithmHistory & algHist = m_algHist.getAlgorithmHistory(pos);
  std::string name=algHist.name();
  int nVer=algHist.version();
  //if name and version in the history is same as selected item
  //get the properties and display it.
  if((algName==name.c_str())&& (nVer==version))
  {
    double duration=algHist.executionDuration();
    Mantid::Kernel::DateAndTime date=algHist.executionDate();
    if(m_execSumGrpBox)m_execSumGrpBox->setData(duration,date);
  }
}
void AlgorithmHistoryWindow::copytoClipboard()
{	
  // We retrieve a string containing the script by outputting the result of GeneratePythonScript
  // to a temp file, and parsing it from there.

  // QTemporaryFile will not allow its files to have extensions, and the GeneratePythonScript
  // validator must contains a list of accepted extensions.  For that reason we choose the
  // workaround of: 
  // - create a temp file through QTemporaryFile;
  // - take its filepath and append ".py" to it;
  // - use the filepath to create our own temp file, which we will handle the deletion of.

  QTemporaryFile temp;
  temp.open();
  temp.close();

  std::string tempFilename = temp.fileName().toStdString() + ".py";

  // Create and run algorithm.
  IAlgorithm_sptr genPyScript = AlgorithmManager::Instance().createUnmanaged("GeneratePythonScript");
  genPyScript->initialize();
  genPyScript->setChild(true); // Use as utility
  genPyScript->setRethrows(true); // Make it throw to catch errors messages and display them in a more obvious place for this window
  genPyScript->setPropertyValue("InputWorkspace",m_wsName.toStdString());
  genPyScript->setPropertyValue("Filename",tempFilename);
  try
  {
    genPyScript->execute();
  }
  catch(std::exception &)
  {
    QMessageBox::information(this, "Generate Python Script", "Unable to generate script, see log window for details.");
    return;
  }

  QString script;
  std::ifstream file(tempFilename.c_str(), std::ifstream::in);
  std::stringstream buffer;

  // Retrieve script from file.
  buffer << file.rdbuf();
  std::string contents(buffer.str());
  script.append(contents.c_str());

  file.close();
  remove(tempFilename.c_str());

  // Send to clipboard.
  QClipboard *clipboard = QApplication::clipboard();
  if(NULL != clipboard)
  {	
    clipboard->setText(script);
  }
}

AlgHistoryProperties::AlgHistoryProperties(QWidget*w,const std::vector<PropertyHistory>& propHist):
  m_Histprop(propHist)
{
  QStringList hList;
  hList<<"Name"<<"Value"<<"Default?:"<<"Direction"<<"";
  m_histpropTree = new  QTreeWidget(w);
  if(m_histpropTree)
  {
    m_histpropTree->setColumnCount(5);
    m_histpropTree->setSelectionMode(QAbstractItemView::NoSelection);
    m_histpropTree->setHeaderLabels(hList);
    m_histpropTree->setGeometry (213,5,350,200);
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
void AlgHistoryProperties::setAlgProperties( const std::vector<PropertyHistory>& histProp)
{
  m_Histprop.assign(histProp.begin(),histProp.end());
}
const std::vector<PropertyHistory>& AlgHistoryProperties:: getAlgProperties()
{
  return m_Histprop;
}
void AlgHistoryProperties::displayAlgHistoryProperties()
{
  QStringList propList;
  std::string sProperty;
  for ( std::vector<Mantid::Kernel::PropertyHistory>::const_iterator pIter = m_Histprop.begin();
	pIter != m_Histprop.end(); ++pIter )
  {
    sProperty=(*pIter).name();
    propList.append(sProperty.c_str());
    sProperty=(*pIter).value();
    propList.append(sProperty.c_str());

    bool bisDefault=(*pIter).isDefault();
    bisDefault? (sProperty="Yes"):(sProperty="No");

    propList.append(sProperty.c_str());
    int nDirection=(*pIter).direction();
    switch(nDirection)
    {
    case 0:{sProperty="Input";break;}
    case 1:{sProperty="Output";break;}
    case 2:{sProperty="InOut";break;}
    default:{sProperty="N/A";break;}
    }
    propList.append(sProperty.c_str());
    QTreeWidgetItem * item= new	QTreeWidgetItem(propList);
    if(m_histpropTree)m_histpropTree->addTopLevelItem(item);
    propList.clear();

  }// end of properties for loop
    
}
void AlgHistoryTreeWidget::treeSelectionChanged()
{	
  QString algName("");int nVersion=-1;int nIndex=-1;
  getSelectedAlgorithmName(algName,nVersion,nIndex);
  emit updateAlgorithmHistoryWindow(algName,nVersion,nIndex);
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
void AlgHistoryTreeWidget::getSelectedAlgorithmName(QString& algName,int & version,int & index)
{	
  QList<QTreeWidgetItem*> items = selectedItems();
  if( !items.empty() )
  {QTreeWidgetItem *item = items[0];
    if(item)
    {	//finding the index of the selected item
      QModelIndex modelIndex=indexFromItem(item);
      int row=modelIndex.row();
      if(row!=0)
      {g_log.debug()<< "It's child Item"<<std::endl;
	index=row;
      }
      else if (row==0)
      {
	//row can be zero for 1st child item and parent item
	QTreeWidgetItem * parent=NULL;
	parent=item->parent();
	if(parent)
	{//if it's child item at row zero set index =0
	  g_log.debug()<< "It's child Item"<<std::endl;
	  index=0;
	}
	else
	{	//if it's parent item set index = -1
	  g_log.debug()<< "It's parent  item "<<std::endl;
	  index=-1;
	}
      }
      QString  str=item->text(0);
      int nDotIndex=str.indexOf(".",0,Qt::CaseSensitive );
      algName=str.left(nDotIndex-2);
      version=str.right(str.length()-nDotIndex-1).toInt();
      g_log.debug()<< "selected alg name =  "<< algName.toStdString()<<" index number =  "<<index<<std::endl;
    }
  }
}
void AlgHistoryTreeWidget::mouseDoubleClickEvent(QMouseEvent *e)
{		
  QString algName("");int nVersion=-1;int nIndex=-1;
  getSelectedAlgorithmName(algName,nVersion,nIndex);
  emit updateAlgorithmHistoryWindow(algName,nVersion,nIndex);
  QTreeWidget::mouseDoubleClickEvent(e);
}
