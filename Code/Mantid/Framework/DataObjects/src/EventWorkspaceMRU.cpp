#include "MantidDataObjects/EventWorkspaceMRU.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataObjects {

using Mantid::Kernel::Mutex;

//----------------------------------------------------------------------------------------------
/** Constructor
 */
EventWorkspaceMRU::EventWorkspaceMRU() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
EventWorkspaceMRU::~EventWorkspaceMRU() {
  // Make sure you free up the memory in the MRUs
  for (size_t i = 0; i < m_bufferedDataY.size(); i++) {
    if (m_bufferedDataY[i]) {
      m_bufferedDataY[i]->clear();
      delete m_bufferedDataY[i];
    }
  }

  for (size_t i = 0; i < m_bufferedDataE.size(); i++) {
    if (m_bufferedDataE[i]) {
      m_bufferedDataE[i]->clear();
      delete m_bufferedDataE[i];
    }
  }

  for (size_t i = 0; i < m_markersToDelete.size(); i++) {
    delete m_markersToDelete[i];
  }
}

//---------------------------------------------------------------------------
/** This function makes sure that there are enough data
 * buffers (MRU's) for E for the number of threads requested.
 * @param thread_num :: thread number that wants a MRU buffer
 */
void EventWorkspaceMRU::ensureEnoughBuffersE(size_t thread_num) const {
  Mutex::ScopedLock _lock(m_changeMruListsMutexE);
  if (m_bufferedDataE.size() <= thread_num) {
    m_bufferedDataE.resize(thread_num + 1, NULL);
    for (size_t i = 0; i < m_bufferedDataE.size(); i++) {
      if (!m_bufferedDataE[i])
        m_bufferedDataE[i] =
            new mru_list(50); // Create a MRU list with this many entries.
    }
  }
}
//---------------------------------------------------------------------------
/** This function makes sure that there are enough data
 * buffers (MRU's) for Y for the number of threads requested.
 * @param thread_num :: thread number that wants a MRU buffer
 */
void EventWorkspaceMRU::ensureEnoughBuffersY(size_t thread_num) const {
  Mutex::ScopedLock _lock(m_changeMruListsMutexY);
  if (m_bufferedDataY.size() <= thread_num) {
    m_bufferedDataY.resize(thread_num + 1, NULL);
    for (size_t i = 0; i < m_bufferedDataY.size(); i++) {
      if (!m_bufferedDataY[i])
        m_bufferedDataY[i] =
            new mru_list(50); // Create a MRU list with this many entries.
    }
  }
}

//---------------------------------------------------------------------------
/// Clear all the data in the MRU buffers
void EventWorkspaceMRU::clear() {
  Mutex::ScopedLock _lock(this->m_toDeleteMutex);

  // FIXME: don't clear the locked ones!
  for (size_t i = 0; i < m_markersToDelete.size(); i++)
    if (!m_markersToDelete[i]->m_locked)
      delete m_markersToDelete[i];
  m_markersToDelete.clear();

  // Make sure you free up the memory in the MRUs
  for (size_t i = 0; i < m_bufferedDataY.size(); i++)
    if (m_bufferedDataY[i]) {
      m_bufferedDataY[i]->clear();
    };

  for (size_t i = 0; i < m_bufferedDataE.size(); i++)
    if (m_bufferedDataE[i]) {
      m_bufferedDataE[i]->clear();
    };
}

//---------------------------------------------------------------------------
/** Find a Y histogram in the MRU
 *
 * @param thread_num :: number of the thread in which this is run
 * @param index :: index of the data to return
 * @return pointer to the MantidVecWithMarker that has the data; NULL if not
 *found.
 */
MantidVecWithMarker *EventWorkspaceMRU::findY(size_t thread_num, size_t index) {
  Mutex::ScopedLock _lock(m_changeMruListsMutexY);
  return m_bufferedDataY[thread_num]->find(index);
}

/** Find a Y histogram in the MRU
 *
 * @param thread_num :: number of the thread in which this is run
 * @param index :: index of the data to return
 * @return pointer to the MantidVecWithMarker that has the data; NULL if not
 *found.
 */
MantidVecWithMarker *EventWorkspaceMRU::findE(size_t thread_num, size_t index) {
  Mutex::ScopedLock _lock(m_changeMruListsMutexE);
  return m_bufferedDataE[thread_num]->find(index);
}

/** Insert a new histogram into the MRU
 *
 * @param thread_num :: thread being accessed
 * @param data :: the new data
 * @return a MantidVecWithMarker * that needs to be deleted, or NULL if nothing
 *needs to be deleted.
 */
void EventWorkspaceMRU::insertY(size_t thread_num, MantidVecWithMarker *data) {
  Mutex::ScopedLock _lock(m_changeMruListsMutexY);
  MantidVecWithMarker *oldData = m_bufferedDataY[thread_num]->insert(data);
  // And clear up the memory of the old one, if it is dropping out.
  if (oldData) {
    if (oldData->m_locked) {
      Mutex::ScopedLock _lock(this->m_toDeleteMutex);
      m_markersToDelete.push_back(oldData);
    } else
      delete oldData;
  }
}

/** Insert a new histogram into the MRU
 *
 * @param thread_num :: thread being accessed
 * @param data :: the new data
 * @return a MantidVecWithMarker * that needs to be deleted, or NULL if nothing
 *needs to be deleted.
 */
void EventWorkspaceMRU::insertE(size_t thread_num, MantidVecWithMarker *data) {
  Mutex::ScopedLock _lock(m_changeMruListsMutexE);
  MantidVecWithMarker *oldData = m_bufferedDataE[thread_num]->insert(data);
  // And clear up the memory of the old one, if it is dropping out.
  if (oldData) {
    if (oldData->m_locked) {
      Mutex::ScopedLock _lock(this->m_toDeleteMutex);
      m_markersToDelete.push_back(oldData);
    } else
      delete oldData;
  }
}

/** Delete any entries in the MRU at the given index
 *
 * @param index :: index to delete.
 */
void EventWorkspaceMRU::deleteIndex(size_t index) {
  Mutex::ScopedLock _lock1(m_changeMruListsMutexE);
  for (size_t i = 0; i < m_bufferedDataE.size(); i++)
    if (m_bufferedDataE[i])
      m_bufferedDataE[i]->deleteIndex(index);
  Mutex::ScopedLock _lock2(m_changeMruListsMutexY);
  for (size_t i = 0; i < m_bufferedDataY.size(); i++)
    if (m_bufferedDataY[i])
      m_bufferedDataY[i]->deleteIndex(index);
}

} // namespace Mantid
} // namespace DataObjects
