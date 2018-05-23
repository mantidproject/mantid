#ifndef MANTIDQTMANTIDWIDGETS_SUBTREE_H_
#define MANTIDQTMANTIDWIDGETS_SUBTREE_H_
#include "MantidQtWidgets/Common/Batch/Row.h"

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

using Subtree = std::vector<Row>;

template <typename RowLocationConstIterator>
bool hasSubtreeRootHigherThanFirstRoot(
    RowLocationConstIterator sortedRegionBegin,
    RowLocationConstIterator sortedRegionEnd) {
  auto firstLocationAtMinimumDepth =
      std::min_element(sortedRegionBegin, sortedRegionEnd,
                       [](RowLocation const &lhs, RowLocation const &rhs)
                           -> bool { return lhs.depth() < rhs.depth(); });
  return firstLocationAtMinimumDepth != sortedRegionBegin;
}

template <typename RowLocationConstIterator>
bool maximumIncreaseInDepthIsOne(RowLocationConstIterator sortedRegionBegin,
                                 RowLocationConstIterator sortedRegionEnd) {
  return std::adjacent_find(sortedRegionBegin, sortedRegionEnd,
                            [](RowLocation const &previous,
                               RowLocation const &current) -> bool {
                              return (previous.depth() - current.depth()) < -1;
                            }) == sortedRegionEnd;
}

template <typename RowLocationConstIterator>
bool hasSubtreeRootLowerThanFirstRoot(
    int firstSubtreeRootDepth, RowLocationConstIterator sortedRegionBegin,
    RowLocationConstIterator sortedRegionEnd) {
  return std::adjacent_find(
             sortedRegionBegin, sortedRegionEnd,
             [firstSubtreeRootDepth](RowLocation const &previous,
                                     RowLocation const &current) -> bool {
               return current.depth() > firstSubtreeRootDepth &&
                      current.depth() <= previous.depth() &&
                      !pathsSameUntilDepth(firstSubtreeRootDepth, current,
                                           previous);
             }) != sortedRegionEnd;
}

template <typename RowLocationConstIterator>
bool allRootsAtSameDepthAndNoDepthGaps(
    int subtreeRootDepth, RowLocationConstIterator sortedRegionBegin,
    RowLocationConstIterator sortedRegionEnd) {
  return !hasSubtreeRootHigherThanFirstRoot(sortedRegionBegin,
                                            sortedRegionEnd) &&
         !hasSubtreeRootLowerThanFirstRoot(subtreeRootDepth, sortedRegionBegin,
                                           sortedRegionEnd) &&
         maximumIncreaseInDepthIsOne(sortedRegionBegin, sortedRegionEnd);
}
}
}
}
#endif // MANTIDQTMANTIDWIDGETS_SUBTREE_H_
