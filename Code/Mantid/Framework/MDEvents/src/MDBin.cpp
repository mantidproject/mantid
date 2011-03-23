#include "MantidMDEvents/MDBin.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace MDEvents
{


  //----------------------------------------------------------------------------------------------
  /** Constructor. Clears the signal and error.
   * Initializes the min and max of all dimensions to include all numbers.
   */
  TMDE(MDBin)::MDBin()
      : m_signal(0), m_errorSquared(0)
  {
    for (size_t i=0; i<nd; i++)
      m_min[i] = CoordType_min;
    for (size_t i=0; i<nd; i++)
      m_max[i] = CoordType_max;
  }
    

} // namespace Mantid
} // namespace MDEvents

