#ifndef MANTIDTREEWIDGETITEM_H
#define MANTIDTREEWIDGETITEM_H

#include <MantidKernel/DateAndTime.h>
#include <QTreeWidgetItem>

class MantidTreeWidget;

/**A class derived from QTreeWidgetItem, to accomodate
* sorting on the items in a MantidTreeWidget.
*/
class MantidTreeWidgetItem : public QTreeWidgetItem {
public:
  explicit MantidTreeWidgetItem(MantidTreeWidget *);
  MantidTreeWidgetItem(QStringList, MantidTreeWidget *);
  void disableIfNode(bool);
  void setSortPos(int o) { m_sortPos = o; }
  int getSortPos() const { return m_sortPos; }

private:
  bool operator<(const QTreeWidgetItem &other) const override;
  MantidTreeWidget *m_parent;
  static Mantid::Kernel::DateAndTime getLastModified(const QTreeWidgetItem *);
  int m_sortPos;
};
#endif // MANTIDTREEWIDGETITEM_H