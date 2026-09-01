// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Plotting/model/PlottingWorkspaceTree.h"
#include "../../../ISISReflectometry/Reduction/RunsTable.h"

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

    tree.rebuild(runsTableWith(successfulRow("12345", {"", "IvsQ_12345", ""})));

    TS_ASSERT_EQUALS(tree.items().size(), 1);
    TS_ASSERT_EQUALS(tree.items()[0].itemType, PlottingWorkspaceTreeItemType::ReductionGroup);
    TS_ASSERT_EQUALS(tree.items()[0].children[0].itemType, PlottingWorkspaceTreeItemType::Run);
    TS_ASSERT_EQUALS(tree.items()[0].children[0].children[0].reducedOutputType, ReducedWorkspaceOutputType::IvsQ);

    auto const plottingWorkspaces = tree.plottingWorkspacesForNames({"unknown", "IvsQ_12345"});
    TS_ASSERT_EQUALS(plottingWorkspaces.size(), 1);
    TS_ASSERT_EQUALS(plottingWorkspaces[0].workspaceName, "IvsQ_12345");
    TS_ASSERT_EQUALS(plottingWorkspaces[0].runNumbers, std::vector<std::string>{"12345"});
  }

  void testPlottingWorkspaceRecordsContainingWorkspaceGroupAndPeriodNumber() {
    addWorkspaceWithRunNumberAndPeriod("IvsQ_binned_12345_2", "12345", 2, 2);
    auto workspaceGroup = std::make_shared<Mantid::API::WorkspaceGroup>();
    workspaceGroup->addWorkspace(
        Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::Workspace>("IvsQ_binned_12345_2"));
    Mantid::API::AnalysisDataService::Instance().addOrReplace("IvsQ_binned_12345", workspaceGroup);
    auto tree = PlottingWorkspaceTree{};

    tree.rebuild(runsTableWith(successfulRow("12345", {"", "", "IvsQ_binned_12345"})));

    auto const plottingWorkspaces = tree.plottingWorkspacesForNames({"IvsQ_binned_12345_2"});
    TS_ASSERT_EQUALS(plottingWorkspaces.size(), 1);
    TS_ASSERT_EQUALS(plottingWorkspaces[0].containingWorkspaceGroupName, "IvsQ_binned_12345");
    TS_ASSERT_EQUALS(plottingWorkspaces[0].periodNumber, 2);
  }

  void testRebuildReplacesExistingItemsAndPlottingWorkspaces() {
    addWorkspaceWithRunNumber("IvsQ_12345", "12345");
    auto tree = PlottingWorkspaceTree{};
    tree.rebuild(runsTableWith(successfulRow("12345", {"", "IvsQ_12345", ""})));

    tree.rebuild(RunsTable({}, 0.0, ReductionJobs{}));

    TS_ASSERT(tree.items().empty());
    TS_ASSERT(tree.plottingWorkspacesForNames({"IvsQ_12345"}).empty());
  }

private:
  RunsTable runsTableWith(Row row) const {
    auto group = Group("Group 1", {std::move(row)});
    group.setSuccess();
    return RunsTable({}, 0.0, ReductionJobs({std::move(group)}));
  }

  Row successfulRow(std::string runNumber, std::vector<std::string> outputs) const {
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
