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
    for (size_t d=0; d<nd; ++d)
      m_min[d] = coord_t_min;
    for (size_t d=0; d<nd; ++d)
      m_max[d] = coord_t_max;
  }
    

} // namespace Mantid
} // namespace MDEvents

