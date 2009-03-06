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
  m_monitorDlg = new MonitorDlg(m_mantidUI->appWindow(), this);
  m_monitorDlg->setVisible(false);
}

AlgorithmMonitor::~AlgorithmMonitor()
{
  if( m_monitorDlg )
  {
    m_monitorDlg->close();
    delete m_monitorDlg;
  }
  cancelAll();
  wait(1000);
  exit();
  wait();
}

void AlgorithmMonitor::add(IAlgorithm_sptr alg)
{
    lock();
    alg->addObserver(m_finishedObserver);
    alg->addObserver(m_errorObserver);
    alg->addObserver(m_progressObserver);
    m_algorithms.push_back(alg->getAlgorithmID());
    ++m_nRunning;
    emit countChanged(m_nRunning);
    unlock();
}

void AlgorithmMonitor::remove(const IAlgorithm* alg)
{
    lock();
    QVector<AlgorithmID>::iterator i = find(m_algorithms.begin(),m_algorithms.end(),alg->getAlgorithmID());
    if (i != m_algorithms.end()) 
    { 
      m_algorithms.erase(i);
      --m_nRunning;
    }
    emit countChanged(m_nRunning);
    unlock();
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
    if (m_monitorDlg->isVisible()) 
    emit needUpdateProgress(pNf->algorithm(),int(pNf->progress*100),QString::fromStdString(pNf->message));
}

void AlgorithmMonitor::handleAlgorithmErrorNotification(const Poco::AutoPtr<Algorithm::ErrorNotification>& pNf)
{
    remove(pNf->algorithm());
}

void AlgorithmMonitor::showDialog()
{
  if( !m_monitorDlg->isVisible() )
  {
    m_monitorDlg->setVisible(true);
    m_monitorDlg->update(m_nRunning);
  }
}

void AlgorithmMonitor::cancel(AlgorithmID id)
{
    IAlgorithm_sptr a = Mantid::API::AlgorithmManager::Instance().getAlgorithm(id);
    if (!a.get()) return;
    a->cancel();
}

void AlgorithmMonitor::cancelAll()
{
    const std::vector<IAlgorithm_sptr>& algs = Mantid::API::AlgorithmManager::Instance().algorithms();
    for(std::vector<IAlgorithm_sptr>::const_iterator a = algs.begin();a!=algs.end();a++)
        if ( std::find(m_algorithms.begin(),m_algorithms.end(),(**a).getAlgorithmID()) ) (**a).cancel();

}
 
//-----------------------------------------------------------------------------------------------//
MonitorDlg::MonitorDlg(QWidget *parent,AlgorithmMonitor *algMonitor):QDialog(parent),m_algMonitor(algMonitor)
{
    m_tree = 0;
    update(0);
    connect(algMonitor,SIGNAL(countChanged(int)),this,SLOT(update(int)), Qt::QueuedConnection);
    connect(algMonitor,SIGNAL(needUpdateProgress(const IAlgorithm*,int, const QString&)),SLOT(updateProgress(const IAlgorithm*,int, const QString&)));

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
    setWindowIcon(QIcon(":/MantidPlot_Icon_32offset.png"));
    resize(500,300);
}

MonitorDlg::~MonitorDlg() 
{
}

void MonitorDlg::update(int)
{
    if (!m_tree)
    {
      m_tree = new QTreeWidget(this);
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

    if( !isVisible() ) return;
    
    m_algMonitor->lock();
    QVector<Mantid::API::AlgorithmID>::const_iterator iend = m_algMonitor->algorithms().end();
    for(QVector<Mantid::API::AlgorithmID>::const_iterator itr = m_algMonitor->algorithms().begin(); 
	itr != iend; ++itr)
    {
      IAlgorithm_sptr alg = Mantid::API::AlgorithmManager::Instance().getAlgorithm(*itr);
      //m_algorithms << alg;
      QStringList iList;
      iList<<QString::fromStdString(alg->name());
      QTreeWidgetItem *algItem = new QTreeWidgetItem(iList);
      m_tree->addTopLevelItem(algItem);
      QProgressBar *algProgress = new QProgressBar;
      algProgress->setAlignment(Qt::AlignHCenter);
      AlgButton *cancelButton = new AlgButton("Cancel", alg);
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

        connect(cancelButton,SIGNAL(clicked(AlgorithmID)),m_algMonitor,SLOT(cancel(AlgorithmID)));
    }
    m_algMonitor->unlock();
}

void MonitorDlg::updateProgress(const IAlgorithm* alg,int p, const QString& msg)
{
  m_algMonitor->lock();
  int index = m_algMonitor->algorithms().indexOf(alg->getAlgorithmID());
  m_algMonitor->unlock();
  QTreeWidgetItem *item = m_tree->topLevelItem(index);
  if( !item ) return;
 
  QProgressBar *algProgress = static_cast<QProgressBar*>( m_tree->itemWidget(item, 1) );
  algProgress->setValue(p);
  algProgress->setFormat(msg + " %p%");
}
