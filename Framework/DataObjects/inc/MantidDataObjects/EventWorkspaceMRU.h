#ifndef MANTID_DATAOBJECTS_EVENTWORKSPACEMRU_H_
#define MANTID_DATAOBJECTS_EVENTWORKSPACEMRU_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidKernel/MRUList.h"
#include "MantidHistogramData/Counts.h"
#include "MantidHistogramData/CountStandardDeviations.h"
#include <vector>
#include <mutex>

namespace Mantid {
namespace DataObjects {

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
  TypeWithMarker(const size_t the_index) : m_index(the_index) {}
  TypeWithMarker(const TypeWithMarker &other) = delete;
  TypeWithMarker &operator=(const TypeWithMarker &other) = delete;

public:
  /// Unique index value.
  size_t m_index;

  /// Pointer to a vector of data
  T m_data;

  /// Function returns a unique index, used for hashing for MRU list
  size_t hashIndexFunction() const { return m_index; }

  /// Set the unique index value.
  void setIndex(const size_t the_index) { m_index = the_index; }
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
  using YWithMarker = TypeWithMarker<HistogramData::Counts>;
  using EWithMarker = TypeWithMarker<HistogramData::CountStandardDeviations>;
  // Typedef for a Most-Recently-Used list of Data objects.
  using mru_listY = Kernel::MRUList<YWithMarker>;
  using mru_listE = Kernel::MRUList<EWithMarker>;

  EventWorkspaceMRU();
  ~EventWorkspaceMRU();

  void ensureEnoughBuffersY(size_t thread_num) const;
  void ensureEnoughBuffersE(size_t thread_num) const;

  void clear();

  HistogramData::Counts findY(size_t thread_num, size_t index);
  HistogramData::CountStandardDeviations findE(size_t thread_num, size_t index);
  void insertY(size_t thread_num, HistogramData::Counts data,
               const size_t index);
  void insertE(size_t thread_num, HistogramData::CountStandardDeviations data,
               const size_t index);

  void deleteIndex(size_t index);

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
  mutable std::mutex m_changeMruListsMutexE;
  mutable std::mutex m_changeMruListsMutexY;
};

} // namespace DataObjects
} // namespace Mantid

#endif /* MANTID_DATAOBJECTS_EVENTWORKSPACEMRU_H_ */
