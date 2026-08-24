// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "GUI/Plotting/model/PlottingWorkspace.h"
#include "GUI/Plotting/view/PlottingWorkspaceTreeDisplayItem.h"

#include <QItemSelection>
#include <QItemSelectionModel>
#include <QObject>
#include <QStandardItemModel>

#include <string>
#include <vector>

class QMouseEvent;
class QStandardItem;

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class WorkspaceTreeView;

/// Adapts evaluated workspace tree state to the Qt tree widget.
class MANTIDQT_ISISREFLECTOMETRY_DLL QtWorkspaceTreeViewAdapter : public QObject {
public:
  /// Attach an adapter and model to the supplied workspace tree view.
  explicit QtWorkspaceTreeViewAdapter(WorkspaceTreeView *workspaceTree, QObject *parent = nullptr);

  /// Replace all displayed evaluated workspace tree items.
  void setItems(std::vector<PlottingWorkspaceTreeDisplayItem> const &items);
  /// Clear all selected tree rows without recursively updating children.
  void clearSelection();
  /// Return selected leaf workspace names.
  std::vector<std::string> selectedWorkspaceNames() const;
  /// Return the number of selected workspace-group nodes.
  size_t selectedWorkspaceGroupCount() const;
  /// Apply the same selection change to descendants marked eligible for parent selection.
  void updateChildSelection(QItemSelection const &selection, QItemSelectionModel::SelectionFlags selectionFlags);

protected:
  /// Intercept mouse input to select or deselect full workspace subtrees.
  bool eventFilter(QObject *watched, QEvent *event) override;

private:
  enum WorkspaceTreeColumn { ItemTypeColumn, OutputTypeColumn, ItemColumn };

  /// Apply muted visual state to every column in a row.
  void setItemMuted(QStandardItem *parent, int row, bool muted);
  /// Add a tree node and its children to the Qt item model.
  void addTreeItem(QStandardItem *parent, PlottingWorkspaceTreeDisplayItem const &item);
  /// Return the canonical item-type column index for any row column index.
  QModelIndex itemIndex(QModelIndex const &index) const;
  /// Return the plotting tree item type stored on a model row.
  PlottingWorkspaceTreeItemType itemType(QModelIndex const &index) const;
  /// Return the ADS workspace name stored on a model row.
  std::string workspaceName(QModelIndex const &index) const;
  /// Return true if the evaluated row can be selected directly.
  bool canSelectDirectly(QModelIndex const &index) const;
  /// Return true if the evaluated row can be selected through a selected parent.
  bool canSelectViaParent(QModelIndex const &index) const;
  /// Return true if a selected row may contribute to the selected workspaces.
  bool canContributeSelection(QModelIndex const &index) const;
  /// Handle row clicks by selecting or deselecting whole subtrees.
  bool handleWorkspaceTreeClick(QMouseEvent const &event);
  /// Return true for keyboard modifiers that should preserve existing selections.
  bool isAdditiveSelectionModifier(QMouseEvent const &event) const;
  /// Return true if any ancestor row is selected.
  bool hasSelectedAncestor(QModelIndex const &index) const;
  /// Return true if the row and all descendants are selected.
  bool isSubtreeSelected(QModelIndex const &parentIndex) const;
  /// Apply a selection change to a row and all eligible descendants.
  void selectSubtree(QModelIndex const &parentIndex, QItemSelectionModel::SelectionFlags selectionFlags);
  /// Apply a selection change to eligible descendants of one row.
  void updateChildSelection(QModelIndex const &parentIndex, QItemSelectionModel::SelectionFlags selectionFlags);

  WorkspaceTreeView *m_workspaceTree;
  QStandardItemModel m_model;
  bool m_updatingSelection;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
