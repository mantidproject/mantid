// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/DataProcessorUI/GenericDataProcessorPresenter.h"

#include <QMetaType>
#include <QThread>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

/**
GenericDataProcessorPresenterThread class to handle a single worker
and parent. The worker must establish its own connection for its
finished() signal however.
*/
class GenericDataProcessorPresenterThread : public QThread {
  Q_OBJECT

public:
  /// Constructor
  GenericDataProcessorPresenterThread(QObject *parent, QObject *worker) : QThread(parent), m_worker(worker) {
    // Establish connections between parent and worker
    connect(this, SIGNAL(started()), worker, SLOT(startWorker()));
    connect(worker, SIGNAL(finished(int)), this, SLOT(workerFinished(int)));
    // Early deletion of thread and worker
    connect(this, SIGNAL(finished()), this, SLOT(deleteLater()), Qt::DirectConnection);

    worker->moveToThread(this);
  }

public slots:
  void workerFinished(const int exitCode) {
    Q_UNUSED(exitCode);
    // queue worker for deletion
    m_worker->deleteLater();
  }

private:
  QObject *const m_worker;
};

} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
