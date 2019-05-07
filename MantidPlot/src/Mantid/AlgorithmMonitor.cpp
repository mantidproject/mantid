// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "AlgorithmMonitor.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/MaskedProperty.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidUI.h"

#include <QThread>
#include <QtGui>

#include <algorithm>

using namespace std;
using namespace Mantid::API;

QMutex AlgorithmMonitor::s_mutex;

//-----------------------------------------------------------------------------
/** Constructor */
AlgorithmMonitor::AlgorithmMonitor(MantidUI *m)
    : m_finishedObserver(
          *this, &AlgorithmMonitor::handleAlgorithmFinishedNotification),
      m_progressObserver(
          *this, &AlgorithmMonitor::handleAlgorithmProgressNotification),
      m_errorObserver(*this,
                      &AlgorithmMonitor::handleAlgorithmErrorNotification),
      m_startingObserver(
          *this, &AlgorithmMonitor::handleAlgorithmStartingNotification),
      m_mantidUI(m), m_nRunning(0) {
  AlgorithmManager::Instance().notificationCenter.addObserver(
      m_startingObserver);
  m_monitorDlg = new MonitorDlg(m_mantidUI->appWindow(), this);
  m_monitorDlg->setVisible(false);
}

//-----------------------------------------------------------------------------
/** Destructor */
AlgorithmMonitor::~AlgorithmMonitor() {
  if (m_monitorDlg) {
    m_monitorDlg->close();
    delete m_monitorDlg;
  }
  cancelAll();
  wait(1000);
  exit();
  wait();
  AlgorithmManager::Instance().notificationCenter.removeObserver(
      m_startingObserver);
}

//-----------------------------------------------------------------------------
/** Add a new algorithm to the list monitored
 *
 * @param alg :: algorithm to monitor.
 */
void AlgorithmMonitor::add(Mantid::API::IAlgorithm_sptr alg) {
  lock();
  alg->addObserver(m_finishedObserver);
  alg->addObserver(m_errorObserver);
  alg->addObserver(m_progressObserver);
  m_algorithms.push_back(alg->getAlgorithmID());
  ++m_nRunning;
  emit algorithmStarted(alg->getAlgorithmID());
  emit countChanged();
  unlock();
}

//-----------------------------------------------------------------------------
/** Remove an algorithm from the list monitored
 *
 * @param alg :: algorithm.
 */
void AlgorithmMonitor::remove(const IAlgorithm *alg) {
  lock();
  QVector<AlgorithmID>::iterator i =
      find(m_algorithms.begin(), m_algorithms.end(), alg->getAlgorithmID());
  if (i != m_algorithms.end()) {
    m_algorithms.erase(i);
    --m_nRunning;
  }
  emit algorithmFinished(alg->getAlgorithmID());
  emit countChanged();
  if (m_algorithms.empty())
    emit allAlgorithmsStopped();
  unlock();
}

void AlgorithmMonitor::update() {}

void AlgorithmMonitor::handleAlgorithmFinishedNotification(
    const Poco::AutoPtr<Algorithm::FinishedNotification> &pNf) {
  remove(pNf->algorithm());
}

void AlgorithmMonitor::handleAlgorithmProgressNotification(
    const Poco::AutoPtr<Algorithm::ProgressNotification> &pNf) {
  emit needUpdateProgress(pNf->algorithm()->getAlgorithmID(),
                          static_cast<double>(pNf->progress * 100),
                          QString::fromStdString(pNf->message),
                          double(pNf->estimatedTime),
                          int(pNf->progressPrecision));
}

void AlgorithmMonitor::handleAlgorithmErrorNotification(
    const Poco::AutoPtr<Algorithm::ErrorNotification> &pNf) {
  remove(pNf->algorithm());
}

//-----------------------------------------------------------------------------
/** Observer called when the AlgorithmManager reports that an algorithm
 * is starting asynchronously.
 * Adds the algorithm to the list.
 *
 * @param pNf :: notification object
 */
void AlgorithmMonitor::handleAlgorithmStartingNotification(
    const Poco::AutoPtr<Mantid::API::AlgorithmStartingNotification> &pNf) {
  add(pNf->getAlgorithm());
}

//-----------------------------------------------------------------------------
/** Slot called to show the monitor dialog */
void AlgorithmMonitor::showDialog() {
  if (!m_monitorDlg->isVisible()) {
    m_monitorDlg->setVisible(true);
    m_monitorDlg->update();
  }
}

//-----------------------------------------------------------------------------
/** Cancel the given algorithm's execution */
void AlgorithmMonitor::cancel(Mantid::API::AlgorithmID id,
                              QPushButton *cancelBtn = nullptr) {
  if ((cancelBtn) && (cancelBtn->text() == "Cancel")) {
    cancelBtn->setText("Cancelling");
    cancelBtn->setEnabled(false);
    IAlgorithm_sptr a =
        Mantid::API::AlgorithmManager::Instance().getAlgorithm(id);
    if (!a.get())
      return;
    a->cancel();
  }
}

