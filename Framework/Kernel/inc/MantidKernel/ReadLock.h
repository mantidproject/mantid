// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
*/
class MANTID_KERNEL_DLL ReadLock {
public:
  ReadLock(const DataItem &item);
  ReadLock(const ReadLock &) = delete;
  ReadLock &operator=(const ReadLock &) = delete;
  virtual ~ReadLock();

private:
  /// Disallow creating the object on the heap
  void *operator new(std::size_t);
  /// Disallow creating the object on the heap
  void *operator new[](std::size_t);

  /// Reference to the data item we are locking
  const DataItem &m_item;
};

} // namespace Kernel
} // namespace Mantid
