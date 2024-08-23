// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidQtWidgets/Spectroscopy/OutputWidget/OutputPlotOptionsModel.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/FallbackBoostOptionalIO.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidQtWidgets/Plotting/MockExternalPlotter.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace MantidQt::CustomInterfaces;
using namespace testing;

namespace {

std::string const GROUP_NAME = "GroupName";
std::string const WORKSPACE_NAME = "WorkspaceName";
std::string const WORKSPACE_INDICES = "0-2,4";

MatrixWorkspace_sptr convertWorkspace2DToMatrix(const Workspace2D_sptr &workspace) {
  return std::dynamic_pointer_cast<MatrixWorkspace>(workspace);
}

MatrixWorkspace_sptr createMatrixWorkspace(std::size_t const &numberOfHistograms, std::size_t const &numberOfBins) {
  return convertWorkspace2DToMatrix(WorkspaceCreationHelper::create2DWorkspace(numberOfHistograms, numberOfBins));
}

TableWorkspace_sptr createTableWorkspace(std::size_t const &size) { return std::make_shared<TableWorkspace>(size); }

void createWorkspaceGroup(std::vector<std::string> const &workspaceNames, std::size_t const &numberOfHistograms,
                          std::size_t const &numberOfBins) {
  auto const workspace = createMatrixWorkspace(numberOfHistograms, numberOfBins);
  for (auto const &workspaceName : workspaceNames)
    AnalysisDataService::Instance().addOrReplace(workspaceName, workspace->clone());

  const auto groupAlg = AlgorithmManager::Instance().create("GroupWorkspaces");
  groupAlg->setLogging(false);
  groupAlg->setProperty("InputWorkspaces", workspaceNames);
  groupAlg->setProperty("OutputWorkspace", GROUP_NAME);
  groupAlg->execute();
}

std::map<std::string, std::string>
constructActions(std::optional<std::map<std::string, std::string>> const &availableActions) {
  std::map<std::string, std::string> actions;
  if (availableActions)
    actions = availableActions.value();
  if (actions.find("Plot Spectra") == actions.end())
    actions["Plot Spectra"] = "Plot Spectra";
  if (actions.find("Plot Bins") == actions.end())
    actions["Plot Bins"] = "Plot Bins";
  if (actions.find("Open Slice Viewer") == actions.end())
    actions["Open Slice Viewer"] = "Open Slice Viewer";
  if (actions.find("Plot Tiled") == actions.end())
    actions["Plot Tiled"] = "Plot Tiled";
  if (actions.find("Plot 3D Surface") == actions.end())
    actions["Plot 3D Surface"] = "Plot 3D Surface";
  return actions;
}

std::map<std::string, std::string> customActions() {
  std::map<std::string, std::string> actions;
  actions["Plot Spectra"] = "Plot Wavelength";
  actions["Plot Bins"] = "Plot Angle";
  return actions;
}

} // namespace

class OutputPlotOptionsModelTest : public CxxTest::TestSuite {
public:
  OutputPlotOptionsModelTest() : m_ads(AnalysisDataService::Instance()) {
    FrameworkManager::Instance();
    m_ads.clear();
  }

  static OutputPlotOptionsModelTest *createSuite() { return new OutputPlotOptionsModelTest(); }

  static void destroySuite(OutputPlotOptionsModelTest *suite) { delete suite; }

  void setUp() override {
    auto plotter = std::make_unique<NiceMock<MockExternalPlotter>>();
    m_plotter = plotter.get();

    m_model = std::make_unique<OutputPlotOptionsModel>(std::move(plotter));
  }

