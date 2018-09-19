#include "MantidQtWidgets/Common/Batch/ExtractSubtrees.h"
#include <boost/iterator/transform_iterator.hpp>
#include <tuple>

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

auto ExtractSubtrees::findEndOfSubtree(
    RandomAccessConstRowIterator subtreeBegin,
    RandomAccessConstRowIterator regionEnd, int subtreeRootDepth) const
    -> RandomAccessConstRowIterator {
  return std::find_if(std::next(subtreeBegin), regionEnd,
                      [subtreeRootDepth](Row const &row) -> bool {
                        return row.location().depth() == subtreeRootDepth;
                      });
}

Subtree ExtractSubtrees::makeSubtreeFromRows(
    RowLocation subtreeRootLocation, RandomAccessConstRowIterator subtreeBegin,
    RandomAccessConstRowIterator subtreeEnd) const {
  auto subtree = std::vector<Row>();
  subtree.reserve(std::distance(subtreeBegin, subtreeEnd));
  std::transform(subtreeBegin, subtreeEnd, std::back_inserter(subtree),
                 [&subtreeRootLocation](Row const &row) -> Row {
                   return Row(row.location().relativeTo(subtreeRootLocation),
                              row.cells());
                 });
  return subtree;
}

std::vector<Subtree>
ExtractSubtrees::makeSubtreesFromRows(std::vector<Row> const &rows,
                                      int subtreeRootDepth) const {
  auto subtrees = std::vector<Subtree>();
  auto current = rows.cbegin();
  while (current != rows.cend()) {
    auto subtreeRootLocation = (*current).location();
    auto subtreeBegin = current;
    auto subtreeEnd =
        findEndOfSubtree(subtreeBegin, rows.cend(), subtreeRootDepth);
    subtrees.emplace_back(
        makeSubtreeFromRows(subtreeRootLocation, subtreeBegin, subtreeEnd));
    current = subtreeEnd;
  }
  return subtrees;
}

RowLocation rowToRowLocation(Row const &row) { return row.location(); }

auto ExtractSubtrees::operator()(std::vector<Row> region) const
    -> boost::optional<std::vector<Subtree>> {
  std::sort(region.begin(), region.end());
  if (!region.empty()) {
    auto subtreeRootDepth = rowToRowLocation(region[0]).depth();
    auto rowLocationBegin =
        boost::make_transform_iterator(region.cbegin(), &rowToRowLocation);
    auto rowLocationEnd =
        boost::make_transform_iterator(region.cend(), &rowToRowLocation);

    if (allSubtreeRootsShareAParentAndAllSubtreeNodesAreConnected(
            subtreeRootDepth, rowLocationBegin, rowLocationEnd))
      return makeSubtreesFromRows(region, subtreeRootDepth);
    else
      return boost::none;
  } else {
    return std::vector<Subtree>();
  }
}

} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
