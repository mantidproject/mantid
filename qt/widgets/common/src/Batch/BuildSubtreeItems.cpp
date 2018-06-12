#include "MantidQtWidgets/Common/Batch/BuildSubtreeItems.h"

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

BuildSubtreeItems::BuildSubtreeItems(
    QtStandardItemTreeModelAdapter &adaptedModel,
    RowLocationAdapter const &rowLocationAdapter)
    : m_adaptedMainModel(adaptedModel), m_rowLocations(rowLocationAdapter) {}

auto BuildSubtreeItems::buildRecursively(int insertionIndex,
                                         RowLocation const &parent,
                                         SubtreeConstIterator current,
                                         SubtreeConstIterator end)
    -> SubtreeConstIterator {
  auto lastRowInsertedAtThisDepth = QModelIndexForMainModel();
  auto const insertionDepth = (*current).location().depth();
  while (current != end) {
    auto currentRow = (*current).location();
    auto currentDepth = currentRow.depth();
    // `currentDepth` tracks the depth of the currentRow whearas
    // `insertionDepth` tracks
    // the depth of the locations this recursive call will insert at.

    if (insertionDepth < currentDepth) {
      current = buildRecursively(currentRow.rowRelativeToParent(),
                                 parent.child(lastRowInsertedAtThisDepth.row()),
                                 current, end);
    } else if (insertionDepth > currentDepth) {
      return current;
    } else {
      lastRowInsertedAtThisDepth = m_adaptedMainModel.insertChildRow(
          modelIndexAt(parent), insertionIndex, (*current).cells());
      ++insertionIndex;
      ++current;
    }
  }
  return end;
}

void BuildSubtreeItems::operator()(RowLocation const &parentOfSubtreeRoot,
                                   int firstInsertionIndex,
                                   Subtree const &subtree) {
  if (!subtree.empty()) {
    buildRecursively(firstInsertionIndex, parentOfSubtreeRoot, subtree.cbegin(),
                     subtree.cend());
  }
}

QModelIndexForMainModel
BuildSubtreeItems::modelIndexAt(RowLocation const &parent) const {
  return m_rowLocations.indexAt(parent);
}
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