  void tearDown() override {
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_plotter));

    m_model.reset();
    m_ads.clear();
  }

  void test_that_the_model_has_been_instantiated() {
    TS_ASSERT(m_plotter);
    TS_ASSERT(m_model);
  }

  void test_that_setWorkspace_will_set_the_workspace_if_the_matrix_workspace_provided_exists_in_the_ADS() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));

    TS_ASSERT(m_model->setWorkspace(WORKSPACE_NAME));
    TS_ASSERT(m_model->workspace());
    TS_ASSERT_EQUALS(m_model->workspace().value(), WORKSPACE_NAME);
  }

  void test_that_setWorkspace_will_not_set_the_workspace_if_the_workspace_provided_does_not_exist_in_the_ADS() {
    TS_ASSERT(!m_model->setWorkspace(WORKSPACE_NAME));
    TS_ASSERT(!m_model->workspace());
  }

  void
  test_that_setWorkspace_will_not_set_the_workspace_if_the_workspace_provided_exists_in_the_ADS_but_is_not_a_matrix_workspace() {
    m_ads.addOrReplace(WORKSPACE_NAME, createTableWorkspace(5));

    TS_ASSERT(!m_model->setWorkspace(WORKSPACE_NAME));
    TS_ASSERT(!m_model->workspace());
  }

  void test_that_removeWorkspace_will_remove_the_workspace_in_the_model() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));

    m_model->setWorkspace(WORKSPACE_NAME);
    m_model->removeWorkspace();

    TS_ASSERT(!m_model->workspace());
  }

  void test_that_setFixedIndices_will_set_the_indices_as_being_fixed() {
    m_model->setFixedIndices(WORKSPACE_INDICES);

    TS_ASSERT(m_model->indicesFixed());
    TS_ASSERT(m_model->indices());
    TS_ASSERT_EQUALS(m_model->indices().value(), WORKSPACE_INDICES);
  }

  void test_that_setFixedIndices_will_not_set_the_indices_as_being_fixed_if_the_indices_are_empty() {
    m_model->setFixedIndices("");

    TS_ASSERT(!m_model->indicesFixed());
    TS_ASSERT(!m_model->indices());
  }

  void test_that_formatIndices_will_format_a_range_of_workspace_indices_when_provided_as_a_comma_list() {
    auto const unformattedIndices("0,1,2,3,4");
    TS_ASSERT_EQUALS(m_model->formatIndices(unformattedIndices), "0-4");
  }

  void test_that_formatIndices_will_format_a_range_of_workspace_indices_when_provided_as_an_unordered_comma_list() {
    auto const unformattedIndices("4,2,0,3,1");
    TS_ASSERT_EQUALS(m_model->formatIndices(unformattedIndices), "0-4");
  }

  void test_that_formatIndices_will_format_a_workspace_indices_string_with_large_spaces() {
    auto const unformattedIndices("    1-   2,  4,3");
    TS_ASSERT_EQUALS(m_model->formatIndices(unformattedIndices), "1-4");
  }

  void test_that_formatIndices_will_format_random_workspace_indice_strings() {
    TS_ASSERT_EQUALS(m_model->formatIndices("10,11,0,7-9,1"), "0-1,7-11");
    TS_ASSERT_EQUALS(m_model->formatIndices(""), "");
    TS_ASSERT_EQUALS(m_model->formatIndices("9,12,3-8"), "3-9,12");
    TS_ASSERT_EQUALS(m_model->formatIndices("  5,6  ,  7,99"), "5-7,99");
    TS_ASSERT_EQUALS(m_model->formatIndices("0-1,2-3,4-5,9"), "0-5,9");
  }

  void test_that_validateIndices_will_return_true_if_the_matrix_workspace_and_workspace_indices_exist() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));

    m_model->setWorkspace(WORKSPACE_NAME);

    TS_ASSERT(m_model->validateIndices(WORKSPACE_INDICES, MantidAxis::Spectrum));
  }

  void test_that_validateIndices_will_return_true_if_the_matrix_workspace_and_bin_indices_exist() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));

    m_model->setWorkspace(WORKSPACE_NAME);

    TS_ASSERT(m_model->validateIndices(WORKSPACE_INDICES, MantidAxis::Bin));
  }

  void
  test_that_validateIndices_will_return_false_if_the_matrix_workspace_exists_but_the_workspace_indices_do_not_exist() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(2, 5));

    m_model->setWorkspace(WORKSPACE_NAME);

    TS_ASSERT(!m_model->validateIndices(WORKSPACE_INDICES, MantidAxis::Spectrum));
  }

  void test_that_validateIndices_will_return_false_if_the_matrix_workspace_exists_but_the_bin_indices_do_not_exist() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 2));

    m_model->setWorkspace(WORKSPACE_NAME);

    TS_ASSERT(!m_model->validateIndices(WORKSPACE_INDICES, MantidAxis::Bin));
  }

  void test_that_validateIndices_will_return_false_if_the_workspace_does_not_exist_in_the_ADS() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));

    m_model->setWorkspace(WORKSPACE_NAME);
    m_ads.clear();

    TS_ASSERT(!m_model->validateIndices(WORKSPACE_INDICES, MantidAxis::Spectrum));
  }

  void test_that_setIndices_will_set_the_indices_if_they_are_valid() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));
    (void)m_model->setWorkspace(WORKSPACE_NAME);

    TS_ASSERT(m_model->setIndices(WORKSPACE_INDICES));
    TS_ASSERT(m_model->indices());
    TS_ASSERT_EQUALS(m_model->indices().value(), WORKSPACE_INDICES);
  }

  void test_that_setIndices_will_not_set_the_indices_if_they_are_invalid() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(2, 5));
    (void)m_model->setWorkspace(WORKSPACE_NAME);

    TS_ASSERT(!m_model->setIndices(WORKSPACE_INDICES));
    TS_ASSERT(!m_model->indices());
  }

  void
  test_that_plotSpectra_will_call_the_plotter_plotSpectra_method_when_a_valid_workspace_and_indices_have_been_set() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));
    m_model->setWorkspace(WORKSPACE_NAME);
    m_model->setIndices(WORKSPACE_INDICES);

    EXPECT_CALL(*m_plotter, plotSpectra(WORKSPACE_NAME, WORKSPACE_INDICES, false)).Times(1);

    m_model->plotSpectra();
  }

  void test_that_plotBins_will_call_the_plotter_plotBins_method_when_a_valid_workspace_and_bin_indices_have_been_set() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));
    m_model->setWorkspace(WORKSPACE_NAME);

    EXPECT_CALL(*m_plotter, plotBins(WORKSPACE_NAME, WORKSPACE_INDICES, false)).Times(1);

    m_model->plotBins(WORKSPACE_INDICES);
  }

  void test_that_showSliceViewer_will_call_the_plotter_showSliceViewer_method_when_a_valid_workspace_has_been_set() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));
    m_model->setWorkspace(WORKSPACE_NAME);

    EXPECT_CALL(*m_plotter, showSliceViewer(WORKSPACE_NAME)).Times(1);

    m_model->showSliceViewer();
  }

  void test_that_plotTiled_will_call_the_plotter_plotTiled_method_when_a_valid_workspace_and_indices_have_been_set() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));
    m_model->setWorkspace(WORKSPACE_NAME);
    m_model->setIndices(WORKSPACE_INDICES);

    EXPECT_CALL(*m_plotter, plotTiled(WORKSPACE_NAME, WORKSPACE_INDICES, false)).Times(1);

    m_model->plotTiled();
  }

  void
  test_that_getAllWorkspaceNames_will_return_all_of_the_expected_workspace_names_when_provided_a_matrix_and_group_workspace() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));
    createWorkspaceGroup({"Workspace1", "Workspace2", "Workspace3"}, 5, 5);

    auto const allWorkspaces = m_model->getAllWorkspaceNames({GROUP_NAME, WORKSPACE_NAME});

    std::vector<std::string> const expectedWorkspaces{"Workspace1", "Workspace2", "Workspace3", WORKSPACE_NAME};
    TS_ASSERT_EQUALS(allWorkspaces, expectedWorkspaces);
  }

  void
  test_that_singleDataPoint_will_return_an_no_error_message_if_the_workspace_has_more_than_one_data_points_to_plot_for_spectrum() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));
    m_model->setWorkspace(WORKSPACE_NAME);

    TS_ASSERT(!m_model->singleDataPoint(MantidAxis::Spectrum));
  }

  void
  test_that_singleDataPoint_will_return_an_no_error_message_if_the_workspace_has_more_than_one_data_points_to_plot_for_bin() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));
    m_model->setWorkspace(WORKSPACE_NAME);

    TS_ASSERT(!m_model->singleDataPoint(MantidAxis::Bin));
  }

  void
  test_that_singleDataPoint_will_return_a_no_error_message_if_the_workspace_has_more_than_one_data_points_to_open_slice_viewer_or_3DSurface() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));
    m_model->setWorkspace(WORKSPACE_NAME);

    TS_ASSERT(!m_model->singleDataPoint(MantidAxis::Both));
  }

  void
  test_that_singleDataPoint_will_return_an_error_message_if_the_workspace_has_a_single_data_point_to_plot_for_spectrum() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 1));
    m_model->setWorkspace(WORKSPACE_NAME);

    TS_ASSERT(m_model->singleDataPoint(MantidAxis::Spectrum));
  }

  void
  test_that_singleDataPoint_will_return_an_error_message_if_the_workspace_has_a_single_data_point_to_plot_for_bin() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(1, 5));
    m_model->setWorkspace(WORKSPACE_NAME);

    TS_ASSERT(m_model->singleDataPoint(MantidAxis::Bin));
  }

  void
  test_that_singleDataPoint_will_return_an_error_message_if_the_workspace_has_one_histogram_to_open_slice_viewer_or_3DSurface() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(1, 5));
    m_model->setWorkspace(WORKSPACE_NAME);

    TS_ASSERT(m_model->singleDataPoint(MantidAxis::Both));
  }

  void
  test_that_singleDataPoint_will_return_an_error_message_if_the_workspace_has_one_bin_to_open_slice_viewer_or_3DSurface() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 1));
    m_model->setWorkspace(WORKSPACE_NAME);

    TS_ASSERT(m_model->singleDataPoint(MantidAxis::Both));
  }

  void
  test_that_singleDataPoint_will_return_an_error_message_if_the_workspace_has_one_bin_and_one_histogram_to_open_slice_viewer_or_3DSurface() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(1, 1));
    m_model->setWorkspace(WORKSPACE_NAME);

    TS_ASSERT(m_model->singleDataPoint(MantidAxis::Both));
  }

  void test_that_availableActions_will_return_the_default_actions_when_none_are_set() {
    TS_ASSERT_EQUALS(m_model->availableActions(), constructActions(std::nullopt));
  }

  void test_that_availableActions_will_return_the_correct_actions_when_they_have_been_set() {
    auto actions = customActions();

    tearDown();
    auto plotter = std::make_unique<NiceMock<MockExternalPlotter>>();
    m_model = std::make_unique<OutputPlotOptionsModel>(std::move(plotter), actions);

    actions["Open Slice Viewer"] = "Open Slice Viewer";
    actions["Plot 3D Surface"] = "Plot 3D Surface";
    actions["Plot Tiled"] = "Plot Tiled";
    TS_ASSERT_EQUALS(m_model->availableActions(), actions);
  }

private:
  AnalysisDataServiceImpl &m_ads;

  NiceMock<MockExternalPlotter> *m_plotter;
  std::unique_ptr<OutputPlotOptionsModel> m_model;
};
