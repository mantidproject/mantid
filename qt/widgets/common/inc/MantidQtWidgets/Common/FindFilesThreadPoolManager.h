// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/FindFilesWorker.h"

#include <QObject>
#include <QRunnable>
#include <QSharedPointer>
#include <QString>
#include <QThreadPool>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace MantidQt {
namespace API {

/** A small helper class to hold a handle to a static thread pool.
 */
class EXPORT_OPT_MANTIDQT_COMMON FindFilesThreadPoolManager : public QObject {
  Q_OBJECT
  using ThreadAllocator = std::function<FindFilesWorker *(const FindFilesSearchParameters &)>;

public:
  /// Create a new thread pool manager for finding files
  FindFilesThreadPoolManager();
  /// Set the worker object allocator for this thread pool
  void setAllocator(ThreadAllocator allocator);

  /// Create a new worker thread. This will cancel any currently running threads
  void createWorker(const QObject *parent, const FindFilesSearchParameters &parameters);
  /// Check if a search is already in progress
  bool isSearchRunning() const;
  /// Block execution and wait for all threads to finish processing
  void waitForDone();
  /// Destroy the static thread pool instance
  static void destroyThreadPool();

signals:
  // Signal emitted to cancel any already running workers
  void disconnectWorkers();

private:
  /// Get a handle to the static file finder thread pool instance
  const std::unique_ptr<QThreadPool> &poolInstance() const;
  /// Cancel the currently running worker
  void cancelWorker();
  /// Connect worker to relevant signals/slots
  void connectWorker(const QObject *parent, const FindFilesWorker *worker);

  /// Handle to the allocator function for creating new worker threads
  ThreadAllocator m_workerAllocator;
};

} // namespace API
} // namespace MantidQt
