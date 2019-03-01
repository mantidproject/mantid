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
#include "MantidHistogramData/HistogramX.h"
#include "MantidHistogramData/HistogramY.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <algorithm>
#include <iterator>
#include <vector>

using namespace Mantid::API;
using namespace Mantid::HistogramData;
using namespace WorkspaceCreationHelper;

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
                               double const &xMin, double const &xMax,
                               int const &yInsertionIndex,
                               int const &xInsertionIndex,
                               std::string const &outputName) {
  auto copyAlg = AlgorithmManager::Instance().create("CopyDataRange");
  copyAlg->setProperty("InputWorkspace", inputWorkspace);
  copyAlg->setProperty("DestWorkspace", destWorkspace);
  copyAlg->setProperty("StartWorkspaceIndex", specMin);
  copyAlg->setProperty("EndWorkspaceIndex", specMax);
  copyAlg->setProperty("XMin", xMin);
  copyAlg->setProperty("XMax", xMax);
  copyAlg->setProperty("InsertionYIndex", yInsertionIndex);
  copyAlg->setProperty("InsertionXIndex", xInsertionIndex);
  copyAlg->setProperty("OutputWorkspace", outputName);
  return copyAlg;
}

IAlgorithm_sptr setUpAlgorithm(std::string const &inputName,
                               std::string const &destName, int const &specMin,
                               int const &specMax, double const &xMin,
                               double const &xMax, int const &yInsertionIndex,
                               int const &xInsertionIndex,
                               std::string const &outputName) {
  return setUpAlgorithm(getADSMatrixWorkspace(inputName),
                        getADSMatrixWorkspace(destName), specMin, specMax, xMin,
                        xMax, yInsertionIndex, xInsertionIndex, outputName);
}

void populateSpectrum(MatrixWorkspace_sptr workspace,
                      std::size_t const &spectrum,
                      std::vector<double> const &yData,
                      std::vector<double> const &xData,
                      std::vector<double> const &eData) {
  workspace->mutableY(spectrum) = HistogramY(yData);
  workspace->mutableX(spectrum) = HistogramX(xData);
  workspace->mutableE(spectrum) = HistogramE(eData);
}

void populateWorkspace(MatrixWorkspace_sptr workspace,
                       std::vector<double> const &yData,
                       std::vector<double> const &xData,
                       std::vector<double> const &eData) {
  for (auto index = 0u; index < workspace->getNumberHistograms(); ++index)
    populateSpectrum(workspace, index, yData, xData, eData);
}

std::vector<double>
constructHistogramData(Mantid::MantidVec::const_iterator fromIterator,
                       Mantid::MantidVec::const_iterator toIterator) {
  std::vector<double> histogram(toIterator - fromIterator);
  std::copy(fromIterator, toIterator, histogram.begin());
  return histogram;
}