//-----------------------------------------------------------------------------
/** Cancel all running algorithms */
void AlgorithmMonitor::cancelAll() {
  // Forward to the AlgorithmManager
  AlgorithmManager::Instance().cancelAll();
}

//-----------------------------------------------------------------------------------------------//
MonitorDlg::MonitorDlg(QWidget *parent, AlgorithmMonitor *algMonitor)
    : QDialog(parent), m_algMonitor(algMonitor) {
  m_tree = nullptr;
  update();
  connect(algMonitor, SIGNAL(countChanged()), this, SLOT(update()),
          Qt::QueuedConnection);
  connect(
      algMonitor,
      SIGNAL(needUpdateProgress(void *, double, const QString &, double, int)),
      SLOT(updateProgress(void *, double, const QString &, double, int)));

  QHBoxLayout *buttonLayout = new QHBoxLayout;
  QPushButton *closeButton = new QPushButton("Close");
  connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
  buttonLayout->addStretch();
  buttonLayout->addWidget(closeButton);

  QVBoxLayout *layout = new QVBoxLayout;
  layout->addWidget(m_tree);
  layout->addLayout(buttonLayout);
  setLayout(layout);
  setWindowTitle("Mantid - Algorithm progress");
  setWindowIcon(QIcon(":/mantidplot.png"));
  resize(500, 300);
}

MonitorDlg::~MonitorDlg() {}

void MonitorDlg::update() {
  if (!m_tree) {
    m_tree = new QTreeWidget(this);
    m_tree->setColumnCount(3);
    m_tree->setSelectionMode(QAbstractItemView::NoSelection);
    // Make the algorithm name column wider
    m_tree->setColumnWidth(0, 220);
    QStringList hList;
    hList << "Algorithm"
          << "Progress"
          << "";
    m_tree->setHeaderLabels(hList);
    QHeaderView *hHeader = (QHeaderView *)m_tree->header();
    hHeader->setResizeMode(1, QHeaderView::Stretch);
    hHeader->setResizeMode(2, QHeaderView::Fixed);
    hHeader->setStretchLastSection(false);
  } else
    m_tree->clear();

  if (!isVisible())
    return;

  m_algMonitor->lock();
  QVector<Mantid::API::AlgorithmID>::const_iterator iend =
      m_algMonitor->algorithms().end();
  for (QVector<Mantid::API::AlgorithmID>::const_iterator itr =
           m_algMonitor->algorithms().begin();
       itr != iend; ++itr) {
    IAlgorithm_sptr alg =
        Mantid::API::AlgorithmManager::Instance().getAlgorithm(*itr);
    if (!alg) {
      continue;
    }
    // m_algorithms << alg;
    QStringList iList;
    iList << QString::fromStdString(alg->name());
    QTreeWidgetItem *algItem = new QTreeWidgetItem(iList);
    m_tree->addTopLevelItem(algItem);
    QProgressBar *algProgress = new QProgressBar;
    algProgress->setAlignment(Qt::AlignHCenter);
    AlgButton *cancelButton = new AlgButton("Cancel", alg);
    m_tree->setItemWidget(algItem, 1, algProgress);
    m_tree->setItemWidget(algItem, 2, cancelButton);
    const std::vector<Mantid::Kernel::Property *> &prop_list =
        alg->getProperties();
    for (std::vector<Mantid::Kernel::Property *>::const_iterator prop =
             prop_list.begin();
         prop != prop_list.end(); ++prop) {
      QStringList lstr;
      Mantid::Kernel::MaskedProperty<std::string> *maskedProp =
          dynamic_cast<Mantid::Kernel::MaskedProperty<std::string> *>(*prop);
      if (maskedProp) {
        lstr << QString::fromStdString(maskedProp->name()) + ": "
             << QString::fromStdString(maskedProp->getMaskedValue());
      } else {
        lstr << QString::fromStdString((**prop).name()) + ": "
             << QString::fromStdString((**prop).value());
      }
      if ((**prop).isDefault())
        lstr << " Default";
      algItem->addChild(new QTreeWidgetItem(lstr));
    }

    connect(
        cancelButton, SIGNAL(clicked(Mantid::API::AlgorithmID, QPushButton *)),
        m_algMonitor, SLOT(cancel(Mantid::API::AlgorithmID, QPushButton *)));
  }
  m_algMonitor->unlock();
}

// The void* corresponds to Mantid::API::AlgorithmID, but Qt wasn't coping with
// the typedef
void MonitorDlg::updateProgress(void *alg, const double p, const QString &msg,
                                double /*estimatedTime*/,
                                int /*progressPrecision*/) {
  m_algMonitor->lock();
  const int index = m_algMonitor->algorithms().indexOf(alg);
  m_algMonitor->unlock();
  if (index == -1)
    return;
  QTreeWidgetItem *item = m_tree->topLevelItem(index);
  if (!item)
    return;

  QProgressBar *algProgress =
      static_cast<QProgressBar *>(m_tree->itemWidget(item, 1));
  algProgress->setValue(int(p));
  algProgress->setFormat(msg + " %p%");
}
