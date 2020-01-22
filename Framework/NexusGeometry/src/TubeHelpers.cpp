// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidNexusGeometry/TubeHelpers.h"
#include "MantidGeometry/Objects/IObject.h"
#include <algorithm>
#include <iterator>
#include <tbb/parallel_sort.h>
#include <tuple>
#include <vector>

namespace Mantid {
namespace NexusGeometry {
namespace TubeHelpers {

/**
 * Discover tubes based on pixel positions. Sort detector ids on the basis of
 * colinear positions.
 * @param detShape : Shape used for all detectors in tubes
 * @param detPositions : All detector positions across all tubes. Indexes match
 * detIDs.
 * @param detIDs : All detector ids across all tubes. Indexes match
 * detPositions.
 * @return. Collection of virtual tubes based on linearity of points.
 */
std::vector<detail::TubeBuilder>
findAndSortTubes(const Mantid::Geometry::IObject &detShape,
                 const Pixels &detPositions,
                 const std::vector<Mantid::detid_t> &detIDs) {
  std::vector<detail::TubeBuilder> tubes;
  tubes.emplace_back(detShape, detPositions.col(0), detIDs[0]);

  // Loop through all detectors and add to tubes
  for (size_t i = 1; i < detIDs.size(); ++i) {
    bool newEntry = true;
    for (auto t = tubes.rbegin(); t != tubes.rend(); ++t) {
      auto &tube = (*t);
      // Adding detector to existing tube
      if (tube.addDetectorIfCoLinear(detPositions.col(i), detIDs[i])) {
        newEntry = false;
        break;
      }
    }

    // Create a new tube if detector does not belong to any tubes
    if (newEntry)
      tubes.emplace_back(detShape, detPositions.col(i), detIDs[i]);
  }

  // Remove "tubes" with only 1 element
  tubes.erase(std::remove_if(tubes.begin(), tubes.end(),
                             [](const detail::TubeBuilder &tube) {
                               return tube.size() == 1;
                             }),
              tubes.end());

  return tubes;
}

/**
 * Establish detector ids for any detector that are NOT part of the tubes and
 * return the correct indices
 *
 * @param tubes
 * @return vector of indices which index the detIDs vector identifying detectors
 * which are not part of tubes
 */
MANTID_NEXUSGEOMETRY_DLL std::vector<size_t>
notInTubes(const std::vector<detail::TubeBuilder> &tubes,
           const std::vector<Mantid::detid_t> &detIDs) {
  std::vector<size_t> indices(detIDs.size());
  std::iota(indices.begin(), indices.end(), 0);
  for (const auto &tube : tubes) {
    const auto &tubeIDs = tube.detIDs();
    indices.erase(std::remove_if(indices.begin(), indices.end(),
                                 [&detIDs, &tubeIDs](auto index) {
                                   return find(tubeIDs.begin(), tubeIDs.end(),
                                               detIDs[index]) != tubeIDs.end();
                                 }),
                  indices.end());
  }

  return indices;
}
} // namespace TubeHelpers
} // namespace NexusGeometry
} // namespace Mantid
