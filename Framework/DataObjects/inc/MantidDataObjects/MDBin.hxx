// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/MDBin.h"
#include <cstddef>

namespace Mantid {
namespace DataObjects {

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

} // namespace DataObjects
} // namespace Mantid
