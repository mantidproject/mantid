#ifndef MANTIDQTMANTIDWIDGETS_FILTERLEAFNODES_H_
#define MANTIDQTMANTIDWIDGETS_FILTERLEAFNODES_H_
#include <QSortFilterProxyModel>
#include <functional>
#include <memory>
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include "MantidQtWidgets/Common/Batch/RowLocationAdapter.h"
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

class RowPredicate {
public:
  bool operator()(RowLocation const& row) const;
  virtual ~RowPredicate() = default;
protected:
  virtual bool rowMeetsCriteria(RowLocation const& row) const = 0;
};

class QtFilterLeafNodes : public QSortFilterProxyModel {
public:
  QtFilterLeafNodes(RowLocationAdapter rowLocation, QObject *parent = nullptr);
  void setPredicate(std::unique_ptr<RowPredicate> predicate);
  void resetPredicate();
  bool isReset() const;
  RowLocation rowLocationAt(QModelIndex const& index) const;
protected:
  bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
private:
  std::unique_ptr<RowPredicate> m_predicate;
  RowLocationAdapter m_rowLocation;
};
}
}
}
#endif // MANTIDQTMANTIDWIDGETS_FILTERLEAFNODES_H_
