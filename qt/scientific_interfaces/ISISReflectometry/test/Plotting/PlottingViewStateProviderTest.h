// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Plotting/presenter/PlotOutputTypeProperties.h"
#include "../../../ISISReflectometry/GUI/Plotting/presenter/PlottingViewStateProvider.h"
#include "../../../ISISReflectometry/TestHelpers/PlottingTestHelpers.h"

#include <cxxtest/TestSuite.h>
#include <string>
#include <utility>
#include <vector>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;

class PlottingViewStateProviderTest : public CxxTest::TestSuite {
public:
  void testOrdinaryOutputsDefineOneRequestUsingViewSettings() {
    for (auto const outputType : {PlotOutputType::ReflectivityCurve, PlotOutputType::SpinAsymmetry,
                                  PlotOutputType::DetectorMap, PlotOutputType::Alignment}) {
      auto const &definitions = plotOutputTypeProperties(outputType).plotRequests();
      TS_ASSERT_EQUALS(definitions.size(), 1);
      TS_ASSERT_EQUALS(definitions.front().outputType, outputType);
      TS_ASSERT(!definitions.front().settingsOverride);
      TS_ASSERT(!PlottingViewStateProvider().outputControlsState(outputType).fixedLayout);
    }
  }

  void testCombinedOutputDefinesOrderedRequestsWithFixedSettings() {
    auto const &definitions = plotOutputTypeProperties(PlotOutputType::ReflectivityAndSpinAsymmetry).plotRequests();
    TS_ASSERT_EQUALS(definitions.size(), 2);
    TS_ASSERT_EQUALS(definitions[0].outputType, PlotOutputType::ReflectivityCurve);
    TS_ASSERT_EQUALS(definitions[1].outputType, PlotOutputType::SpinAsymmetry);
    TS_ASSERT(definitions[0].settingsOverride);
    TS_ASSERT(definitions[1].settingsOverride);
    auto const &first = definitions[0].settingsOverride.value();
    auto const &second = definitions[1].settingsOverride.value();
    TS_ASSERT_EQUALS(first.layout, PlotLayout::Overplot);
    TS_ASSERT(!first.addToExistingPlot);
    TS_ASSERT(!first.tiledVertically);
    TS_ASSERT(!first.shareXAxisWhenTiled);
    TS_ASSERT(!first.shareYAxisWhenTiled);
    TS_ASSERT_EQUALS(second.layout, PlotLayout::Tiled);
    TS_ASSERT(second.addToExistingPlot);
    TS_ASSERT(second.tiledVertically);
    TS_ASSERT(second.shareXAxisWhenTiled);
    TS_ASSERT(!second.shareYAxisWhenTiled);
  }

  void testCombinedOutputUsesSpinAsymmetrySelectionRules() {
    PlottingViewStateProvider provider;
    auto const items = std::vector<PlottingWorkspaceTreeItem>{
        groupItem("Group", {workspaceItem("stitched", ReducedWorkspaceOutputType::IvsQBinned),
                            runItem("12345", {workspaceItem("binned", ReducedWorkspaceOutputType::IvsQBinned),
                                              workspaceItem("unbinned", ReducedWorkspaceOutputType::IvsQ),
                                              workspaceItem("lambda", ReducedWorkspaceOutputType::IvsLambda)})})};
    TS_ASSERT_EQUALS(provider.plottingWorkspaceTreeItemStates(items, PlotOutputType::ReflectivityAndSpinAsymmetry),
                     provider.plottingWorkspaceTreeItemStates(items, PlotOutputType::SpinAsymmetry));
  }

  void testCombinedOutputOnlyEnablesPlotForANonemptySelectionWhenIdle() {
    PlottingViewStateProvider provider;
    auto const outputType = PlotOutputType::ReflectivityAndSpinAsymmetry;
    TS_ASSERT_EQUALS(provider.plotActionState(true, 1, 1, outputType, true, true, true),
                     (PlotActionState{true, false, false, false, false, false, false}));
    TS_ASSERT_EQUALS(provider.plotActionState(true, 0, 0, outputType, false, false, false), PlotActionState{});
    TS_ASSERT_EQUALS(provider.plotActionState(false, 2, 2, outputType, true, true, true), PlotActionState{});
    TS_ASSERT(provider.outputControlsState(outputType).fixedLayout);
  }

