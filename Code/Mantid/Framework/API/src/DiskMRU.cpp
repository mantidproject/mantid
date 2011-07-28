#include "MantidAPI/DiskMRU.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace API
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  DiskMRU::DiskMRU()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  DiskMRU::~DiskMRU()
  {
    // The item pointers are NOT owned by the MRU.
    list.clear();
  }
  


} // namespace Mantid
} // namespace API

