// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Plotting/presenter/PlottingViewStateBuilder.h"
#include "../../../ISISReflectometry/GUI/Plotting/presenter/PlottingWorkspaceTreeDisplayStateBuilder.h"

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

  void testWorkspaceTreeDisplayStateMutesStitchedOutputsForAlignment() {
    PlottingWorkspaceTreeDisplayStateBuilder builder;
    auto const workspaceItems = std::vector<PlottingWorkspaceTreeItem>{groupItem(
        "Group 1", {workspaceItem("Group 1", {}, "stitched_12345", PlottingWorkspaceOutputType::IvsQBinned),
                    runItem("12345", {workspaceItem("IvsQ_binned_12345", PlottingWorkspaceOutputType::IvsQBinned)})})};

    auto const displayItems = builder.workspaceItemsForPlotOutputType(workspaceItems, PlotOutputType::Alignment);

    TS_ASSERT(displayItems[0].children[0].muted);
    TS_ASSERT_EQUALS(displayItems[0].children[0].selectionMode, PlottingWorkspaceTreeSelectionMode::None);
    TS_ASSERT(!displayItems[0].children[1].children[0].muted);
    TS_ASSERT_EQUALS(displayItems[0].children[1].children[0].selectionMode,
                     PlottingWorkspaceTreeSelectionMode::DirectAndParent);
  }

private:
  PlottingWorkspaceTreeItem groupItem(std::string label, std::vector<PlottingWorkspaceTreeItem> children) const {
    return {
        std::move(label),   PlottingWorkspaceTreeItemType::Group, PlottingWorkspaceOutputType::None, "Group 1", {}, "",
        std::move(children)};
  }

  PlottingWorkspaceTreeItem runItem(std::string label, std::vector<PlottingWorkspaceTreeItem> children) const {
    auto runNumbers = std::vector<std::string>{label};
    return {std::move(label),
            PlottingWorkspaceTreeItemType::Run,
            PlottingWorkspaceOutputType::None,
            "Group 1",
            std::move(runNumbers),
            "",
            std::move(children)};
  }

  PlottingWorkspaceTreeItem workspaceItem(std::string groupName, std::vector<std::string> runNumbers, std::string label,
                                          PlottingWorkspaceOutputType outputType) const {
    auto const workspaceName = label;
    return {std::move(label),
            PlottingWorkspaceTreeItemType::Workspace,
            outputType,
            std::move(groupName),
            std::move(runNumbers),
            workspaceName,
            {}};
  }

  PlottingWorkspaceTreeItem workspaceItem(std::string label, PlottingWorkspaceOutputType outputType) const {
    return workspaceItem("Group 1", {"12345"}, std::move(label), outputType);
  }
};
