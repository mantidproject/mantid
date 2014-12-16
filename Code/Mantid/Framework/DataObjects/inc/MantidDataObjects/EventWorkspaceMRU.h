#ifndef MANTID_DATAOBJECTS_EVENTWORKSPACEMRU_H_
#define MANTID_DATAOBJECTS_EVENTWORKSPACEMRU_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidKernel/MRUList.h"
#include <vector>
#include "MantidKernel/MultiThreaded.h"

namespace Mantid {
namespace DataObjects {

//============================================================================
//============================================================================
/**
 * This little class holds a MantidVec of data and an index marker that
 * is used for uniqueness.
 * This is used in the MRUList.
 *
 */
class MantidVecWithMarker {
public:
  /**
   * Constructor.
   * @param the_index :: unique index into the workspace of this data
   * @param locked :: reference to a bool that will be set to true if
   *        the marker should NOT be deleted
   */
  MantidVecWithMarker(const size_t the_index, bool &locked)
      : m_index(the_index), m_locked(locked) {}

  /// Destructor
  ~MantidVecWithMarker() {
    m_data.clear();
    // Trick to release the allocated memory
    // MantidVec().swap(m_data);
  }

private:
  /// Unimplemented, private copy constructor
  MantidVecWithMarker(const MantidVecWithMarker &other);
  /// Unimplemented, private assignment operator
  MantidVecWithMarker &operator=(const MantidVecWithMarker &other);

public:
  /// Unique index value.
  size_t m_index;

  /// Pointer to a vector of data
  MantidVec m_data;

  /// Function returns a unique index, used for hashing for MRU list
  size_t hashIndexFunction() const { return m_index; }

  /// Set the unique index value.
  void setIndex(const size_t the_index) { m_index = the_index; }

  /// Locked: can't be deleted. This will point to the EventList's m_lockedMRU
  /// bool.
  bool &m_locked;
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
  // Typedef for a Most-Recently-Used list of Data objects.
  typedef Mantid::Kernel::MRUList<MantidVecWithMarker> mru_list;
  // Typedef for a vector of MRUlists.
  typedef std::vector<mru_list *> mru_lists;

  EventWorkspaceMRU();
  ~EventWorkspaceMRU();

  void ensureEnoughBuffersY(size_t thread_num) const;
  void ensureEnoughBuffersE(size_t thread_num) const;

  void clear();

  MantidVecWithMarker *findY(size_t thread_num, size_t index);
  MantidVecWithMarker *findE(size_t thread_num, size_t index);
  void insertY(size_t thread_num, MantidVecWithMarker *data);
  void insertE(size_t thread_num, MantidVecWithMarker *data);

  void deleteIndex(size_t index);

  /** Return how many entries in the Y MRU list are used.
   * Only used in tests. It only returns the 0-th MRU list size.
   * @return :: number of entries in the MRU list. */
  size_t MRUSize() const { return this->m_bufferedDataY[0]->size(); }

protected:
  /// The most-recently-used list of dataY histograms
  mutable mru_lists m_bufferedDataY;

  /// The most-recently-used list of dataE histograms
  mutable mru_lists m_bufferedDataE;

  /// These markers will be deleted when they are NOT locked
  mutable std::vector<MantidVecWithMarker *> m_markersToDelete;

  /// Mutex around accessing m_markersToDelete
  Kernel::Mutex m_toDeleteMutex;

  /// Mutex when adding entries in the MRU list
  mutable Kernel::Mutex m_changeMruListsMutexE;
  mutable Kernel::Mutex m_changeMruListsMutexY;
};

} // namespace DataObjects
} // namespace Mantid

#endif /* MANTID_DATAOBJECTS_EVENTWORKSPACEMRU_H_ */
