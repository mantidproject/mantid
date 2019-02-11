// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_REPLACEINDIRECTFITRESULTBINTEST_H_
#define MANTID_REPLACEINDIRECTFITRESULTBINTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidHistogramData/HistogramE.h"
#include "MantidHistogramData/HistogramY.h"
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace Mantid::HistogramData;

namespace {

std::string const INPUT_NAME("Workspace_s0_to_s2_Result");
std::string const SINGLE_BIN_NAME("Workspace_s0_Result");
auto const OUTPUT_NAME("Output_Result");

MatrixWorkspace_sptr getADSMatrixWorkspace(std::string const &workspaceName) {
  return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      workspaceName);
}

WorkspaceGroup_sptr getADSGroupWorkspace(std::string const &workspaceName) {
  return AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
      workspaceName);
}

IAlgorithm_sptr setUpReplaceAlgorithm(MatrixWorkspace_sptr inputWorkspace,
                                      MatrixWorkspace_sptr singleBinWorkspace,
                                      std::string const &outputName) {
  auto replaceAlg =
      AlgorithmManager::Instance().create("ReplaceIndirectFitResultBin");
  replaceAlg->setProperty("InputWorkspace", inputWorkspace);
  replaceAlg->setProperty("SingleBinWorkspace", singleBinWorkspace);
  replaceAlg->setProperty("OutputWorkspace", outputName);
  return replaceAlg;
}

IAlgorithm_sptr setUpReplaceAlgorithm(std::string const &inputName,
                                      std::string const &singleBinName,
                                      std::string const &outputName) {
  return setUpReplaceAlgorithm(getADSMatrixWorkspace(inputName),
                               getADSMatrixWorkspace(singleBinName),
                               outputName);
}

