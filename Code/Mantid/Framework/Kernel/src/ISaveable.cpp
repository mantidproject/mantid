#include "MantidKernel/ISaveable.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace Kernel
{


//----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ISaveable::ISaveable()
  : m_id(0),m_fileIndexStart(0),m_fileNumEvents(0)
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Copy constructor
   */
  ISaveable::ISaveable(const ISaveable & other)
  : m_id(other.m_id),m_fileIndexStart(other.m_fileIndexStart),m_fileNumEvents(other.m_fileNumEvents)
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ISaveable::ISaveable(const size_t id,const uint64_t fileIndexStart,uint64_t fileNumEvents)
  : m_id(id),m_fileIndexStart(fileIndexStart),m_fileNumEvents(fileNumEvents)
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ISaveable::~ISaveable()
  {
  }

  void ISaveable::setFileIndex(uint64_t newPos, uint64_t newSize)
  {
      m_fileIndexStart= newPos;
      m_fileNumEvents = newSize;
  }

  
  //-----------------------------------------------------------------------------------------------
  /** Helper method for sorting MDBoxBasees by file position.
   * MDGridBoxes return 0 for file position and so aren't sorted.
   *
   * @param a :: an MDBoxBase pointer
   * @param b :: an MDBoxBase pointer
   * @return
   */
  
  inline bool CompareFilePosition (const ISaveable * a, const ISaveable * b)
  {
    return (a->getFilePosition() < b->getFilePosition());
  }

  //-----------------------------------------------------------------------------------------------
  /** Static method for sorting a list of MDBoxBase pointers by their file position,
   * ascending. This should optimize the speed of loading a bit by
   * reducing the amount of disk seeking.
   *
   * @param boxes :: ref to a vector of boxes. It will be sorted in-place.
   */
  void ISaveable::sortObjByFilePos(std::vector<ISaveable *> & boxes)
  {
    std::sort( boxes.begin(), boxes.end(), CompareFilePosition);
  }



} // namespace Mantid
} // namespace Kernel

