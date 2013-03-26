#include "MantidKernel/Saveable.h"
#include "MantidKernel/System.h"
#include <limits>

namespace Mantid
{
namespace Kernel
{


//----------------------------------------------------------------------------------------------
  /** Constructor
   */
  Saveable::Saveable()
      :ISaveable(),
       m_Busy(false),m_dataChanged(false),m_wasSaved(false),m_isLoaded(false)
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Copy constructor --> big qusetions about the validity of such implementation
   */
  Saveable::Saveable(const Saveable & other)
      :ISaveable(other),
        m_Busy(other.m_Busy),m_dataChanged(other.m_dataChanged),m_wasSaved(other.m_wasSaved),m_isLoaded(false)
  {
  }

 /* Saveable::Saveable(const size_t id)
      :ISaveable(id),
       m_Busy(false),m_dataChanged(false),m_wasSaved(false),m_isLoaded(false)
  {
  }*/

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  Saveable::~Saveable()
  {
  }

  

  /** Set the start/end point in the file where the events are located
     * @param newPos :: start point,
     * @param newSize :: number of events in the file   
     * @param wasSaved :: flag to mark if the info was saved, by default it does
   */
  void Saveable::setFilePosition(uint64_t newPos, size_t newSize, bool wasSaved)
  {  
      this->m_fileIndexStart=newPos;  
      this->m_fileNumEvents =static_cast<uint64_t>(newSize);
      m_wasSaved = wasSaved;
  }




} // namespace Mantid
} // namespace Kernel

