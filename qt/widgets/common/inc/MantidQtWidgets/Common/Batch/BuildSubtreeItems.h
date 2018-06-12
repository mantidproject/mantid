/**
See the developer documentation for Batch Widget at
developer.mantidproject.org/BatchWidget/index.html

Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
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
