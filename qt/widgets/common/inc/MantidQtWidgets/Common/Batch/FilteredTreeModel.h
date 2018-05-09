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
