// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/EventWorkspaceMRU.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace DataObjects {

EventWorkspaceMRU::~EventWorkspaceMRU() {
  // Make sure you free up the memory in the MRUs
  {
    Poco::ScopedWriteRWLock _lock(m_changeMruListsMutexY);
    for (auto &data : m_bufferedDataY) {
      if (data) {
        data->clear();
        delete data;
      }
    }
  }
  Poco::ScopedWriteRWLock _lock2(m_changeMruListsMutexE);
  for (auto &data : m_bufferedDataE) {
    if (data) {
      data->clear();
      delete data;
    }
  }
}

//---------------------------------------------------------------------------
/** This function makes sure that there are enough data
 * buffers (MRU's) for E for the number of threads requested.
 * @param thread_num :: thread number that wants a MRU buffer
 */
void EventWorkspaceMRU::ensureEnoughBuffersE(size_t thread_num) const {
  Poco::ScopedWriteRWLock _lock(m_changeMruListsMutexE);
  if (m_bufferedDataE.size() <= thread_num) {
    m_bufferedDataE.resize(thread_num + 1, nullptr);
    for (auto &data : m_bufferedDataE) {
      if (!data)
        data = new mru_listE(50); // Create a MRU list with this many entries.
    }
  }
}
//---------------------------------------------------------------------------
/** This function makes sure that there are enough data
 * buffers (MRU's) for Y for the number of threads requested.
 * @param thread_num :: thread number that wants a MRU buffer
 */
void EventWorkspaceMRU::ensureEnoughBuffersY(size_t thread_num) const {
  Poco::ScopedWriteRWLock _lock(m_changeMruListsMutexY);
  if (m_bufferedDataY.size() <= thread_num) {
    m_bufferedDataY.resize(thread_num + 1, nullptr);
    for (auto &data : m_bufferedDataY) {
      if (!data)
        data = new mru_listY(50); // Create a MRU list with this many entries.
    }
  }
}

//---------------------------------------------------------------------------
/// Clear all the data in the MRU buffers
void EventWorkspaceMRU::clear() {
  {
    // Make sure you free up the memory in the MRUs
    Poco::ScopedWriteRWLock _lock(m_changeMruListsMutexY);
    for (auto &data : m_bufferedDataY) {
      if (data) {
        data->clear();
      }
    }
  }

  Poco::ScopedWriteRWLock _lock(m_changeMruListsMutexE);
  for (auto &data : m_bufferedDataE) {
    if (data) {
      data->clear();
    }
  }
}

//---------------------------------------------------------------------------
/** Find a Y histogram in the MRU
 *
 * @param thread_num :: number of the thread in which this is run
 * @param index :: index of the data to return
 * @return pointer to the TypeWithMarker that has the data; NULL if not found.
 */
Kernel::cow_ptr<HistogramData::HistogramY>
EventWorkspaceMRU::findY(size_t thread_num, const EventList *index) {
  Poco::ScopedReadRWLock _lock(m_changeMruListsMutexY);
  auto result = m_bufferedDataY[thread_num]->find(
      reinterpret_cast<std::uintptr_t>(index));
  if (result)
    return result->m_data;
  return YType(nullptr);
}

/** Find a Y histogram in the MRU
 *
 * @param thread_num :: number of the thread in which this is run
 * @param index :: index of the data to return
 * @return pointer to the TypeWithMarker that has the data; NULL if not found.
 */
Kernel::cow_ptr<HistogramData::HistogramE>
EventWorkspaceMRU::findE(size_t thread_num, const EventList *index) {
  Poco::ScopedReadRWLock _lock(m_changeMruListsMutexE);
  auto result = m_bufferedDataE[thread_num]->find(
      reinterpret_cast<std::uintptr_t>(index));
  if (result)
    return result->m_data;
  return EType(nullptr);
}

/** Insert a new histogram into the MRU
 *
 * @param thread_num :: thread being accessed
 * @param data :: the new data
 * @param index :: index of the data to insert
 */
void EventWorkspaceMRU::insertY(size_t thread_num, YType data,
                                const EventList *index) {
  Poco::ScopedReadRWLock _lock(m_changeMruListsMutexY);
  auto yWithMarker =
      new TypeWithMarker<YType>(reinterpret_cast<std::uintptr_t>(index));
  yWithMarker->m_data = std::move(data);
  auto oldData = m_bufferedDataY[thread_num]->insert(yWithMarker);
  // And clear up the memory of the old one, if it is dropping out.
  delete oldData;
}

/** Insert a new histogram into the MRU
 *
 * @param thread_num :: thread being accessed
 * @param data :: the new data
 * @param index :: index of the data to insert
 */
void EventWorkspaceMRU::insertE(size_t thread_num, EType data,
                                const EventList *index) {
  Poco::ScopedReadRWLock _lock(m_changeMruListsMutexE);
  auto eWithMarker =
      new TypeWithMarker<EType>(reinterpret_cast<std::uintptr_t>(index));
  eWithMarker->m_data = std::move(data);
  auto oldData = m_bufferedDataE[thread_num]->insert(eWithMarker);
  // And clear up the memory of the old one, if it is dropping out.
  delete oldData;
}

/** Delete any entries in the MRU at the given index
 *
 * @param index :: index to delete.
 */
void EventWorkspaceMRU::deleteIndex(const EventList *index) {
  {
    Poco::ScopedReadRWLock _lock1(m_changeMruListsMutexE);
    for (auto &data : m_bufferedDataE) {
      if (data) {
        data->deleteIndex(reinterpret_cast<std::uintptr_t>(index));
      }
    }
  }
  Poco::ScopedReadRWLock _lock2(m_changeMruListsMutexY);
  for (auto &data : m_bufferedDataY) {
    if (data) {
      data->deleteIndex(reinterpret_cast<std::uintptr_t>(index));
    }
  }
}

size_t EventWorkspaceMRU::MRUSize() const {
  if (m_bufferedDataY.empty()) {
    return 0;
  } else {
    return this->m_bufferedDataY.front()->size();
  }
}

} // namespace DataObjects
} // namespace Mantid
