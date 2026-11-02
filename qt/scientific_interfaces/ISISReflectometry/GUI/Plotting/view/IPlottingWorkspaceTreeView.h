// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "Common/DllConfig.h"
#include <string>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/// Prepared text and visual state for one workspace tree row.
struct MANTIDQT_ISISREFLECTOMETRY_DLL PlottingWorkspaceTreeViewItem {
  std::string itemTypeLabel;
  std::string outputTypeLabel;
  std::string label;
  std::vector<PlottingWorkspaceTreeViewItem> children;
  bool muted{false};
};

/// Row indices from the root to a workspace tree node. Empty denotes the root.
using PlottingWorkspaceTreePath = std::vector<int>;

class MANTIDQT_ISISREFLECTOMETRY_DLL PlottingWorkspaceTreeViewSubscriber {
public:
  virtual ~PlottingWorkspaceTreeViewSubscriber() = default;
  /// Notify a primary-button click, with additive-selection and shift modifiers translated by the view.
  virtual void notifyItemClicked(PlottingWorkspaceTreePath const &path, bool additive, bool shift) = 0;
  /// Notify selection changes, including those made programmatically.
  virtual void notifySelectionChanged(std::vector<PlottingWorkspaceTreePath> const &deselected,
                                      std::vector<PlottingWorkspaceTreePath> const &selected) = 0;
};

/// Toolkit-independent rendering and selection operations for the workspace tree.
class MANTIDQT_ISISREFLECTOMETRY_DLL IPlottingWorkspaceTreeView {
public:
  virtual ~IPlottingWorkspaceTreeView() = default;
  virtual void subscribe(PlottingWorkspaceTreeViewSubscriber *notifyee) = 0;
  virtual void setItems(std::vector<PlottingWorkspaceTreeViewItem> const &items) = 0;
  virtual void expandAll() = 0;
  virtual void setVisible(PlottingWorkspaceTreePath const &path, bool visible) = 0;
  virtual bool isVisible(PlottingWorkspaceTreePath const &path) const = 0;
  virtual void clearSelection() = 0;
  virtual void setSelected(PlottingWorkspaceTreePath const &path, bool selected) = 0;
  virtual bool isSelected(PlottingWorkspaceTreePath const &path) const = 0;
  virtual std::vector<PlottingWorkspaceTreePath> selectedItems() const = 0;
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
