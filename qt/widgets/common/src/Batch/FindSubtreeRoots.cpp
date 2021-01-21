// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/Batch/FindSubtreeRoots.h"
#include "MantidQtWidgets/Common/Batch/Subtree.h"
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

auto FindSubtreeRoots::operator()(std::vector<RowLocation> region) -> boost::optional<std::vector<RowLocation>> {
  std::sort(region.begin(), region.end());
  if (!region.empty()) {
    auto subtreeRootDepth = region[0].depth();
    if (allSubtreeRootsShareAParentAndAllSubtreeNodesAreConnected(subtreeRootDepth, region.cbegin(), region.cend())) {
      removeIfDepthNotEqualTo(region, subtreeRootDepth);
      return region;
    } else {
      return boost::none;
    }
  } else {
    return std::vector<RowLocation>();
  }
}

void FindSubtreeRoots::removeIfDepthNotEqualTo(std::vector<RowLocation> &region, int expectedDepth) const {
  region.erase(std::remove_if(
                   region.begin(), region.end(),
                   [expectedDepth](RowLocation const &location) -> bool { return location.depth() != expectedDepth; }),
               region.end());
}

} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
