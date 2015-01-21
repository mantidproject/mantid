#ifndef MANTID_KERNEL_READLOCK_H_
#define MANTID_KERNEL_READLOCK_H_

#include "MantidKernel/DllConfig.h"

namespace Mantid {
namespace Kernel {
//---------------------------------------------------------------------------
// Forward Declarations
//---------------------------------------------------------------------------
class DataItem;

/** Scoped, read-only lock for thread-safe access to DataItems.
 *
 * Acquire a ReadLock on any DataItem (e.g. a Workspace) that you are
 * going to be reading in a thread. This prevents any thread from
 * acquiring a Write lock on it, and blocks until any write lock
 * is unlocked.
 *
 * The read lock is automatically unlocked when the variable goes
 * out of scope (in the destructor).
 *
 * Sample Usage:
 *  {
 *    ReadLock _lock(*workspace_sptr);
 *    // Read the workspace
 *  }
 *  // Lock has been released when _lock when out of scope.

  @date 2012-01-20

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_KERNEL_DLL ReadLock {
public:
  ReadLock(const DataItem &item);
  virtual ~ReadLock();

private:
  /// Private copy constructor - NO COPY ALLOWED
  ReadLock(const ReadLock &);
  /// Private assignment operator - NO ASSIGNMENT ALLOWED
  ReadLock &operator=(const ReadLock &);
  /// Disallow creating the object on the heap
  void *operator new(size_t);
  /// Disallow creating the object on the heap
  void *operator new[](size_t);

  /// Reference to the data item we are locking
  const DataItem &m_item;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_READLOCK_H_ */
