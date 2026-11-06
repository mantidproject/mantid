// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Plotting/model/PlottingWorkspaceTree.h"
#include "../../../ISISReflectometry/Reduction/RunsTable.h"
#include "../../../ISISReflectometry/TestHelpers/PlottingTestHelpers.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;

class PlottingWorkspaceTreeTest : public CxxTest::TestSuite {
public:
  void tearDown() override { Mantid::API::AnalysisDataService::Instance().clear(); }

  void testRebuildOwnsWorkspaceHierarchyAndResolvesPlottingWorkspacesByName() {
    addWorkspaceWithRunNumber("IvsQ_12345", "12345");
    auto tree = PlottingWorkspaceTree{};

    tree.rebuild(runsTableWith(successfulRow("12345", {{""}, {"IvsQ_12345"}, {""}})));

    TS_ASSERT_EQUALS(tree.items().size(), 1);
    TS_ASSERT_EQUALS(tree.items()[0].itemType, PlottingWorkspaceTreeItemType::ReductionGroup);
    TS_ASSERT_EQUALS(tree.items()[0].children[0].itemType, PlottingWorkspaceTreeItemType::Run);
    TS_ASSERT_EQUALS(tree.items()[0].children[0].children[0].reducedOutputType, ReducedWorkspaceOutputType::IvsQ);

    auto const plottingWorkspaces = tree.plottingWorkspacesForNames({"unknown", "IvsQ_12345"});
    TS_ASSERT_EQUALS(plottingWorkspaces.size(), 1);
    TS_ASSERT_EQUALS(plottingWorkspaces[0].workspaceName, "IvsQ_12345");
    TS_ASSERT_EQUALS(plottingWorkspaces[0].runNumbers, std::vector<std::string>{"12345"});
  }

  void testPlottingWorkspaceRetainsSummedRunLog() {
    addWorkspaceWithRunNumber("IvsQ_12345+12346", "12345+12346");
    auto row = Row({"12345", "12346"}, 0.5, TransmissionRunPair(), RangeInQ(), std::nullopt, ReductionOptionsMap(),
                   ReductionWorkspaces({}, TransmissionRunPair()));
    row.setOutputNames({{""}, {"IvsQ_12345+12346"}, {""}});
    row.setSuccess();
    auto tree = PlottingWorkspaceTree{};
    tree.rebuild(runsTableWith(std::move(row)));

    auto const workspaces = tree.plottingWorkspacesForNames({"IvsQ_12345+12346"});
    TS_ASSERT_EQUALS(workspaces.size(), 1);
    TS_ASSERT_EQUALS(workspaces[0].runNumbers, std::vector<std::string>{"12345+12346"});
  }

  void testPlottingWorkspaceRecordsContainingWorkspaceGroupAndPeriodNumber() {
    addWorkspaceWithRunNumberAndPeriod("IvsQ_binned_12345_2", "12345", 2, 2);
    auto workspaceGroup = std::make_shared<Mantid::API::WorkspaceGroup>();
    workspaceGroup->addWorkspace(
        Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::Workspace>("IvsQ_binned_12345_2"));
    Mantid::API::AnalysisDataService::Instance().addOrReplace("IvsQ_binned_12345", workspaceGroup);
    auto tree = PlottingWorkspaceTree{};

    tree.rebuild(runsTableWith(successfulRow("12345", {{""}, {""}, {"IvsQ_binned_12345"}})));

    auto const plottingWorkspaces = tree.plottingWorkspacesForNames({"IvsQ_binned_12345_2"});
    TS_ASSERT_EQUALS(plottingWorkspaces.size(), 1);
    TS_ASSERT_EQUALS(plottingWorkspaces[0].context.scopeItems.at(PlotSelectionScope::WorkspaceGroup).workspaceName,
                     "IvsQ_binned_12345");
    TS_ASSERT_EQUALS(plottingWorkspaces[0].periodNumber, 2);
    auto const &context = plottingWorkspaces[0].context;
    TS_ASSERT_EQUALS(context.origin, PlottingWorkspaceOrigin::RunOutput);
    TS_ASSERT_EQUALS(context.scopeItems.at(PlotSelectionScope::ReductionGroup).path, (PlottingWorkspaceTreePath{0}));
    TS_ASSERT_EQUALS(context.scopeItems.at(PlotSelectionScope::Run).path, (PlottingWorkspaceTreePath{0, 0}));
    TS_ASSERT_EQUALS(context.scopeItems.at(PlotSelectionScope::WorkspaceGroup).path,
                     (PlottingWorkspaceTreePath{0, 0, 0}));
    TS_ASSERT_EQUALS(context.scopeItems.at(PlotSelectionScope::Workspace).path,
                     (PlottingWorkspaceTreePath{0, 0, 0, 0}));
    TS_ASSERT_EQUALS(context.scopeItems.at(PlotSelectionScope::Workspace).workspaceName, "IvsQ_binned_12345_2");
    TS_ASSERT_EQUALS(context, tree.items()[0].children[0].children[0].children[0].context);
  }

