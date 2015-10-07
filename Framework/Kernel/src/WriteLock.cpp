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

} // namespace Mantid
} // namespace Kernel
