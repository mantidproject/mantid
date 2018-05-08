#include "MantidQtWidgets/Common/Batch/ExtractSubtrees.h"
#include <tuple>
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

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
      if (!extractionResult.isUnsuitableTree()) {
        done = extractionResult.shouldNotContinue();
        std::tie(rowIt, rowDataIt) = extractionResult.currentPosition();
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

auto ExtractSubtrees::extractSubtreeRecursive(
    Subtree &subtree, RowLocation const &rootRelativeToTree, RowLocation parent,
    int currentDepth, RowLocationConstIterator currentRow,
    RowLocationConstIterator endRow, RowDataConstIterator currentRowData)
    -> RecursiveSubtreeExtractionResult {
  auto childIndex = 0;
  while (currentRow != endRow) {
    auto depthOfCurrentRow = (*currentRow).depth();
    if (depthOfCurrentRow > currentDepth) {
      if (isCorrectDepthForChild(currentDepth, depthOfCurrentRow)) {
        auto extractionResult = extractSubtreeRecursive(
            subtree, rootRelativeToTree, subtree.back().location(),
            currentDepth + 1, currentRow, endRow, currentRowData);
        if (!extractionResult.isUnsuitableTree()) {
          if (extractionResult.shouldContinue())
            std::tie(currentRow, currentRowData) =
                extractionResult.currentPosition();
          else
            return extractionResult;
        } else {
          return unsuitableTree();
        }
      } else
        return unsuitableTree();
    } else if (depthOfCurrentRow < currentDepth) {
      if (currentIsInDifferentSubtree(depthOfCurrentRow, rootRelativeToTree))
        return finishedSubtree(currentRow, currentRowData);
      else
        return continueOnLevelAbove(currentRow, currentRowData);
    } else {
      subtree.emplace_back(parent.child(childIndex), *currentRowData);
      ++childIndex;
      ++currentRow;
      ++currentRowData;
    }
  }
  return finishedSubtree(currentRow, currentRowData);
}

bool ExtractSubtrees::isChildOfPrevious(RowLocation const &location) const {
  return location.isChildOf(m_previousNode);
}

bool ExtractSubtrees::isSiblingOfPrevious(RowLocation const &location) const {
  return location.isSiblingOf(m_previousNode);
}

bool ExtractSubtrees::isCorrectDepthForChild(int parentDepth,
                                             int maybeChildDepth) {
  return maybeChildDepth == parentDepth + 1;
}

bool ExtractSubtrees::currentIsInDifferentSubtree(
    int depthOfCurrentRow, RowLocation const &rootRelativeToTree) {
  return depthOfCurrentRow < rootRelativeToTree.depth();
}

RecursiveSubtreeExtractionResult
ExtractSubtrees::continueOnLevelAbove(RowLocationConstIterator currentRow,
                                      RowDataConstIterator currentRowData) {
  return RecursiveSubtreeExtractionResult::continueOnLevelAbove(currentRow,
                                                                currentRowData);
}

RecursiveSubtreeExtractionResult
ExtractSubtrees::finishedSubtree(RowLocationConstIterator currentRow,
                                 RowDataConstIterator currentRowData) {
  return RecursiveSubtreeExtractionResult::finishedSubtree(currentRow,
                                                           currentRowData);
}

RecursiveSubtreeExtractionResult ExtractSubtrees::unsuitableTree() {
  return RecursiveSubtreeExtractionResult::unsuitableTree();
}

RecursiveSubtreeExtractionResult::RecursiveSubtreeExtractionResult(
    bool shouldContinue, bool isUnsuitableTree,
    std::pair<RowLocationConstIterator, RowDataConstIterator> const &
        currentPosition)
    : m_shouldContinue(shouldContinue), m_isUnsuitableTree(isUnsuitableTree),
      m_currentPosition(currentPosition) {}

bool RecursiveSubtreeExtractionResult::shouldContinue() const {
  return m_shouldContinue;
}

bool RecursiveSubtreeExtractionResult::shouldNotContinue() const {
  return !m_shouldContinue;
}

bool RecursiveSubtreeExtractionResult::isUnsuitableTree() const {
  return m_isUnsuitableTree;
}

auto RecursiveSubtreeExtractionResult::currentPosition() const
    -> std::pair<RowLocationConstIterator, RowDataConstIterator> const & {
  return m_currentPosition;
}

RecursiveSubtreeExtractionResult
RecursiveSubtreeExtractionResult::finishedSubtree(
    RowLocationConstIterator currentRow, RowDataConstIterator currentRowData) {
  return RecursiveSubtreeExtractionResult(/*shouldContinue=*/false,
                                          /*regionHasGaps=*/false,
                                          {currentRow, currentRowData});
}

RecursiveSubtreeExtractionResult
RecursiveSubtreeExtractionResult::continueOnLevelAbove(
    RowLocationConstIterator currentRow, RowDataConstIterator currentRowData) {
  return RecursiveSubtreeExtractionResult(/*shouldContinue=*/true,
                                          /*regionHasGaps=*/false,
                                          {currentRow, currentRowData});
}

RecursiveSubtreeExtractionResult
RecursiveSubtreeExtractionResult::unsuitableTree() {
  return RecursiveSubtreeExtractionResult(
      /*shouldContinue=*/false,
      /*regionHasGaps=*/true,
      std::pair<RowLocationConstIterator, RowDataConstIterator>());
}
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
