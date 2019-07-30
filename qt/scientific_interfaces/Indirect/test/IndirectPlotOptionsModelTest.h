// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_INDIRECTPLOTOPTIONSMODELTEST_H_
#define MANTIDQT_INDIRECTPLOTOPTIONSMODELTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "IndirectPlotOptionsModel.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace MantidQt::CustomInterfaces;
using namespace testing;

namespace {

std::string const WORKSPACE_NAME = "WorkspaceName";
std::string const WORKSPACE_INDICES = "0-2,4";

MatrixWorkspace_sptr convertWorkspace2DToMatrix(Workspace2D_sptr workspace) {
  return boost::dynamic_pointer_cast<MatrixWorkspace>(workspace);
}

MatrixWorkspace_sptr
createMatrixWorkspace(std::size_t const &numberOfHistograms,
                      std::size_t const &numberOfBins) {
  return convertWorkspace2DToMatrix(WorkspaceCreationHelper::create2DWorkspace(
      numberOfHistograms, numberOfBins));
}

TableWorkspace_sptr createTableWorkspace(std::size_t const &size) {
  return boost::make_shared<TableWorkspace>(size);
}

/// Mock object to mock an IndirectTab
class ParentTab : public IndirectTab {
private:
  void setup() override{};
  void run() override{};
  bool validate() override { return true; };
};

} // namespace

GNU_DIAG_OFF_SUGGEST_OVERRIDE

/// Mock object to mock an IndirectTab
class MockIndirectPlotter : public IndirectPlotter {
public:
  /// Public Methods
  MOCK_METHOD2(plotSpectra, void(std::string const &workspaceName,
                                 std::string const &workspaceIndices));
  MOCK_METHOD2(plotBins, void(std::string const &workspaceName,
                              std::string const &binIndices));
  MOCK_METHOD1(plotContour, void(std::string const &workspaceName));
  MOCK_METHOD2(plotTiled, void(std::string const &workspaceName,
                               std::string const &workspaceIndices));
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

class IndirectPlotOptionsModelTest : public CxxTest::TestSuite {
public:
  IndirectPlotOptionsModelTest() : m_ads(AnalysisDataService::Instance()) {
    m_ads.clear();
  }

  static IndirectPlotOptionsModelTest *createSuite() {
    return new IndirectPlotOptionsModelTest();
  }

  static void destroySuite(IndirectPlotOptionsModelTest *suite) {
    delete suite;
  }

  void setUp() override {
    m_plotter = new NiceMock<MockIndirectPlotter>();
    m_model = std::make_unique<IndirectPlotOptionsModel>(m_plotter);
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

  void
  test_that_setWorkspace_will_set_the_workspace_if_the_matrix_workspace_provided_exists_in_the_ADS() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));

