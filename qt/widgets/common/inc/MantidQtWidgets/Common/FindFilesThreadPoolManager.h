#ifndef MANTIDQTMANTIDWIDGETS_FINDFILESTHREADPOOLMANAGER_H_
#define MANTIDQTMANTIDWIDGETS_FINDFILESTHREADPOOLMANAGER_H_

#include "MantidQtWidgets/Common/FindFilesWorker.h"

#include <QThreadPool>
#include <QRunnable>
#include <QString>
#include <string>
#include <vector>
#include <functional>

namespace MantidQt {
namespace API {

/**
 * A small helper class to hold the thread pool
 */
class FindFilesThreadPoolManager {
  typedef std::function<FindFilesWorker*(const FindFilesSearchParameters&)> ThreadAllocator;

public:
  FindFilesThreadPoolManager();
  void setAllocator(ThreadAllocator allocator);;

  void createWorker(const QObject* parent,
                    const FindFilesSearchParameters& parameters);
  void cancelWorker(const QObject *parent);
  bool isSearchRunning() const;
  void waitForDone() const;

private:
  FindFilesWorker* m_currentWorker;
  static QThreadPool m_pool;
  ThreadAllocator m_workerAllocator;
};

}
}

#endif // MANTIDQTMANTIDWIDGETS_FINDFILESTHREADPOOLMANAGER_H_
