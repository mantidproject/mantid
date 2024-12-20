// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/MDGeometry/MDBoxImplicitFunction.h"
#include "MantidGeometry/MDGeometry/MDPlane.h"
#include "MantidKernel/VMD.h"

#include <algorithm>

using Mantid::Kernel::VMD;

namespace Mantid::Geometry {

//----------------------------------------------------------------------------------------------
/** Constructor with min/max dimensions.
 *
 * The dimensions must be IN THE SAME ORDER and the SAME LENGTH as the
 * nd dimensions of the MDEventWorkspace on which they will be applied.
 *
 * @param min :: nd-sized vector of the minimum edge of the box in each
 *dimension
 * @param max :: nd-sized vector of the maximum edge of the box
 */
MDBoxImplicitFunction::MDBoxImplicitFunction(const std::vector<coord_t> &min, const std::vector<coord_t> &max)
    : m_max(max), m_min(min) {
  construct(VMD(min), VMD(max));
}

//----------------------------------------------------------------------------------------------
/** Constructor with min/max dimensions.
 *
 * The dimensions must be IN THE SAME ORDER and the SAME LENGTH as the
 * nd dimensions of the MDEventWorkspace on which they will be applied.
 *
 * @param min :: nd-sized vector of the minimum edge of the box in each
 *dimension
 * @param max :: nd-sized vector of the maximum edge of the box
 */
MDBoxImplicitFunction::MDBoxImplicitFunction(const Mantid::Kernel::VMD &min, const Mantid::Kernel::VMD &max)
    : m_max(max), m_min(min) {
  construct(min, max);
}

//----------------------------------------------------------------------------------------------
/** Constructor helper method
 * @param min :: nd-sized vector of the minimum edge of the box in each
 * dimension
 * @param max :: nd-sized vector of the maximum edge of the box
 * */
void MDBoxImplicitFunction::construct(const Mantid::Kernel::VMD &min, const Mantid::Kernel::VMD &max) {
  size_t nd = min.size();
  if (max.size() != nd)
    throw std::invalid_argument("MDBoxImplicitFunction::ctor(): Min and max vector sizes must match!");
  if (nd == 0 || nd > 100)
    throw std::invalid_argument("MDBoxImplicitFunction::ctor(): Invalid number of dimensions!");

  double boxVolume = 1;
  for (size_t d = 0; d < nd; d++) {
    boxVolume *= (max[d] - min[d]);

    // Make two parallel planes per dimension

    // Normal on the min side, so it faces towards +X
    std::vector<coord_t> normal_min(nd, 0);
    normal_min[d] = +1.0;
    // Origin just needs to have its X set to the value. Other coords are
    // irrelevant
    std::vector<coord_t> origin_min(nd, 0);
    origin_min[d] = static_cast<coord_t>(min[d]);
    // Build the plane
    MDPlane p_min(normal_min, origin_min);
    this->addPlane(p_min);

    // Normal on the max side, so it faces towards -X
    std::vector<coord_t> normal_max(nd, 0);
    normal_max[d] = -1.0;
    // Origin just needs to have its X set to the value. Other coords are
    // irrelevant
    std::vector<coord_t> origin_max(nd, 0);
    origin_max[d] = static_cast<coord_t>(max[d]);
    // Build the plane
    MDPlane p_max(normal_max, origin_max);
    this->addPlane(p_max);
  }
  m_volume = boxVolume;
}

/**
 * Calculate volume
 * @return box volume
 */
double MDBoxImplicitFunction::volume() const { return m_volume; }

/**
 * Calculate the fraction of a box residing inside this implicit function
 * @param boxExtents to get fraction for
 * @return fraction 0 to 1
 */
double
MDBoxImplicitFunction::fraction(const std::vector<boost::tuple<Mantid::coord_t, Mantid::coord_t>> &boxExtents) const {

  size_t nd = m_min.size();
  coord_t frac = 1;

  for (size_t d = 0; d < nd; ++d) {

    const coord_t min = boxExtents[d].get<0>();
    const coord_t max = boxExtents[d].get<1>();

    // Check that there is overlap at all. There must be overlap in ALL
    // dimensions for the fraction to be > 0, so abort early if not.
    if (max < m_min[d] || min > m_max[d]) {
      frac = 0;
      break;
    }

    const coord_t dBoxRange = (max - min); // max-min
    const coord_t dInnerMin = std::max(m_min[d], min);
    const coord_t dInnerMax = std::min(m_max[d], max);
    const coord_t dOverlap = dInnerMax - dInnerMin;

    frac *= dOverlap / dBoxRange;
  }

  return frac;
}

} // namespace Mantid::Geometry
