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

using namespace Mantid::API;

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
    void add(IAlgorithm_sptr alg);
    /// Removes stopped algorithm
    void remove(const IAlgorithm* alg);
    /// Returns number of running algorithms
    int count(){return m_algorithms.size();}
    /// Returns pointers to running algorithms
    const QVector<AlgorithmID>& algorithms(){return m_algorithms;}
    void lock(){s_mutex.lock();}
    void unlock(){s_mutex.unlock();}
signals:
    void countChanged(int);
    void needUpdateProgress(const IAlgorithm* alg,int p, const QString& msg);
protected:

    /// Algorithm notifiv=cation handlers
    void handleAlgorithmFinishedNotification(const Poco::AutoPtr<Algorithm::FinishedNotification>& pNf);
    Poco::NObserver<AlgorithmMonitor, Algorithm::FinishedNotification> m_finishedObserver;

    void handleAlgorithmProgressNotification(const Poco::AutoPtr<Algorithm::ProgressNotification>& pNf);
    Poco::NObserver<AlgorithmMonitor, Algorithm::ProgressNotification> m_progressObserver;

    void handleAlgorithmErrorNotification(const Poco::AutoPtr<Algorithm::ErrorNotification>& pNf);
    Poco::NObserver<AlgorithmMonitor, Algorithm::ErrorNotification> m_errorObserver;


public slots:
    void update();
    void showDialog();
    void cancel(AlgorithmID);
    void cancelAll();
private:
    MantidUI *m_mantidUI;
    int m_nRunning;                    // number of running algorithms
    QVector<AlgorithmID> m_algorithms; // IDs of running algorithms
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
    void update(int n);
    void updateProgress(const IAlgorithm* alg,int p, const QString& msg);
private:
    QVector<const IAlgorithm*> m_algorithms;
    AlgorithmMonitor *m_algMonitor;
    QTreeWidget *m_tree;
};

class AlgButton:public QPushButton
{
    Q_OBJECT
public:
    AlgButton(const QString& text,IAlgorithm_sptr alg):
    QPushButton(text),m_alg(alg->getAlgorithmID())
    {
        connect(this,SIGNAL(clicked()),this,SLOT(sendClicked()));
    }
private slots:
    void sendClicked(){emit clicked(m_alg);}
signals:
    void clicked(AlgorithmID );
private:
    AlgorithmID m_alg;
};

#endif /* ALGMONITOR_H */
