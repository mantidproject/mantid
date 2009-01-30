#include "AlgMonitor.h"
#include "MantidUI.h"
#include "MantidDock.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/PropertyManager.h"

#include <QtGui>
#include <QThread>

#include <iostream>
#include <algorithm>

using namespace std;

QMutex AlgorithmMonitor::s_mutex;

AlgorithmMonitor::AlgorithmMonitor(MantidUI *m) : 
m_finishedObserver(*this, &AlgorithmMonitor::handleAlgorithmFinishedNotification),
m_progressObserver(*this, &AlgorithmMonitor::handleAlgorithmProgressNotification),
m_errorObserver(*this, &AlgorithmMonitor::handleAlgorithmErrorNotification),
m_mantidUI(m), m_nRunning(0)
{
}

AlgorithmMonitor::~AlgorithmMonitor()
{
  cancelAll();
  wait(1000);
  exit();
  wait();
}

void AlgorithmMonitor::add(Algorithm *alg)
{
    lock();
    alg->notificationCenter.addObserver(m_finishedObserver);
    alg->notificationCenter.addObserver(m_errorObserver);
    alg->notificationCenter.addObserver(m_progressObserver);
    m_algorithms.push_back(alg);
    ++m_nRunning;
    emit countChanged(m_nRunning);
    unlock();
    //    std::cerr << "Add to Monitor Algorithms running " << m_nRunning << "\n";

}

void AlgorithmMonitor::remove(const Algorithm *alg)
{
    lock();
    QVector<Algorithm*>::iterator i = find(m_algorithms.begin(),m_algorithms.end(),alg);
    if (i != m_algorithms.end()) 
    { 
      m_algorithms.erase(i);
      --m_nRunning;
    }
    unlock();
    //std::cerr << "Monitor removal Algorithms running " << m_nRunning << "\n";
    emit countChanged(m_nRunning);

}

Algorithm_sptr AlgorithmMonitor::getShared(const Algorithm *alg)
{
    const std::vector<Algorithm_sptr>& algs = Mantid::API::AlgorithmManager::Instance().algorithms();
    for(std::vector<Algorithm_sptr>::const_iterator a = algs.begin();a!=algs.end();a++)
        if (a->get() == alg) return *a;
    return Algorithm_sptr();
}

void AlgorithmMonitor::update()
{
}

void AlgorithmMonitor::handleAlgorithmFinishedNotification(const Poco::AutoPtr<Algorithm::FinishedNotification>& pNf)
{
  remove(pNf->algorithm());
}

void AlgorithmMonitor::handleAlgorithmProgressNotification(const Poco::AutoPtr<Algorithm::ProgressNotification>& pNf)
{
    if (m_monitorDlg) emit needUpdateProgress(pNf->algorithm(),int(pNf->progress*100),QString::fromStdString(pNf->message));
}

void AlgorithmMonitor::handleAlgorithmErrorNotification(const Poco::AutoPtr<Algorithm::ErrorNotification>& pNf)
{
    remove(pNf->algorithm());
}

void AlgorithmMonitor::showDialog()
{
    m_monitorDlg = new MonitorDlg(m_mantidUI->appWindow(),this);
    m_monitorDlg->exec();
    m_monitorDlg = 0;
}

void AlgorithmMonitor::cancel(Algorithm *alg)
{
    Algorithm_sptr a = getShared(alg);
    if (!a.get()) return;
    a->cancel();
}

void AlgorithmMonitor::cancelAll()
{
  for_each(m_algorithms.begin(), m_algorithms.end(), mem_fun(&Mantid::API::Algorithm::cancel));
}

//-----------------------------------------------------------------------------------------------//
MonitorDlg::MonitorDlg(QWidget *parent,AlgorithmMonitor *algMonitor):QDialog(parent),m_algMonitor(algMonitor)
{
    setAttribute(Qt::WA_DeleteOnClose);
    m_tree = 0;
    update(0);
    connect(algMonitor,SIGNAL(countChanged(int)),this,SLOT(update(int)));
    connect(algMonitor,SIGNAL(needUpdateProgress(const Algorithm*,int, const QString&)),SLOT(updateProgress(const Algorithm*,int, const QString&)));

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    QPushButton *closeButton = new QPushButton("Close");
    connect(closeButton,SIGNAL(clicked()),this,SLOT(close()));
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(m_tree);
    layout->addLayout(buttonLayout);
	setLayout(layout);
	setWindowTitle("Mantid - Algorithm progress");
    resize(500,300);
}

MonitorDlg::~MonitorDlg()
{
}

void MonitorDlg::update(int)
{
    if (!m_tree)
    {
        m_tree = new QTreeWidget;
        m_tree->setColumnCount(3);
        m_tree->setSelectionMode(QAbstractItemView::NoSelection);
        QStringList hList;
        hList<<"Algorithm"<<"Progress"<<"";
        m_tree->setHeaderLabels(hList);
        QHeaderView* hHeader = (QHeaderView*)m_tree->header();
        hHeader->setResizeMode(1,QHeaderView::Stretch);
        hHeader->setResizeMode(2,QHeaderView::Fixed);
        hHeader->setStretchLastSection(false);
    }
    else
        m_tree->clear();
    
    m_algMonitor->lock();
    QVector<Mantid::API::Algorithm*>::const_iterator iend = m_algMonitor->algorithms().end();
    for(QVector<Mantid::API::Algorithm*>::const_iterator itr = m_algMonitor->algorithms().begin(); 
	itr != iend; ++itr)
    {
      const Algorithm* alg = *itr;
      m_algorithms << alg;
      QStringList iList;
      iList<<QString::fromStdString(alg->name());
      QTreeWidgetItem *algItem = new QTreeWidgetItem(iList);
      m_tree->addTopLevelItem(algItem);
      QProgressBar *algProgress = new QProgressBar;
      algProgress->setAlignment(Qt::AlignHCenter);
      AlgButton *cancelButton = new AlgButton("Cancel", *itr);
      m_tree->setItemWidget(algItem,1,algProgress);
      m_tree->setItemWidget(algItem,2,cancelButton);
      const std::vector< Mantid::Kernel::Property* >& prop_list = alg->getProperties();
        for(std::vector< Mantid::Kernel::Property* >::const_iterator prop=prop_list.begin();prop!=prop_list.end();prop++)
	  {
            QStringList lstr;
            lstr  << QString::fromStdString((**prop).name()) + ": " << QString::fromStdString((**prop).value());
            if ((**prop).isDefault()) lstr << " Default";
            algItem->addChild(new QTreeWidgetItem(lstr));
        }

        connect(cancelButton,SIGNAL(clicked(Algorithm *)),m_algMonitor,SLOT(cancel(Algorithm *)));
    }
    m_algMonitor->unlock();
}

void MonitorDlg::updateProgress(const Algorithm* alg,int p, const QString& msg)
{
    int i = m_algorithms.indexOf(alg);
    if (i >= 0)
    {
        QTreeWidgetItem *item = m_tree->topLevelItem(i);
        if (item)
        {
            QProgressBar *algProgress = static_cast<QProgressBar*>(m_tree->itemWidget(item,1));
            algProgress->setValue(p);
            algProgress->setFormat(msg + " %p%");
        }
    }
}
