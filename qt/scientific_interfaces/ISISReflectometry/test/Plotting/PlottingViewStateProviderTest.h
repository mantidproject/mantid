// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Plotting/presenter/PlottingViewStateProvider.h"

#include <cxxtest/TestSuite.h>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;

class PlottingViewStateProviderTest : public CxxTest::TestSuite {
public:
  void testPlotActionStateEnablesAddToExistingActionsForSingleCompatibleReflectivitySelection() {
    PlottingViewStateProvider provider;

    auto const state = provider.plotActionState(true, 1, 0, PlotOutputType::ReflectivityCurve, true, true, true);

    TS_ASSERT_EQUALS(state.plotIndividualEnabled, false);
    TS_ASSERT_EQUALS(state.plotOverplotEnabled, true);
    TS_ASSERT_EQUALS(state.plotTiledEnabled, true);
    TS_ASSERT_EQUALS(state.plotTiledVerticallyEnabled, true);
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

  void testWorkspaceTypesAreFilteredEvenWhenParentMatches() {
    PlottingViewStateProvider provider;
    auto const items = filterItems();
    for (auto type : {ReducedWorkspaceOutputType::IvsQBinned, ReducedWorkspaceOutputType::IvsLambda,
                      ReducedWorkspaceOutputType::IvsQ}) {
      auto const states = provider.plottingWorkspaceTreeItemStates(items, PlotOutputType::DetectorMap,
                                                                   boost::regex("^sample$"), {type});
      TS_ASSERT(states[0].visible);
      TS_ASSERT(states[0].children[0].visible);
      for (auto const &child : states[0].children[0].children) {
        TS_ASSERT_EQUALS(child.visible, child.reducedOutputType == type);
      }
    }
  }

  void testMatchingLeafShowsAncestorsAndHidesOtherBranches() {
    PlottingViewStateProvider provider;
    auto const states = provider.plottingWorkspaceTreeItemStates(
        filterItems(), PlotOutputType::ReflectivityCurve, boost::regex("^binned$"),
        {ReducedWorkspaceOutputType::IvsQBinned, ReducedWorkspaceOutputType::IvsQ});

    TS_ASSERT(states[0].visible);
    TS_ASSERT(states[0].children[0].visible);
    TS_ASSERT(states[0].children[0].children[0].visible);
    TS_ASSERT(!states[0].children[0].children[1].visible);
    TS_ASSERT(!states[0].children[1].visible);
    TS_ASSERT(!states[1].visible);
  }

  void testMatchingRunAdmitsOnlyItsDescendants() {
    PlottingViewStateProvider provider;
    auto const states = provider.plottingWorkspaceTreeItemStates(
        filterItems(), PlotOutputType::ReflectivityCurve, boost::regex("^12345$"),
        {ReducedWorkspaceOutputType::IvsQBinned, ReducedWorkspaceOutputType::IvsQ});

    TS_ASSERT(states[0].children[0].children[0].visible);
    TS_ASSERT(states[0].children[0].children[1].visible);
    TS_ASSERT(!states[0].children[0].children[2].visible);
    TS_ASSERT(!states[0].children[1].visible);
  }

  void testEmptyParentsRemainHiddenEvenWhenMatching() {
    PlottingViewStateProvider provider;
    auto const items = std::vector<PlottingWorkspaceTreeItem>{groupItem("sample", {runItem("12345", {})})};
    auto const states = provider.plottingWorkspaceTreeItemStates(
        items, PlotOutputType::ReflectivityCurve, boost::regex("sample"), {ReducedWorkspaceOutputType::IvsQBinned});
    TS_ASSERT(!states[0].visible);
    TS_ASSERT(!states[0].children[0].visible);
  }

  void testNoCheckedTypesHidesAllParents() {
    PlottingViewStateProvider provider;
    auto const states = provider.plottingWorkspaceTreeItemStates(filterItems(), PlotOutputType::ReflectivityCurve,
                                                                 boost::regex("sample"), {});
    TS_ASSERT(!states[0].visible);
    TS_ASSERT(!states[1].visible);
  }

  void testRegexIsCaseSensitiveAndAnEmptyExpressionRestoresMatches() {
    PlottingViewStateProvider provider;
    auto const items = filterItems();
    auto const hidden = provider.plottingWorkspaceTreeItemStates(
        items, PlotOutputType::ReflectivityCurve, boost::regex("BINNED"), {ReducedWorkspaceOutputType::IvsQBinned});
    TS_ASSERT(!hidden[0].visible);
    auto const shown = provider.plottingWorkspaceTreeItemStates(
        items, PlotOutputType::ReflectivityCurve, boost::regex(""), {ReducedWorkspaceOutputType::IvsQBinned});
    TS_ASSERT(shown[0].visible);
    TS_ASSERT(shown[0].children[0].children[0].visible);
    TS_ASSERT(shown[0].children[1].visible);
  }

  void testIncompleteSpinGroupIsVisibleButCannotContributeToSelection() {
    PlottingViewStateProvider provider;
    auto const items = std::vector<PlottingWorkspaceTreeItem>{
        groupItem("sample", {runItem("12345", {{"spin_group",
                                                PlottingWorkspaceTreeItemType::WorkspaceGroup,
                                                ReducedWorkspaceOutputType::None,
                                                "spin_group",
                                                {workspaceItem("spin_1", ReducedWorkspaceOutputType::IvsQBinned),
                                                 workspaceItem("spin_2", ReducedWorkspaceOutputType::IvsQBinned),
                                                 workspaceItem("spin_3", ReducedWorkspaceOutputType::IvsQBinned),
                                                 workspaceItem("spin_4", ReducedWorkspaceOutputType::IvsQBinned)}}})})};
    auto const states = provider.plottingWorkspaceTreeItemStates(
        items, PlotOutputType::SpinAsymmetry, boost::regex("^spin_[12]$"), {ReducedWorkspaceOutputType::IvsQBinned});
    auto const &group = states[0].children[0].children[0];
    TS_ASSERT(group.visible);
    TS_ASSERT(group.children[0].visible);
    TS_ASSERT(!group.children[2].visible);
    TS_ASSERT_EQUALS(group.selectionMode, PlottingWorkspaceTreeSelectionMode::None);
    for (auto const &child : group.children) {
      TS_ASSERT_EQUALS(child.selectionMode, PlottingWorkspaceTreeSelectionMode::None);
    }
    auto const restored = provider.plottingWorkspaceTreeItemStates(
        items, PlotOutputType::SpinAsymmetry, boost::regex(""), {ReducedWorkspaceOutputType::IvsQBinned});
    TS_ASSERT_EQUALS(restored[0].children[0].children[0].selectionMode,
                     PlottingWorkspaceTreeSelectionMode::DirectAndParent);
    TS_ASSERT_EQUALS(restored[0].children[0].children[0].children[0].selectionMode,
                     PlottingWorkspaceTreeSelectionMode::ParentOnly);
  }

  void testRegexMatchingFailureExcludesOnlyAffectedLabels() {
    PlottingViewStateProvider provider;
    auto const difficultLabel = std::string(200, 'a') + 'b';
    auto const expression = boost::regex("(a+)+$");
    TS_ASSERT_THROWS(boost::regex_search(difficultLabel, expression), std::runtime_error const &);
    auto const states = provider.plottingWorkspaceTreeItemStates(
        {workspaceItem(difficultLabel, ReducedWorkspaceOutputType::IvsQBinned),
         workspaceItem("aaa", ReducedWorkspaceOutputType::IvsQBinned)},
        PlotOutputType::ReflectivityCurve, expression, {ReducedWorkspaceOutputType::IvsQBinned});
    TS_ASSERT(!states[0].visible);
    TS_ASSERT(states[1].visible);
  }

private:
  std::vector<PlottingWorkspaceTreeItem> filterItems() const {
    return {groupItem("sample",
                      {runItem("12345", {workspaceItem("binned", ReducedWorkspaceOutputType::IvsQBinned),
                                         workspaceItem("q", ReducedWorkspaceOutputType::IvsQ),
                                         workspaceItem("lambda", ReducedWorkspaceOutputType::IvsLambda)}),
                       runItem("67890", {workspaceItem("other_binned", ReducedWorkspaceOutputType::IvsQBinned)})}),
            groupItem("empty", {})};
  }
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
