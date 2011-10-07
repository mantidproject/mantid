//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidKernel/DataItem.h"

namespace Mantid
{
  namespace Kernel
  {

    /**
     * Destructor. Required in cpp do avoid linker errors when other projects try to inherit from DataItem
     */
    DataItem::~DataItem()
    {
    }
  
  } // namespace Mantid
} // namespace Kernel

