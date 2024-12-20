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
*/
class MANTID_KERNEL_DLL WriteLock {
public:
  WriteLock(const DataItem &item);
  WriteLock(const WriteLock &) = delete;
  WriteLock &operator=(const WriteLock &) = delete;
  virtual ~WriteLock();

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
