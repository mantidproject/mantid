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
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
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

  void testAlignmentCreatesPlotWorkspaceGroupForSelectedWorkspace() {
    createAlignmentInputWorkspace("alignment_input", {1.0, 3.0, 6.0, 3.0, 1.0});
    auto model = PlottingModel{};
    auto const workspaces = workspaceSelections({"alignment_input"});

    auto const workspacesForPlotting =
        model.workspacesForPlotting(workspaces, PlotOutputOptions{PlotOutputType::Alignment});

    TS_ASSERT_EQUALS(workspacesForPlotting.size(), 1);
    TS_ASSERT_EQUALS(workspacesForPlotting[0], "__isis_reflectometry_alignment_0");
    auto group = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::WorkspaceGroup>(
        "__isis_reflectometry_alignment_0");
    TS_ASSERT_EQUALS(group->size(), 3);
    assertYValue("__isis_reflectometry_alignment_0_raw", 6.0, 2);
    assertXValue("__isis_reflectometry_alignment_0_raw", 102.0, 2);
    TS_ASSERT(Mantid::API::AnalysisDataService::Instance().doesExist("__isis_reflectometry_alignment_0_fitted_peak"));
    TS_ASSERT(Mantid::API::AnalysisDataService::Instance().doesExist("__isis_reflectometry_alignment_0_peak_centre"));
  }

  void testAlignmentCreatesOnePlotWorkspaceGroupPerSelectedWorkspace() {
    createAlignmentInputWorkspace("alignment_input_1", {1.0, 3.0, 6.0, 3.0, 1.0});
    createAlignmentInputWorkspace("alignment_input_2", {2.0, 5.0, 8.0, 5.0, 2.0});
    auto model = PlottingModel{};
    auto const workspaces = workspaceSelections({"alignment_input_1", "alignment_input_2"});

    auto const workspacesForPlotting =
        model.workspacesForPlotting(workspaces, PlotOutputOptions{PlotOutputType::Alignment});

    auto const expected =
        std::vector<std::string>{"__isis_reflectometry_alignment_0", "__isis_reflectometry_alignment_1"};
    TS_ASSERT_EQUALS(workspacesForPlotting, expected);
    TS_ASSERT(Mantid::API::AnalysisDataService::Instance().doesExist("__isis_reflectometry_alignment_0"));
    TS_ASSERT(Mantid::API::AnalysisDataService::Instance().doesExist("__isis_reflectometry_alignment_1"));
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

  void createAlignmentInputWorkspace(std::string const &name, std::vector<double> const &yValues) {
    auto workspace = Mantid::API::WorkspaceFactory::Instance().create("Workspace2D", yValues.size(), 2, 1);
    for (size_t workspaceIndex = 0; workspaceIndex < yValues.size(); ++workspaceIndex) {
      workspace->dataX(workspaceIndex)[0] = 0.0;
      workspace->dataX(workspaceIndex)[1] = 1.0;
      workspace->dataY(workspaceIndex)[0] = yValues[workspaceIndex];
      workspace->dataE(workspaceIndex)[0] = 1.0;
      workspace->getSpectrum(workspaceIndex).addDetectorID(static_cast<int>(100 + workspaceIndex));
    }
    Mantid::API::AnalysisDataService::Instance().addOrReplace(name, workspace);
  }

  void assertYValue(std::string const &workspaceName, double expected, size_t const workspaceIndex = 0) {
    auto workspace =
        Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>(workspaceName);
    TS_ASSERT_DELTA(workspace->y(0)[workspaceIndex], expected, 1e-12);
  }

  void assertXValue(std::string const &workspaceName, double expected, size_t const workspaceIndex) {
    auto workspace =
        Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>(workspaceName);
    TS_ASSERT_DELTA(workspace->x(0)[workspaceIndex], expected, 1e-12);
  }
};