  void testPlotActionStateEnablesAddToExistingActionsForSingleCompatibleReflectivitySelection() {
    PlottingViewStateProvider provider;

    auto const state = provider.plotActionState(true, 1, 0, PlotOutputType::ReflectivityCurve, true, true, true);

    TS_ASSERT_EQUALS(state.plotIndividualEnabled, false);
    TS_ASSERT_EQUALS(state.plotOverplotEnabled, true);
    TS_ASSERT_EQUALS(state.plotTiledEnabled, true);
    TS_ASSERT_EQUALS(state.plotTiledVerticallyEnabled, true);
    TS_ASSERT_EQUALS(state.shareAxesWhenTiledEnabled, true);
    TS_ASSERT_EQUALS(state.addToExistingPlotEnabled, true);
    TS_ASSERT_EQUALS(state.addToExistingPlotChecked, true);
  }

  void testPlotActionStateEnablesTiledAddToExistingForNonOverplottableActiveFigure() {
    PlottingViewStateProvider provider;

    auto const state = provider.plotActionState(true, 1, 0, PlotOutputType::ReflectivityCurve, true, true, false);

    TS_ASSERT_EQUALS(state.plotIndividualEnabled, false);
    TS_ASSERT_EQUALS(state.plotOverplotEnabled, false);
    TS_ASSERT_EQUALS(state.plotTiledEnabled, true);
    TS_ASSERT_EQUALS(state.plotTiledVerticallyEnabled, true);
    TS_ASSERT_EQUALS(state.shareAxesWhenTiledEnabled, true);
    TS_ASSERT_EQUALS(state.addToExistingPlotEnabled, true);
    TS_ASSERT_EQUALS(state.addToExistingPlotChecked, true);
  }

  void testPlotActionStateDisablesAddToExistingForDetectorMap() {
    PlottingViewStateProvider provider;

    auto const state = provider.plotActionState(true, 1, 0, PlotOutputType::DetectorMap, true, true, true);

    TS_ASSERT_EQUALS(state.plotIndividualEnabled, true);
    TS_ASSERT_EQUALS(state.plotOverplotEnabled, false);
    TS_ASSERT_EQUALS(state.plotTiledEnabled, false);
    TS_ASSERT_EQUALS(state.plotTiledVerticallyEnabled, true);
    TS_ASSERT_EQUALS(state.shareAxesWhenTiledEnabled, true);
    TS_ASSERT_EQUALS(state.addToExistingPlotEnabled, false);
    TS_ASSERT_EQUALS(state.addToExistingPlotChecked, false);
  }

  void testPlottingWorkspaceTreeItemStatesMuteStitchedOutputsForAlignment() {
    PlottingViewStateProvider provider;
    auto const plottingWorkspaceTreeItems = std::vector<PlottingWorkspaceTreeItem>{groupItem(
        "Group 1", {workspaceItem("stitched_12345", ReducedWorkspaceOutputType::IvsQBinned),
                    runItem("12345", {workspaceItem("IvsQ_binned_12345", ReducedWorkspaceOutputType::IvsQBinned)})})};

    auto const itemStates =
        provider.plottingWorkspaceTreeItemStates(plottingWorkspaceTreeItems, PlotOutputType::Alignment);

    TS_ASSERT(itemStates[0].children[0].muted);
    TS_ASSERT_EQUALS(itemStates[0].children[0].selectionMode, PlottingWorkspaceTreeSelectionMode::None);
    TS_ASSERT(!itemStates[0].children[1].children[0].muted);
    TS_ASSERT_EQUALS(itemStates[0].children[1].children[0].selectionMode,
                     PlottingWorkspaceTreeSelectionMode::DirectAndParent);
  }

private:
  PlottingWorkspaceTreeItem groupItem(std::string label, std::vector<PlottingWorkspaceTreeItem> children) const {
    return {std::move(label), PlottingWorkspaceTreeItemType::ReductionGroup, ReducedWorkspaceOutputType::None, "",
            std::move(children)};
  }

  PlottingWorkspaceTreeItem runItem(std::string label, std::vector<PlottingWorkspaceTreeItem> children) const {
    return {std::move(label), PlottingWorkspaceTreeItemType::Run, ReducedWorkspaceOutputType::None, "",
            std::move(children)};
  }

  PlottingWorkspaceTreeItem workspaceItem(std::string label, ReducedWorkspaceOutputType reducedOutputType) const {
    auto const workspaceName = label;
    return {std::move(label), PlottingWorkspaceTreeItemType::Workspace, reducedOutputType, workspaceName, {}};
  }
};
