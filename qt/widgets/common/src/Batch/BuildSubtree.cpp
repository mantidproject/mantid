#include "MantidQtWidgets/Common/Batch/BuildSubtree.h"
#include <tuple>

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {
BuildSubtree::BuildSubtree(QStandardItemModel &model) : m_model(model) {}

void BuildSubtree::operator()(QStandardItem *parentOfSubtreeRootItem,
                              RowLocation const &parentOfSubtreeRoot, int index,
                              Subtree const &subtree) {
  if (!subtree.empty())
    buildRecursively(parentOfSubtreeRootItem, index, parentOfSubtreeRoot, 0,
                     subtree.cbegin(), subtree.cend());
}

auto BuildSubtree::buildRecursively(QStandardItem *parentItem, int index,
                                    RowLocation const &parent, int depth,
                                    SubtreeConstIterator current,
                                    SubtreeConstIterator end)
    -> SubtreeConstIterator {
  parentItem->insertRow(index, rowFromRowText((*current).cells()));
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
      parentItem->appendRow(rowFromRowText((*current).cells()));
      ++current;
    }
  }
  return end;
}

} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
