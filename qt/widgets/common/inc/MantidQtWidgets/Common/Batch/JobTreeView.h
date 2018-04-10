#ifndef MANTIDQTMANTIDWIDGETS_JOBTREEVIEW_H_
#define MANTIDQTMANTIDWIDGETS_JOBTREEVIEW_H_
#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include "MantidQtWidgets/Common/Batch/QtTreeCursorNavigation.h"
#include "MantidQtWidgets/Common/Batch/QtStandardItemTreeAdapter.h"

#include <QTreeView>

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

class IJobTreeViewSubscriber {
public:
  virtual void notifyCellChanged(RowLocation itemIndex, int column,
                                 std::string newValue) = 0;
  virtual void notifyRowInserted(RowLocation itemIndex) = 0;
  virtual void notifyRowRemoved(RowLocation itemIndex) = 0;
  virtual void
  notifySelectedRowsChanged(std::vector<RowLocation> const &selection) = 0;
};

class EXPORT_OPT_MANTIDQT_COMMON JobTreeView : public QTreeView {
  Q_OBJECT
public:
  // JobTreeView(QWidget *parent = nullptr);
  JobTreeView(QStringList const &columnHeadings, QWidget *parent = nullptr);

  void subscribe(IJobTreeViewSubscriber &subscriber);

  void insertChildRowOf(RowLocation const &parent, int beforeRow,
                        std::vector<std::string> const &rowText);
  void insertChildRowOf(RowLocation const &parent, int beforeRow);
  void appendChildRowOf(RowLocation const &parent);
  void appendChildRowOf(RowLocation const &parentLocation,
                        std::vector<std::string> const &rowText);

  void removeRowAt(RowLocation const &location);

  std::vector<std::string> rowTextAt(RowLocation const &location) const;
  void setRowTextAt(RowLocation const &location,
                    std::vector<std::string> const &rowText);

  std::string textAt(RowLocation location, int column);
  void setTextAt(RowLocation location, int column, std::string const &cellText);

  QModelIndex moveCursor(CursorAction cursorAction,
                         Qt::KeyboardModifiers modifiers) override;

protected:
  void keyPressEvent(QKeyEvent *event) override;
  void setHeaderLabels(QStringList const &columnHeadings);

private:
  void make(QModelIndex const &){};

  QModelIndex expanded(QModelIndex const &index);
  QModelIndex editAt(QModelIndex const &index);

  QModelIndex applyNavigationResult(QtTreeCursorNavigationResult const &result);
  QModelIndex findOrMakeCellBelow(QModelIndex const &index);

  QList<QStandardItem *>
  rowFromRowText(std::vector<std::string> const &rowText) const;
  std::vector<std::string> rowTextFromRow(QModelIndex firstCellIndex) const;

  QModelIndex modelIndexAt(RowLocation const &location, int column = 0) const;
  RowLocation rowLocationAt(QModelIndex const &index) const;
  QStandardItem *modelItemAt(RowLocation const &location, int column = 0) const;

  QStandardItem *modelItemFromIndex(QModelIndex const &location) const;

  QtTreeCursorNavigation navigation() const;
  QtStandardItemMutableTreeAdapter adaptedModel();
  QtStandardItemTreeAdapter const adaptedModel() const;

  IJobTreeViewSubscriber *m_notifyee;
  QStandardItemModel m_model;
};
}
}
}
#endif // MANTIDQTMANTIDWIDGETS_JOBTREEVIEW_H_
