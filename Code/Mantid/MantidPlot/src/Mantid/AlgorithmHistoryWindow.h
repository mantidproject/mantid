#ifndef ALGORITHMHISTORYWINDOW_H 
#define ALGORITHMHISTORYWINDOW_H

#include "MantidAPI/WorkspaceHistory.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/EnvironmentHistory.h"
#include "MantidKernel/Logger.h"

#include "MantidQtAPI/MantidDialog.h"

#include <QTreeWidget>
#include <QGroupBox>
#include <QPushButton>

//------------------------------------------------------------------------------
// Forward declarations
//------------------------------------------------------------------------------
class QMouseEvent;
class QLineEdit;
class QLabel;
class QFileDialog;

//------------------------------------------------------------------------------
// Mantid Forward declarations
//------------------------------------------------------------------------------

namespace Mantid 
{
  namespace API 
  {
    class Workspace;
  }
}

class AlgHistoryTreeWidget:public QTreeWidget
{
  Q_OBJECT
  signals:
  void updateAlgorithmHistoryWindow(QString algName,int algVersion,int Index);
public:
  /// Constructor
  AlgHistoryTreeWidget(QWidget *w):QTreeWidget(w),m_algName(""),m_nVersion(0)
  {
    connect(this,SIGNAL(itemSelectionChanged()),this,SLOT(treeSelectionChanged()));
  }
  void getSelectedAlgorithmName(QString& algName,int & version,int & index);
  const QString getAlgorithmName();
  const int& getAlgorithmVersion();
public slots:
  void treeSelectionChanged();
private:
  void mouseDoubleClickEvent(QMouseEvent *e);
  void setAlgorithmName(const QString& );
  void setAlgorithmVersion(const int& version);
	
private:
  QString m_algName;
  int m_nVersion;
  static Mantid::Kernel::Logger& g_log;

};

class AlgExecSummaryGrpBox: public QGroupBox
{
  Q_OBJECT
  public:
  AlgExecSummaryGrpBox(QWidget*w);
  AlgExecSummaryGrpBox(QString,QWidget*w);
  ~AlgExecSummaryGrpBox();
  void setData(const double execDuration,const Mantid::Kernel::DateAndTime execDate);
private:
  QLineEdit* getAlgExecDurationCtrl()const {return m_execDurationEdit;}
  QLineEdit* getAlgExecDateCtrl() const{ return m_execDateTimeEdit;}
private:
  QLabel *m_execDurationlabel;
  QLineEdit *m_execDurationEdit;
  QLabel *m_Datelabel;
  QLineEdit*m_execDateTimeEdit;
  QString m_algexecDuration;	
};

class AlgEnvHistoryGrpBox: public QGroupBox
{
  Q_OBJECT
  public:
  AlgEnvHistoryGrpBox(QWidget*w);
  AlgEnvHistoryGrpBox(QString,QWidget*w);
  ~AlgEnvHistoryGrpBox();

  QLineEdit* getosNameEdit()const {return m_osNameEdit;}
  QLineEdit* getosVersionEdit()const {return m_osVersionEdit;}
  QLineEdit* getfrmworkVersionEdit()const {return m_frmwkVersnEdit;}
  void fillEnvHistoryGroupBox(const Mantid::Kernel::EnvironmentHistory& envHist);
private:
  QLabel *m_osNameLabel;
  QLineEdit *m_osNameEdit;
  QLabel *m_osVersionLabel;
  QLineEdit *m_osVersionEdit;
  QLabel *m_frmworkVersionLabel;
  QLineEdit *m_frmwkVersnEdit;
};

class AlgHistoryProperties;
class AlgorithmHistoryWindow: public MantidQt::API::MantidDialog
{
  Q_OBJECT
  signals:
  void updateAlgorithmHistoryWindow(QString algName);
public:
  AlgorithmHistoryWindow(QWidget *parent,const boost::shared_ptr<const Mantid::API::Workspace>);
  ~AlgorithmHistoryWindow();
private slots:
  void updateAll( QString algName,int algVersion,int nIndex);
	
  void copytoClipboard();
  void writeToScriptFile();
private:
  AlgExecSummaryGrpBox* createExecSummaryGrpBox();
  AlgEnvHistoryGrpBox* createEnvHistGrpBox(const Mantid::Kernel::EnvironmentHistory& envHistory);
  AlgHistoryProperties * createAlgHistoryPropWindow();
  void populateAlgHistoryTreeWidget();
  QFileDialog* createScriptDialog(const QString& algName);
  void updateExecSummaryGrpBox(const QString& algName,const int & version,int index);
  void updateAlgHistoryProperties(QString algName,int version,int pos);
  void concatVersionwithName(QString& algName,const int version);
  static Mantid::Kernel::Logger& g_log;
	
private:
  const Mantid::API::WorkspaceHistory & m_algHist;
  QPushButton *m_scriptButtonFile;
  QPushButton *m_scriptButtonClipboard;
  AlgHistoryTreeWidget *m_Historytree;
  AlgHistoryProperties * m_histPropWindow; 
  AlgExecSummaryGrpBox *m_execSumGrpBox ;
  AlgEnvHistoryGrpBox * m_envHistGrpBox;
  QString m_wsName;
};

class AlgHistoryProperties: public QObject
{
  Q_OBJECT
  public:
  AlgHistoryProperties(QWidget*w,const std::vector<Mantid::Kernel::PropertyHistory>& propHist);
  void displayAlgHistoryProperties();
  void clearData();
  void setAlgProperties( const std::vector<Mantid::Kernel::PropertyHistory>& histProp);
  const std::vector<Mantid::Kernel::PropertyHistory>& getAlgProperties();
public:
  QTreeWidget *m_histpropTree;
private:
  std::vector<Mantid::Kernel::PropertyHistory> m_Histprop;
};
#endif



