// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include "GUI/Common/PlotOptions.h"
#include "GUI/Plotting/model/PlottingWorkspace.h"

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

/// Owns workspace tree model data, selection propagation and output-type filtering.
class MANTIDQT_ISISREFLECTOMETRY_DLL WorkspaceTreeController : public QObject {
public:
  /// Attach a controller and model to the supplied workspace tree view.
  explicit WorkspaceTreeController(WorkspaceTreeView *workspaceTree, QObject *parent = nullptr);

  /// Replace all displayed workspace tree items.
  void setItems(std::vector<PlottingWorkspaceTreeItem> const &items);
  /// Clear all selected tree rows without recursively updating children.
  void clearSelection();
  /// Update muting and selection rules for the selected plot output type.
  void setCurrentPlotOutputType(PlotOutputType outputType);
  /// Return selected leaf workspace names.
  std::vector<std::string> selectedWorkspaceNames() const;
  /// Return the number of selected workspace-group nodes that are valid for the current output type.
  size_t selectedWorkspaceGroupCount() const;
  /// Apply the same selection change to selectable descendants of selected rows.
  void updateChildSelection(QItemSelection const &selection, QItemSelectionModel::SelectionFlags selectionFlags);

protected:
  /// Intercept mouse input to select or deselect full workspace subtrees.
  bool eventFilter(QObject *watched, QEvent *event) override;

private:
  enum WorkspaceTreeColumn { ItemTypeColumn, OutputTypeColumn, ItemColumn };

  void setItemsMutedForCurrentPlotOutputType();
  /// Recursively mute tree rows that cannot be selected for the current output type.
  void setItemsMutedForCurrentPlotOutputType(QStandardItem *parent);
  /// Apply muted visual state to every column in a row.
  void setItemMuted(QStandardItem *parent, int row, bool muted);
  /// Add a tree node and its children to the Qt item model.
  void addTreeItem(QStandardItem *parent, PlottingWorkspaceTreeItem const &item);
  /// Return the canonical item-type column index for any row column index.
  QModelIndex itemIndex(QModelIndex const &index) const;
  /// Return the plotting tree item type stored on a model row.
  PlottingWorkspaceTreeItemType itemType(QModelIndex const &index) const;
  /// Return the reduced workspace output type stored on a model row.
  PlottingWorkspaceOutputType outputType(QModelIndex const &index) const;
  /// Return the ADS workspace name stored on a model row.
  std::string workspaceName(QModelIndex const &index) const;
  /// Return true if a row has no child rows.
  bool isWorkspaceItem(QModelIndex const &index) const;
  /// Return true if a row has a selectable workspace descendant.
  bool hasWorkspaceDescendant(QModelIndex const &index) const;
  /// Return true if every workspace descendant is valid for the current output type.
  bool allWorkspaceDescendantsIncludedForCurrentPlotOutputType(QModelIndex const &index) const;
  /// Return true if the row can be selected for the current output type.
  bool isSelectableForCurrentPlotOutputType(QModelIndex const &index) const;
  /// Return true if the workspace row has an output type accepted by the current plot output.
  bool isWorkspaceIncludedForCurrentPlotOutputType(QModelIndex const &index) const;
  /// Return true if a postprocessed group output should be hidden from the current output type.
  bool isPostprocessedGroupOutputExcludedForCurrentPlotOutputType(QModelIndex const &index) const;
  /// Return true if the row represents a stitched/postprocessed group-level output.
  bool isPostprocessedGroupOutputItem(QModelIndex const &index) const;
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
  PlotOutputType m_plotOutputType;
  bool m_updatingSelection;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
