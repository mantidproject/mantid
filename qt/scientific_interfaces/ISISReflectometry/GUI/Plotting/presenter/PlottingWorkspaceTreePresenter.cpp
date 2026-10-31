// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "PlottingWorkspaceTreePresenter.h"

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {
namespace {
std::string displayName(PlottingWorkspaceTreeItemType itemType) {
  switch (itemType) {
  case PlottingWorkspaceTreeItemType::ReductionGroup:
    return "Group";
  case PlottingWorkspaceTreeItemType::Run:
    return "Run";
  case PlottingWorkspaceTreeItemType::WorkspaceGroup:
    return "WorkspaceGroup";
  case PlottingWorkspaceTreeItemType::Workspace:
    return "Workspace";
  }
  throw std::runtime_error("Unexpected plotting workspace tree item type.");
}

std::string displayName(ReducedWorkspaceOutputType outputType) {
  switch (outputType) {
  case ReducedWorkspaceOutputType::None:
    return "";
  case ReducedWorkspaceOutputType::IvsQ:
    return "IvsQ";
  case ReducedWorkspaceOutputType::IvsLambda:
    return "IvsLambda";
  case ReducedWorkspaceOutputType::IvsQBinned:
    return "IvsQBinned";
  }
  throw std::runtime_error("Unexpected reduced workspace output type.");
}

PlottingWorkspaceTreeViewItem viewItem(PlottingWorkspaceTreeItemState const &itemState) {
  auto children = std::vector<PlottingWorkspaceTreeViewItem>{};
  children.reserve(itemState.children.size());
  std::transform(itemState.children.cbegin(), itemState.children.cend(), std::back_inserter(children), viewItem);
  return {displayName(itemState.itemType), displayName(itemState.reducedOutputType), itemState.label,
          std::move(children), itemState.muted};
}

PlottingWorkspaceTreePath childPath(PlottingWorkspaceTreePath path, int row) {
  path.emplace_back(row);
  return path;
}

PlottingWorkspaceTreePath parentPath(PlottingWorkspaceTreePath path) {
  if (!path.empty()) {
    path.pop_back();
  }
  return path;
}
} // namespace

PlottingWorkspaceTreePresenter::PlottingWorkspaceTreePresenter(IPlottingWorkspaceTreeView &view) : m_view(view) {
  m_view.subscribe(this);
}

PlottingWorkspaceTreePresenter::~PlottingWorkspaceTreePresenter() { m_view.subscribe(nullptr); }

void PlottingWorkspaceTreePresenter::setPlottingWorkspaceTreeItemStates(
    std::vector<PlottingWorkspaceTreeItemState> const &itemStates) {
  auto items = std::vector<PlottingWorkspaceTreeViewItem>{};
  items.reserve(itemStates.size());
  std::transform(itemStates.cbegin(), itemStates.cend(), std::back_inserter(items), viewItem);
  // Removing rows can emit selection notifications for the previous tree.
  m_view.setItems(items);
  m_itemStates = itemStates;
  applyFilter();
  m_view.expandAll();
}

void PlottingWorkspaceTreePresenter::filterWorkspaces(std::string const &text,
                                                      std::vector<ReducedWorkspaceOutputType> const &outputTypes) {
  if (text != m_filterText) {
    try {
      m_filterExpression = boost::regex(text);
      m_filterText = text;
    } catch (boost::regex_error const &) {
      // Keep using the last valid expression while the user edits incomplete syntax.
    }
  }
  m_filterOutputTypes = outputTypes;
  applyFilter();
  m_view.expandAll();
}

PlottingWorkspaceTreeItemState const *
PlottingWorkspaceTreePresenter::item(PlottingWorkspaceTreePath const &path) const {
  auto const *items = &m_itemStates;
  PlottingWorkspaceTreeItemState const *result = nullptr;
  for (auto row : path) {
    if (row < 0 || static_cast<size_t>(row) >= items->size()) {
      return nullptr;
    }
    result = &(*items)[row];
    items = &result->children;
  }
  return result;
}

int PlottingWorkspaceTreePresenter::rowCount(PlottingWorkspaceTreePath const &path) const {
  if (path.empty()) {
    return static_cast<int>(m_itemStates.size());
  }
  auto const *node = item(path);
  return node ? static_cast<int>(node->children.size()) : 0;
}

void PlottingWorkspaceTreePresenter::applyFilter() {
  for (auto row = 0; row < rowCount({}); ++row) {
    filterItem({row}, false);
  }
}

bool PlottingWorkspaceTreePresenter::filterItem(PlottingWorkspaceTreePath const &path, bool ancestorMatches) {
  auto const &node = *item(path);
  auto const matches = ancestorMatches || matchesFilter(node);
  auto visible = false;
  if (node.itemType == PlottingWorkspaceTreeItemType::Workspace) {
    visible = matches && outputTypeEnabled(node);
  } else {
    for (auto row = 0; row < rowCount(path); ++row) {
      visible = filterItem(childPath(path, row), matches) || visible;
    }
  }
  m_view.setVisible(path, visible);
  return visible;
}

bool PlottingWorkspaceTreePresenter::matchesFilter(PlottingWorkspaceTreeItemState const &node) const {
  try {
    return boost::regex_search(node.label, m_filterExpression);
  } catch (std::runtime_error const &) {
    return false;
  }
}

bool PlottingWorkspaceTreePresenter::outputTypeEnabled(PlottingWorkspaceTreeItemState const &node) const {
  return std::find(m_filterOutputTypes.cbegin(), m_filterOutputTypes.cend(), node.reducedOutputType) !=
         m_filterOutputTypes.cend();
}

void PlottingWorkspaceTreePresenter::clearSelection() {
  m_updatingSelection = true;
  m_view.clearSelection();
  m_updatingSelection = false;
}

std::vector<std::string> PlottingWorkspaceTreePresenter::selectedPlottingWorkspaceNames() const {
  auto workspaces = std::vector<std::string>{};
  for (auto const &path : m_view.selectedItems()) {
    auto const *node = item(path);
    if (isAtomicWorkspaceGroup(path)) {
      appendWorkspaceNames(path, workspaces);
    } else if (node && node->itemType == PlottingWorkspaceTreeItemType::Workspace &&
               node->selectionMode != PlottingWorkspaceTreeSelectionMode::None &&
               !isAtomicWorkspaceGroup(parentPath(path))) {
      auto name = node->workspaceName;
      if (std::find(workspaces.cbegin(), workspaces.cend(), name) == workspaces.cend()) {
        workspaces.emplace_back(std::move(name));
      }
    }
  }
  return workspaces;
}

size_t PlottingWorkspaceTreePresenter::selectedPlottingWorkspaceGroupCount() const {
  auto count = size_t{0};
  for (auto const &path : m_view.selectedItems()) {
    auto const *node = item(path);
    if (node && node->itemType == PlottingWorkspaceTreeItemType::WorkspaceGroup && canSelectDirectly(path)) {
      ++count;
    }
  }
  return count;
}

bool PlottingWorkspaceTreePresenter::canSelectDirectly(PlottingWorkspaceTreePath const &path) const {
  auto const *node = item(path);
  return node && node->selectionMode == PlottingWorkspaceTreeSelectionMode::DirectAndParent;
}

bool PlottingWorkspaceTreePresenter::canSelectViaParent(PlottingWorkspaceTreePath const &path) const {
  auto const *node = item(path);
  return node && (node->selectionMode == PlottingWorkspaceTreeSelectionMode::ParentOnly ||
                  node->selectionMode == PlottingWorkspaceTreeSelectionMode::DirectAndParent);
}

// Infer an atomic workspace group when every child workspace is selectable only through its parent.
// An atomic workspace group will require all of its children for the selected plot output type.
bool PlottingWorkspaceTreePresenter::isAtomicWorkspaceGroup(PlottingWorkspaceTreePath const &path) const {
  auto const *node = item(path);
  if (!node || node->itemType != PlottingWorkspaceTreeItemType::WorkspaceGroup || !canSelectDirectly(path) ||
      node->children.empty()) {
    return false;
  }
  for (auto row = 0; row < rowCount(path); ++row) {
    auto const child = childPath(path, row);
    if (item(child)->itemType != PlottingWorkspaceTreeItemType::Workspace || !canSelectViaParent(child) ||
        canSelectDirectly(child)) {
      return false;
    }
  }
  return true;
}

void PlottingWorkspaceTreePresenter::appendWorkspaceNames(PlottingWorkspaceTreePath const &path,
                                                          std::vector<std::string> &workspaceNames) const {
  for (auto row = 0; row < rowCount(path); ++row) {
    auto const child = childPath(path, row);
    auto const &node = *item(child);
    if (node.itemType == PlottingWorkspaceTreeItemType::Workspace &&
        node.selectionMode != PlottingWorkspaceTreeSelectionMode::None) {
      auto name = node.workspaceName;
      if (std::find(workspaceNames.cbegin(), workspaceNames.cend(), name) == workspaceNames.cend()) {
        workspaceNames.emplace_back(std::move(name));
      }
    } else {
      appendWorkspaceNames(child, workspaceNames);
    }
  }
}

void PlottingWorkspaceTreePresenter::notifyItemClicked(PlottingWorkspaceTreePath const &path, bool additive,
                                                       bool shift) {
  if (!canSelectDirectly(path)) {
    return;
  }
  if (hasSelectedAncestor(path)) {
    if (!shift) {
      selectSubtree(path, true, true);
    }
    return;
  }

  auto const subtreeSelected = isSubtreeSelected(path);
  selectSubtree(path, !subtreeSelected, !additive && !subtreeSelected);
}

bool PlottingWorkspaceTreePresenter::hasSelectedAncestor(PlottingWorkspaceTreePath const &path) const {
  auto ancestor = parentPath(path);
  while (!ancestor.empty()) {
    if (m_view.isSelected(ancestor)) {
      return true;
    }
    ancestor = parentPath(ancestor);
  }
  return false;
}

bool PlottingWorkspaceTreePresenter::isSubtreeSelected(PlottingWorkspaceTreePath const &path) const {
  if (!m_view.isSelected(path)) {
    return false;
  }
  for (auto row = 0; row < rowCount(path); ++row) {
    auto const child = childPath(path, row);
    if (!m_view.isVisible(child)) {
      continue;
    }
    if (!isSubtreeSelected(child)) {
      return false;
    }
  }
  return true;
}

void PlottingWorkspaceTreePresenter::selectSubtree(PlottingWorkspaceTreePath const &path, bool selected, bool clear) {
  m_updatingSelection = true;
  if (clear) {
    m_view.clearSelection();
  }
  m_view.setSelected(path, selected);
  updateChildSelection(path, selected);
  m_updatingSelection = false;
}

void PlottingWorkspaceTreePresenter::notifySelectionChanged(std::vector<PlottingWorkspaceTreePath> const &deselected,
                                                            std::vector<PlottingWorkspaceTreePath> const &selected) {
  updateChildSelection(deselected, false);
  updateChildSelection(selected, true);
}

void PlottingWorkspaceTreePresenter::updateChildSelection(std::vector<PlottingWorkspaceTreePath> const &paths,
                                                          bool selected) {
  if (m_updatingSelection) {
    return;
  }
  m_updatingSelection = true;
  for (auto const &path : paths) {
    updateChildSelection(path, selected);
  }
  m_updatingSelection = false;
}

void PlottingWorkspaceTreePresenter::updateChildSelection(PlottingWorkspaceTreePath const &path, bool selected) {
  for (auto row = 0; row < rowCount(path); ++row) {
    auto const child = childPath(path, row);
    if (!m_view.isVisible(child)) {
      continue;
    }
    if (!selected || canSelectDirectly(child) || canSelectViaParent(child)) {
      m_view.setSelected(child, selected);
    }
    updateChildSelection(child, selected);
  }
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
