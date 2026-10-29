// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IPlottingWorkspaceTreeAdapterView.h"
#include <QBrush>
#include <QItemSelection>
#include <QPalette>
#include <QStandardItemModel>
#include <QTreeView>

class QMouseEvent;
class QStandardItem;

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace PlottingWorkspaceTreeView {
/// Item data role used to mark rows muted by the current plot output type.
auto constexpr mutedRole = Qt::UserRole + 4;
/// Background brush for rows that are not selectable for the current plot output type.
inline QBrush mutedBackgroundBrush(QPalette const &palette) { return palette.brush(QPalette::AlternateBase); }
} // namespace PlottingWorkspaceTreeView

/// Renders the plotting workspace tree and translates Qt input into presenter notifications.
class MANTIDQT_ISISREFLECTOMETRY_DLL QtPlottingWorkspaceTreeView : public QTreeView,
                                                                   public IPlottingWorkspaceTreeAdapterView {
  Q_OBJECT
public:
  explicit QtPlottingWorkspaceTreeView(QWidget *parent = nullptr);

  using QTreeView::isVisible;
  using QTreeView::setVisible;

  void subscribe(PlottingWorkspaceTreeAdapterViewSubscriber *notifyee) override;
  void setItems(std::vector<PlottingWorkspaceTreeViewItem> const &items) override;
  void expandAll() override;
  void setVisible(PlottingWorkspaceTreePath const &path, bool visible) override;
  bool isVisible(PlottingWorkspaceTreePath const &path) const override;
  void clearSelection() override;
  void setSelected(PlottingWorkspaceTreePath const &path, bool selected) override;
  bool isSelected(PlottingWorkspaceTreePath const &path) const override;
  std::vector<PlottingWorkspaceTreePath> selectedItems() const override;

signals:
  /// Notify the containing tab after a tree selection change has been handled.
  void workspaceSelectionChanged();

protected:
  bool viewportEvent(QEvent *event) override;
  void drawRow(QPainter *painter, QStyleOptionViewItem const &option, QModelIndex const &index) const override;

private:
  enum PlottingWorkspaceTreeColumn { ItemTypeColumn, OutputTypeColumn, ItemColumn };

  void setItemMuted(QStandardItem *parent, int row, bool muted);
  void addItem(QStandardItem *parent, PlottingWorkspaceTreeViewItem const &itemState);
  PlottingWorkspaceTreePath pathForIndex(QModelIndex index) const;
  QModelIndex indexForPath(PlottingWorkspaceTreePath const &path) const;
  std::vector<PlottingWorkspaceTreePath> pathsForSelection(QItemSelection const &selection) const;
  bool handlePlottingWorkspaceTreeClick(QMouseEvent const &event);

  QStandardItemModel m_model;
  PlottingWorkspaceTreeAdapterViewSubscriber *m_notifyee{nullptr};
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
