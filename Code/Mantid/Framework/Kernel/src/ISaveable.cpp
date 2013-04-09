#include "MantidKernel/ISaveable.h"
#include "MantidKernel/System.h"
#include <limits>
//#include "MantidKernel/INode.h"

namespace Mantid
{
namespace Kernel
{

  /** Constructor    */
  ISaveable::ISaveable():
  m_fileIndexStart(std::numeric_limits<uint64_t>::max() ),m_fileNumEvents(0),m_BufMemorySize(0)
  {}

  //----------------------------------------------------------------------------------------------
  /** Copy constructor --> needed for std containers and not to copy mutexes   */
  ISaveable::ISaveable(const ISaveable & other):
     m_fileIndexStart(other.m_fileIndexStart),m_fileNumEvents(other.m_fileNumEvents),
     m_BufPosition(other.m_BufPosition),
     m_BufMemorySize(other.m_BufMemorySize)
 { }

  

  /** Method stores the position of the object in Disc buffer and returns the size of this object for disk buffer to store 
   * @param bufPosition -- the allocator which specifies the position of the object in the list of objects to write
   * @returns the size of the object it currently occupies in memory. This size is also stored by the object itself for further references
  */
  size_t ISaveable::setBufferPosition(std::list<ISaveable *>::iterator bufPosition)
  {
      Mutex::ScopedLock _lock(m_setter);

      m_BufPosition = boost::optional<std::list<ISaveable *>::iterator >(bufPosition);
      m_BufMemorySize  = this->getDataMemorySize();

      return m_BufMemorySize ;
  }

  /** private function which used by the disk buffer to save the contents of the  */
  void ISaveable::saveAt(uint64_t newPos, uint64_t newSize)
  {

      Mutex::ScopedLock _lock(m_setter);

      // load old contents if it was there
      if(this->wasSaved())
         this->load();
      // set new position, derived by the disk buffer
      m_fileIndexStart= newPos;
      m_fileNumEvents = newSize;
      // save in the new location
      this->save();
      this->clearDataFromMemory();      
  }
  /// clears the state of the object, and indicate that it is not stored in buffer any more 
  void ISaveable::clearBufferState()
  {
      Mutex::ScopedLock _lock(m_setter);

      m_BufMemorySize=0;
      m_BufPosition = boost::optional<std::list<ISaveable *>::iterator>();

  }
} // namespace Mantid
} // namespace Kernel

