#ifndef MANTIDQTMANTIDWIDGETS_BUILDSUBTREE_H_
#define MANTIDQTMANTIDWIDGETS_BUILDSUBTREE_H_
#include "MantidQtWidgets/Common/Batch/AssertOrThrow.h"
#include "MantidQtWidgets/Common/Batch/QtStandardItemTreeAdapter.h"
#include "MantidQtWidgets/Common/Batch/RowLocationAdapter.h"
#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/Batch/Subtree.h"
#include <QStandardItem>

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

class EXPORT_OPT_MANTIDQT_COMMON BuildSubtreeItems {
public:
  using SubtreeConstIterator = typename Subtree::const_iterator;
  BuildSubtreeItems(QtStandardItemTreeModelAdapter &adaptedModel,
                    RowLocationAdapter const &rowLocationAdapter);
  QModelIndexForMainModel modelIndexAt(RowLocation const &parent) const;

  void operator()(RowLocation const &parentOfSubtreeRoot, int index,
                  Subtree const &subtree);

  SubtreeConstIterator buildRecursively(int index, RowLocation const &parent,
                                        SubtreeConstIterator current,
                                        SubtreeConstIterator end);

private:
  QtStandardItemTreeModelAdapter &m_adaptedMainModel;
  RowLocationAdapter m_rowLocations;
};

} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
#endif
