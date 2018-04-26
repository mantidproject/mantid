#ifndef MANTIDQTMANTIDWIDGETS_FILTERLEAFNODES_H_
#define MANTIDQTMANTIDWIDGETS_FILTERLEAFNODES_H_
#include <QSortFilterProxyModel>
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

class QtFilterLeafNodes : public QSortFilterProxyModel {
public:
  QtFilterLeafNodes(QObject *parent = nullptr);

protected:
  bool filterAcceptsRow(int row, const QModelIndex &parent) const override;
};
}
}
}
#endif // MANTIDQTMANTIDWIDGETS_FILTERLEAFNODES_H_
