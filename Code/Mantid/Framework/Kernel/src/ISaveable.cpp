#include "MantidKernel/ISaveable.h"
#include "MantidKernel/System.h"
#include <limits>

namespace Mantid
{
namespace Kernel
{


//----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ISaveable::ISaveable()
  : m_id(0),m_fileIndexStart(std::numeric_limits<uint64_t>::max() ),m_fileNumEvents(0),
  m_Busy(false),m_dataChanged(false)
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Copy constructor --> big qusetions about the validity of such implementation
   */
  ISaveable::ISaveable(const ISaveable & other)
  : m_id(other.m_id),m_fileIndexStart(other.m_fileIndexStart),m_fileNumEvents(other.m_fileNumEvents),
   m_Busy(other.m_Busy),m_dataChanged(other.m_dataChanged)
  {
  }

 ISaveable::ISaveable(const size_t id)
  : m_id(id),m_fileIndexStart(std::numeric_limits<uint64_t>::max() ),m_fileNumEvents(0),
    m_Busy(false),m_dataChanged(false)
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ISaveable::~ISaveable()
  {
  }

  void ISaveable::saveAt(uint64_t newPos, uint64_t newSize)
  {
      m_fileIndexStart= newPos;
      m_fileNumEvents = newSize;
      this->save();
      m_dataChanged = false;
  }

  /** Set the start/end point in the file where the events are located
     * @param start :: start point,
     * @param numEvents :: number of events in the file   */
    void ISaveable::setFilePosition(uint64_t newPos,uint64_t newSize)
    {  
      m_fileIndexStart=newPos;  
      m_fileNumEvents =newSize;
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

