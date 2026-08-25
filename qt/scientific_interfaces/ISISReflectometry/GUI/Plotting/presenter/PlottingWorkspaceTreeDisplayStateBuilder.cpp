// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "PlottingWorkspaceTreeDisplayStateBuilder.h"

#include "GUI/Plotting/presenter/PlotOutputTypeProperties.h"

#include <algorithm>
#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
bool isWorkspaceItem(PlottingWorkspaceTreeItem const &item) { return item.children.empty(); }

bool isPostprocessedGroupOutputItem(PlottingWorkspaceTreeItem const &item, bool parentIsGroup,
                                    bool parentIsWorkspaceGroup, bool grandparentIsGroup) {
  if (item.itemType != PlottingWorkspaceTreeItemType::WorkspaceGroup &&
      item.itemType != PlottingWorkspaceTreeItemType::Workspace) {
    return false;
  }
  return parentIsGroup || (parentIsWorkspaceGroup && grandparentIsGroup);
}

bool isPostprocessedGroupOutputExcluded(PlottingWorkspaceTreeItem const &item,
                                        PlotOutputTypeProperties const &properties, bool parentIsGroup,
                                        bool parentIsWorkspaceGroup, bool grandparentIsGroup) {
  return properties.excludesPostprocessedGroupOutputs() &&
         isPostprocessedGroupOutputItem(item, parentIsGroup, parentIsWorkspaceGroup, grandparentIsGroup);
}

bool isWorkspaceIncluded(PlottingWorkspaceTreeItem const &item, PlotOutputTypeProperties const &properties,
                         bool parentIsGroup, bool parentIsWorkspaceGroup, bool grandparentIsGroup) {
  if (isPostprocessedGroupOutputExcluded(item, properties, parentIsGroup, parentIsWorkspaceGroup, grandparentIsGroup)) {
    return false;
  }
  return properties.includesReducedWorkspaceOutput(item.reducedOutputType);
}

bool hasWorkspaceDescendant(PlottingWorkspaceTreeItem const &item) {
  if (isWorkspaceItem(item)) {
    return item.itemType == PlottingWorkspaceTreeItemType::Workspace;
  }
  return std::any_of(item.children.cbegin(), item.children.cend(), hasWorkspaceDescendant);
}

bool allWorkspaceDescendantsIncluded(PlottingWorkspaceTreeItem const &item, PlotOutputTypeProperties const &properties,
                                     bool parentIsGroup, bool parentIsWorkspaceGroup, bool grandparentIsGroup);

bool isSelectable(PlottingWorkspaceTreeItem const &item, PlotOutputTypeProperties const &properties, bool parentIsGroup,
                  bool parentIsWorkspaceGroup, bool grandparentIsGroup) {
  if (isPostprocessedGroupOutputExcluded(item, properties, parentIsGroup, parentIsWorkspaceGroup, grandparentIsGroup)) {
    return false;
  }
  if (!properties.allowsItemType(item.itemType)) {
    return false;
  }
  if (item.itemType == PlottingWorkspaceTreeItemType::Workspace) {
    return isWorkspaceIncluded(item, properties, parentIsGroup, parentIsWorkspaceGroup, grandparentIsGroup);
  }
  if (item.itemType == PlottingWorkspaceTreeItemType::WorkspaceGroup) {
    return hasWorkspaceDescendant(item) &&
           allWorkspaceDescendantsIncluded(item, properties, parentIsGroup, parentIsWorkspaceGroup, grandparentIsGroup);
  }
  return true;
}

bool allWorkspaceDescendantsIncluded(PlottingWorkspaceTreeItem const &item, PlotOutputTypeProperties const &properties,
                                     bool parentIsGroup, bool parentIsWorkspaceGroup, bool grandparentIsGroup) {
  if (isWorkspaceItem(item)) {
    return item.itemType != PlottingWorkspaceTreeItemType::Workspace ||
           isWorkspaceIncluded(item, properties, parentIsGroup, parentIsWorkspaceGroup, grandparentIsGroup);
  }

  auto const itemIsGroup = item.itemType == PlottingWorkspaceTreeItemType::ReductionGroup;
  auto const itemIsWorkspaceGroup = item.itemType == PlottingWorkspaceTreeItemType::WorkspaceGroup;
  return std::all_of(item.children.cbegin(), item.children.cend(), [&](auto const &child) {
    return allWorkspaceDescendantsIncluded(child, properties, itemIsGroup, itemIsWorkspaceGroup, parentIsGroup);
  });
}

PlottingWorkspaceTreeSelectionMode selectionMode(bool canSelectDirectly, bool canSelectViaParent) {
  if (canSelectDirectly && canSelectViaParent) {
    return PlottingWorkspaceTreeSelectionMode::DirectAndParent;
  }
  if (canSelectDirectly) {
    return PlottingWorkspaceTreeSelectionMode::Direct;
  }
  if (canSelectViaParent) {
    return PlottingWorkspaceTreeSelectionMode::ParentOnly;
  }
  return PlottingWorkspaceTreeSelectionMode::None;
}

PlottingWorkspaceTreeDisplayItem evaluateItemState(PlottingWorkspaceTreeItem const &item,
                                                   PlotOutputTypeProperties const &properties,
                                                   bool parentIsGroup = false, bool parentIsWorkspaceGroup = false,
                                                   bool grandparentIsGroup = false) {
  auto const itemIsGroup = item.itemType == PlottingWorkspaceTreeItemType::ReductionGroup;
  auto const itemIsWorkspaceGroup = item.itemType == PlottingWorkspaceTreeItemType::WorkspaceGroup;
  auto children = std::vector<PlottingWorkspaceTreeDisplayItem>{};
  children.reserve(item.children.size());
  for (auto const &child : item.children) {
    children.emplace_back(evaluateItemState(child, properties, itemIsGroup, itemIsWorkspaceGroup, parentIsGroup));
  }
  auto const canSelectItemDirectly =
      isSelectable(item, properties, parentIsGroup, parentIsWorkspaceGroup, grandparentIsGroup);
  auto const canSelectItemViaParent =
      canSelectItemDirectly ||
      (item.itemType == PlottingWorkspaceTreeItemType::Workspace &&
       isWorkspaceIncluded(item, properties, parentIsGroup, parentIsWorkspaceGroup, grandparentIsGroup));
  return {item.label,
          item.itemType,
          item.reducedOutputType,
          item.workspaceName,
          std::move(children),
          !canSelectItemDirectly,
          selectionMode(canSelectItemDirectly, canSelectItemViaParent)};
}
} // namespace

std::vector<PlottingWorkspaceTreeDisplayItem>
PlottingWorkspaceTreeDisplayStateBuilder::build(std::vector<PlottingWorkspaceTreeItem> const &items,
                                                PlotOutputType outputType) const {
  auto const &properties = plotOutputTypeProperties(outputType);
  auto evaluatedItems = std::vector<PlottingWorkspaceTreeDisplayItem>{};
  evaluatedItems.reserve(items.size());
  for (auto const &item : items) {
    evaluatedItems.emplace_back(evaluateItemState(item, properties));
  }
  return evaluatedItems;
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
