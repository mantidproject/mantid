//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidKernel/DataItem.h"
#include <Poco/RWLock.h>

namespace Mantid {
namespace Kernel {

/** Default constructor
 */
DataItem::DataItem() { m_lock = new Poco::RWLock(); }

/** Copy constructor
 */
DataItem::DataItem(const DataItem & /*other*/) {
  // Always make a unique lock!
  m_lock = new Poco::RWLock();
}

/**
 * Destructor. Required in cpp do avoid linker errors when other projects try to
 * inherit from DataItem
 */
DataItem::~DataItem() {
  delete m_lock;
  m_lock = NULL;
}

/** Private method to access the RWLock object.
 *
 * @return the RWLock object.
 */
Poco::RWLock *DataItem::getLock() const { return m_lock; }

} // namespace Mantid
} // namespace Kernel
