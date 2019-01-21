// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_INDIRECTFITOPTIONSMODELTEST_H_
#define MANTIDQT_INDIRECTFITOPTIONSMODELTEST_H_

#include <cxxtest/TestSuite.h>

#include "IndirectFitOutputOptionsModel.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces::IDA;

namespace {

/// Note that the number of labels must be equal to the numberOfSpectra given to
/// a workspace
std::vector<std::string> getThreeAxisLabels() {
  return {"Amplitude", "HWHM", "PeakCentre"};
}

std::vector<SpectrumToPlot>
getExpectedAllSpectra(std::size_t const &numberOfWorkspaces,
                      std::size_t const &numberOfSpectra,
                      std::string const &workspaceName = "") {
  std::vector<SpectrumToPlot> spectraToPlot;
  for (auto i = 0u; i < numberOfWorkspaces; ++i)
    for (auto j = 0u; j < numberOfSpectra; ++j)
      spectraToPlot.emplace_back(std::make_pair(workspaceName, j));
  return spectraToPlot;
}

std::vector<SpectrumToPlot>
getExpectedParameterSpectra(std::size_t const &numberOfWorkspaces,
                            std::size_t const &index,
                            std::string const &workspaceName = "") {
  std::vector<SpectrumToPlot> spectraToPlot;
  for (auto i = 0u; i < numberOfWorkspaces; ++i)
    spectraToPlot.emplace_back(std::make_pair(workspaceName, index));
  return spectraToPlot;
}

} // namespace

class IndirectFitOutputOptionsModelTest : public CxxTest::TestSuite {
public:
  /// WorkflowAlgorithms do not appear in the FrameworkManager without this line
  IndirectFitOutputOptionsModelTest() { FrameworkManager::Instance(); }

  static IndirectFitOutputOptionsModelTest *createSuite() {
    return new IndirectFitOutputOptionsModelTest();
  }

  static void destroySuite(IndirectFitOutputOptionsModelTest *suite) {
    delete suite;
  }

  void setUp() override {
    m_model = std::make_unique<IndirectFitOutputOptionsModel>();
  }

  void tearDown() override { m_model.reset(); }

  void
  test_that_the_model_is_instantiated_without_stored_workspaces_or_spectraToPlot() {
    TS_ASSERT(!m_model->getResultWorkspace());
    TS_ASSERT(!m_model->getPDFWorkspace());
    TS_ASSERT(m_model->getSpectraToPlot().empty());
  }

  void test_that_setResultWorkspace_will_set_the_stored_result_group() {
    auto const resultGroup = createGroupWorkspace(2, 3);

    m_model->setResultWorkspace(resultGroup);

    TS_ASSERT_EQUALS(m_model->getResultWorkspace(), resultGroup);
  }

  void test_that_setPDFWorkspace_will_set_the_stored_PDF_group() {
    auto const pdfGroup = createGroupWorkspace(2, 3);

    m_model->setPDFWorkspace(pdfGroup);

    TS_ASSERT_EQUALS(m_model->getPDFWorkspace(), pdfGroup);
  }

  void test_that_removePDFWorkspace_will_remove_the_stored_PDF_workspace() {
    auto const pdfGroup = createGroupWorkspace(2, 3);

    m_model->setPDFWorkspace(pdfGroup);
    m_model->removePDFWorkspace();

    TS_ASSERT(!m_model->getPDFWorkspace());
  }

  void
  test_that_isResultGroupPlottable_returns_true_if_it_contains_a_workspace_with_more_than_one_data_point() {
    auto const resultGroup = createGroupWorkspace(2, 3, 10);

    m_model->setResultWorkspace(resultGroup);

    TS_ASSERT(m_model->isResultGroupPlottable());
  }

  void
  test_that_isResultGroupPlottable_returns_false_if_it_does_not_contain_a_workspace_with_more_than_one_data_point() {
    auto const resultGroup = createGroupWorkspace(2, 3, 1);

    m_model->setResultWorkspace(resultGroup);

    TS_ASSERT(!m_model->isResultGroupPlottable());
  }

  void
  test_that_isPDFGroupPlottable_returns_true_if_it_contains_a_workspace_with_more_than_one_data_point() {
    auto const pdfGroup = createGroupWorkspace(2, 3, 10);

    m_model->setPDFWorkspace(pdfGroup);

    TS_ASSERT(m_model->isPDFGroupPlottable());
  }

  void
  test_that_isPDFGroupPlottable_returns_false_if_it_does_not_contain_a_workspace_with_more_than_one_data_point() {
    auto const pdfGroup = createGroupWorkspace(2, 3, 1);

    m_model->setPDFWorkspace(pdfGroup);

    TS_ASSERT(!m_model->isPDFGroupPlottable());
  }

  void test_that_clearSpectraToPlot_will_remove_the_stored_spectraToPlot() {
    auto const resultGroup =
        createGroupWorkspaceWithTextAxes(2, getThreeAxisLabels(), 3);

    m_model->setResultWorkspace(resultGroup);
    m_model->plotResult("Amplitude");
    m_model->clearSpectraToPlot();

    TS_ASSERT(m_model->getSpectraToPlot().empty());
  }

