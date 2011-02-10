#ifndef ALGMONITOR_H
#define ALGMONITOR_H

#include "MantidAPI/Algorithm.h"

#include <Poco/NObserver.h>

#include <QObject>
#include <QVector>
#include <QDialog>
#include <QPushButton>
#include <QMutex>
#include <QThread>

class QLabel;
class QTreeWidget;
class MantidUI;
class MonitorDlg;

class AlgorithmMonitor: public QThread
{
	Q_OBJECT

public:
  /// Constructor
  AlgorithmMonitor(MantidUI *m);
  /// Destructor
  ~AlgorithmMonitor();
  /// Add algorithm to monitor
  void add(Mantid::API::IAlgorithm_sptr alg);
  /// Removes stopped algorithm
  void remove(const Mantid::API::IAlgorithm* alg);
  /// Returns number of running algorithms
  int count(){return m_algorithms.size();}
  /// Returns pointers to running algorithms
  const QVector<Mantid::API::AlgorithmID>& algorithms(){return m_algorithms;}
  void lock(){s_mutex.lock();}
  void unlock(){s_mutex.unlock();}
signals:
  void algorithmStarted(void* alg);
  void algorithmFinished(void* alg);
  void needUpdateProgress(void* alg,int p, const QString& msg);
  void countChanged();
  void allAlgorithmsStopped();
protected:

  /// Algorithm notification handlers
  void handleAlgorithmFinishedNotification(const Poco::AutoPtr<Mantid::API::Algorithm::FinishedNotification>& pNf);
  Poco::NObserver<AlgorithmMonitor, Mantid::API::Algorithm::FinishedNotification> m_finishedObserver;

  void handleAlgorithmProgressNotification(const Poco::AutoPtr<Mantid::API::Algorithm::ProgressNotification>& pNf);
  Poco::NObserver<AlgorithmMonitor, Mantid::API::Algorithm::ProgressNotification> m_progressObserver;

  void handleAlgorithmErrorNotification(const Poco::AutoPtr<Mantid::API::Algorithm::ErrorNotification>& pNf);
  Poco::NObserver<AlgorithmMonitor, Mantid::API::Algorithm::ErrorNotification> m_errorObserver;


public slots:
  void update();
  void showDialog();
  void cancel(Mantid::API::AlgorithmID);
  void cancelAll();
private:
  MantidUI *m_mantidUI;
  int m_nRunning;                    // number of running algorithms
  QVector<Mantid::API::AlgorithmID> m_algorithms; // IDs of running algorithms
  MonitorDlg* m_monitorDlg;
  static QMutex s_mutex;
};

class MonitorDlg: public QDialog
{
  Q_OBJECT
public:
  MonitorDlg(QWidget *parent,AlgorithmMonitor *algMonitor);
  ~MonitorDlg();
public slots:
  void update();
  // The void* corresponds to Mantid::API::AlgorithmID, but Qt wasn't coping with the typedef
  void updateProgress(void* alg, const int p, const QString& msg);
private:
  AlgorithmMonitor *m_algMonitor;
  QTreeWidget *m_tree;
};

class AlgButton:public QPushButton
{
  Q_OBJECT
public:
  AlgButton(const QString& text,Mantid::API::IAlgorithm_sptr alg):
  QPushButton(text),m_alg(alg->getAlgorithmID())
  {
    connect(this,SIGNAL(clicked()),this,SLOT(sendClicked()));
  }
private slots:
  void sendClicked(){emit clicked(m_alg);}
signals:
  void clicked(Mantid::API::AlgorithmID );
private:
  Mantid::API::AlgorithmID m_alg;
};

#endif /* ALGMONITOR_H */
