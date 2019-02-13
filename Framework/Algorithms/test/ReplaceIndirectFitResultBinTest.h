// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_COPYDATARANGETEST_H_
#define MANTID_COPYDATARANGETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidHistogramData/HistogramE.h"
#include "MantidHistogramData/HistogramY.h"
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"

#include <algorithm>

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace Mantid::HistogramData;

namespace {

std::string const INPUT_NAME("Input_Workspace");
std::string const DESTINATION_NAME("Destination_Workspace");
auto const OUTPUT_NAME("Output_Workspace");

MatrixWorkspace_sptr getADSMatrixWorkspace(std::string const &workspaceName) {
  return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      workspaceName);
}

IAlgorithm_sptr setUpAlgorithm(MatrixWorkspace_sptr inputWorkspace,
                               MatrixWorkspace_sptr destWorkspace,
                               int const &specMin, int const &specMax,
                               int const &xMin, int const &xMax,
                               int const &yInsertionIndex,
                               int const &xInsertionIndex,
                               std::string const &outputName) {
  auto copyAlg = AlgorithmManager::Instance().create("CopyDataRange");
  copyAlg->setProperty("InputWorkspace", inputWorkspace);
  copyAlg->setProperty("DestWorkspace", destWorkspace);
  copyAlg->setProperty("StartWorkspaceIndex", specMin);
  copyAlg->setProperty("EndWorkspaceIndex", specMax);
  copyAlg->setProperty("XMinIndex", xMin);
  copyAlg->setProperty("XMaxIndex", xMax);
  copyAlg->setProperty("InsertionYIndex", yInsertionIndex);
  copyAlg->setProperty("InsertionXIndex", xInsertionIndex);
  return copyAlg;
}

IAlgorithm_sptr setUpAlgorithm(std::string const &inputName,
                               std::string const &destName, int const &specMin,
                               int const &specMax, int const &xMin,
                               int const &xMax, int const &yInsertionIndex,
                               int const &xInsertionIndex,
                               std::string const &outputName) {
  return setUpAlgorithm(getADSMatrixWorkspace(inputName),
                        getADSMatrixWorkspace(destName), specMin, specMax, xMin,
                        xMax, yInsertionIndex, xInsertionIndex, outputName);
}

void populateWorkspace(MatrixWorkspace_sptr workspace,
                       std::vector<double> const &yData,
                       std::vector<double> const &eData) {
  for (auto i = 0u; i < workspace->getNumberHistograms(); ++i) {
    workspace->mutableY(i) = HistogramY(yData);
    workspace->mutableE(i) = HistogramE(eData);
  }
}

void populateOutputWorkspace(MatrixWorkspace_sptr workspace,
                             std::vector<double> const &yData,
                             std::vector<double> const &eData) {
  std::vector<double> histogram;
  auto const numberOfBins = yData.size() / workspace->getNumberHistograms();
  for (auto i = 0u; i < workspace->getNumberHistograms(); ++i) {
    std::copy(yData.begin() + i * numberOfBins,
              yData.begin() + (i + 1) * numberOfBins, histogram.begin());
    workspace->mutableY(i) = HistogramY(histogram);
    std::copy(eData.begin() + i * numberOfBins,
              eData.begin() + (i + 1) * numberOfBins, histogram.begin());
    workspace->mutableE(i) = HistogramE(histogram);
  }
}

ITableWorkspace_sptr compareWorkspaces(MatrixWorkspace_sptr workspace1,
                                       MatrixWorkspace_sptr workspace2,
                                       double tolerance = 0.000001) {
  auto compareAlg = AlgorithmManager::Instance().create("CompareWorkspaces");
  compareAlg->setProperty("Workspace1", workspace1);
  compareAlg->setProperty("Workspace2", workspace2);
  compareAlg->setProperty("Tolerance", tolerance);
  compareAlg->execute();
  return compareAlg->getProperty("Messages");
}

} // namespace

class ReplaceIndirectFitResultBinTest : public CxxTest::TestSuite {
public:
  /// WorkflowAlgorithms do not appear in the FrameworkManager without this line
  ReplaceIndirectFitResultBinTest() { FrameworkManager::Instance(); }

  static ReplaceIndirectFitResultBinTest *createSuite() {
    return new ReplaceIndirectFitResultBinTest();
  }

