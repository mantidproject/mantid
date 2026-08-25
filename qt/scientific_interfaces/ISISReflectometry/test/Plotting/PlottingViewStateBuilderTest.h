// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Plotting/presenter/PlottingViewStateBuilder.h"

#include <cxxtest/TestSuite.h>
#include <string>
#include <utility>
#include <vector>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;

class PlottingViewStateBuilderTest : public CxxTest::TestSuite {
public:
  void testPlotActionStateEnablesAddToExistingActionsForSingleCompatibleReflectivitySelection() {
    PlottingViewStateBuilder builder;

    auto const state = builder.plotActionState(true, 1, 0, PlotOutputType::ReflectivityCurve, true, true, true);

    TS_ASSERT_EQUALS(state.plotIndividualEnabled, false);
    TS_ASSERT_EQUALS(state.plotOverplotEnabled, true);
    TS_ASSERT_EQUALS(state.plotTiledEnabled, true);
    TS_ASSERT_EQUALS(state.plotTiledVerticallyEnabled, true);
    TS_ASSERT_EQUALS(state.addToExistingPlotEnabled, true);
    TS_ASSERT_EQUALS(state.addToExistingPlotChecked, true);
  }

  void testPlotActionStateEnablesTiledAddToExistingForNonOverplottableActiveFigure() {
    PlottingViewStateBuilder builder;

    auto const state = builder.plotActionState(true, 1, 0, PlotOutputType::ReflectivityCurve, true, true, false);

    TS_ASSERT_EQUALS(state.plotIndividualEnabled, false);
    TS_ASSERT_EQUALS(state.plotOverplotEnabled, false);
    TS_ASSERT_EQUALS(state.plotTiledEnabled, true);
    TS_ASSERT_EQUALS(state.plotTiledVerticallyEnabled, true);
    TS_ASSERT_EQUALS(state.addToExistingPlotEnabled, true);
    TS_ASSERT_EQUALS(state.addToExistingPlotChecked, true);
  }

  void testPlotActionStateDisablesAddToExistingForDetectorMap() {
    PlottingViewStateBuilder builder;

    auto const state = builder.plotActionState(true, 1, 0, PlotOutputType::DetectorMap, true, true, true);

    TS_ASSERT_EQUALS(state.plotIndividualEnabled, true);
    TS_ASSERT_EQUALS(state.plotOverplotEnabled, false);
    TS_ASSERT_EQUALS(state.plotTiledEnabled, false);
    TS_ASSERT_EQUALS(state.plotTiledVerticallyEnabled, true);
    TS_ASSERT_EQUALS(state.addToExistingPlotEnabled, false);
    TS_ASSERT_EQUALS(state.addToExistingPlotChecked, false);
  }

  void testPlottingWorkspaceTreeItemStatesMuteStitchedOutputsForAlignment() {
    PlottingViewStateBuilder builder;
    auto const plottingWorkspaceTreeItems = std::vector<PlottingWorkspaceTreeItem>{groupItem(
        "Group 1", {workspaceItem("stitched_12345", ReducedWorkspaceOutputType::IvsQBinned),
                    runItem("12345", {workspaceItem("IvsQ_binned_12345", ReducedWorkspaceOutputType::IvsQBinned)})})};

    auto const itemStates =
        builder.plottingWorkspaceTreeItemStates(plottingWorkspaceTreeItems, PlotOutputType::Alignment);

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
