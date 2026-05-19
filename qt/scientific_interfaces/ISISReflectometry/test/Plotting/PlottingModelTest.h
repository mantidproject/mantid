// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Plotting/PlottingModel.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

#include <cmath>
#include <cxxtest/TestSuite.h>
#include <optional>

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
    auto const workspaces = workspaceSelections({"up", "down"}, "polarized_group");

    auto const workspacesForPlotting =
        model.workspacesForPlotting(workspaces, PlotOutputOptions{PlotOutputType::SpinAsymmetry});

    TS_ASSERT_EQUALS(workspacesForPlotting.size(), 1);
    TS_ASSERT_EQUALS(workspacesForPlotting[0], "__isis_refl_spin_asym_0");
    assertYValue("__isis_refl_spin_asym_0", 0.5);
    TS_ASSERT(Mantid::API::AnalysisDataService::Instance().doesExist("__isis_refl_spin_asym_0"));
  }

  void testSpinAsymmetryIgnoresWorkspacesWithoutWorkspaceGroupContext() {
    createConstantWorkspace("up", 6.0);
    createConstantWorkspace("down", 2.0);
    auto model = PlottingModel{};
    auto const workspaces = workspaceSelections({"up", "down"});

    auto const workspacesForPlotting =
        model.workspacesForPlotting(workspaces, PlotOutputOptions{PlotOutputType::SpinAsymmetry});

    TS_ASSERT(workspacesForPlotting.empty());
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
    TS_ASSERT_EQUALS(workspacesForPlotting[0], "__isis_refl_spin_asym_0");
    assertYValue("__isis_refl_spin_asym_0", 0.6);
  }

  void testSpinAsymmetryReturnsNoWorkspacesForUnsupportedSelectionSize() {
    createConstantWorkspace("only_one", 6.0);
    auto model = PlottingModel{};
    auto const workspaces = workspaceSelections({"only_one"}, "polarized_group");

    auto const workspacesForPlotting =
        model.workspacesForPlotting(workspaces, PlotOutputOptions{PlotOutputType::SpinAsymmetry});

    TS_ASSERT(workspacesForPlotting.empty());
  }

  void testAlignmentCreatesPlotWorkspaceGroupWithDetectorIndexXAxisForSelectedWorkspace() {
    createConstantWorkspace("IvsQ_12345", -100.0);
    createAlignmentInputWorkspace("raw_input_name", "12345", alignmentYValues(100, 7.0));
    createTOFGroup({"raw_input_name"});
    auto model = PlottingModel{};
    auto const workspaces = workspaceSelections({"IvsQ_12345"});

    auto const workspacesForPlotting = model.workspacesForPlotting(workspaces, alignmentOutputOptions());

    TS_ASSERT_EQUALS(workspacesForPlotting.size(), 1);
    TS_ASSERT_EQUALS(workspacesForPlotting[0], "__isis_refl_align_IvsQ_12345");
    auto group = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::WorkspaceGroup>(
        "__isis_refl_align_IvsQ_12345");
    TS_ASSERT_EQUALS(group->size(), 3);
    assertYValue("__isis_refl_align_IvsQ_12345_raw_sub_bg", 6.0, 96);
    assertXValue("__isis_refl_align_IvsQ_12345_raw_sub_bg", 96.0, 96);
    TS_ASSERT(Mantid::API::AnalysisDataService::Instance().doesExist("__isis_refl_align_IvsQ_12345_fitted_peak"));
    TS_ASSERT(Mantid::API::AnalysisDataService::Instance().doesExist("__isis_refl_align_IvsQ_12345_peak_centre"));
  }

  void testAlignmentCreatesOnePlotWorkspaceGroupPerSelectedWorkspace() {
    createConstantWorkspace("IvsQ_12345", -100.0);
    createConstantWorkspace("IvsQ_22345", -100.0);
    createAlignmentInputWorkspace("raw_input_name_1", "12345", alignmentYValues(100, 7.0));
    createAlignmentInputWorkspace("raw_input_name_2", "22345", alignmentYValues(110, 9.0));
    createTOFGroup({"raw_input_name_1", "raw_input_name_2"});
    auto model = PlottingModel{};
    auto const workspaces = std::vector<PlottingWorkspaceSelection>{workspaceSelection("IvsQ_12345", {"12345"}),
                                                                    workspaceSelection("IvsQ_22345", {"22345"})};

    auto const workspacesForPlotting = model.workspacesForPlotting(workspaces, alignmentOutputOptions());

    auto const expected = std::vector<std::string>{"__isis_refl_align_IvsQ_12345", "__isis_refl_align_IvsQ_22345"};
    TS_ASSERT_EQUALS(workspacesForPlotting, expected);
    TS_ASSERT(Mantid::API::AnalysisDataService::Instance().doesExist("__isis_refl_align_IvsQ_12345"));
    TS_ASSERT(Mantid::API::AnalysisDataService::Instance().doesExist("__isis_refl_align_IvsQ_22345"));
    TS_ASSERT(Mantid::API::AnalysisDataService::Instance().doesExist("__isis_refl_align_IvsQ_12345_raw_sub_bg"));
    TS_ASSERT(Mantid::API::AnalysisDataService::Instance().doesExist("__isis_refl_align_IvsQ_22345_raw_sub_bg"));
  }

  void testAlignmentDoesNotCreateWorkspaceForAddedRuns() {
    createConstantWorkspace("IvsQ_12345+12346", -100.0);
    createAlignmentInputWorkspace("summed_raw_input_name", "12345+12346", {1.0, 3.0, 6.0, 3.0, 1.0});
    createTOFGroup({"summed_raw_input_name"});
    auto model = PlottingModel{};
    auto const workspaces =
        std::vector<PlottingWorkspaceSelection>{workspaceSelection("IvsQ_12345+12346", {"12345", "12346"})};

    auto const workspacesForPlotting = model.workspacesForPlotting(workspaces, alignmentOutputOptions());

    TS_ASSERT(workspacesForPlotting.empty());
  }

  void testAlignmentFindsPeriodSpecificTOFWorkspaceInTOFGroup() {
    createConstantWorkspace("IvsQ_12345_2", -100.0);
    createAlignmentInputWorkspace("period_1_raw_input_name", "12345", alignmentYValues(100, 7.0), 1);
    createAlignmentInputWorkspace("period_2_raw_input_name", "12345", alignmentYValues(100, 10.0), 2);
    createTOFGroup({"period_1_raw_input_name", "period_2_raw_input_name"});
    auto model = PlottingModel{};
    auto const workspaces = std::vector<PlottingWorkspaceSelection>{workspaceSelection("IvsQ_12345_2", {"12345"}, 2)};

    auto const workspacesForPlotting = model.workspacesForPlotting(workspaces, alignmentOutputOptions());

    auto const expected = std::vector<std::string>{"__isis_refl_align_IvsQ_12345_2"};
    TS_ASSERT_EQUALS(workspacesForPlotting, expected);
    assertYValue("__isis_refl_align_IvsQ_12345_2_raw_sub_bg", 9.0, 96);
  }

  void testAlignmentThrowsForUnsupportedInstrument() {
    createConstantWorkspace("IvsQ_12345", -100.0);
    createAlignmentInputWorkspace("raw_input_name", "12345", alignmentYValues(100, 7.0));
    createTOFGroup({"raw_input_name"});
    auto model = PlottingModel{};
    auto outputOptions = PlotOutputOptions{PlotOutputType::Alignment};
    outputOptions.instrumentName = "UNKNOWN";

    TS_ASSERT_THROWS(model.workspacesForPlotting(workspaceSelections({"IvsQ_12345"}), outputOptions),
                     std::invalid_argument const &);
  }

  void testDetectorMapCreatesPlotWorkspaceFromSelectedWorkspacesRawTOFWorkspace() {
    createConstantWorkspace("IvsQ_12345", -100.0);
    createDetectorMapInputWorkspace("raw_input_name", "12345", 10.0);
    createTOFGroup({"raw_input_name"});
    auto model = PlottingModel{};
    auto const workspaces = workspaceSelections({"IvsQ_12345"});

    auto const workspacesForPlotting = model.workspacesForPlotting(workspaces, detectorMapOutputOptions());

    auto const expected = std::vector<std::string>{"__isis_refl_det_map_IvsQ_12345"};
    TS_ASSERT_EQUALS(workspacesForPlotting, expected);
    auto outputWorkspace = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>(
        "__isis_refl_det_map_IvsQ_12345");
    TS_ASSERT_EQUALS(outputWorkspace->getNumberHistograms(), 640);
    TS_ASSERT_DELTA(outputWorkspace->y(0)[0], 14.0, 1e-12);
    TS_ASSERT_DELTA((*(outputWorkspace->getAxis(1)))(0), 0.0, 1e-12);
  }

  void testDetectorMapCreatesOnePlotWorkspacePerSelectedWorkspace() {
    createConstantWorkspace("IvsQ_12345", -100.0);
    createConstantWorkspace("IvsQ_22345", -100.0);
    createDetectorMapInputWorkspace("raw_input_name_1", "12345", 10.0);
    createDetectorMapInputWorkspace("raw_input_name_2", "22345", 20.0);
    createTOFGroup({"raw_input_name_1", "raw_input_name_2"});
    auto model = PlottingModel{};
    auto const workspaces = std::vector<PlottingWorkspaceSelection>{workspaceSelection("IvsQ_12345", {"12345"}),
                                                                    workspaceSelection("IvsQ_22345", {"22345"})};

    auto const workspacesForPlotting = model.workspacesForPlotting(workspaces, detectorMapOutputOptions());

    auto const expected = std::vector<std::string>{"__isis_refl_det_map_IvsQ_12345", "__isis_refl_det_map_IvsQ_22345"};
    TS_ASSERT_EQUALS(workspacesForPlotting, expected);
    TS_ASSERT(Mantid::API::AnalysisDataService::Instance().doesExist("__isis_refl_det_map_IvsQ_12345"));
    TS_ASSERT(Mantid::API::AnalysisDataService::Instance().doesExist("__isis_refl_det_map_IvsQ_22345"));
  }

  void testDetectorMapFindsPeriodSpecificTOFWorkspaceInTOFGroup() {
    createConstantWorkspace("IvsQ_12345_2", -100.0);
    createDetectorMapInputWorkspace("period_1_raw_input_name", "12345", 10.0, 1);
    createDetectorMapInputWorkspace("period_2_raw_input_name", "12345", 20.0, 2);
    createTOFGroup({"period_1_raw_input_name", "period_2_raw_input_name"});
    auto model = PlottingModel{};
    auto const workspaces = std::vector<PlottingWorkspaceSelection>{workspaceSelection("IvsQ_12345_2", {"12345"}, 2)};

    auto const workspacesForPlotting = model.workspacesForPlotting(workspaces, detectorMapOutputOptions());

    auto const expected = std::vector<std::string>{"__isis_refl_det_map_IvsQ_12345_2"};
    TS_ASSERT_EQUALS(workspacesForPlotting, expected);
    auto outputWorkspace = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>(
        "__isis_refl_det_map_IvsQ_12345_2");
    TS_ASSERT_DELTA(outputWorkspace->y(0)[0], 24.0, 1e-12);
  }

  void testDetectorMapCanConvertXAxisToLambda() {
    createConstantWorkspace("IvsQ_12345", -100.0);
    createDetectorMapInputWorkspace("raw_input_name", "12345", 10.0);
    createTOFGroup({"raw_input_name"});
    auto model = PlottingModel{};

    auto const workspacesForPlotting =
        model.workspacesForPlotting(workspaceSelections({"IvsQ_12345"}),
                                    detectorMapOutputOptions(DetectorMapXAxis::Lambda, DetectorMapYAxis::DetectorId));

    TS_ASSERT_EQUALS(workspacesForPlotting.size(), 1);
    auto outputWorkspace = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>(
        "__isis_refl_det_map_IvsQ_12345");
    TS_ASSERT_EQUALS(outputWorkspace->getAxis(0)->unit()->unitID(), "Wavelength");
  }

  void testDetectorMapCanUseThetaYAxis() {
    createConstantWorkspace("IvsQ_12345", -100.0);
    createDetectorMapInputWorkspace("raw_input_name", "12345", 10.0);
    createTOFGroup({"raw_input_name"});
    auto model = PlottingModel{};

    model.workspacesForPlotting(workspaceSelections({"IvsQ_12345"}),
                                detectorMapOutputOptions(DetectorMapXAxis::TimeOfFlight, DetectorMapYAxis::Theta));

    auto outputWorkspace = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>(
        "__isis_refl_det_map_IvsQ_12345");
    TS_ASSERT_DIFFERS((*(outputWorkspace->getAxis(1)))(0), 0.0);
  }

  void testDetectorMapThrowsForUnsupportedInstrument() {
    createConstantWorkspace("IvsQ_12345", -100.0);
    createDetectorMapInputWorkspace("raw_input_name", "12345", 10.0);
    createTOFGroup({"raw_input_name"});
    auto model = PlottingModel{};
    auto outputOptions = detectorMapOutputOptions();
    outputOptions.instrumentName = "UNKNOWN";

    TS_ASSERT_THROWS(model.workspacesForPlotting(workspaceSelections({"IvsQ_12345"}), outputOptions),
                     std::invalid_argument const &);
  }

