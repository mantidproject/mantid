#include "MantidQtWidgets/Common/Batch/BuildSubtreeItems.h"
#include <tuple>

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {
void BuildSubtreeItems::operator()(QStandardItem *parentOfSubtreeRootItem,
                                   RowLocation const &parentOfSubtreeRoot,
                                   int index, Subtree const &subtree) {
  if (!subtree.empty())
    buildRecursively(parentOfSubtreeRootItem, index, parentOfSubtreeRoot, 0,
                     subtree.cbegin(), subtree.cend());
}

auto BuildSubtreeItems::buildRecursively(QStandardItem *parentItem, int index,
                                         RowLocation const &parent, int depth,
                                         SubtreeConstIterator current,
                                         SubtreeConstIterator end)
    -> SubtreeConstIterator {
  parentItem->insertRow(index, rowFromCells((*current).cells()));
  ++current;
  while (current != end) {
    auto currentRow = (*current).location();
    auto currentDepth = currentRow.depth();

    if (depth < currentDepth) {
      current = buildRecursively(parentItem->child(index),
                                 currentRow.rowRelativeToParent(),
                                 parent.child(currentRow.rowRelativeToParent()),
                                 depth + 1, current, end);
    } else if (depth > currentDepth) {
      return current;
    } else {
      parentItem->appendRow(rowFromCells((*current).cells()));
      ++current;
    }
  }
  return end;
}

} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
