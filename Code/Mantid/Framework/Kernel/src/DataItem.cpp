//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidKernel/DataItem.h"

namespace Mantid
{
  namespace Kernel
  {

    /** Default constructor
     */
    DataItem::DataItem()
    {
      m_lock = new Poco::RWLock();
    }

    /**
     * Destructor. Required in cpp do avoid linker errors when other projects try to inherit from DataItem
     */
    DataItem::~DataItem()
    {
      delete m_lock;
    }

    /** Private method to access the RWLock object.
     *
     * @return the RWLock object.
     */
    Poco::RWLock * DataItem::getLock()
    {
      return m_lock;
    }

  
  } // namespace Mantid
} // namespace Kernel