  void
  test_that_getSpectraToPlot_will_return_an_empty_vector_if_none_of_the_workspaces_are_plottable() {
    auto const resultGroup =
        createGroupWorkspaceWithTextAxes(2, getThreeAxisLabels(), 3, 1);

    m_model->setResultWorkspace(resultGroup);
    m_model->plotResult("Amplitude");

    TS_ASSERT(m_model->getSpectraToPlot().empty());
  }

  void
  test_that_getSpectraToPlot_will_return_an_empty_vector_if_the_parameter_passed_does_not_exist() {
    auto const resultGroup =
        createGroupWorkspaceWithTextAxes(2, getThreeAxisLabels(), 3);

    m_model->setResultWorkspace(resultGroup);
    m_model->plotResult("Not a parameter");

    TS_ASSERT(m_model->getSpectraToPlot().empty());
  }

  void
  test_that_getSpectraToPlot_will_return_a_vector_with_the_correct_number_of_spectra_information_when_plotting_all() {
    auto const resultGroup =
        createGroupWorkspaceWithTextAxes(2, getThreeAxisLabels(), 3);

    m_model->setResultWorkspace(resultGroup);
    m_model->plotResult("All");

    /// Here the size should be equal to numberOfWorkspaces * numberOfSpectra as
    /// it plots all the spectra in each of the workspaces
    TS_ASSERT_EQUALS(m_model->getSpectraToPlot().size(), 6)
  }

  void
  test_that_getSpectraToPlot_will_return_a_vector_with_the_correct_number_of_spectra_information_when_plotting_a_parameter() {
    auto const resultGroup =
        createGroupWorkspaceWithTextAxes(2, getThreeAxisLabels(), 3);

    m_model->setResultWorkspace(resultGroup);
    m_model->plotResult("Amplitude");

    /// Here the size should be equal to the numberOfWorkspaces as it will be
    /// plotting one spectra from each workspace
    TS_ASSERT_EQUALS(m_model->getSpectraToPlot().size(), 2)
  }

  void
  test_that_getSpectraToPlot_will_return_a_vector_containing_the_correct_spectra_indices_when_plotting_all() {
    auto const resultGroup =
        createGroupWorkspaceWithTextAxes(2, getThreeAxisLabels(), 3);

    m_model->setResultWorkspace(resultGroup);
    m_model->plotResult("All");

    TS_ASSERT_EQUALS(m_model->getSpectraToPlot(), getExpectedAllSpectra(2, 3))
  }

  void
  test_that_getSpectraToPlot_will_return_a_vector_containing_the_correct_spectra_indices_when_plotting_a_parameter() {
    auto const resultGroup =
        createGroupWorkspaceWithTextAxes(2, getThreeAxisLabels(), 3);

    m_model->setResultWorkspace(resultGroup);
    m_model->plotResult("HWHM"); /// This parameter has a workspace index of 1

    TS_ASSERT_EQUALS(m_model->getSpectraToPlot(),
                     getExpectedParameterSpectra(2, 1))
  }

  void test_that_plotResult_will_throw_when_there_is_no_result_workspace_set() {
    TS_ASSERT_THROWS(m_model->plotResult("HWHM"), std::runtime_error);
  }

  void test_that_plotPDF_will_throw_when_there_is_no_pdf_workspace_set() {
    TS_ASSERT_THROWS(m_model->plotPDF("WorkspaceName", "HWHM"),
                     std::runtime_error);
  }

  void test_that_saveResult_will_throw_when_there_is_no_result_workspace_set() {
    TS_ASSERT_THROWS(m_model->saveResult(), std::runtime_error);
  }

  void
  test_that_getWorkspaceParameters_will_return_an_empty_vector_if_the_group_is_not_set() {
    TS_ASSERT(m_model->getWorkspaceParameters("Result Group").empty());
  }

  void
  test_that_getWorkspaceParameters_will_return_the_axis_labels_of_the_result_group() {
    auto const axisLabels = getThreeAxisLabels();
    auto const resultGroup = createGroupWorkspaceWithTextAxes(2, axisLabels, 3);

    m_model->setResultWorkspace(resultGroup);

    TS_ASSERT_EQUALS(m_model->getWorkspaceParameters("Result Group"),
                     axisLabels);
  }

  void
  test_that_getWorkspaceParameters_will_return_the_axis_labels_of_the_pdf_group() {
    auto const axisLabels = getThreeAxisLabels();
    auto const pdfGroup = createGroupWorkspaceWithTextAxes(2, axisLabels, 3);

    m_model->setPDFWorkspace(pdfGroup);

    TS_ASSERT_EQUALS(m_model->getWorkspaceParameters("PDF Group"), axisLabels);
  }

  void
  test_that_getPDFWorkspaceNames_will_return_an_empty_vector_if_the_pdf_group_is_not_set() {
    TS_ASSERT(m_model->getPDFWorkspaceNames().empty());
  }

  void
  test_that_getPDFWorkspaceNames_will_return_the_expected_workspace_names_when_the_pdf_group_is_set() {
    auto const pdfGroup =
        createGroupWorkspaceWithTextAxes(2, getThreeAxisLabels(), 3);

    m_model->setPDFWorkspace(pdfGroup);

    /// Note that the names are blank because the workspaces haven't been named
    for (auto const &name : m_model->getPDFWorkspaceNames())
      TS_ASSERT_EQUALS(name, "");
  }

  void test_test() {}

private:
  std::unique_ptr<IndirectFitOutputOptionsModel> m_model;
};

#endif
