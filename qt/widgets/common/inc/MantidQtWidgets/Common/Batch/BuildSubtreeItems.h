// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/**
See the developer documentation for Batch Widget at
developer.mantidproject.org/BatchWidget/index.html
*/
#pragma once
#include "MantidQtWidgets/Common/Batch/AssertOrThrow.h"
#include "MantidQtWidgets/Common/Batch/QtStandardItemTreeAdapter.h"
#include "MantidQtWidgets/Common/Batch/RowLocationAdapter.h"
#include "MantidQtWidgets/Common/Batch/Subtree.h"
#include "MantidQtWidgets/Common/DllOption.h"
#include <QStandardItem>

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

class EXPORT_OPT_MANTIDQT_COMMON BuildSubtreeItems {
public:
  using SubtreeConstIterator = typename Subtree::const_iterator;
  BuildSubtreeItems(QtStandardItemTreeModelAdapter &adaptedModel, RowLocationAdapter const &rowLocationAdapter);
  QModelIndexForMainModel modelIndexAt(RowLocation const &parent) const;

  void operator()(RowLocation const &parentOfSubtreeRoot, int index, Subtree const &subtree);

  SubtreeConstIterator buildRecursively(int index, RowLocation const &parent, SubtreeConstIterator current,
                                        SubtreeConstIterator end);

private:
  QtStandardItemTreeModelAdapter &m_adaptedMainModel;
  RowLocationAdapter m_rowLocations;
};

} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
