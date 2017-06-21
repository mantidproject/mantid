#ifndef MANTID_DATAOBJECTS_EVENTWORKSPACEMRU_H_
#define MANTID_DATAOBJECTS_EVENTWORKSPACEMRU_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidKernel/MRUList.h"
#include "MantidHistogramData/HistogramY.h"
#include "MantidHistogramData/HistogramE.h"

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

  Copyright &copy; 2011-2 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
 National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport EventWorkspaceMRU {
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
  size_t MRUSize() const { return this->m_bufferedDataY[0]->size(); }

protected:
  /// The most-recently-used list of dataY histograms
  mutable std::vector<mru_listY *> m_bufferedDataY;

  /// The most-recently-used list of dataE histograms
  mutable std::vector<mru_listE *> m_bufferedDataE;

  /// Mutex when adding entries in the MRU list
  mutable Poco::RWLock m_changeMruListsMutexE;
  mutable Poco::RWLock m_changeMruListsMutexY;
};

} // namespace DataObjects
} // namespace Mantid

#endif /* MANTID_DATAOBJECTS_EVENTWORKSPACEMRU_H_ */
