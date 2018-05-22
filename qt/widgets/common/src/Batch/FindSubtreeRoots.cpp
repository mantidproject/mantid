#include "MantidQtWidgets/Common/Batch/FindSubtreeRoots.h"
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

auto FindSubtreeRoots::operator()(std::vector<RowLocation> region)
    -> boost::optional<std::vector<RowLocation>> {
  std::sort(region.begin(), region.end());
  if (!hasSubtreeRootHigherThanFirstRoot(region)) {
    if (maximumIncreaseInDepthIsOne(region)) {
      if (!region.empty())
        removeIfDepthNotEqualTo(region, region[0].depth());
      return region;
    }
  }
  return boost::none;
}

bool FindSubtreeRoots::hasSubtreeRootHigherThanFirstRoot(
    std::vector<RowLocation> const &sortedRegion) {
  auto firstLocationAtMinimumDepth =
      std::min_element(sortedRegion.cbegin(), sortedRegion.cend(),
                       [](RowLocation const &lhs, RowLocation const &rhs)
                           -> bool { return lhs.depth() < rhs.depth(); });
  return firstLocationAtMinimumDepth != sortedRegion.cbegin();
}

bool FindSubtreeRoots::maximumIncreaseInDepthIsOne(
    std::vector<RowLocation> const &region) {
  auto previous = region.cbegin();
  if (previous != region.cend()) {
    auto current = region.cbegin() + 1;
    for (; current != region.cend(); ++current, ++previous) {
      auto currentDepth = (*current).depth();
      auto previousDepth = (*previous).depth();
      if ((previousDepth - currentDepth) < -1)
        return false;
    }
  }
  return true;
}

void FindSubtreeRoots::removeIfDepthNotEqualTo(std::vector<RowLocation> &region,
                                               int expectedDepth) {
  region.erase(
      std::remove_if(region.begin(), region.end(),
                     [expectedDepth](RowLocation const &location)
                         -> bool { return location.depth() != expectedDepth; }),
      region.end());
}

} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