void populateOutputWorkspace(MatrixWorkspace_sptr workspace,
                             std::vector<double> const &yData,
                             std::vector<double> const &eData) {
  std::vector<double> const xData = {2.1, 2.2, 2.3, 2.4, 2.5, 2.6};
  auto const numberOfBins = yData.size() / workspace->getNumberHistograms();
  for (auto i = 0u; i < workspace->getNumberHistograms(); ++i) {
    auto const startIndex = i * numberOfBins;
    auto const endIndex = (i + 1) * numberOfBins;
    populateSpectrum(workspace, i,
                     constructHistogramData(yData.begin() + startIndex,
                                            yData.begin() + endIndex),
                     xData,
                     constructHistogramData(eData.begin() + startIndex,
                                            eData.begin() + endIndex));
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

class CopyDataRangeTest : public CxxTest::TestSuite {
public:
  static CopyDataRangeTest *createSuite() { return new CopyDataRangeTest(); }

  static void destroySuite(CopyDataRangeTest *suite) { delete suite; }

  void setUp() override { setUpWorkspaces(INPUT_NAME, DESTINATION_NAME); }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void test_that_the_algorithm_does_not_throw_when_given_valid_properties() {
    auto algorithm = setUpAlgorithm(INPUT_NAME, DESTINATION_NAME, 0, 3, 2.1,
                                    2.4, 0, 0, OUTPUT_NAME);

    TS_ASSERT_THROWS_NOTHING(algorithm->execute());
  }

  void
  test_that_the_algorithm_produces_an_output_workspace_with_the_correct_data() {
    auto algorithm = setUpAlgorithm(INPUT_NAME, DESTINATION_NAME, 0, 3, 2.1,
                                    2.41, 0, 0, OUTPUT_NAME);

    algorithm->execute();

    auto const output = getADSMatrixWorkspace(OUTPUT_NAME);
    auto const expectedOutput = create2DWorkspace(5, 5);

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
    auto algorithm = setUpAlgorithm(INPUT_NAME, DESTINATION_NAME, 2, 3, 2.21,
                                    2.41, 2, 2, OUTPUT_NAME);

    algorithm->execute();

    auto const output = getADSMatrixWorkspace(OUTPUT_NAME);
    auto const expectedOutput = create2DWorkspace(5, 5);

    populateOutputWorkspace(
        expectedOutput, {25.0, 26.0, 27.0, 28.0, 29.0, 25.0, 26.0, 27.0, 28.0,
                         29.0, 25.0, 26.0, 1.2,  1.3,  1.4,  25.0, 26.0, 1.2,
                         1.3,  1.4,  25.0, 26.0, 27.0, 28.0, 29.0},
        {2.5, 2.6, 2.7, 2.8, 2.9, 2.5, 2.6, 2.7, 2.8, 2.9, 2.5, 2.6, 0.2,
         0.3, 0.4, 2.5, 2.6, 0.2, 0.3, 0.4, 2.5, 2.6, 2.7, 2.8, 2.9});
    TS_ASSERT(!compareWorkspaces(output, expectedOutput));
  }

  void
  test_that_the_algorithm_produces_an_output_workspace_with_the_correct_data_when_transfering_a_block_which_is_a_single_line() {
    auto algorithm = setUpAlgorithm(INPUT_NAME, DESTINATION_NAME, 2, 2, 2.11,
                                    2.51, 0, 0, OUTPUT_NAME);

    algorithm->execute();

    auto const output = getADSMatrixWorkspace(OUTPUT_NAME);
    auto const expectedOutput = create2DWorkspace(5, 5);
    populateOutputWorkspace(
        expectedOutput, {1.1,  1.2,  1.3,  1.4,  1.5,  25.0, 26.0, 27.0, 28.0,
                         29.0, 25.0, 26.0, 27.0, 28.0, 29.0, 25.0, 26.0, 27.0,
                         28.0, 29.0, 25.0, 26.0, 27.0, 28.0, 29.0},
        {0.1, 0.2, 0.3, 0.4, 0.5, 2.5, 2.6, 2.7, 2.8, 2.9, 2.5, 2.6, 2.7,
         2.8, 2.9, 2.5, 2.6, 2.7, 2.8, 2.9, 2.5, 2.6, 2.7, 2.8, 2.9});
    TS_ASSERT(!compareWorkspaces(output, expectedOutput));
  }

  void
  test_that_the_algorithm_changes_the_input_workspace_with_the_correct_data_when_the_output_and_destination_workspaces_are_the_same() {
    auto algorithm = setUpAlgorithm(INPUT_NAME, DESTINATION_NAME, 0, 3, 2.11,
                                    2.41, 0, 0, DESTINATION_NAME);

    algorithm->execute();

    auto const output = getADSMatrixWorkspace(DESTINATION_NAME);
    auto const expectedOutput = create2DWorkspace(5, 5);

    populateOutputWorkspace(
        expectedOutput,
        {1.1, 1.2,  1.3, 1.4, 29.0, 1.1, 1.2,  1.3,  1.4,  29.0, 1.1,  1.2, 1.3,
         1.4, 29.0, 1.1, 1.2, 1.3,  1.4, 29.0, 25.0, 26.0, 27.0, 28.0, 29.0},
        {0.1, 0.2, 0.3, 0.4, 2.9, 0.1, 0.2, 0.3, 0.4, 2.9, 0.1, 0.2, 0.3,
         0.4, 2.9, 0.1, 0.2, 0.3, 0.4, 2.9, 2.5, 2.6, 2.7, 2.8, 2.9});
    TS_ASSERT(!compareWorkspaces(output, expectedOutput));
  }

  void
  test_that_the_algorithm_throws_when_provided_a_StartWorkspaceIndex_which_is_larger_than_the_EndWorkspaceIndex() {
    auto algorithm = setUpAlgorithm(INPUT_NAME, DESTINATION_NAME, 2, 1, 2.1,
                                    2.4, 0, 0, OUTPUT_NAME);
    TS_ASSERT_THROWS(algorithm->execute(), std::runtime_error);
  }

  void
  test_that_the_algorithm_throws_when_provided_an_EndWorkspaceIndex_which_is_larger_than_the_number_of_histograms_in_the_input_workspace() {
    auto algorithm = setUpAlgorithm(INPUT_NAME, DESTINATION_NAME, 0, 5, 2.1,
                                    2.4, 0, 0, OUTPUT_NAME);
    TS_ASSERT_THROWS(algorithm->execute(), std::runtime_error);
  }

  void
  test_that_the_algorithm_throws_when_provided_a_xMin_which_comes_later_on_than_larger_than_xMax() {
    auto algorithm = setUpAlgorithm(INPUT_NAME, DESTINATION_NAME, 0, 3, 2.4,
                                    2.1, 0, 0, OUTPUT_NAME);
    TS_ASSERT_THROWS(algorithm->execute(), std::runtime_error);
  }

  void
  test_that_the_algorithm_throws_when_provided_a_block_of_data_which_will_not_fit_in_the_destination_workspace_in_the_y_direction() {
    auto algorithm = setUpAlgorithm(INPUT_NAME, DESTINATION_NAME, 0, 3, 2.1,
                                    2.4, 4, 0, OUTPUT_NAME);
    TS_ASSERT_THROWS(algorithm->execute(), std::runtime_error);
  }

  void
  test_that_the_algorithm_throws_when_provided_a_block_of_data_which_will_not_fit_in_the_destination_workspace_in_the_x_direction() {
    auto algorithm = setUpAlgorithm(INPUT_NAME, DESTINATION_NAME, 0, 3, 2.1,
                                    2.4, 0, 4, OUTPUT_NAME);
    TS_ASSERT_THROWS(algorithm->execute(), std::runtime_error);
  }

private:
  void setUpWorkspaces(
      std::string const &inputName, std::string const &destName,
      int inputNumberOfSpectra = 5, int destNumberOfSpectra = 5,
      int inputNumberOfBins = 5, int destNumberOfBins = 5,
      std::vector<double> const &inputYValues = {1.1, 1.2, 1.3, 1.4, 1.5},
      std::vector<double> const &inputXValues = {2.1, 2.2, 2.3, 2.4, 2.5, 2.6},
      std::vector<double> const &inputEValues = {0.1, 0.2, 0.3, 0.4, 0.5},
      std::vector<double> const &destYValues = {25.0, 26.0, 27.0, 28.0, 29.0},
      std::vector<double> const &destXValues = {2.1, 2.2, 2.3, 2.4, 2.5, 2.6},
      std::vector<double> const &destEValues = {2.5, 2.6, 2.7, 2.8, 2.9}) {
    auto const inputWorkspace =
        create2DWorkspace(inputNumberOfSpectra, inputNumberOfBins);
    auto const destWorkspace =
        create2DWorkspace(destNumberOfSpectra, destNumberOfBins);

    populateWorkspace(inputWorkspace, inputYValues, inputXValues, inputEValues);
    populateWorkspace(destWorkspace, destYValues, destXValues, destEValues);

    AnalysisDataService::Instance().addOrReplace(inputName, inputWorkspace);
    AnalysisDataService::Instance().addOrReplace(destName, destWorkspace);
  }
};

#endif /* MANTID_COPYDATARANGETEST_H_ */
