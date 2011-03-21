#ifndef ALGORITHMHISTORYWINDOW_H 
#define ALGORITHMHISTORYWINDOW_H

#include <QWidget>
#include <QTreeWidget>
#include <QMainWindow>
#include <QGroupBox>
#include <QDateTimeEdit>
#include <QLabel>
#include <QDateTimeEdit>
#include <QDialog>
#include <ctime>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QFormLayout>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QClipboard>

#include "MantidAPI/Workspace.h"
#include "MantidAPI/AlgorithmHistory.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/EnvironmentHistory.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/Property.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidQtAPI/MantidDialog.h"
#include "../ApplicationWindow.h"
#include "MantidKernel/Logger.h"
#include "QMessageBox"

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
  AlgExecSummaryGrpBox(QWidget*w):QGroupBox(w){}
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
  AlgEnvHistoryGrpBox(QWidget*w):QGroupBox(w){}
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

class AlgHistScriptButton:public QPushButton
{
  Q_OBJECT
  public:
  AlgHistScriptButton(QWidget*w):QPushButton(w){}
  AlgHistScriptButton(QString title,QWidget* w);
  ~AlgHistScriptButton();
};

class AlgHistoryProperties;
class AlgorithmHistoryWindow: public MantidQt::API::MantidDialog
{
  Q_OBJECT
  signals:
  void updateAlgorithmHistoryWindow(QString algName);
public:
  AlgorithmHistoryWindow(QWidget*w) : MantidQt::API::MantidDialog(w){}
  AlgorithmHistoryWindow(ApplicationWindow *w,const std::vector<Mantid::API::AlgorithmHistory>&alghist,
			 const Mantid::Kernel::EnvironmentHistory&);
  ~AlgorithmHistoryWindow();
private slots:
  void updateAll( QString algName,int algVersion,int nIndex);
	
  void copytoClipboard();
  void writeToScriptFile();
private:
  AlgExecSummaryGrpBox* createExecSummaryGrpBox();
  AlgEnvHistoryGrpBox* createEnvHistGrpBox(const Mantid::Kernel::EnvironmentHistory& envHistory);
  //AlgHistoryProperties * createAlgHistoryPropWindow(const QString& algName,int version);
  AlgHistoryProperties * createAlgHistoryPropWindow();
  void populateAlgHistoryTreeWidget();
  QPushButton * CreateScriptButton();
  QFileDialog* createScriptDialog(const QString& algName);
  //void updateExecSummaryGrpBox(const QString& algName,const int & version);
  void updateExecSummaryGrpBox(const QString& algName,const int & version,int index);
  //void updateAlgHistoryProperties(QString algName,int version);
  void updateAlgHistoryProperties(QString algName,int version,int pos);
  void concatVersionwithName(QString& algName,const int version);
  void  generateScript(QString& script);
  std::string sanitizePropertyName(const std::string & name);
  void handleException( const std::exception& e );
  void setAlgorithmName(const QString& algName);
  const QString& getAlgorithmName() const;
  void setAlgorithmVersion(const int& version);
  const int& getAlgorithmVersion()const;
  static Mantid::Kernel::Logger& g_log;
	
private:
  std::vector<Mantid::API::AlgorithmHistory> m_algHist;
  QPushButton *m_scriptButton;
  AlgHistoryTreeWidget *m_Historytree;
  AlgHistoryProperties * m_histPropWindow; 
  AlgExecSummaryGrpBox *m_execSumGrpBox ;
  AlgEnvHistoryGrpBox * m_envHistGrpBox;
  QString m_algName;
  int m_nVersion;
};

class AlgHistoryProperties: public QObject
{
  Q_OBJECT
  public:
  //AlgHistoryProperties(QWidget *w,
  //const std::vector<Mantid::API::AlgorithmHistory> &);
  AlgHistoryProperties(QWidget*w,const std::vector<Mantid::Kernel::PropertyHistory>& propHist);
  void displayAlgHistoryProperties();
  void clearData();
  void setAlgProperties( const std::vector<Mantid::Kernel::PropertyHistory>& histProp);
  const std::vector<Mantid::Kernel::PropertyHistory>& getAlgProperties();
public:
  QTreeWidget *m_histpropTree;
private:
  //std::vector<Mantid::API::AlgorithmHistory>m_algHist;
  std::vector<Mantid::Kernel::PropertyHistory> m_Histprop;
};
#endif



