// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "PlottingViewStateProvider.h"

#include "GUI/Plotting/presenter/PlotOutputTypeProperties.h"

#include <algorithm>
#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
auto constexpr minimumSelectedItemsForMultiPlot = size_t{2};

bool hasSelectedItems(size_t selectedItemCount) { return selectedItemCount > 0; }

bool hasEnoughSelectedItemsForMultiPlot(size_t selectedItemCount, size_t selectedPlottingWorkspaceGroupCount,
                                        PlotOutputTypeProperties const &plotProperties) {
  if (plotProperties.requiresWorkspaceGroupsForMultiPlot()) {
    return selectedPlottingWorkspaceGroupCount >= minimumSelectedItemsForMultiPlot;
  }
  return selectedItemCount >= minimumSelectedItemsForMultiPlot;
}

bool hasEnoughSelectedItemsForMultiOutputPlot(size_t selectedItemCount, size_t selectedPlottingWorkspaceGroupCount,
                                              PlotOutputTypeProperties const &plotProperties, bool addToExistingPlot) {
  return addToExistingPlot ? hasSelectedItems(selectedItemCount)
                           : hasEnoughSelectedItemsForMultiPlot(selectedItemCount, selectedPlottingWorkspaceGroupCount,
                                                                plotProperties);
}

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

PlottingWorkspaceTreeItemState plottingWorkspaceTreeItemState(PlottingWorkspaceTreeItem const &item,
                                                              PlotOutputTypeProperties const &properties,
                                                              bool parentIsGroup = false,
                                                              bool parentIsWorkspaceGroup = false,
                                                              bool grandparentIsGroup = false) {
  auto const itemIsGroup = item.itemType == PlottingWorkspaceTreeItemType::ReductionGroup;
  auto const itemIsWorkspaceGroup = item.itemType == PlottingWorkspaceTreeItemType::WorkspaceGroup;
  auto childStates = std::vector<PlottingWorkspaceTreeItemState>{};
  childStates.reserve(item.children.size());
  std::transform(item.children.cbegin(), item.children.cend(), std::back_inserter(childStates), [&](const auto &child) {
    return plottingWorkspaceTreeItemState(child, properties, itemIsGroup, itemIsWorkspaceGroup, parentIsGroup);
  });
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
          std::move(childStates),
          !canSelectItemDirectly,
          selectionMode(canSelectItemDirectly, canSelectItemViaParent)};
}
} // namespace

std::vector<PlotOutputTypeViewItem>
PlottingViewStateProvider::outputTypeViewItems(std::vector<PlotOutputType> const &outputTypes) const {
  auto viewItems = std::vector<PlotOutputTypeViewItem>{};
  viewItems.reserve(outputTypes.size());
  std::transform(outputTypes.cbegin(), outputTypes.cend(), std::back_inserter(viewItems), [&](const auto &outputType) {
    return PlotOutputTypeViewItem{outputType, plotOutputTypeProperties(outputType).displayName()};
  });
  return viewItems;
}

PlotOutputControlsState PlottingViewStateProvider::outputControlsState(PlotOutputType outputType) const {
  auto const &plotProperties = plotOutputTypeProperties(outputType);
  return {plotProperties.showsPlotProperties(), plotProperties.showsDetectorMapProperties(),
          plotProperties.showsAlignmentProperties()};
}

PlotActionState PlottingViewStateProvider::plotActionState(bool outputSelectionEnabled,
                                                           size_t selectedPlottingWorkspaceCount,
                                                           size_t selectedPlottingWorkspaceGroupCount,
                                                           PlotOutputType outputType, bool addToExistingPlot,
                                                           bool hasActiveReflectometryFigure,
                                                           bool activePlotOverplotCompatible) const {
  auto const &plotProperties = plotOutputTypeProperties(outputType);
  auto const addToExistingEnabled =
      outputSelectionEnabled && hasActiveReflectometryFigure && plotProperties.supportsAddToExistingPlot();
  auto const addToExistingChecked = addToExistingPlot && addToExistingEnabled;
  auto const hasEnoughSelectedItems = hasEnoughSelectedItemsForMultiOutputPlot(
      selectedPlottingWorkspaceCount, selectedPlottingWorkspaceGroupCount, plotProperties, addToExistingChecked);
  return {outputSelectionEnabled && !addToExistingChecked && hasSelectedItems(selectedPlottingWorkspaceCount),
          outputSelectionEnabled && plotProperties.supportsOverplot() &&
              (!addToExistingChecked || activePlotOverplotCompatible) && hasEnoughSelectedItems,
          outputSelectionEnabled && hasEnoughSelectedItems,
          outputSelectionEnabled && hasSelectedItems(selectedPlottingWorkspaceCount),
          addToExistingEnabled,
          addToExistingChecked};
}

std::vector<PlottingWorkspaceTreeItemState>
PlottingViewStateProvider::plottingWorkspaceTreeItemStates(std::vector<PlottingWorkspaceTreeItem> const &items,
                                                           PlotOutputType outputType) const {
  auto const &properties = plotOutputTypeProperties(outputType);
  auto itemStates = std::vector<PlottingWorkspaceTreeItemState>{};
  itemStates.reserve(items.size());
  std::transform(items.cbegin(), items.cend(), std::back_inserter(itemStates),
                 [&](const auto &item) { return plottingWorkspaceTreeItemState(item, properties); });
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
