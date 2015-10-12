#include "MantidKernel/ReadLock.h"
#include "MantidKernel/DataItem.h"

#include <Poco/RWLock.h>

namespace Mantid {
namespace Kernel {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ReadLock::ReadLock(const DataItem &item) : m_item(item) {
  // Acquire a read lock.
  m_item.m_lock->readLock();
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ReadLock::~ReadLock() {
  // Unlock
  m_item.m_lock->unlock();
}

} // namespace Mantid
} // namespace Kernel
