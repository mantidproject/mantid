#include "MantidQtWidgets/Common/FindFilesThreadPoolManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/VectorHelper.h"

#include <Poco/File.h>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace MantidQt::API;

QThreadPool FindFilesThreadPoolManager::m_pool;

FindFilesThreadPoolManager::FindFilesThreadPoolManager()
    : m_currentWorker{nullptr} {
  m_workerAllocator = [](const FindFilesSearchParameters &parameters) {
    return new FindFilesWorker(parameters);
  };
}

/** Set the allocator function for the thread pool.
 *
 * This is currently used for unit testing so that a fake QRunnable object
 * can be used in place of a real worker.
 *
 * @param allocator :: The thread allocator function to user to create new
 * worker objects
 */
void FindFilesThreadPoolManager::setAllocator(ThreadAllocator allocator) {
  m_workerAllocator = allocator;
}

void FindFilesThreadPoolManager::createWorker(
    const QObject *parent, const FindFilesSearchParameters &parameters) {
  cancelWorker(parent);

  // if parent is null then don't do anything as there will be no
  // object listening for the search result
  if (!parent)
    return;

  m_currentWorker = m_workerAllocator(parameters);

  // Hook up slots for when the thread finishes. By default Qt uses queued
  // connections when connecting signals/slots between threads. Instead here
  // we explicitly choose to use a direct connection so the found result is
  // immediately returned to the GUI thread.
  parent->connect(m_currentWorker,
                  SIGNAL(finished(const FindFilesSearchResults &)), parent,
                  SLOT(inspectThreadResult(const FindFilesSearchResults &)),
                  Qt::DirectConnection);
  parent->connect(m_currentWorker,
                  SIGNAL(finished(const FindFilesSearchResults &)), parent,
                  SIGNAL(fileFindingFinished()), Qt::DirectConnection);

  // pass ownership to the thread pool
  // we do not need to worry about deleting m_currentWorker
  m_pool.start(m_currentWorker);
}

/** Cancel the currently running worker
 *
 * This will disconnect any signals from the worker and then let the
 * internal QThreadPool clean up the worker at a later time.
 *
 * @param parent :: the parent widget to disconnect signals for.
 */
void FindFilesThreadPoolManager::cancelWorker(const QObject *parent) {
  if (!isSearchRunning())
    return;

  // Just disconnect any signals from the worker. We leave the worker to
  // continue running in the background because 1) terminating it directly
  // is dangerous (we have no idea what it's currently doing from here) and 2)
  // waiting for it to finish before starting a new thread locks up the GUI
  // event loop.
  m_currentWorker->disconnect(parent);
  m_currentWorker = nullptr;
}

/** Check if a search is currently executing.
 *
 * @returns true if the current worker object is null
 */
bool FindFilesThreadPoolManager::isSearchRunning() const {
  return m_currentWorker != nullptr;
}

/** Wait for all threads in the pool to finish running.
 *
 * Warning: This call will block execution unitl ALL threads in the pool
 * have finished executing. Using this in a GUI thread will cause the GUI
 * to freeze up.
 */
void FindFilesThreadPoolManager::waitForDone() {
  m_pool.waitForDone();
  m_currentWorker = nullptr;
}