void populateWorkspace(MatrixWorkspace_sptr workspace,
                       std::vector<double> const &yData,
                       std::vector<double> const &eData) {
  for (auto i = 0u; i < workspace->getNumberHistograms(); ++i) {
    workspace->mutableY(i) = HistogramY(yData);
    workspace->mutableE(i) = HistogramE(eData);
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
    setUpResultWorkspaces(INPUT_NAME, SINGLE_BIN_NAME);
    auto algorithm =
        setUpReplaceAlgorithm(INPUT_NAME, SINGLE_BIN_NAME, OUTPUT_NAME);

    TS_ASSERT_THROWS_NOTHING(algorithm->execute());
  }

  void
  test_that_the_algorithm_produces_an_output_workspace_with_the_correct_data() {
    setUpResultWorkspaces(INPUT_NAME, SINGLE_BIN_NAME);
    auto algorithm =
        setUpReplaceAlgorithm(INPUT_NAME, SINGLE_BIN_NAME, OUTPUT_NAME);

    algorithm->execute();

    auto const output = getADSMatrixWorkspace(OUTPUT_NAME);
    auto const expectedOutput =
        createWorkspaceWithBinValues(3, {2.0, 3.0, 4.0}, 3);
    populateWorkspace(expectedOutput, {1.1, 25.0, 1.3}, {0.1, 2.5, 0.3});
    TS_ASSERT(!compareWorkspaces(output, expectedOutput));
  }

  void
  test_that_the_algorithm_produces_an_output_workspace_which_is_put_into_a_group_with_the_correct_number_of_workspaces() {
    setUpResultWorkspaces(INPUT_NAME, SINGLE_BIN_NAME);
    auto algorithm =
        setUpReplaceAlgorithm(INPUT_NAME, SINGLE_BIN_NAME, OUTPUT_NAME);

    algorithm->execute();

    assertIsInGroupWithEntries(OUTPUT_NAME, 2);
  }

  void
  test_that_the_algorithm_produces_an_output_workspace_which_is_put_into_a_group_with_the_correct_number_of_workspaces_when_the_inputName_and_outputName_are_the_same() {
    setUpResultWorkspaces(INPUT_NAME, SINGLE_BIN_NAME);
    auto algorithm =
        setUpReplaceAlgorithm(INPUT_NAME, SINGLE_BIN_NAME, INPUT_NAME);

    algorithm->execute();

    assertIsInGroupWithEntries(INPUT_NAME, 1);
  }

  void
  test_that_the_algorithm_throws_when_provided_a_singleBinWorkspace_with_more_than_one_bin() {
    setUpResultWorkspaces(INPUT_NAME, SINGLE_BIN_NAME, 3, 3, {2.0, 3.0, 4.0},
                          {1.1, 1.2, 1.3}, {0.1, 0.2, 0.3}, {3.0, 4.0},
                          {25.0, 26.0}, {2.5, 2.6});
    auto algorithm =
        setUpReplaceAlgorithm(INPUT_NAME, SINGLE_BIN_NAME, OUTPUT_NAME);

    TS_ASSERT_THROWS(algorithm->execute(), std::runtime_error);
  }

  void
  test_that_the_algorithm_throws_when_provided_an_inputWorkspace_with_only_one_bin() {
    setUpResultWorkspaces(INPUT_NAME, SINGLE_BIN_NAME, 3, 3, {3.0}, {1.2},
                          {0.2}, {3.0}, {25.0}, {2.5});
    auto algorithm =
        setUpReplaceAlgorithm(INPUT_NAME, SINGLE_BIN_NAME, OUTPUT_NAME);

    TS_ASSERT_THROWS(algorithm->execute(), std::runtime_error);
  }

  void
  test_that_the_algorithm_throws_when_provided_a_two_workspaces_with_different_a_numbers_of_histograms() {
    setUpResultWorkspaces(INPUT_NAME, SINGLE_BIN_NAME, 3, 2, {2.0, 3.0, 4.0},
                          {1.1, 1.2, 1.3}, {0.1, 0.2, 0.3}, {3.0}, {25.0},
                          {2.5});
    auto algorithm =
        setUpReplaceAlgorithm(INPUT_NAME, SINGLE_BIN_NAME, OUTPUT_NAME);

    TS_ASSERT_THROWS(algorithm->execute(), std::runtime_error);
  }

  void
  test_that_the_algorithm_throws_when_provided_a_singleBinWorkspace_with_a_name_not_ending_with_Result() {
    setUpResultWorkspaces(INPUT_NAME, "Wrong_Name", 3, 3, {2.0, 3.0, 4.0},
                          {1.1, 1.2, 1.3}, {0.1, 0.2, 0.3}, {3.0}, {25.0},
                          {2.5});
    auto algorithm =
        setUpReplaceAlgorithm(INPUT_NAME, "Wrong_Name", OUTPUT_NAME);

    TS_ASSERT_THROWS(algorithm->execute(), std::runtime_error);
  }

  void
  test_that_the_algorithm_throws_when_provided_an_inputWorkspace_with_a_name_not_ending_with_Result() {
    setUpResultWorkspaces("Wrong_Name", SINGLE_BIN_NAME, 3, 3, {2.0, 3.0, 4.0},
                          {1.1, 1.2, 1.3}, {0.1, 0.2, 0.3}, {3.0}, {25.0},
                          {2.5});
    auto algorithm =
        setUpReplaceAlgorithm("Wrong_Name", SINGLE_BIN_NAME, OUTPUT_NAME);

    TS_ASSERT_THROWS(algorithm->execute(), std::runtime_error);
  }

  void
  test_that_the_algorithm_throws_when_provided_an_empty_string_for_the_output_workspace_namessasdasdasdas() {
    setUpResultWorkspaces(INPUT_NAME, SINGLE_BIN_NAME, 3, 3, {2.0, 3.0, 4.0},
                          {1.1, 1.2, 1.3}, {0.1, 0.2, 0.3}, {1000.0}, {25.0},
                          {2.5});
    auto algorithm =
        setUpReplaceAlgorithm(INPUT_NAME, SINGLE_BIN_NAME, OUTPUT_NAME);

    TS_ASSERT_THROWS(algorithm->execute(), std::runtime_error);
  }

private:
  void setUpResultWorkspaces(
      std::string const &inputName, std::string const &singleBinName,
      int inputNumberOfSpectra = 3, int singleNumberOfSpectra = 3,
      std::vector<double> const &inputBins = {2.0, 3.0, 4.0},
      std::vector<double> const &inputYValues = {1.1, 1.2, 1.3},
      std::vector<double> const &inputEValues = {0.1, 0.2, 0.3},
      std::vector<double> const &singleBin = {3.0},
      std::vector<double> const &singleYValue = {25.0},
      std::vector<double> const &singleEValue = {2.5}) {
    auto const inputWorkspace = createWorkspaceWithBinValues(
        inputNumberOfSpectra, inputBins, static_cast<int>(inputBins.size()));
    auto const singleBinWorkspace = createWorkspaceWithBinValues(
        singleNumberOfSpectra, singleBin, static_cast<int>(singleBin.size()));

    populateWorkspace(inputWorkspace, inputYValues, inputEValues);
    populateWorkspace(singleBinWorkspace, singleYValue, singleEValue);

    createSingleWorkspaceGroup(inputName, inputWorkspace);
    createSingleWorkspaceGroup(singleBinName, singleBinWorkspace);
  }

  void createSingleWorkspaceGroup(std::string const &workspaceName,
                                  MatrixWorkspace_sptr const &workspace) {
    m_ads->addOrReplace(workspaceName, workspace);

    auto group = boost::make_shared<WorkspaceGroup>();
    group->addWorkspace(workspace);
    m_ads->addOrReplace(workspaceName + "s", group);
  }

  void assertIsInGroupWithEntries(std::string const &outputName,
                                  int numberOfEntries) {
    auto const group = getADSGroupWorkspace(INPUT_NAME + "s");
    auto const output = getADSMatrixWorkspace(outputName);

    TS_ASSERT(group);
    TS_ASSERT(group->contains(output));
    TS_ASSERT_EQUALS(group->getNumberOfEntries(), numberOfEntries);
  }

  std::unique_ptr<SetUpADSWithWorkspace> m_ads;
};

#endif /* MANTID_REPLACEINDIRECTFITRESULTBINTEST_H_ */
