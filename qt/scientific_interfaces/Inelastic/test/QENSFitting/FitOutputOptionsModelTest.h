// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "QENSFitting/FitOutputOptionsModel.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidFrameworkTestHelpers/IndirectFitDataCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces::Inelastic;

namespace {

std::size_t const NUMBER_OF_WORKSPACES(2);
int const NUMBER_OF_SPECTRA(3);

std::size_t toSizet(int value) { return static_cast<std::size_t>(value); }

std::vector<std::string> getThreeAxisLabels() { return {"Amplitude", "HWHM", "PeakCentre"}; }

std::vector<SpectrumToPlot> getExpectedAllSpectra(std::size_t const &numberOfWorkspaces,
                                                  std::size_t const &numberOfSpectra,
                                                  std::string const &workspaceName = "") {
  std::vector<SpectrumToPlot> spectraToPlot;
  spectraToPlot.reserve(numberOfWorkspaces * numberOfSpectra);
  for (auto i = 0u; i < numberOfWorkspaces; ++i)
    for (auto j = 0u; j < numberOfSpectra; ++j)
      spectraToPlot.emplace_back(std::make_pair(workspaceName, j));
  return spectraToPlot;
}

std::vector<SpectrumToPlot> getExpectedParameterSpectra(std::size_t const &numberOfWorkspaces, std::size_t const &index,
                                                        std::string const &workspaceName = "") {
  std::vector<SpectrumToPlot> spectraToPlot;
  spectraToPlot.reserve(numberOfWorkspaces);
  for (auto i = 0u; i < numberOfWorkspaces; ++i)
    spectraToPlot.emplace_back(std::make_pair(workspaceName, index));
  return spectraToPlot;
}

} // namespace

class FitOutputOptionsModelTest : public CxxTest::TestSuite {
public:
  static FitOutputOptionsModelTest *createSuite() { return new FitOutputOptionsModelTest(); }

  static void destroySuite(FitOutputOptionsModelTest *suite) { delete suite; }

  void setUp() override {
    m_ads = std::make_unique<SetUpADSWithWorkspace>("Name", createWorkspace(3, 4));

    m_groupWorkspace = createGroupWorkspaceWithTextAxes(NUMBER_OF_WORKSPACES, getThreeAxisLabels(), NUMBER_OF_SPECTRA);
    m_model = std::make_unique<FitOutputOptionsModel>();
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();

    m_groupWorkspace.reset();
    m_model.reset();
  }

  void test_that_the_model_is_instantiated_without_stored_workspaces() {
    TS_ASSERT(!m_model->getResultWorkspace());
    TS_ASSERT(!m_model->getPDFWorkspace());
  }

  void test_that_setResultWorkspace_will_set_the_stored_result_group() {
    m_model->setResultWorkspace(m_groupWorkspace);
    TS_ASSERT_EQUALS(m_model->getResultWorkspace(), m_groupWorkspace);
  }

  void test_that_setPDFWorkspace_will_set_the_stored_PDF_group() {
    m_model->setPDFWorkspace(m_groupWorkspace);
    TS_ASSERT_EQUALS(m_model->getPDFWorkspace(), m_groupWorkspace);
  }

  void test_that_removePDFWorkspace_will_remove_the_stored_PDF_workspace() {
    m_model->setPDFWorkspace(m_groupWorkspace);
    m_model->removePDFWorkspace();

    TS_ASSERT(!m_model->getPDFWorkspace());
  }

  void test_that_isSelectedGroupPlottable_returns_true_when_passed_the_result_group_string_with_a_result_group_set() {
    m_model->setResultWorkspace(m_groupWorkspace);
    TS_ASSERT(m_model->isSelectedGroupPlottable("Result Group"));
  }

  void test_that_isSelectedGroupPlottable_returns_false_when_passed_the_pdf_group_string_when_a_pdf_group_is_not_set() {
    TS_ASSERT(!m_model->isSelectedGroupPlottable("PDF Group"));
  }

