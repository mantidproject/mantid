#ifndef MANTIDQTMANTIDWIDGETS_GENERICDATAPROCESSORPRESENTER_GENERICDATAPROCESSORPRESENTERTHREAD_H
#define MANTIDQTMANTIDWIDGETS_GENERICDATAPROCESSORPRESENTER_GENERICDATAPROCESSORPRESENTERTHREAD_H

#include "MantidQtWidgets/Common/DataProcessorUI/GenericDataProcessorPresenter.h"

#include <QThread>
#include <QMetaType>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

/**
GenericDataProcessorPresenterThread class to handle a single worker
and parent. The worker must establish its own connection for its
finished() signal however.

Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD
Oak Ridge National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class GenericDataProcessorPresenterThread : public QThread {
  Q_OBJECT

public:
  /// Constructor
  GenericDataProcessorPresenterThread(QObject *parent, QObject *worker)
      : QThread(parent), m_worker(worker) {
    // Establish connections between parent and worker
    connect(this, SIGNAL(started()), worker, SLOT(startWorker()));
    connect(worker, SIGNAL(finished(int)), this, SLOT(workerFinished(int)));
    // Early deletion of thread and worker
    connect(this, SIGNAL(finished()), this, SLOT(deleteLater()),
            Qt::DirectConnection);

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

#endif // MANTIDQTMANTIDWIDGETS_GENERICDATAPROCESSORPRESENTER_GENERICDATAPROCESSORPRESENTERTHREAD_H
