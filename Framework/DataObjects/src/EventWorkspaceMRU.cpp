// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/EventWorkspaceMRU.h"

#include <algorithm>

namespace Mantid::DataObjects {

EventWorkspaceMRU::~EventWorkspaceMRU() {
  // Make sure you free up the memory in the MRUs
  {
    Poco::ScopedWriteRWLock _lock(m_changeMruListsMutexY);
    for (auto &data : m_bufferedDataY) {
      if (data) {
        data->clear();
      }
    }
  }
  Poco::ScopedWriteRWLock _lock2(m_changeMruListsMutexE);
  for (auto &data : m_bufferedDataE) {
    if (data) {
      data->clear();
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
    m_bufferedDataE.resize(thread_num + 1);
    // Replaces empty data with an MRU list with this many entries.
    std::transform(m_bufferedDataE.begin(), m_bufferedDataE.end(), m_bufferedDataE.begin(),
                   [](auto &data) { return data ? std::move(data) : std::move(std::make_unique<mru_listE>(50)); });
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
    m_bufferedDataY.resize(thread_num + 1);
    // Replaces empty data with an MRU list with this many entries.
    std::transform(m_bufferedDataY.begin(), m_bufferedDataY.end(), m_bufferedDataY.begin(),
                   [](auto &data) { return data ? std::move(data) : std::move(std::make_unique<mru_listY>(50)); });
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
Kernel::cow_ptr<HistogramData::HistogramY> EventWorkspaceMRU::findY(size_t thread_num, const EventList *index) {
  Poco::ScopedReadRWLock _lock(m_changeMruListsMutexY);
  auto result = m_bufferedDataY[thread_num]->find(reinterpret_cast<std::uintptr_t>(index));
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
Kernel::cow_ptr<HistogramData::HistogramE> EventWorkspaceMRU::findE(size_t thread_num, const EventList *index) {
  Poco::ScopedReadRWLock _lock(m_changeMruListsMutexE);
  auto result = m_bufferedDataE[thread_num]->find(reinterpret_cast<std::uintptr_t>(index));
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
void EventWorkspaceMRU::insertY(size_t thread_num, YType data, const EventList *index) {
  Poco::ScopedReadRWLock _lock(m_changeMruListsMutexY);
  auto yWithMarker = std::make_shared<TypeWithMarker<YType>>(reinterpret_cast<std::uintptr_t>(index));
  yWithMarker->m_data = std::move(data);
  m_bufferedDataY[thread_num]->insert(yWithMarker);
  // the memory is cleared automatically due to being a smart_ptr
}

/** Insert a new histogram into the MRU
 *
 * @param thread_num :: thread being accessed
 * @param data :: the new data
 * @param index :: index of the data to insert
 */
void EventWorkspaceMRU::insertE(size_t thread_num, EType data, const EventList *index) {
  Poco::ScopedReadRWLock _lock(m_changeMruListsMutexE);
  auto eWithMarker = std::make_shared<TypeWithMarker<EType>>(reinterpret_cast<std::uintptr_t>(index));
  eWithMarker->m_data = std::move(data);
  m_bufferedDataE[thread_num]->insert(eWithMarker);
  // And clear up the memory of the old one, if it is dropping out.
}

/** Delete any entries in the MRU at the given index
 *
 * @param index :: index to delete.
 */
void EventWorkspaceMRU::deleteIndex(const EventList *index) {
  // acquire lock if the buffer isn't empty
  if (!m_bufferedDataE.empty()) {
    Poco::ScopedReadRWLock _lock1(m_changeMruListsMutexE);
    for (auto &data : m_bufferedDataE) {
      if (data) {
        data->deleteIndex(reinterpret_cast<std::uintptr_t>(index));
      }
    }
  }
  // acquire lock if the buffer isn't empty
  if ((!m_bufferedDataY.empty())) {
    Poco::ScopedReadRWLock _lock2(m_changeMruListsMutexY);
    for (auto &data : m_bufferedDataY) {
      if (data) {
        data->deleteIndex(reinterpret_cast<std::uintptr_t>(index));
      }
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

} // namespace Mantid::DataObjects
