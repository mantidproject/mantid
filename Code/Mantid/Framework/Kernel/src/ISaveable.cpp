#include "MantidKernel/ISaveable.h"
#include "MantidKernel/System.h"
#include <limits>
#include "MantidKernel/INode.h"

namespace Mantid
{
namespace Kernel
{

  /** Constructor
   */
  ISaveable::ISaveable()
  : m_FileId(0),m_fileIndexStart(std::numeric_limits<uint64_t>::max() ),m_fileNumEvents(0),m_BufMemorySize(0)
  {}

  //----------------------------------------------------------------------------------------------
  /** Copy constructor --> needed? for std containers.
   */
  /*ISaveable::ISaveable(const ISaveable & other)
  : m_FileId(other.m_FileId),m_fileIndexStart(other.m_fileIndexStart),m_fileNumEvents(other.m_fileNumEvents)
  { }*/

  ISaveable::ISaveable(const size_t fileId)
  : m_FileId(fileId),m_fileIndexStart(std::numeric_limits<uint64_t>::max() ),m_fileNumEvents(0),m_BufMemorySize(0)
  {}


  //-----------------------------------------------------------------------------------------------
  /** Helper method for sorting MDBoxBasees by file position.
   * MDGridBoxes return 0 for file position and so aren't sorted.
   *
   * @param a :: an MDBoxBase pointer
   * @param b :: an MDBoxBase pointer
   * @return
   */
  
  inline bool CompareFilePosition (const INode * const a, const INode * const b)
  {
    const ISaveable *const as = a->getISaveable();
    if(!as)return false;
    const ISaveable *const bs = b->getISaveable();
    if(!bs)return false;

    return (as->getFileId() < bs->getFileId());
  }

  //-----------------------------------------------------------------------------------------------
  /** Static method for sorting a list of MDBoxBase pointers by their file position,
   * ascending. This should optimize the speed of loading a bit by
   * reducing the amount of disk seeking.
   *
   * @param boxes :: ref to a vector of boxes. It will be sorted in-place.
   */
  void ISaveable::sortObjByFileID(std::vector<INode *const> & boxes)
  {
    std::sort( boxes.begin(), boxes.end(), CompareFilePosition);
  }

  /** Method stores the position of the object in Disc buffer and returns the size of this object for disk buffer to store 
   * @param bufPosition -- the allocator which specifies the position of the object in the list of objects to write
   * @returns the size of the object it currently occupies in memory. This size is also stored by the object itself for further references
  */
  size_t ISaveable::setBufferPosition(std::list<ISaveable *const>::iterator &bufPosition)
  {
      m_BufPosition = boost::optional<std::list<ISaveable *const>::iterator >(bufPosition);
      m_BufMemorySize  = this->getDataMemorySize();
      return m_BufMemorySize ;
  }

  /** private function which used by the disk buffer to save the contents of the  */
  void ISaveable::saveAt(uint64_t newPos, uint64_t newSize)
  {
   
      // load everything which is not in memory yet
      this->load(); 
      m_fileIndexStart= newPos;
      m_fileNumEvents = newSize;
      this->save();
      this->clearDataFromMemory();      
  }
  
  /// clears the state of the object, and indicate that it is not stored in buffer any more 
  void ISaveable::clearBufferState()
  {
      m_BufMemorySize=0;
      m_BufPosition = boost::optional<std::list<ISaveable *const>::iterator>();
  }
} // namespace Mantid
} // namespace Kernel

