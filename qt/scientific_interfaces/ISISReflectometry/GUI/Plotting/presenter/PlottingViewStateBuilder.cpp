// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "PlottingViewStateBuilder.h"

#include "GUI/Plotting/presenter/PlotOutputTypeProperties.h"

#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
auto constexpr minimumSelectedItemsForMultiPlot = size_t{2};

bool hasSelectedItems(size_t selectedItemCount) { return selectedItemCount > 0; }

bool hasEnoughSelectedItemsForMultiPlot(size_t selectedItemCount, size_t selectedWorkspaceGroupCount,
                                        PlotOutputTypeProperties const &plotProperties) {
  if (plotProperties.requiresWorkspaceGroupsForMultiPlot()) {
    return selectedWorkspaceGroupCount >= minimumSelectedItemsForMultiPlot;
  }
  return selectedItemCount >= minimumSelectedItemsForMultiPlot;
}

bool hasEnoughSelectedItemsForMultiOutputPlot(size_t selectedItemCount, size_t selectedWorkspaceGroupCount,
                                              PlotOutputTypeProperties const &plotProperties, bool addToExistingPlot) {
  return addToExistingPlot
             ? hasSelectedItems(selectedItemCount)
             : hasEnoughSelectedItemsForMultiPlot(selectedItemCount, selectedWorkspaceGroupCount, plotProperties);
}
} // namespace

std::vector<PlotOutputTypeViewItem>
PlottingViewStateBuilder::outputTypeViewItems(std::vector<PlotOutputType> const &outputTypes) const {
  auto viewItems = std::vector<PlotOutputTypeViewItem>{};
  viewItems.reserve(outputTypes.size());
  for (auto outputType : outputTypes) {
    viewItems.push_back({outputType, plotOutputTypeProperties(outputType).displayName()});
  }
  return viewItems;
}

PlotOutputControlsState PlottingViewStateBuilder::outputControlsState(PlotOutputType outputType) const {
  auto const &plotProperties = plotOutputTypeProperties(outputType);
  return {plotProperties.showsPlotProperties(), plotProperties.showsDetectorMapProperties(),
          plotProperties.showsAlignmentProperties()};
}

PlotActionState PlottingViewStateBuilder::plotActionState(bool outputSelectionEnabled, size_t selectedWorkspaceCount,
                                                          size_t selectedWorkspaceGroupCount, PlotOutputType outputType,
                                                          bool addToExistingPlot, bool hasActiveReflectometryFigure,
                                                          bool activePlotOverplotCompatible) const {
  auto const &plotProperties = plotOutputTypeProperties(outputType);
  auto const addToExistingEnabled =
      outputSelectionEnabled && hasActiveReflectometryFigure && plotProperties.supportsAddToExistingPlot();
  auto const addToExistingChecked = addToExistingPlot && addToExistingEnabled;
  auto const hasEnoughSelectedItems = hasEnoughSelectedItemsForMultiOutputPlot(
      selectedWorkspaceCount, selectedWorkspaceGroupCount, plotProperties, addToExistingChecked);
  return {outputSelectionEnabled && !addToExistingChecked && hasSelectedItems(selectedWorkspaceCount),
          outputSelectionEnabled && plotProperties.supportsOverplot() &&
              (!addToExistingChecked || activePlotOverplotCompatible) && hasEnoughSelectedItems,
          outputSelectionEnabled && hasEnoughSelectedItems,
          outputSelectionEnabled && hasSelectedItems(selectedWorkspaceCount),
          addToExistingEnabled,
          addToExistingChecked};
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
