#ifndef MANTIDQTMANTIDWIDGETS_JOBTREEVIEW_H_
#define MANTIDQTMANTIDWIDGETS_JOBTREEVIEW_H_
#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include "MantidQtWidgets/Common/Batch/QtTreeCursorNavigation.h"
#include "MantidQtWidgets/Common/Batch/QtStandardItemTreeAdapter.h"
#include <boost/optional.hpp>

#include <QTreeView>

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

class EXPORT_OPT_MANTIDQT_COMMON JobTreeViewSubscriber {
public:
  virtual void notifyCellChanged(RowLocation const &itemIndex, int column,
                                 std::string const &newValue) = 0;
  virtual void notifyRowInserted(RowLocation const &newRowLocation) = 0;
  virtual void notifyRemoveRowsRequested(
      std::vector<RowLocation> const &locationsOfRowsToRemove) = 0;

  virtual void notifyCopyRowsRequested(
      std::vector<RowLocation> const &locationsOfRowsToCopy) = 0;
  virtual ~JobTreeViewSubscriber() = default;
};

class EXPORT_OPT_MANTIDQT_COMMON JobTreeView : public QTreeView {
  Q_OBJECT
public:
  // JobTreeView(QWidget *parent = nullptr);
  JobTreeView(QStringList const &columnHeadings, QWidget *parent = nullptr);

  void subscribe(JobTreeViewSubscriber &subscriber);

  void insertChildRowOf(RowLocation const &parent, int beforeRow,
                        std::vector<std::string> const &rowText);
  void insertChildRowOf(RowLocation const &parent, int beforeRow);
  void appendChildRowOf(RowLocation const &parent);
  void appendChildRowOf(RowLocation const &parentLocation,
                        std::vector<std::string> const &rowText);

  void removeRowAt(RowLocation const &location);
  void removeRows(std::vector<RowLocation> rowsToRemove);

  template <typename InputIterator>
  void removeRows(InputIterator begin, InputIterator end);

  std::vector<std::string> rowTextAt(RowLocation const &location) const;
  void setRowTextAt(RowLocation const &location,
                    std::vector<std::string> const &rowText);

  std::string textAt(RowLocation location, int column) const;
  void setTextAt(RowLocation location, int column, std::string const &cellText);

  QModelIndex moveCursor(CursorAction cursorAction,
                         Qt::KeyboardModifiers modifiers) override;
  std::vector<RowLocation> selectedRowLocations() const;

  bool isOnlyChild(QModelIndex const &index) const;
  bool isOnlyChildOfRoot(QModelIndex const &index) const;
  QModelIndex siblingIfExistsElseParent(QModelIndex const &index) const;
  bool rowRemovalWouldBeIneffective(QModelIndex const &indexToRemove) const;

  using QTreeView::edit;

protected:
  void keyPressEvent(QKeyEvent *event) override;
  bool edit(const QModelIndex &index, EditTrigger trigger,
            QEvent *event) override;
  void setHeaderLabels(QStringList const &columnHeadings);
  void removeSelectedRequested();
  void copySelectedRequested();

protected slots:
  void commitData(QWidget *) override;

private:
  void make(QModelIndex const &){};
  void appendAndEditAtChildRow();
  void appendAndEditAtRowBelow();
  bool indexesAreOnSameRow(QModelIndex const &a, QModelIndex const &b) const;

  QModelIndex expanded(QModelIndex const &index);
  void editAt(QModelIndex const &index);

  QModelIndex applyNavigationResult(QtTreeCursorNavigationResult const &result);
  std::pair<QModelIndex, bool> findOrMakeCellBelow(QModelIndex const &index);

  QList<QStandardItem *>
  rowFromRowText(std::vector<std::string> const &rowText) const;
  std::vector<std::string> rowTextFromRow(QModelIndex firstCellIndex) const;

  QModelIndex modelIndexAt(RowLocation const &location, int column = 0) const;
  boost::optional<QModelIndex> modelIndexIfExistsAt(RowLocation const &location, int column = 0) const;
  RowLocation rowLocationAt(QModelIndex const &index) const;
  QStandardItem *modelItemAt(RowLocation const &location, int column = 0) const;

  QStandardItem *modelItemFromIndex(QModelIndex const &location) const;

  bool hasEditorOpen() const;

  QtTreeCursorNavigation navigation() const;
  QtStandardItemMutableTreeAdapter adaptedModel();
  QtStandardItemTreeAdapter const adaptedModel() const;

  JobTreeViewSubscriber *m_notifyee;
  QStandardItemModel m_model;
  QModelIndex m_lastEdited;
  bool m_hasEditorOpen;
};

template <typename InputIterator>
void JobTreeView::removeRows(InputIterator begin, InputIterator end) {
  removeRows(std::vector<RowLocation>(begin, end));
}
}
}
}
#endif // MANTIDQTMANTIDWIDGETS_JOBTREEVIEW_H_
