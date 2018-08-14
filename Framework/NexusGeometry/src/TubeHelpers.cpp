#include "MantidNexusGeometry/TubeHelpers.h"
#include "MantidGeometry/Objects/IObject.h"

namespace Mantid {
namespace NexusGeometry {
namespace TubeHelpers {

std::vector<detail::TubeBuilder>
findAndSortTubes(const Mantid::Geometry::IObject &shape,
                 const Pixels &positions, const std::vector<int> &detIDs) {
  std::vector<detail::TubeBuilder> tubes;

  tubes.emplace_back(shape, positions.col(0), detIDs[0]);
  bool newEntry;

  // Loop through all detectors and add to tubes
  for (size_t i = 1; i < detIDs.size(); ++i) {
    newEntry = true;
    for (auto t = tubes.rbegin(); t != tubes.rend(); ++t) {
      auto &tube = (*t);
      // Adding detector to existing tube
      if (tube.addDetectorIfCoLinear(positions.col(i), detIDs[i])) {
        newEntry = false;
        break;
      }
    }

    // Create a new tube if detector does not belong to any tubes
    if (newEntry)
      tubes.emplace_back(shape, positions.col(i), detIDs[i]);
  }

  // Remove "tubes" with only 1 element
  tubes.erase(std::remove_if(tubes.begin(), tubes.end(),
                             [](const detail::TubeBuilder &tube) {
                               return tube.size() == 1;
                             }),
              tubes.end());

  return tubes;
}
} // namespace TubeHelpers
} // namespace NexusGeometry
} // namespace Mantid