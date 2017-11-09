#ifndef MANTIDQTMANTIDWIDGETS_FINDFILESTHREADPOOLMANAGER_H_
#define MANTIDQTMANTIDWIDGETS_FINDFILESTHREADPOOLMANAGER_H_

#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/FindFilesWorker.h"

#include <QObject>
#include <QSharedPointer>
#include <QRunnable>
#include <QString>
#include <QThreadPool>
#include <functional>
#include <string>
#include <vector>

namespace MantidQt {
namespace API {

/** A small helper class to hold a handle to a static thread pool.
 */
class EXPORT_OPT_MANTIDQT_COMMON FindFilesThreadPoolManager : public QObject {
	Q_OBJECT
  typedef std::function<FindFilesWorker *(const FindFilesSearchParameters &)>
      ThreadAllocator;

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

signals:
  // Signal emitted to cancel any already running workers
  void disconnectWorkers();

private slots:
  // Handle when a search is finished
  void searchFinished();

private:
  /// Cancel the currently running worker
  void cancelWorker(const QObject *parent);
  /// Connect worker to relevant signals/slots
  void connectWorker(const QObject* parent, const FindFilesWorker* worker);

  /// Handle to a local QThread pool
  static QThreadPool m_pool;
  /// Handle to the allocator function for creating new worker threads
  ThreadAllocator m_workerAllocator;
  /// Flag set if a search is currently running
  bool m_searchIsRunning;
};

} // namespace API
} // namespace MantidQt

#endif // MANTIDQTMANTIDWIDGETS_FINDFILESTHREADPOOLMANAGER_H_
