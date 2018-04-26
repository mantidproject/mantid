#include "MantidQtWidgets/Common/Batch/ExtractSubtrees.h"
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

void ExtractSubtrees::nodeWasSubtreeRoot(RowLocation const &rowLocation) {
  previousWasRoot = true;
  previousNode = rowLocation;
}

void ExtractSubtrees::nodeWasNotSubtreeRoot(RowLocation const &rowLocation) {
  previousWasRoot = false;
  previousNode = rowLocation;
}

bool ExtractSubtrees::isChildOfPrevious(RowLocation const &location) const {
  return location.isChildOf(previousNode);
}

bool ExtractSubtrees::isSiblingOfPrevious(RowLocation const &location) const {
  return location.isSiblingOf(previousNode);
}

auto ExtractSubtrees::operator()(std::vector<RowLocation> region,
                                  std::vector<Row> regionData)
    -> boost::optional<std::vector<Subtree>> {
  assertOrThrow(
      region.size() == regionData.size(),
      "ExtractSubtrees: region must have a length identical to regionData");
  if (!region.empty()) {
    std::sort(region.begin(), region.end());
    nodeWasSubtreeRoot(*region.cbegin());
    auto subtrees = std::vector<Subtree>();
    auto current = region.begin() + 1;
    auto currentData = regionData.begin() + 1;
    auto subtree = Subtree({{RowLocation(), std::move(*regionData.cbegin())}});

    auto currentSubtreeRoot = region.cbegin();
    for (; current != region.end(); ++current, ++currentData) {
      auto &currentNode = *current;
      if (isChildOfPrevious(currentNode) ||
          (!previousWasRoot && isSiblingOfPrevious(currentNode))) {
        nodeWasNotSubtreeRoot(currentNode);
        subtree.emplace_back(currentNode.relativeTo(*currentSubtreeRoot), *currentData);
      } else if (currentNode.isDescendantOf(*currentSubtreeRoot)) {
        if (previousNode.depth() < currentNode.depth())
          return boost::none;
        else
          nodeWasNotSubtreeRoot(currentNode); // Dead case?
        subtree.emplace_back(currentNode.relativeTo(*currentSubtreeRoot), std::move(*currentData));
      } else {
        nodeWasSubtreeRoot(currentNode);
        subtrees.emplace_back(std::move(subtree));
        subtree = Subtree({{RowLocation(), std::move(*currentData)}});
        currentSubtreeRoot = current;
      }
    }
    subtrees.emplace_back(std::move(subtree));
    return subtrees;
  } else {
    return {};
  }
}
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
