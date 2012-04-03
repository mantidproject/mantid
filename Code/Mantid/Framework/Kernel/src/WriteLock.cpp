#include "MantidKernel/WriteLock.h"
#include "MantidKernel/System.h"
#include <iostream>

namespace Mantid
{
namespace Kernel
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  WriteLock::WriteLock(const DataItem & item)
  : m_item(item)
  {
    std::cout << "Write-lock acquired of " << item.name() << std::endl;
    // Acquire a write lock.
    m_item.m_lock->writeLock();
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  WriteLock::~WriteLock()
  {
    std::cout << "Write-lock released of " << m_item.name() << std::endl;
    // Unlock
    m_item.m_lock->unlock();
  }
  


} // namespace Mantid
} // namespace Kernel