    TS_ASSERT(m_model->setWorkspace(WORKSPACE_NAME));
    TS_ASSERT(m_model->workspace());
    TS_ASSERT_EQUALS(m_model->workspace().get(), WORKSPACE_NAME);
  }

  void
  test_that_setWorkspace_will_not_set_the_workspace_if_the_workspace_provided_does_not_exist_in_the_ADS() {
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
    TS_ASSERT_EQUALS(m_model->indices().get(), WORKSPACE_INDICES);
  }

  void
  test_that_setFixedIndices_will_not_set_the_indices_as_being_fixed_if_the_indices_are_empty() {
    m_model->setFixedIndices("");

    TS_ASSERT(!m_model->indicesFixed());
    TS_ASSERT(!m_model->indices());
  }

  void
  test_that_formatIndices_will_format_a_range_of_workspace_indices_when_provided_as_a_comma_list() {
    auto const unformattedIndices("0,1,2,3,4");
    TS_ASSERT_EQUALS(m_model->formatIndices(unformattedIndices), "0-4");
  }

  void
  test_that_formatIndices_will_format_a_range_of_workspace_indices_when_provided_as_an_unordered_comma_list() {
    auto const unformattedIndices("4,2,0,3,1");
    TS_ASSERT_EQUALS(m_model->formatIndices(unformattedIndices), "0-4");
  }

  void
  test_that_formatIndices_will_format_a_workspace_indices_string_with_large_spaces() {
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

  void
  test_that_validateIndices_will_return_true_if_the_matrix_workspace_and_workspace_indices_exist() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));

    m_model->setWorkspace(WORKSPACE_NAME);

    TS_ASSERT(
        m_model->validateIndices(WORKSPACE_INDICES, MantidAxis::Spectrum));
  }

  void
  test_that_validateIndices_will_return_true_if_the_matrix_workspace_and_bin_indices_exist() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));

    m_model->setWorkspace(WORKSPACE_NAME);

    TS_ASSERT(m_model->validateIndices(WORKSPACE_INDICES, MantidAxis::Bin));
  }

  void
  test_that_validateIndices_will_return_false_if_the_matrix_workspace_exists_but_the_workspace_indices_do_not_exist() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(2, 5));

    m_model->setWorkspace(WORKSPACE_NAME);

    TS_ASSERT(
        !m_model->validateIndices(WORKSPACE_INDICES, MantidAxis::Spectrum));
  }

  void
  test_that_validateIndices_will_return_false_if_the_matrix_workspace_exists_but_the_bin_indices_do_not_exist() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 2));

    m_model->setWorkspace(WORKSPACE_NAME);

    TS_ASSERT(!m_model->validateIndices(WORKSPACE_INDICES, MantidAxis::Bin));
  }

  void
  test_that_validateIndices_will_return_false_if_the_workspace_does_not_exist_in_the_ADS() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));

    m_model->setWorkspace(WORKSPACE_NAME);
    m_ads.clear();

    TS_ASSERT(
        !m_model->validateIndices(WORKSPACE_INDICES, MantidAxis::Spectrum));
  }

  void test_that_setIndices_will_set_the_indices_if_they_are_valid() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));
    (void)m_model->setWorkspace(WORKSPACE_NAME);

    TS_ASSERT(m_model->setIndices(WORKSPACE_INDICES));
    TS_ASSERT(m_model->indices());
    TS_ASSERT_EQUALS(m_model->indices().get(), WORKSPACE_INDICES);
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

    EXPECT_CALL(*m_plotter, plotSpectra(WORKSPACE_NAME, WORKSPACE_INDICES))
        .Times(1);

    m_model->plotSpectra();
  }

  void
  test_that_plotBins_will_call_the_plotter_plotBins_method_when_a_valid_workspace_and_bin_indices_have_been_set() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));
    m_model->setWorkspace(WORKSPACE_NAME);
    m_model->setIndices(WORKSPACE_INDICES);

    EXPECT_CALL(*m_plotter, plotBins(WORKSPACE_NAME, WORKSPACE_INDICES))
        .Times(1);

    m_model->plotBins();
  }

  void
  test_that_plotContour_will_call_the_plotter_plotContour_method_when_a_valid_workspace_has_been_set() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));
    m_model->setWorkspace(WORKSPACE_NAME);

    EXPECT_CALL(*m_plotter, plotContour(WORKSPACE_NAME)).Times(1);

    m_model->plotContour();
  }

  void
  test_that_plotTiled_will_call_the_plotter_plotTiled_method_when_a_valid_workspace_and_indices_have_been_set() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));
    m_model->setWorkspace(WORKSPACE_NAME);
    m_model->setIndices(WORKSPACE_INDICES);

    EXPECT_CALL(*m_plotter, plotTiled(WORKSPACE_NAME, WORKSPACE_INDICES))
        .Times(1);

    m_model->plotTiled();
  }

private:
  AnalysisDataServiceImpl &m_ads;

  std::unique_ptr<IndirectPlotOptionsModel> m_model;
  MockIndirectPlotter *m_plotter;
};

#endif /* MANTIDQT_INDIRECTPLOTOPTIONSMODELTEST_H_ */
