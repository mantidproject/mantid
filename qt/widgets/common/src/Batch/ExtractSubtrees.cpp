#include "MantidQtWidgets/Common/Batch/ExtractSubtrees.h"
#include <tuple>
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

bool ExtractSubtrees::isChildOfPrevious(RowLocation const &location) const {
  return location.isChildOf(previousNode);
}

bool ExtractSubtrees::isSiblingOfPrevious(RowLocation const &location) const {
  return location.isSiblingOf(previousNode);
}

bool ExtractSubtrees::isCorrectDepthForChild(int parentDepth,
                                             int maybeChildDepth) {
  return maybeChildDepth == parentDepth + 1;
}

bool ExtractSubtrees::currentIsInDifferentSubtree(
    int depthOfCurrentRow, RowLocation const &rootRelativeToTree) {
  return depthOfCurrentRow < rootRelativeToTree.depth();
}

RecursiveSubtreeExtractionResult const
ExtractSubtrees::finishedSubtree(RowLocationConstIterator currentRow,
                                 RowDataConstIterator currentRowData) {
  return RecursiveSubtreeExtractionResult(true, {currentRow, currentRowData});
}

RecursiveSubtreeExtractionResult const
ExtractSubtrees::continueOnLevelAbove(RowLocationConstIterator currentRow,
                                      RowDataConstIterator currentRowData) {
  return RecursiveSubtreeExtractionResult(false, {currentRow, currentRowData});
}

RecursiveSubtreeExtractionResult const ExtractSubtrees::reportUnsuitableTree() {
  return RecursiveSubtreeExtractionResult();
}

RecursiveSubtreeExtractionResult::RecursiveSubtreeExtractionResult(
    bool shouldNotContinue,
    std::pair<RowLocationConstIterator, RowDataConstIterator> const &
        currentPosition)
    : shouldNotContinue(shouldNotContinue), regionHasGaps(false),
      currentPosition(currentPosition) {}

RecursiveSubtreeExtractionResult::RecursiveSubtreeExtractionResult()
    : shouldNotContinue(false), regionHasGaps(true), currentPosition() {}

auto ExtractSubtrees::extractSubtreeRecursive(
    Subtree &subtree, RowLocation const &rootRelativeToTree, RowLocation parent,
    int currentDepth, RowLocationConstIterator currentRow,
    RowLocationConstIterator endRow, RowDataConstIterator currentRowData)
    -> RecursiveSubtreeExtractionResult {
  auto childCount = 0;
  while (currentRow != endRow) {
    auto depthOfCurrentRow = (*currentRow).depth();
    if (depthOfCurrentRow > currentDepth) {
      if (isCorrectDepthForChild(currentDepth, depthOfCurrentRow)) {
        auto extractionResult = extractSubtreeRecursive(
            subtree, rootRelativeToTree, subtree.back().location(),
            currentDepth + 1, currentRow, endRow, currentRowData);
        if (!extractionResult.regionHasGaps) {
          if (!extractionResult.shouldNotContinue)
            std::tie(currentRow, currentRowData) =
                extractionResult.currentPosition;
          else
            return extractionResult;
        } else {
          return reportUnsuitableTree();
        }
      } else
        return reportUnsuitableTree();
    } else if (depthOfCurrentRow < currentDepth) {
      if (currentIsInDifferentSubtree(depthOfCurrentRow, rootRelativeToTree))
        return finishedSubtree(currentRow, currentRowData);
      else
        return continueOnLevelAbove(currentRow, currentRowData);
    } else {
      subtree.emplace_back(parent.child(childCount), *currentRowData);
      ++childCount;
      ++currentRow;
      ++currentRowData;
    }
  }
  return finishedSubtree(currentRow, currentRowData);
}

auto ExtractSubtrees::operator()(std::vector<RowLocation> region,
                                 std::vector<std::vector<Cell>> regionData)
    -> boost::optional<std::vector<Subtree>> {
  assertOrThrow(
      region.size() == regionData.size(),
      "ExtractSubtrees: region must have a length identical to regionData");
  std::sort(region.begin(), region.end());
  auto rowIt = region.cbegin();
  auto rowDataIt = regionData.cbegin();
  auto done = false;
  auto subtrees = std::vector<Subtree>();

  while (rowIt != region.end() && !done) {
    auto subtree = Subtree({Row(RowLocation(), std::move(*rowDataIt))});
    auto rowDepth = (*rowIt).depth();
    auto nextRowIt = rowIt + 1;
    if (nextRowIt == region.cend() || rowDepth + 1 >= (*nextRowIt).depth()) {
      auto extractionResult = extractSubtreeRecursive(
          subtree, *rowIt, subtree[0].location(), rowDepth + 1, nextRowIt,
          region.end(), rowDataIt + 1);
      if (!extractionResult.regionHasGaps) {
        done = extractionResult.shouldNotContinue;
        std::tie(rowIt, rowDataIt) = extractionResult.currentPosition;
        subtrees.emplace_back(std::move(subtree));
      } else {
        return boost::none;
      }
    } else {
      return boost::none;
    }
  }
  return subtrees;
}
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