  static void destroySuite(ReplaceIndirectFitResultBinTest *suite) {
    delete suite;
  }

  void setUp() override {
    m_ads =
        std::make_unique<SetUpADSWithWorkspace>("Name", createWorkspace(3, 4));
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();
    m_ads.reset();
  }

  void test_that_the_algorithm_does_not_throw_when_given_valid_properties() {
    setUpWorkspaces(INPUT_NAME, DESTINATION_NAME);
    auto algorithm = setUpAlgorithm(INPUT_NAME, DESTINATION_NAME, 0, 3, 0, 3, 0,
                                    0, OUTPUT_NAME);

    TS_ASSERT_THROWS_NOTHING(algorithm->execute());
  }

  void
  test_that_the_algorithm_produces_an_output_workspace_with_the_correct_data() {
    setUpWorkspaces(INPUT_NAME, DESTINATION_NAME);
    auto algorithm = setUpAlgorithm(INPUT_NAME, DESTINATION_NAME, 0, 3, 0, 3, 0,
                                    0, OUTPUT_NAME);

    algorithm->execute();

    auto const output = getADSMatrixWorkspace(OUTPUT_NAME);
    auto const expectedOutput = createWorkspace(5, 5);
    populateOutputWorkspace(
        expectedOutput,
        {1.1, 1.2,  1.3, 1.4, 29.0, 1.1, 1.2,  1.3,  1.4,  29.0, 1.1,  1.2, 1.3,
         1.4, 29.0, 1.1, 1.2, 1.3,  1.4, 29.0, 25.0, 26.0, 27.0, 28.0, 29.0},
        {0.1, 0.2, 0.3, 0.4, 2.9, 0.1, 0.2, 0.3, 0.4, 2.9, 0.1, 0.2, 0.3,
         0.4, 2.9, 0.1, 0.2, 0.3, 0.4, 2.9, 2.5, 2.6, 2.7, 2.8, 2.9});
    TS_ASSERT(!compareWorkspaces(output, expectedOutput));
  }

  void
  test_that_the_algorithm_produces_an_output_workspace_with_the_correct_data_when_the_start_indices_are_not_zero() {
    setUpWorkspaces(INPUT_NAME, DESTINATION_NAME);
    auto algorithm = setUpAlgorithm(INPUT_NAME, DESTINATION_NAME, 2, 3, 1, 3, 2,
                                    2, OUTPUT_NAME);

    algorithm->execute();

    auto const output = getADSMatrixWorkspace(OUTPUT_NAME);
    auto const expectedOutput = createWorkspace(5, 5);
    //populateOutputWorkspace(
    //    expectedOutput,
    //    {2.5, 2.6,  2.7, 2.8, 2.9, 2.5, 2.6,  2.7,  2.8,  2.9,  2.5,  1.2, 1.3,
    //     1.4, 29.0, 2.5, 1.2, 1.3, 1.4, 29.0, 25.0, 26.0, 27.0, 28.0, 29.0},
    //    {0.1, 0.2, 0.3, 0.4, 2.9, 0.1, 0.2, 0.3, 0.4, 2.9, 0.1, 0.2, 0.3,
    //     0.4, 2.9, 0.1, 0.2, 0.3, 0.4, 2.9, 2.5, 2.6, 2.7, 2.8, 2.9});
    //populateWorkspace(expectedOutput, {25.0, 26.0, 1.3, 1.4, 29.0},
    //                  {0.1, 0.2, 0.3, 0.4, 2.9});
    TS_ASSERT(!compareWorkspaces(output, expectedOutput));
  }

  void
  test_that_the_algorithm_throws_when_provided_a_singleBinWorkspace_with_more_than_one_bin() {
    setUpWorkspaces(INPUT_NAME, DESTINATION_NAME, 3, 3, {2.0, 3.0, 4.0},
                    {1.1, 1.2, 1.3}, {0.1, 0.2, 0.3}, {3.0, 4.0}, {25.0, 26.0},
                    {2.5, 2.6});
    auto algorithm = setUpAlgorithm(INPUT_NAME, DESTINATION_NAME, OUTPUT_NAME);

    TS_ASSERT_THROWS(algorithm->execute(), std::runtime_error);
  }

