#include "MantidKernel/ReadLock.h"
#include "MantidKernel/System.h"
#include <iostream>

namespace Mantid
{
namespace Kernel
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ReadLock::ReadLock(const DataItem & item)
  : m_item(item)
  {
    std::cout << "Read-lock acquired of " << item.name() << std::endl;
    // Acquire a read lock.
    m_item.m_lock->readLock();
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ReadLock::~ReadLock()
  {
    // Unlock
    std::cout << "Read-lock released of " << m_item.name() << std::endl;
    m_item.m_lock->unlock();
  }
  


} // namespace Mantid
} // namespace Kernel