  void testContextPathsFollowInsertedRowsAndDistinguishRepeatedLabels() {
    addWorkspaceGroup("stitched", {"stitched_up", "stitched_down"});
    addWorkspaceGroup("group_a", {"a_up", "a_down"});
    addWorkspaceGroup("group_b", {"b_up", "b_down"});
    addWorkspaceWithRunNumber("standalone", "12345");
    auto emptyRow = successfulRow("missing", {{"missing_lambda"}, {"missing_q"}, {"missing_binned"}});
    auto rowA = successfulRow("12345", {{"missing_lambda"}, {"missing_q"}, {"group_a"}});
    auto rowB = successfulRow("12345", {{""}, {""}, {"group_b"}});
    auto firstGroup = Group("Repeated label", {emptyRow, rowA, rowB});
    firstGroup.setOutputNames({{"stitched"}});
    firstGroup.setSuccess();
    auto secondGroup = Group("Repeated label", {successfulRow("12345", {{""}, {"standalone"}, {""}})});
    secondGroup.setSuccess();
    auto tree = PlottingWorkspaceTree{};

    tree.rebuild(RunsTable({}, 0.0, ReductionJobs({Group("Empty", {}), firstGroup, secondGroup})));

    TS_ASSERT_EQUALS(tree.items().size(), 2);
    TS_ASSERT_EQUALS(tree.items()[0].label, tree.items()[1].label);
    TS_ASSERT_EQUALS(tree.items()[0].children[1].label, tree.items()[0].children[2].label);
    auto const selected = tree.plottingWorkspacesForNames({"a_up", "b_up", "standalone", "stitched_up"});
    TS_ASSERT_EQUALS(selected[0].context.scopeItems.at(PlotSelectionScope::Run).path,
                     (PlottingWorkspaceTreePath{0, 1}));
    TS_ASSERT_EQUALS(selected[1].context.scopeItems.at(PlotSelectionScope::Run).path,
                     (PlottingWorkspaceTreePath{0, 2}));
    TS_ASSERT_EQUALS(selected[0].context.scopeItems.at(PlotSelectionScope::WorkspaceGroup).path,
                     (PlottingWorkspaceTreePath{0, 1, 0}));
    TS_ASSERT_EQUALS(selected[1].context.scopeItems.at(PlotSelectionScope::WorkspaceGroup).path,
                     (PlottingWorkspaceTreePath{0, 2, 0}));
    TS_ASSERT_EQUALS(selected[2].context.scopeItems.at(PlotSelectionScope::Workspace).path,
                     (PlottingWorkspaceTreePath{1, 0, 0}));
    TS_ASSERT(!selected[2].context.scopeItems.contains(PlotSelectionScope::WorkspaceGroup));
    TS_ASSERT_EQUALS(selected[3].context.origin, PlottingWorkspaceOrigin::PostprocessedGroupOutput);
    TS_ASSERT(!selected[3].context.scopeItems.contains(PlotSelectionScope::Run));
    TS_ASSERT_EQUALS(selected[3].context.scopeItems.at(PlotSelectionScope::WorkspaceGroup).path,
                     (PlottingWorkspaceTreePath{0, 0}));
    auto const &stitchedGroup = tree.items()[0].children[0];
    TS_ASSERT_EQUALS(stitchedGroup.context.origin, PlottingWorkspaceOrigin::PostprocessedGroupOutput);
    TS_ASSERT_EQUALS(stitchedGroup.children[1].context.origin, PlottingWorkspaceOrigin::PostprocessedGroupOutput);
    TS_ASSERT_EQUALS(stitchedGroup.children[1].context.scopeItems.at(PlotSelectionScope::Workspace).path,
                     (PlottingWorkspaceTreePath{0, 0, 1}));
    TS_ASSERT_EQUALS(stitchedGroup.context.scopeItems.at(PlotSelectionScope::WorkspaceGroup).workspaceName, "stitched");
  }

  void testRebuildReplacesExistingItemsAndPlottingWorkspaces() {
    addWorkspaceWithRunNumber("IvsQ_12345", "12345");
    auto tree = PlottingWorkspaceTree{};
    tree.rebuild(runsTableWith(successfulRow("12345", {{""}, {"IvsQ_12345"}, {""}})));

    tree.rebuild(RunsTable({}, 0.0, ReductionJobs{}));

    TS_ASSERT(tree.items().empty());
    TS_ASSERT(tree.plottingWorkspacesForNames({"IvsQ_12345"}).empty());
  }

private:
  void addWorkspaceGroup(std::string const &name, std::vector<std::string> const &members) const {
    auto group = std::make_shared<Mantid::API::WorkspaceGroup>();
    for (auto const &member : members) {
      addWorkspaceWithRunNumber(member, "12345");
      group->addWorkspace(Mantid::API::AnalysisDataService::Instance().retrieve(member));
    }
    Mantid::API::AnalysisDataService::Instance().addOrReplace(name, group);
  }

  RunsTable runsTableWith(Row row) const {
    auto group = Group("Group 1", {std::move(row)});
    group.setSuccess();
    return RunsTable({}, 0.0, ReductionJobs({std::move(group)}));
  }

  Row successfulRow(std::string runNumber, std::vector<std::vector<std::string>> outputs) const {
    auto row = Row({std::move(runNumber)}, 0.5, TransmissionRunPair(), RangeInQ(), std::nullopt, ReductionOptionsMap(),
                   ReductionWorkspaces({}, TransmissionRunPair()));
    row.setOutputNames(std::move(outputs));
    row.setSuccess();
    return row;
  }

  void addWorkspaceWithRunNumber(std::string const &workspaceName, std::string const &runNumber) const {
    auto workspace = WorkspaceCreationHelper::create2DWorkspace(1, 1);
    workspace->mutableRun().addProperty("run_number", runNumber);
    Mantid::API::AnalysisDataService::Instance().addOrReplace(workspaceName, workspace);
  }

  void addWorkspaceWithRunNumberAndPeriod(std::string const &workspaceName, std::string const &runNumber,
                                          int numberOfPeriods, int currentPeriod) const {
    auto workspace = WorkspaceCreationHelper::create2DWorkspace(1, 1);
    workspace->mutableRun().addProperty("run_number", runNumber);
    workspace->mutableRun().addProperty("nperiods", numberOfPeriods);
    workspace->mutableRun().addProperty("current_period", currentPeriod);
    Mantid::API::AnalysisDataService::Instance().addOrReplace(workspaceName, workspace);
  }
};
