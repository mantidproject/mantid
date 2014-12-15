#ifndef MANTID_KERNEL_WRITELOCK_H_
#define MANTID_KERNEL_WRITELOCK_H_

#include "MantidKernel/DllConfig.h"

namespace Mantid
{
namespace Kernel
{
  //---------------------------------------------------------------------------
  // Forward Declarations
  //---------------------------------------------------------------------------
  class DataItem;

  /** Scoped write-lock for thread-safe access to DataItems.
   *
   * Acquire a WriteLock on a workspace that you will be modifying.
   * This blocks any other thread from reading/writing to the
   * workspace.
   *
   * Note that normally, you SHOULD NOT CALL THIS IN AN ALGORITHM,
   * because read- or write-locking of the input or output
   * workspaces is taken care of in the Algorithm class.
   *
   * The write lock is automatically unlocked when the variable goes
   * out of scope (in the destructor). Multiple read-locks can
   * be acquired simultaneously, but only one write-lock.
   *
   * Sample Usage:
   *  {
   *    WriteLock _lock(*workspace_sptr);
   *    // Modify the workspace
   *  }
   *  // Lock has been released when _lock when out of scope.
    
    @date 2012-01-20

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class DLLExport WriteLock
  {
  public:
    WriteLock(const DataItem & item);
    virtual ~WriteLock();
    
  private:
    /// Private copy constructor - NO COPY ALLOWED
    WriteLock(const WriteLock &);
    /// Private assignment operator - NO ASSIGNMENT ALLOWED
    WriteLock& operator=(const WriteLock &);
    /// Disallow creating the object on the heap
    void *operator new( size_t );
    /// Disallow creating the object on the heap
    void *operator new[]( size_t );


    /// Reference to the data item we are locking
    const DataItem & m_item;
  };


} // namespace Kernel
} // namespace Mantid

#endif  /* MANTID_KERNEL_WRITELOCK_H_ */
