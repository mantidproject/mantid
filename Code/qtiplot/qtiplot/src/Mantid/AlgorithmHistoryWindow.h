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

#include "MantidAPI/Workspace.h"
#include "MantidAPI/AlgorithmHistory.h"
#include "MantidKernel/EnvironmentHistory.h"
#include "../ApplicationWindow.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
typedef time_t dateAndTime;

class AlgHistoryTreeWidget:public QTreeWidget
{
    Q_OBJECT
signals:
	void updateAlgorithmHistoryWindow(QString algName);
public:
	/// Constructor
	AlgHistoryTreeWidget(QWidget *w):QTreeWidget(w)
	{
		connect(this,SIGNAL(itemSelectionChanged()),this,SLOT(treeSelectionChanged()));
	}
private:
    void mouseDoubleClickEvent(QMouseEvent *e);
	void getAlgorithmName(QString& algName, int& version);
public slots:
	void treeSelectionChanged();
};
class AlgExecSummaryGrpBox: public QGroupBox
{
	 Q_OBJECT
public:
	AlgExecSummaryGrpBox(QWidget*w):QGroupBox(w){}
	//AlgExecSummaryGrpBox(QString,AlgorithmHistoryWindow *w);
	AlgExecSummaryGrpBox(QString,QWidget*w);
	void setData(const double execDuration,const dateAndTime execDate);
private:
	QLineEdit* getAlgExecDuration()const {return m_execDurationEdit;}
	QLineEdit* getAlgExecDate() const{ return m_execDateTimeEdit;}
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

	QLineEdit* getosNameEdit()const {return m_osNameEdit;}
	QLineEdit* getosVersionEdit()const {return m_osVersionEdit;}
	QLineEdit* getfrmworkVersionEdit()const {return m_frmworkVersionEdit;}
private:
	QLabel *m_osNameLabel;
	QLineEdit *m_osNameEdit;
	QLabel *m_osVersionLabel;
	QLineEdit *m_osVersionEdit;
	QLabel *m_frmworkVersionLabel;
	QLineEdit *m_frmworkVersionEdit;
};


class AlgHistoryProperties;
class AlgorithmHistoryWindow: public QDialog //QMainWindow //QDockWidget
{
	Q_OBJECT
signals:
	void updateAlgorithmHistoryWindow(QString algName);
public:
	//AlgorithmHistoryWindow(QWidget*w):QMainWindow(w){}
	AlgorithmHistoryWindow(QWidget*w):QDialog(w){}
	AlgorithmHistoryWindow(ApplicationWindow *w,
		const std::vector<AlgorithmHistory>&alghist,
		const EnvironmentHistory&);
~AlgorithmHistoryWindow();
public slots:
		void updateData( QString algName);
private:
	AlgExecSummaryGrpBox* CreateExecSummaryGroupBox();
	AlgEnvHistoryGrpBox* CreateEnvHistoryGroupBox();
	AlgHistoryProperties * CreateAlgHistoryPropertiesWindow(const QString&);
	void populateAlgHistoryTreeWidget();
	void fillExecutionSummaryGroupBox();
	void fillEnvHistoryGroupBox(const EnvironmentHistory& envHist);
	void updateExecutionSummaryGroupBox(const QString& algName);
	void getAlgNamewithVersion(std::string& algName,const int version);
private:
	std::vector<Mantid::API::AlgorithmHistory>m_algHist;
	//ApplicationWindow * m_appWindow;
	AlgHistoryTreeWidget *m_Historytree;
	//QTreeWidget *m_tree;
	AlgHistoryProperties * m_histPropWindow; 
	AlgExecSummaryGrpBox *m_execSumGrpBox ;
	AlgEnvHistoryGrpBox * m_envHistGrpBox;
};

class AlgHistoryProperties: public QObject// public QMainWindow //QDockWidget
{
    Q_OBJECT
public:
   // AlgHistoryProperties(QWidget*w):QMainWindow(w){}
    AlgHistoryProperties(AlgorithmHistoryWindow *w,
		const std::vector<Mantid::API::AlgorithmHistory> &,
		const QString&);
	void displayAlgHistoryProperties(const QString&);
	void clearData();
protected:
    QTreeWidget *m_histpropTree;
private:
	std::vector<Mantid::API::AlgorithmHistory>m_algHist;
};
#endif



