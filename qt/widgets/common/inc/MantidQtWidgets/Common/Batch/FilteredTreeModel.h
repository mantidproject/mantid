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
#ifndef MANTIDQTMANTIDWIDGETS_FILTEREDTREEMODEL_H_
#define MANTIDQTMANTIDWIDGETS_FILTEREDTREEMODEL_H_
#include <QSortFilterProxyModel>
#include <memory>
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include "MantidQtWidgets/Common/Batch/RowPredicate.h"
#include "MantidQtWidgets/Common/Batch/RowLocationAdapter.h"
#include "MantidQtWidgets/Common/DllOption.h"
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

class EXPORT_OPT_MANTIDQT_COMMON FilteredTreeModel
    : public QSortFilterProxyModel {
public:
  FilteredTreeModel(RowLocationAdapter rowLocation, QObject *parent = nullptr);
  void setPredicate(std::unique_ptr<RowPredicate> predicate);
  void resetPredicate();
  bool isReset() const;
  RowLocation rowLocationAt(QModelIndex const &index) const;

protected:
  bool filterAcceptsRow(int row, const QModelIndex &parent) const override;

private:
  std::unique_ptr<RowPredicate> m_predicate;
  RowLocationAdapter m_rowLocation;
};
}
}
}
#endif // MANTIDQTMANTIDWIDGETS_FILTEREDTREEMODEL_H_
