// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/WriteLock.h"
#include "MantidKernel/DataItem.h"
#include <Poco/RWLock.h>

namespace Mantid {
namespace Kernel {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
WriteLock::WriteLock(const DataItem &item) : m_item(item) {
  // Acquire a write lock.
  m_item.m_lock->writeLock();
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
WriteLock::~WriteLock() {
  // Unlock
  m_item.m_lock->unlock();
}

} // namespace Kernel
} // namespace Mantid
