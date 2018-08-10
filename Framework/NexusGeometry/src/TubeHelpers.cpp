#include "MantidNexusGeometry/TubeHelpers.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidNexusGeometry/Tube.h"

namespace Mantid {
namespace NexusGeometry {
namespace TubeHelpers {

std::vector<detail::Tube> findTubes(const Mantid::Geometry::IObject &shape,
                                    const Pixels &positions,
                                    const std::vector<int> &detIDs) {
  std::vector<detail::Tube> tubes;

  tubes.emplace_back(shape, positions.col(0), detIDs[0]);
  bool newEntry;

  for (size_t i = 1; i < detIDs.size(); ++i) {
    newEntry = true;
    for (auto t = tubes.rbegin(); t != tubes.rend(); ++t) {
      auto &tube = (*t);
      if (tube.addDetectorIfCoLinear(positions.col(i), detIDs[i])) {
        newEntry = false;
        break;
      }
    }

    if (newEntry)
      tubes.emplace_back(shape, positions.col(i), detIDs[i]);
  }

  return tubes;
}
} // namespace TubeHelpers
} // namespace NexusGeometry
} // namespace Mantid