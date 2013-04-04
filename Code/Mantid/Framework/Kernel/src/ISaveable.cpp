#include "MantidKernel/ISaveable.h"
#include "MantidKernel/System.h"
#include <limits>
//#include "MantidKernel/INode.h"

namespace Mantid
{
namespace Kernel
{

  /** Constructor
   */
  ISaveable::ISaveable():
  // m_FileId(0),
  m_fileIndexStart(std::numeric_limits<uint64_t>::max() ),m_fileNumEvents(0),m_BufMemorySize(0)
  {}

  //----------------------------------------------------------------------------------------------
  /** Copy constructor --> needed for std containers and not to copy mutexes   */
  ISaveable::ISaveable(const ISaveable & other):
     m_fileIndexStart(other.m_fileIndexStart),m_fileNumEvents(other.m_fileNumEvents)
 { }

  //ISaveable::ISaveable(const size_t fileId):
  // m_FileId(fileId),m_fileIndexStart(std::numeric_limits<uint64_t>::max() ),m_fileNumEvents(0),m_BufMemorySize(0)
  //{}


  /** Method stores the position of the object in Disc buffer and returns the size of this object for disk buffer to store 
   * @param bufPosition -- the allocator which specifies the position of the object in the list of objects to write
   * @returns the size of the object it currently occupies in memory. This size is also stored by the object itself for further references
  */
  size_t ISaveable::setBufferPosition(std::list<ISaveable *const>::iterator &bufPosition)
  {
      m_setter.lock();
      m_BufPosition = boost::optional<std::list<ISaveable *const>::iterator >(bufPosition);
      m_BufMemorySize  = this->getDataMemorySize();
      m_setter.unlock();
      return m_BufMemorySize ;
  }

  /** private function which used by the disk buffer to save the contents of the  */
  void ISaveable::saveAt(uint64_t newPos, uint64_t newSize)
  {
      m_setter.lock();
      // load old contents if it was there
      if(this->wasSaved())
         this->load();
      // set new position, derived by the disk buffer
      m_fileIndexStart= newPos;
      m_fileNumEvents = newSize;
      // save in the new location
      this->save();
      this->clearDataFromMemory();      
      m_setter.unlock();
  }
  /// clears the state of the object, and indicate that it is not stored in buffer any more 
  void ISaveable::clearBufferState()
  {
      m_setter.lock();
      m_BufMemorySize=0;
      m_BufPosition = boost::optional<std::list<ISaveable *const>::iterator>();
      m_setter.unlock();
  }
} // namespace Mantid
} // namespace Kernel

