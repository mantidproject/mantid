#ifndef MANTIDQTMANTIDWIDGETS_FINDFILESTHREADPOOLMANAGER_H_
#define MANTIDQTMANTIDWIDGETS_FINDFILESTHREADPOOLMANAGER_H_

#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/FindFilesWorker.h"

#include <QObject>
#include <QRunnable>
#include <QSharedPointer>
#include <QString>
#include <QThreadPool>
#include <functional>
#include <string>
#include <vector>
#include <memory>

namespace MantidQt {
namespace API {

/** A small helper class to hold a handle to a static thread pool.
 */
class EXPORT_OPT_MANTIDQT_COMMON FindFilesThreadPoolManager : public QObject {
  Q_OBJECT
  using ThreadAllocator =
      std::function<FindFilesWorker *(const FindFilesSearchParameters &)>;

public:
  /// Create a new thread pool manager for finding files
  FindFilesThreadPoolManager();
  /// Set the worker object allocator for this thread pool
  void setAllocator(ThreadAllocator allocator);

  /// Create a new worker thread. This will cancel any currently running threads
  void createWorker(const QObject *parent,
                    const FindFilesSearchParameters &parameters);
  /// Check if a search is already in progress
  bool isSearchRunning() const;
  /// Block execution and wait for all threads to finish processing
  void waitForDone();
  /// Destroy the static thread pool instance
  static void destroyThreadPool();

signals:
  // Signal emitted to cancel any already running workers
  void disconnectWorkers();

private slots:
  // Handle when a search is finished
  void searchFinished();

private:
  /// Get a handle to the static file finder thread pool instance
  const std::unique_ptr<QThreadPool> &poolInstance() const;
  /// Cancel the currently running worker
  void cancelWorker();
  /// Connect worker to relevant signals/slots
  void connectWorker(const QObject *parent, const FindFilesWorker *worker);

  /// Handle to the allocator function for creating new worker threads
  ThreadAllocator m_workerAllocator;
  /// Flag set if a search is currently running
  bool m_searchIsRunning;
};

} // namespace API
} // namespace MantidQt

#endif // MANTIDQTMANTIDWIDGETS_FINDFILESTHREADPOOLMANAGER_H_
