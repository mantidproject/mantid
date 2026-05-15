// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Plotting/PlottingModel.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;

class PlottingModelTest : public CxxTest::TestSuite {
public:
  void tearDown() override { Mantid::API::AnalysisDataService::Instance().clear(); }

  void testReturnsSelectedWorkspacesForNonSpinAsymmetryPlotOutputTypes() {
    auto model = PlottingModel{};
    auto const workspaces = workspaceSelections({"IvsQ_12345", "IvsQ_22345"});

    auto const workspacesForPlotting =
        model.workspacesForPlotting(workspaces, PlotOutputOptions{PlotOutputType::ReflectivityCurve});

    auto const expected = std::vector<std::string>{"IvsQ_12345", "IvsQ_22345"};
    TS_ASSERT_EQUALS(workspacesForPlotting, expected);
  }

  void testReturnsSelectedWorkspaceGroupChildrenForNonSpinAsymmetryPlotOutputTypes() {
    auto model = PlottingModel{};
    auto const workspaces = workspaceSelections({"IvsQ_12345_1", "IvsQ_12345_2"}, "IvsQ_12345");

    auto const workspacesForPlotting =
        model.workspacesForPlotting(workspaces, PlotOutputOptions{PlotOutputType::ReflectivityCurve});

    auto const expected = std::vector<std::string>{"IvsQ_12345_1", "IvsQ_12345_2"};
    TS_ASSERT_EQUALS(workspacesForPlotting, expected);
  }

  void testSpinAsymmetryCreatesAsymmetryForTwoWorkspaces() {
    createConstantWorkspace("up", 6.0);
    createConstantWorkspace("down", 2.0);
    auto model = PlottingModel{};
    auto const workspaces = workspaceSelections({"up", "down"});

    auto const workspacesForPlotting =
        model.workspacesForPlotting(workspaces, PlotOutputOptions{PlotOutputType::SpinAsymmetry});

    TS_ASSERT_EQUALS(workspacesForPlotting.size(), 1);
    TS_ASSERT_EQUALS(workspacesForPlotting[0], "__isis_reflectometry_spin_asymmetry_0");
    assertYValue("__isis_reflectometry_spin_asymmetry_0", 0.5);
    TS_ASSERT(Mantid::API::AnalysisDataService::Instance().doesExist("__isis_reflectometry_spin_asymmetry_0"));
  }

  void testSpinAsymmetryUsesFirstAndLastSelectedChildrenFromWorkspaceGroup() {
    createConstantWorkspace("uu", 8.0);
    createConstantWorkspace("ud", 5.0);
    createConstantWorkspace("du", 4.0);
    createConstantWorkspace("dd", 2.0);
    auto model = PlottingModel{};
    auto const workspaces = workspaceSelections({"uu", "ud", "du", "dd"}, "polarized_group");

    auto const workspacesForPlotting =
        model.workspacesForPlotting(workspaces, PlotOutputOptions{PlotOutputType::SpinAsymmetry});

    TS_ASSERT_EQUALS(workspacesForPlotting.size(), 1);
    TS_ASSERT_EQUALS(workspacesForPlotting[0], "__isis_reflectometry_spin_asymmetry_0");
    assertYValue("__isis_reflectometry_spin_asymmetry_0", 0.6);
  }

  void testSpinAsymmetryReturnsNoWorkspacesForUnsupportedSelectionSize() {
    createConstantWorkspace("only_one", 6.0);
    auto model = PlottingModel{};
    auto const workspaces = workspaceSelections({"only_one"});

    auto const workspacesForPlotting =
        model.workspacesForPlotting(workspaces, PlotOutputOptions{PlotOutputType::SpinAsymmetry});

    TS_ASSERT(workspacesForPlotting.empty());
  }

private:
  std::vector<PlottingWorkspaceSelection> workspaceSelections(std::vector<std::string> workspaceNames,
                                                              std::string const &workspaceGroupName = "") {
    auto selections = std::vector<PlottingWorkspaceSelection>{};
    selections.reserve(workspaceNames.size());
    for (auto const &workspaceName : workspaceNames) {
      selections.push_back(
          {workspaceName, PlottingWorkspaceOutputType::IvsQBinned, "Group 1", {"12345"}, workspaceGroupName});
    }
    return selections;
  }

  void createConstantWorkspace(std::string const &name, double yValue) {
    Mantid::API::AnalysisDataService::Instance().addOrReplace(
        name, WorkspaceCreationHelper::create1DWorkspaceConstant(1, yValue, 0.0, false));
  }

  void assertYValue(std::string const &workspaceName, double expected) {
    auto workspace =
        Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>(workspaceName);
    TS_ASSERT_DELTA(workspace->y(0).front(), expected, 1e-12);
  }
};
