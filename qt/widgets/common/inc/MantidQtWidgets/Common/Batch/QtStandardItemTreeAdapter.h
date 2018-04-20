#ifndef MANTIDQTMANTIDWIDGETS_TREEITEMMODELADAPTER_H_
#define MANTIDQTMANTIDWIDGETS_TREEITEMMODELADAPTER_H_
#include "MantidQtWidgets/Common/DllOption.h"
#include <QStandardItemModel>

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

class EXPORT_OPT_MANTIDQT_COMMON QtStandardItemTreeAdapter {
public:
  QtStandardItemTreeAdapter(QStandardItemModel const &model);

  QModelIndex rootModelIndex() const;

  QStandardItem const *modelItemFromIndex(QModelIndex const &index) const;

  QList<QStandardItem *>
  rowFromRowText(std::vector<std::string> const &rowText) const;

  std::string textFromCell(QModelIndex index) const;
  std::vector<std::string> rowTextFromRow(QModelIndex firstCellIndex) const;

  QList<QStandardItem *> emptyRow() const;

//  int childRowCount(QModelIndex location);

private:
  QStandardItemModel const &model() const;
  QStandardItemModel const* m_model;
};

class EXPORT_OPT_MANTIDQT_COMMON QtStandardItemMutableTreeAdapter : public QtStandardItemTreeAdapter {
public:
  QtStandardItemMutableTreeAdapter(QStandardItemModel &model);

  QModelIndex appendEmptySiblingRow(QModelIndex const &index);
  QModelIndex appendSiblingRow(QModelIndex const &index,
                               QList<QStandardItem *> cells);

  QModelIndex appendEmptyChildRow(QModelIndex const &parent);
  QModelIndex appendChildRow(QModelIndex const &parent,
                             QList<QStandardItem *> cells);

  QModelIndex insertChildRow(QModelIndex const &parent, int column,
                             QList<QStandardItem *> cells);
  QModelIndex insertEmptyChildRow(QModelIndex const &parent, int column);
  QStandardItem *modelItemFromIndex(QModelIndex const &index);

  void setTextAtCell(QModelIndex index, std::string const& newText);
  void removeRowAt(QModelIndex const &index);

private:
  QStandardItemModel &model();
  QStandardItemModel *m_model;
};
}
}
}
#endif // MANTIDQTMANTIDWIDGETS_TREEITEMMODELADAPTER_H_
