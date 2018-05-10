#include "MantidQtWidgets/Common/Batch/BuildSubtreeItems.h"

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

BuildSubtreeItems::BuildSubtreeItems(
    QtStandardItemTreeModelAdapter &adaptedModel,
    RowLocationAdapter const &rowLocationAdapter)
    : m_adaptedMainModel(adaptedModel), m_rowLocations(rowLocationAdapter) {}

void BuildSubtreeItems::operator()(RowLocation const &parentOfSubtreeRoot,
                                   int index, Subtree const &subtree) {
  if (!subtree.empty()) {
    buildRecursively(index, parentOfSubtreeRoot, 0, subtree.cbegin(),
                     subtree.cend());
  }
}

auto BuildSubtreeItems::buildRecursively(int index, RowLocation const &parent,
                                         int depth,
                                         SubtreeConstIterator current,
                                         SubtreeConstIterator end)
    -> SubtreeConstIterator {
  auto insertedRow = m_adaptedMainModel.insertChildRow(
      m_rowLocations.indexAt(parent), index, (*current).cells());
  ++current;
  while (current != end) {
    auto currentRow = (*current).location();
    auto currentDepth = currentRow.depth();

    if (depth < currentDepth) {
      current = buildRecursively(currentRow.rowRelativeToParent(),
                                 parent.child(insertedRow.row()), depth + 1,
                                 current, end);
    } else if (depth > currentDepth) {
      return current;
    } else {
      m_adaptedMainModel.appendChildRow(m_rowLocations.indexAt(parent),
                                        (*current).cells());
      ++current;
    }
  }
  return end;
}

} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