  void
  test_that_the_algorithm_throws_when_provided_an_inputWorkspace_with_only_one_bin() {
    setUpWorkspaces(INPUT_NAME, DESTINATION_NAME, 3, 3, {3.0}, {1.2}, {0.2},
                    {3.0}, {25.0}, {2.5});
    auto algorithm = setUpAlgorithm(INPUT_NAME, DESTINATION_NAME, OUTPUT_NAME);

    TS_ASSERT_THROWS(algorithm->execute(), std::runtime_error);
  }

  void
  test_that_the_algorithm_throws_when_provided_two_workspaces_with_a_different_number_of_histograms() {
    setUpWorkspaces(INPUT_NAME, DESTINATION_NAME, 3, 2, {2.0, 3.0, 4.0},
                    {1.1, 1.2, 1.3}, {0.1, 0.2, 0.3}, {3.0}, {25.0}, {2.5});
    auto algorithm = setUpAlgorithm(INPUT_NAME, DESTINATION_NAME, OUTPUT_NAME);

    TS_ASSERT_THROWS(algorithm->execute(), std::runtime_error);
  }

  void
  test_that_the_algorithm_throws_when_provided_a_singleBinWorkspace_with_a_name_not_ending_with_Result() {
    setUpWorkspaces(INPUT_NAME, "Wrong_Name", 3, 3, {2.0, 3.0, 4.0},
                    {1.1, 1.2, 1.3}, {0.1, 0.2, 0.3}, {3.0}, {25.0}, {2.5});
    auto algorithm = setUpAlgorithm(INPUT_NAME, "Wrong_Name", OUTPUT_NAME);

    TS_ASSERT_THROWS(algorithm->execute(), std::runtime_error);
  }

  void
  test_that_the_algorithm_throws_when_provided_an_inputWorkspace_with_a_name_not_ending_with_Result() {
    setUpWorkspaces("Wrong_Name", DESTINATION_NAME, 3, 3, {2.0, 3.0, 4.0},
                    {1.1, 1.2, 1.3}, {0.1, 0.2, 0.3}, {3.0}, {25.0}, {2.5});
    auto algorithm =
        setUpAlgorithm("Wrong_Name", DESTINATION_NAME, OUTPUT_NAME);

    TS_ASSERT_THROWS(algorithm->execute(), std::runtime_error);
  }

  void
  test_that_the_algorithm_throws_when_provided_an_empty_string_for_the_output_workspace_namessasdasdasdas() {
    setUpWorkspaces(INPUT_NAME, DESTINATION_NAME, 3, 3, {2.0, 3.0, 4.0},
                    {1.1, 1.2, 1.3}, {0.1, 0.2, 0.3}, {1000.0}, {25.0}, {2.5});
    auto algorithm = setUpAlgorithm(INPUT_NAME, DESTINATION_NAME, OUTPUT_NAME);

    TS_ASSERT_THROWS(algorithm->execute(), std::runtime_error);
  }

private:
  void setUpWorkspaces(
      std::string const &inputName, std::string const &destName,
      int inputNumberOfSpectra = 5, int destNumberOfSpectra = 5,
      int inputNumberOfBins = 5, int destNumberOfBins = 5,
      std::vector<double> const &inputYValues = {1.1, 1.2, 1.3, 1.4, 1.5},
      std::vector<double> const &inputEValues = {0.1, 0.2, 0.3, 0.4, 0.5},
      std::vector<double> const &destYValues = {25.0, 26.0, 27.0, 28.0, 29.0},
      std::vector<double> const &destEValues = {2.5, 2.6, 2.7, 2.8, 2.9}) {
    auto const inputWorkspace =
        createWorkspace(inputNumberOfSpectra, inputNumberOfBins);
    auto const destWorkspace =
        createWorkspace(destNumberOfSpectra, destNumberOfBins);

    populateWorkspace(inputWorkspace, inputYValues, inputEValues);
    populateWorkspace(destWorkspace, destYValues, destEValues);

    m_ads->addOrReplace(inputName, inputWorkspace);
    m_ads->addOrReplace(destName, destWorkspace);
  }

  std::unique_ptr<SetUpADSWithWorkspace> m_ads;
};

#endif /* MANTID_COPYDATARANGETEST_H_ */
