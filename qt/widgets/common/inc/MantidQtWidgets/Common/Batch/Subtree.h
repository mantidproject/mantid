// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/**
See the developer documentation for Batch Widget at
developer.mantidproject.org/BatchWidget/index.html
*/
#pragma once
#include "MantidQtWidgets/Common/Batch/Row.h"

#include <algorithm>

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

using Subtree = std::vector<Row>;

template <typename RowLocationConstIterator>
bool hasSubtreeRootShallowerThanFirstRoot(RowLocationConstIterator sortedRegionBegin,
                                          RowLocationConstIterator sortedRegionEnd) {
  auto firstLocationAtMinimumDepth =
      std::min_element(sortedRegionBegin, sortedRegionEnd, [](RowLocation const &lhs, RowLocation const &rhs) -> bool {
        return lhs.depth() < rhs.depth();
      });
  return firstLocationAtMinimumDepth != sortedRegionBegin;
}

template <typename RowLocationConstIterator>
bool allNodesDescendedFromParentOfFirstRoot(RowLocationConstIterator sortedRegionBegin,
                                            RowLocationConstIterator sortedRegionEnd) {
  auto firstRootParent = (*sortedRegionBegin).parent();
  return std::all_of(sortedRegionBegin + 1, sortedRegionEnd, [&firstRootParent](RowLocation const &location) -> bool {
    return location.isDescendantOf(firstRootParent);
  });
}

template <typename RowLocationConstIterator>
bool maximumIncreaseInDepthIsOne(RowLocationConstIterator sortedRegionBegin, RowLocationConstIterator sortedRegionEnd) {
  return std::adjacent_find(sortedRegionBegin, sortedRegionEnd,
                            [](RowLocation const &previous, RowLocation const &current) -> bool {
                              return (previous.depth() - current.depth()) < -1;
                            }) == sortedRegionEnd;
}

template <typename RowLocationConstIterator>
bool hasSubtreeRootDeeperThanFirstRoot(int firstSubtreeRootDepth, RowLocationConstIterator sortedRegionBegin,
                                       RowLocationConstIterator sortedRegionEnd) {
  return std::adjacent_find(sortedRegionBegin, sortedRegionEnd,
                            [firstSubtreeRootDepth](RowLocation const &previous, RowLocation const &current) -> bool {
                              return current.depth() > firstSubtreeRootDepth &&
                                     !pathsSameUntilDepth(firstSubtreeRootDepth, current, previous);
                            }) != sortedRegionEnd;
}

template <typename RowLocationConstIterator>
bool allSubtreeRootsShareAParentAndAllSubtreeNodesAreConnected(int subtreeRootDepth,
                                                               RowLocationConstIterator sortedRegionBegin,
                                                               RowLocationConstIterator sortedRegionEnd) {
  return allNodesDescendedFromParentOfFirstRoot(sortedRegionBegin, sortedRegionEnd) &&
         maximumIncreaseInDepthIsOne(sortedRegionBegin, sortedRegionEnd) &&
         !hasSubtreeRootDeeperThanFirstRoot(subtreeRootDepth, sortedRegionBegin, sortedRegionEnd);
}
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
