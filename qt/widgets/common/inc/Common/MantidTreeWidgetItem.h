#ifndef MANTIDTREEWIDGETITEM_H
#define MANTIDTREEWIDGETITEM_H

#include "MantidQtMantidWidgets/WidgetDllOption.h"
#include <MantidKernel/DateAndTime.h>
#include <MantidQtAPI/WorkspaceObserver.h>
#include <QTreeWidgetItem>

namespace MantidQt {
namespace MantidWidgets {
class MantidTreeWidget;

/**A class derived from QTreeWidgetItem, to accomodate
* sorting on the items in a MantidTreeWidget.
*/
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS MantidTreeWidgetItem
    : public QTreeWidgetItem {
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
}
}
#endif // MANTIDTREEWIDGETITEM_H