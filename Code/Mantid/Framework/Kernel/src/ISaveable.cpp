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
  : m_id(0)
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ISaveable::ISaveable(const size_t id)
  : m_id(id)
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ISaveable::~ISaveable()
  {
  }
  


} // namespace Mantid
} // namespace Kernel