  void test_that_isResultGroupPlottable_returns_true_if_it_contains_a_workspace_with_more_than_one_data_point() {
    m_model->setResultWorkspace(m_groupWorkspace);
    TS_ASSERT(m_model->isResultGroupPlottable());
  }

  void
  test_that_isResultGroupPlottable_returns_false_if_it_does_not_contain_a_workspace_with_more_than_one_data_point() {
    auto const resultGroup =
        createGroupWorkspaceWithTextAxes(NUMBER_OF_WORKSPACES, getThreeAxisLabels(), NUMBER_OF_SPECTRA, 1);

    m_model->setResultWorkspace(resultGroup);

    TS_ASSERT(!m_model->isResultGroupPlottable());
  }

  void test_that_isPDFGroupPlottable_returns_true_if_it_contains_a_workspace_with_more_than_one_data_point() {
    m_model->setPDFWorkspace(m_groupWorkspace);
    TS_ASSERT(m_model->isPDFGroupPlottable());
  }

  void test_that_isPDFGroupPlottable_returns_false_if_it_does_not_contain_a_workspace_with_more_than_one_data_point() {
    auto const pdfGroup =
        createGroupWorkspaceWithTextAxes(NUMBER_OF_WORKSPACES, getThreeAxisLabels(), NUMBER_OF_SPECTRA, 1);

    m_model->setPDFWorkspace(pdfGroup);

    TS_ASSERT(!m_model->isPDFGroupPlottable());
  }

  void test_that_plotResult_will_return_an_empty_vector_if_none_of_the_workspaces_are_plottable() {
    auto const resultGroup =
        createGroupWorkspaceWithTextAxes(NUMBER_OF_WORKSPACES, getThreeAxisLabels(), NUMBER_OF_SPECTRA, 1);

    m_model->setResultWorkspace(resultGroup);

    TS_ASSERT(m_model->plotResult("Amplitude").empty());
  }

  void test_that_plotResult_will_return_an_empty_vector_if_the_parameter_passed_does_not_exist() {
    m_model->setResultWorkspace(m_groupWorkspace);
    TS_ASSERT(m_model->plotResult("Not a parameter").empty());
  }

  void
  test_that_getSpectraToPlot_will_return_a_vector_with_the_correct_number_of_spectra_information_when_plotting_all() {
    m_model->setResultWorkspace(m_groupWorkspace);

    /// Here the size should be equal to numberOfWorkspaces * numberOfSpectra as
    /// it plots all the spectra in each of the workspaces
    auto const expectedSize = NUMBER_OF_WORKSPACES * toSizet(NUMBER_OF_SPECTRA);
    TS_ASSERT_EQUALS(m_model->plotResult("All").size(), expectedSize);
  }

  void
  test_that_getSpectraToPlot_will_return_a_vector_with_the_correct_number_of_spectra_information_when_plotting_a_parameter() {
    m_model->setResultWorkspace(m_groupWorkspace);

    /// Here the size should be equal to the numberOfWorkspaces as it will be
    /// plotting one spectra from each workspace
    TS_ASSERT_EQUALS(m_model->plotResult("Amplitude").size(), NUMBER_OF_WORKSPACES);
  }

  void test_that_getSpectraToPlot_will_return_a_vector_containing_the_correct_spectra_indices_when_plotting_all() {
    m_model->setResultWorkspace(m_groupWorkspace);

    TS_ASSERT_EQUALS(m_model->plotResult("All"), getExpectedAllSpectra(NUMBER_OF_WORKSPACES, NUMBER_OF_SPECTRA));
  }

  void
  test_that_getSpectraToPlot_will_return_a_vector_containing_the_correct_spectra_indices_when_plotting_a_parameter() {
    m_model->setResultWorkspace(m_groupWorkspace);

    auto const parameterIndex(1); /// This parameter has a workspace index of 1
    TS_ASSERT_EQUALS(m_model->plotResult("HWHM"), getExpectedParameterSpectra(NUMBER_OF_WORKSPACES, parameterIndex));
  }

