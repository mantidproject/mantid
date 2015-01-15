#include "MantidMDEvents/MDBin.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace MDEvents {

//----------------------------------------------------------------------------------------------
/** Constructor. Clears the signal and error.
 * Initializes the min and max of all dimensions to include all numbers.
 */
TMDE(MDBin)::MDBin() : m_signal(0), m_errorSquared(0) {
  for (size_t d = 0; d < nd; ++d)
    m_min[d] = -std::numeric_limits<coord_t>::max();
  for (size_t d = 0; d < nd; ++d)
    m_max[d] = +std::numeric_limits<coord_t>::max();
}

} // namespace Mantid
} // namespace MDEvents
