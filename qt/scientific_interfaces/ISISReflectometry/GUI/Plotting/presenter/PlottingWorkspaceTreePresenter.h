// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "GUI/Plotting/view/IPlottingWorkspaceTreeView.h"
#include "GUI/Plotting/view/PlottingViewState.h"
#include <boost/regex.hpp>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/// Applies workspace tree filtering and selection rules independently of the GUI toolkit.
class MANTIDQT_ISISREFLECTOMETRY_DLL PlottingWorkspaceTreePresenter : public PlottingWorkspaceTreeViewSubscriber {
public:
  explicit PlottingWorkspaceTreePresenter(IPlottingWorkspaceTreeView &view);
  ~PlottingWorkspaceTreePresenter() override;

  void setPlottingWorkspaceTreeItemStates(std::vector<PlottingWorkspaceTreeItemState> const &itemStates);
  void filterWorkspaces(std::string const &text, std::vector<ReducedWorkspaceOutputType> const &outputTypes);
  void clearSelection();
  std::vector<std::string> selectedPlottingWorkspaceNames() const;
  size_t selectedPlottingWorkspaceGroupCount() const;
  void notifyItemClicked(PlottingWorkspaceTreePath const &path, bool additive, bool shift) override;
  void notifySelectionChanged(std::vector<PlottingWorkspaceTreePath> const &deselected,
                              std::vector<PlottingWorkspaceTreePath> const &selected) override;

private:
  PlottingWorkspaceTreeItemState const *item(PlottingWorkspaceTreePath const &path) const;
  int rowCount(PlottingWorkspaceTreePath const &path) const;
  bool canSelectDirectly(PlottingWorkspaceTreePath const &path) const;
  bool canSelectViaParent(PlottingWorkspaceTreePath const &path) const;
  bool isAtomicWorkspaceGroup(PlottingWorkspaceTreePath const &path) const;
  void appendWorkspaceNames(PlottingWorkspaceTreePath const &path, std::vector<std::string> &workspaceNames) const;
  void applyFilter();
  bool filterItem(PlottingWorkspaceTreePath const &path, bool ancestorMatches);
  bool matchesFilter(PlottingWorkspaceTreeItemState const &item) const;
  bool outputTypeEnabled(PlottingWorkspaceTreeItemState const &item) const;
  bool hasSelectedAncestor(PlottingWorkspaceTreePath const &path) const;
  bool isSubtreeSelected(PlottingWorkspaceTreePath const &path) const;
  void selectSubtree(PlottingWorkspaceTreePath const &path, bool selected, bool clear);
  void updateChildSelection(std::vector<PlottingWorkspaceTreePath> const &paths, bool selected);
  void updateChildSelection(PlottingWorkspaceTreePath const &path, bool selected);

  IPlottingWorkspaceTreeView &m_view;
  std::vector<PlottingWorkspaceTreeItemState> m_itemStates;
  bool m_updatingSelection{false};
  std::string m_filterText;
  boost::regex m_filterExpression{""};
  std::vector<ReducedWorkspaceOutputType> m_filterOutputTypes{ReducedWorkspaceOutputType::IvsQBinned};
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
