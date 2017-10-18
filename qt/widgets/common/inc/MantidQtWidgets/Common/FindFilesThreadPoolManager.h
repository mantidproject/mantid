#ifndef MANTIDQTMANTIDWIDGETS_FINDFILESTHREADPOOLMANAGER_H_
#define MANTIDQTMANTIDWIDGETS_FINDFILESTHREADPOOLMANAGER_H_

#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/FindFilesWorker.h"

#include <QRunnable>
#include <QString>
#include <QThreadPool>
#include <functional>
#include <string>
#include <vector>

namespace MantidQt {
namespace API {

/**
 * A small helper class to hold the thread pool
 */
class EXPORT_OPT_MANTIDQT_COMMON FindFilesThreadPoolManager {
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

private:
  /// Cancel the currently running thread
  void cancelWorker(const QObject *parent);

  /// Handle to the currently executing worker thread
  FindFilesWorker *m_currentWorker;
  /// Handle to a local QThread pool
  static QThreadPool m_pool;
  /// Handle to the allocator function for creating new worker threads
  ThreadAllocator m_workerAllocator;
};

} // namespace API
} // namespace MantidQt

#endif // MANTIDQTMANTIDWIDGETS_FINDFILESTHREADPOOLMANAGER_H_
