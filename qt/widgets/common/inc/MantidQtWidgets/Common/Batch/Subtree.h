/**
Copyright &copy; 2011-16 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
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