  void test_that_plotResult_will_throw_when_there_is_no_result_workspace_set() {
    TS_ASSERT_THROWS(m_model->plotResult("HWHM"), const std::runtime_error &);
  }

  void test_that_plotPDF_will_throw_when_there_is_no_pdf_workspace_set() {
    TS_ASSERT_THROWS(m_model->plotPDF("WorkspaceName", "HWHM"), const std::runtime_error &);
  }

  void test_that_saveResult_will_throw_when_there_is_no_result_workspace_set() {
    TS_ASSERT_THROWS(m_model->saveResult(), const std::runtime_error &);
  }

  void test_that_getWorkspaceParameters_will_return_an_empty_vector_if_the_group_is_not_set() {
    TS_ASSERT(m_model->getWorkspaceParameters("Result Group").empty());
  }

  void test_that_getWorkspaceParameters_will_return_the_axis_labels_of_the_result_group() {
    m_model->setResultWorkspace(m_groupWorkspace);

    auto const axisLabels = getThreeAxisLabels();
    TS_ASSERT_EQUALS(m_model->getWorkspaceParameters("Result Group"), axisLabels);
  }

  void test_that_getWorkspaceParameters_will_return_the_axis_labels_of_the_pdf_group() {
    m_model->setPDFWorkspace(m_groupWorkspace);

    auto const axisLabels = getThreeAxisLabels();
    TS_ASSERT_EQUALS(m_model->getWorkspaceParameters("PDF Group"), axisLabels);
  }

  void test_that_getPDFWorkspaceNames_will_return_an_empty_vector_if_the_pdf_group_is_not_set() {
    TS_ASSERT(m_model->getPDFWorkspaceNames().empty());
  }

  void test_that_getPDFWorkspaceNames_will_return_the_expected_workspace_names_when_the_pdf_group_is_set() {
    m_model->setPDFWorkspace(m_groupWorkspace);

    /// Note that the names are blank because the workspaces haven't been named
    for (auto const &name : m_model->getPDFWorkspaceNames())
      TS_ASSERT_EQUALS(name, "");
  }

  void test_that_isResultGroupSelected_returns_true_when_passed_the_result_group_string() {
    TS_ASSERT(m_model->isResultGroupSelected("Result Group"));
  }

  void test_that_isResultGroupSelected_returns_false_when_passed_the_pdf_group_string() {
    TS_ASSERT(!m_model->isResultGroupSelected("PDF Group"));
  }

  void test_that_replaceFitResult_will_throw_when_provided_an_empty_inputWorkspace_name() {
    auto const singleBinName("Workspace_s0_Result");
    auto const outputName("Output_Result");

    TS_ASSERT_THROWS(m_model->replaceFitResult("", singleBinName, outputName), const std::runtime_error &);
  }

  void test_that_replaceFitResult_will_throw_when_provided_an_empty_singleBinWorkspace_name() {
    auto const inputName("Workspace_s0_to_s2_Result");
    auto const outputName("Output_Result");

    TS_ASSERT_THROWS(m_model->replaceFitResult(inputName, "", outputName), const std::runtime_error &);
  }

  void test_that_replaceFitResult_will_throw_when_provided_an_empty_outputWorkspace_name() {
    auto const inputName("Workspace_s0_to_s2_Result");
    auto const singleBinName("Workspace_s0_Result");

    TS_ASSERT_THROWS(m_model->replaceFitResult(inputName, singleBinName, ""), const std::runtime_error &);
  }

private:
  std::unique_ptr<SetUpADSWithWorkspace> m_ads;
  WorkspaceGroup_sptr m_groupWorkspace;
  std::unique_ptr<FitOutputOptionsModel> m_model;
};