private:
  PlotOutputOptions alignmentOutputOptions() {
    auto outputOptions = PlotOutputOptions{PlotOutputType::Alignment};
    outputOptions.instrumentName = "POLREF";
    return outputOptions;
  }

  PlotOutputOptions detectorMapOutputOptions(DetectorMapXAxis xAxis = DetectorMapXAxis::TimeOfFlight,
                                             DetectorMapYAxis yAxis = DetectorMapYAxis::DetectorId) {
    auto outputOptions = PlotOutputOptions{PlotOutputType::DetectorMap, xAxis, yAxis};
    outputOptions.instrumentName = "POLREF";
    return outputOptions;
  }

  std::vector<double> alignmentYValues(size_t const peakWorkspaceIndex, double const peakHeight) {
    auto values = std::vector<double>(644, 1.0);
    auto const sigma = 4.0;
    for (size_t index = 0; index < values.size(); ++index) {
      auto const offset = static_cast<double>(index) - static_cast<double>(peakWorkspaceIndex);
      values[index] += (peakHeight - 1.0) * std::exp(-0.5 * offset * offset / (sigma * sigma));
    }
    return values;
  }

  PlottingWorkspaceSelection workspaceSelection(std::string workspaceName, std::vector<std::string> runNumbers,
                                                std::optional<int> period = std::nullopt,
                                                std::string const &workspaceGroupName = "") {
    return {std::move(workspaceName),
            PlottingWorkspaceOutputType::IvsQBinned,
            "Group 1",
            std::move(runNumbers),
            workspaceGroupName,
            period};
  }

  std::vector<PlottingWorkspaceSelection> workspaceSelections(std::vector<std::string> workspaceNames,
                                                              std::string const &workspaceGroupName = "") {
    auto selections = std::vector<PlottingWorkspaceSelection>{};
    selections.reserve(workspaceNames.size());
    for (auto const &workspaceName : workspaceNames) {
      selections.push_back(workspaceSelection(workspaceName, {"12345"}, std::nullopt, workspaceGroupName));
    }
    return selections;
  }

  void createConstantWorkspace(std::string const &name, double yValue) {
    Mantid::API::AnalysisDataService::Instance().addOrReplace(
        name, WorkspaceCreationHelper::create1DWorkspaceConstant(1, yValue, 0.0, false));
  }

  void createAlignmentInputWorkspace(std::string const &name, std::string const &runNumber,
                                     std::vector<double> const &yValues,
                                     std::optional<int> const currentPeriod = std::nullopt) {
    auto workspace = Mantid::API::WorkspaceFactory::Instance().create("Workspace2D", yValues.size(), 2, 1);
    workspace->mutableRun().addProperty("run_number", runNumber);
    if (currentPeriod) {
      workspace->mutableRun().addProperty("current_period", *currentPeriod);
    }
    for (size_t workspaceIndex = 0; workspaceIndex < yValues.size(); ++workspaceIndex) {
      workspace->dataX(workspaceIndex)[0] = 0.0;
      workspace->dataX(workspaceIndex)[1] = 1.0;
      workspace->dataY(workspaceIndex)[0] = yValues[workspaceIndex];
      workspace->dataE(workspaceIndex)[0] = 1.0;
      workspace->getSpectrum(workspaceIndex).addDetectorID(static_cast<int>(100 + workspaceIndex));
    }
    Mantid::API::AnalysisDataService::Instance().addOrReplace(name, workspace);
  }

  void createDetectorMapInputWorkspace(std::string const &name, std::string const &runNumber, double const yOffset,
                                       std::optional<int> const currentPeriod = std::nullopt) {
    auto workspace = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(644, 3, false, false, true, "POLREF");
    workspace->mutableRun().addProperty("run_number", runNumber);
    if (currentPeriod) {
      workspace->mutableRun().addProperty("current_period", *currentPeriod);
    }
    for (size_t workspaceIndex = 0; workspaceIndex < workspace->getNumberHistograms(); ++workspaceIndex) {
      for (size_t bin = 0; bin < workspace->y(workspaceIndex).size(); ++bin) {
        workspace->dataY(workspaceIndex)[bin] = yOffset + static_cast<double>(workspaceIndex);
        workspace->dataE(workspaceIndex)[bin] = 1.0;
      }
    }
    Mantid::API::AnalysisDataService::Instance().addOrReplace(name, workspace);
  }

  void createTOFGroup(std::vector<std::string> const &workspaceNames) {
    auto group = std::make_shared<Mantid::API::WorkspaceGroup>();
    for (auto const &workspaceName : workspaceNames) {
      group->addWorkspace(
          Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>(workspaceName));
    }
    Mantid::API::AnalysisDataService::Instance().addOrReplace("TOF", group);
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
