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

bool isPostprocessedGroupOutputExcluded(PlottingWorkspaceTreeItem const &item,
                                        PlotOutputTypeProperties const &properties) {
  return properties.excludesPostprocessedGroupOutputs() &&
         item.context.origin == PlottingWorkspaceOrigin::PostprocessedGroupOutput;
}

bool isWorkspaceIncluded(PlottingWorkspaceTreeItem const &item, PlotOutputTypeProperties const &properties) {
  if (isPostprocessedGroupOutputExcluded(item, properties)) {
    return false;
  }
  return properties.includesReducedWorkspaceOutput(item.reducedOutputType);
}

bool hasWorkspaceDescendant(PlottingWorkspaceTreeItem const &item) {
  if (item.itemType == PlottingWorkspaceTreeItemType::Workspace) {
    return true;
  }
  return std::any_of(item.children.cbegin(), item.children.cend(), hasWorkspaceDescendant);
}

bool allWorkspaceDescendantsIncluded(PlottingWorkspaceTreeItem const &item,
                                     PlotOutputTypeProperties const &properties) {
  if (item.itemType == PlottingWorkspaceTreeItemType::Workspace) {
    return isWorkspaceIncluded(item, properties);
  }
  return std::all_of(item.children.cbegin(), item.children.cend(),
                     [&](auto const &child) { return allWorkspaceDescendantsIncluded(child, properties); });
}

bool isSelectable(PlottingWorkspaceTreeItem const &item, PlotOutputTypeProperties const &properties) {
  if (isPostprocessedGroupOutputExcluded(item, properties)) {
    return false;
  }
  if (!properties.allowsItemType(item.itemType)) {
    return false;
  }
  if (item.itemType == PlottingWorkspaceTreeItemType::Workspace) {
    return isWorkspaceIncluded(item, properties);
  }
  if (item.itemType == PlottingWorkspaceTreeItemType::WorkspaceGroup) {
    return hasWorkspaceDescendant(item) && allWorkspaceDescendantsIncluded(item, properties);
  }
  return true;
}

PlottingWorkspaceTreeSelectionMode selectionMode(bool canSelectDirectly, bool canSelectViaParent) {
  if (canSelectDirectly) {
    return PlottingWorkspaceTreeSelectionMode::DirectAndParent;
  }
  if (canSelectViaParent) {
    return PlottingWorkspaceTreeSelectionMode::ParentOnly;
  }
  return PlottingWorkspaceTreeSelectionMode::None;
}

PlottingWorkspaceTreeItemState plottingWorkspaceTreeItemState(PlottingWorkspaceTreeItem const &item,
                                                              PlotOutputTypeProperties const &properties) {
  auto childStates = std::vector<PlottingWorkspaceTreeItemState>{};
  childStates.reserve(item.children.size());
  std::transform(item.children.cbegin(), item.children.cend(), std::back_inserter(childStates),
                 [&](auto const &child) { return plottingWorkspaceTreeItemState(child, properties); });
  auto const canSelectItemDirectly = isSelectable(item, properties);
  auto const hasRequiredScope = item.context.scopeItems.contains(properties.selectionScope());
  auto const canSelectItemViaParent =
      canSelectItemDirectly || (item.itemType == PlottingWorkspaceTreeItemType::Workspace && hasRequiredScope &&
                                isWorkspaceIncluded(item, properties));
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
  if (plotProperties.plotRequests().size() > 1) {
    return {.plotIndividualEnabled = outputSelectionEnabled && hasSelectedItems(selectedPlottingWorkspaceCount)};
  }
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
  return itemStates;
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
