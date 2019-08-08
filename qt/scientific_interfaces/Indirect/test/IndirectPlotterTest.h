// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_INDIRECTPLOTTERTEST_H_
#define MANTIDQT_INDIRECTPLOTTERTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "IndirectPlotter.h"

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

} // namespace

GNU_DIAG_OFF_SUGGEST_OVERRIDE

/// Mock object to mock an IndirectTab
class MockIPyRunner : public IPyRunner {
public:
  /// Public Methods
  MOCK_METHOD1(runPythonCode, void(std::string const &pythonCode));
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

class IndirectPlotterTest : public CxxTest::TestSuite {
public:
  IndirectPlotterTest() : m_ads(AnalysisDataService::Instance()) {
    m_ads.clear();
  }

  static IndirectPlotterTest *createSuite() {
    return new IndirectPlotterTest();
  }

  static void destroySuite(IndirectPlotterTest *suite) { delete suite; }

  void setUp() override {
    m_pyRunner = std::make_unique<MockIPyRunner>();
    m_plotter = std::make_unique<IndirectPlotter>(m_pyRunner.get());
  }

  void tearDown() override {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_pyRunner));

    m_plotter.reset();
    m_ads.clear();
  }

  void test_that_the_plotter_has_been_instantiated() {
    TS_ASSERT(m_pyRunner);
    TS_ASSERT(m_plotter);
  }

  void
  test_that_plotSpectra_will_attempt_to_run_python_code_using_the_IPyRunner() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));

    EXPECT_CALL(*m_pyRunner, runPythonCode(_)).Times(1);

    m_plotter->plotSpectra(WORKSPACE_NAME, WORKSPACE_INDICES);
  }

  void
  test_that_plotBins_will_attempt_to_run_python_code_using_the_IPyRunner() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));

    EXPECT_CALL(*m_pyRunner, runPythonCode(_)).Times(1);

    m_plotter->plotBins(WORKSPACE_NAME, WORKSPACE_INDICES);
  }

  void
  test_that_validate_will_return_true_if_the_matrix_workspace_and_workspace_indices_exist() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));

    TS_ASSERT(m_plotter->validate(WORKSPACE_NAME, WORKSPACE_INDICES,
                                  MantidAxis::Spectrum));
  }

  void
  test_that_validate_will_return_true_if_the_matrix_workspace_and_bin_indices_exist() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));

    TS_ASSERT(m_plotter->validate(WORKSPACE_NAME, WORKSPACE_INDICES,
                                  MantidAxis::Bin));
  }

  void
  test_that_validate_will_return_false_if_the_matrix_workspace_exists_but_the_workspace_indices_do_not_exist() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(2, 5));

    TS_ASSERT(!m_plotter->validate(WORKSPACE_NAME, WORKSPACE_INDICES,
                                   MantidAxis::Spectrum));
  }

  void
  test_that_validate_will_return_false_if_the_matrix_workspace_exists_but_the_bin_indices_do_not_exist() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 2));

    TS_ASSERT(!m_plotter->validate(WORKSPACE_NAME, WORKSPACE_INDICES,
                                   MantidAxis::Bin));
  }

  void
  test_that_validate_will_return_false_if_the_workspace_does_not_exist_in_the_ADS() {
    TS_ASSERT(!m_plotter->validate(WORKSPACE_NAME, WORKSPACE_INDICES,
                                   MantidAxis::Spectrum));
  }

  void
  test_that_validate_will_return_false_if_the_workspace_is_not_a_matrix_workspace() {
    m_ads.addOrReplace(WORKSPACE_NAME, createTableWorkspace(5));

    TS_ASSERT(!m_plotter->validate(WORKSPACE_NAME, WORKSPACE_INDICES,
                                   MantidAxis::Spectrum));
  }

  void
  test_that_validate_will_return_true_if_the_matrix_workspace_exists_but_no_indices_are_provided() {
    m_ads.addOrReplace(WORKSPACE_NAME, createMatrixWorkspace(5, 5));

    TS_ASSERT(m_plotter->validate(WORKSPACE_NAME));
  }

  void
  test_that_validate_will_return_false_if_the_workspace_is_not_a_matrix_and_no_indices_are_provided() {
    m_ads.addOrReplace(WORKSPACE_NAME, createTableWorkspace(5));

    TS_ASSERT(!m_plotter->validate(WORKSPACE_NAME));
  }

private:
  AnalysisDataServiceImpl &m_ads;

  std::unique_ptr<MockIPyRunner> m_pyRunner;
  std::unique_ptr<IndirectPlotter> m_plotter;
};

#endif /* MANTIDQT_INDIRECTPLOTTERTEST_H_ */
