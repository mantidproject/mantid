#ifndef MANTIDNEXUSGEOMETRY_TUBEHELPERS_H
#define MANTIDNEXUSGEOMETRY_TUBEHELPERS_H
#include "MantidNexusGeometry/Tube.h"
#include <Eigen/Core>
#include <vector>
// Eigen typedefs
using Pixels = Eigen::Matrix<double, 3, Eigen::Dynamic>;

namespace Mantid {
namespace Geometry {
class IObject;
}
namespace NexusGeometry {
namespace detail {
class Tube;
}
namespace TubeHelpers {
MANTID_NEXUSGEOMETRY_DLL std::vector<detail::Tube>
findTubes(const Mantid::Geometry::IObject &shape, const Pixels &positions,
          const std::vector<int> &detIDs);
} // namespace TubeHelpers
} // namespace NexusGeometry
} // namespace Mantid
#endif // MANTIDNEXUSGEOMETRY_TUBEHELPER_H