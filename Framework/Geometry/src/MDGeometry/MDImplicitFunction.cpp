// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"

namespace Mantid::Geometry {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
MDImplicitFunction::MDImplicitFunction() : m_nd(0), m_numPlanes(0) {}

//----------------------------------------------------------------------------------------------
/** Add a bounded plane to this implicit function
 *
 * @param plane :: MDPlane to add.
 */
void MDImplicitFunction::addPlane(const MDPlane &plane) {
  // Number of dimensions must match
  if (!m_planes.empty()) {
    if (m_nd != plane.getNumDims())
      throw std::invalid_argument("MDImplicitFunction::addPlane(): cannot add "
                                  "a plane with different number of dimensions "
                                  "as the previous ones.");
  }
  m_planes.emplace_back(plane);
  m_nd = plane.getNumDims();
  m_numPlanes = m_planes.size();
}

} // namespace Mantid::Geometry
