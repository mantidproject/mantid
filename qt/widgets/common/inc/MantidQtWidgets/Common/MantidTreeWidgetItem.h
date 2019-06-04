// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDTREEWIDGETITEM_H
#define MANTIDTREEWIDGETITEM_H

#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidQtWidgets/Common/WorkspaceObserver.h"
#include <QTreeWidgetItem>

namespace MantidQt {
namespace MantidWidgets {
class MantidTreeWidget;

/**A class derived from QTreeWidgetItem, to accomodate
 * sorting on the items in a MantidTreeWidget.
 */
class EXPORT_OPT_MANTIDQT_COMMON MantidTreeWidgetItem : public QTreeWidgetItem {
public:
  explicit MantidTreeWidgetItem(MantidTreeWidget * /*parent*/);
  MantidTreeWidgetItem(QStringList /*list*/, MantidTreeWidget * /*parent*/);
  void disableIfNode(bool);
  void setSortPos(int o) { m_sortPos = o; }
  int getSortPos() const { return m_sortPos; }

private:
  bool operator<(const QTreeWidgetItem &other) const override;
  MantidTreeWidget *m_parent;
  static Mantid::Types::Core::DateAndTime
  getLastModified(const QTreeWidgetItem * /*item*/);
  std::size_t getMemorySize() const;
  int m_sortPos;
};
} // namespace MantidWidgets
} // namespace MantidQt
#endif // MANTIDTREEWIDGETITEM_H