// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/DllConfig.h"
#include "MantidHistogramData/HistogramE.h"
#include "MantidHistogramData/HistogramY.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/MRUList.h"
#include "MantidKernel/cow_ptr.h"

#include "Poco/RWLock.h"

#include <cstdint>
#include <vector>

namespace Mantid {
namespace DataObjects {

class EventList;

//============================================================================
//============================================================================
/**
 * This little class holds data and an index marker that is used for uniqueness.
 * This is used in the MRUList.
 */
template <class T> class TypeWithMarker {
public:
  /**
   * Constructor.
   * @param the_index :: unique index into the workspace of this data
   */
  TypeWithMarker(const uintptr_t the_index) : m_index(the_index) {}
  TypeWithMarker(const TypeWithMarker &other) = delete;
  TypeWithMarker &operator=(const TypeWithMarker &other) = delete;

public:
  /// Unique index value.
  uintptr_t m_index;

  /// Pointer to a vector of data
  T m_data;

  /// Function returns a unique index, used for hashing for MRU list
  uintptr_t hashIndexFunction() const { return m_index; }

  /// Set the unique index value.
  void setIndex(const uintptr_t the_index) { m_index = the_index; }
};

//============================================================================
//============================================================================
/** This is a container for the MRU (most-recently-used) list
 * of generated histograms.
 */
class MANTID_DATAOBJECTS_DLL EventWorkspaceMRU {
public:
  using YType = Kernel::cow_ptr<HistogramData::HistogramY>;
  using EType = Kernel::cow_ptr<HistogramData::HistogramE>;
  using YWithMarker = TypeWithMarker<YType>;
  using EWithMarker = TypeWithMarker<EType>;
  // Typedef for a Most-Recently-Used list of Data objects.
  using mru_listY = Kernel::MRUList<YWithMarker>;
  using mru_listE = Kernel::MRUList<EWithMarker>;

  ~EventWorkspaceMRU();

  void ensureEnoughBuffersY(size_t thread_num) const;
  void ensureEnoughBuffersE(size_t thread_num) const;

  void clear();

  YType findY(size_t thread_num, const EventList *index);
  EType findE(size_t thread_num, const EventList *index);
  void insertY(size_t thread_num, YType data, const EventList *index);
  void insertE(size_t thread_num, EType data, const EventList *index);

  void deleteIndex(const EventList *index);

  /** Return how many entries in the Y MRU list are used.
   * Only used in tests. It only returns the 0-th MRU list size.
   * @return :: number of entries in the MRU list. */
  size_t MRUSize() const;

protected:
  /// The most-recently-used list of dataY histograms
  mutable std::vector<std::unique_ptr<mru_listY>> m_bufferedDataY;

  /// The most-recently-used list of dataE histograms
  mutable std::vector<std::unique_ptr<mru_listE>> m_bufferedDataE;

  /// Mutex when adding entries in the MRU list
  mutable Poco::RWLock m_changeMruListsMutexE;
  mutable Poco::RWLock m_changeMruListsMutexY;
};

} // namespace DataObjects
} // namespace Mantid
