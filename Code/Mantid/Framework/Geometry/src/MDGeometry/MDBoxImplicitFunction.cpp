#include "MantidGeometry/MDGeometry/MDBoxImplicitFunction.h"
#include "MantidGeometry/MDGeometry/MDPlane.h"
#include "MantidKernel/System.h"
#include "MantidKernel/VMD.h"

using Mantid::Kernel::VMD;

namespace Mantid {
namespace Geometry {

//----------------------------------------------------------------------------------------------
/** Default ctor
 */
MDBoxImplicitFunction::MDBoxImplicitFunction() : MDImplicitFunction() {}

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
MDBoxImplicitFunction::MDBoxImplicitFunction(const std::vector<coord_t> &min,
                                             const std::vector<coord_t> &max) {
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
MDBoxImplicitFunction::MDBoxImplicitFunction(const Mantid::Kernel::VMD &min,
                                             const Mantid::Kernel::VMD &max) {
  construct(min, max);
}

//----------------------------------------------------------------------------------------------
/** Constructor helper method
 * @param min :: nd-sized vector of the minimum edge of the box in each
 * dimension
 * @param max :: nd-sized vector of the maximum edge of the box
 * */
void MDBoxImplicitFunction::construct(const Mantid::Kernel::VMD &min,
                                      const Mantid::Kernel::VMD &max) {
  size_t nd = min.size();
  if (max.size() != nd)
    throw std::invalid_argument(
        "MDBoxImplicitFunction::ctor(): Min and max vector sizes must match!");
  if (nd == 0 || nd > 100)
    throw std::invalid_argument(
        "MDBoxImplicitFunction::ctor(): Invalid number of dimensions!");

  for (size_t d = 0; d < nd; d++) {
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
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
MDBoxImplicitFunction::~MDBoxImplicitFunction() {}

} // namespace Mantid
} // namespace Geometry
