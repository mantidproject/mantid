// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_GROUPPROCESSINGALGORITHMTEST_H_
#define MANTID_CUSTOMINTERFACES_GROUPPROCESSINGALGORITHMTEST_H_
#include "../../../ISISReflectometry/Common/ModelCreationHelper.h"
#include "../../../ISISReflectometry/GUI/Batch/GroupProcessingAlgorithm.h"
#include "../../../ISISReflectometry/Reduction/Batch.h"

#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces;
using namespace MantidQt::CustomInterfaces::ModelCreationHelper;

class GroupProcessingAlgorithmTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GroupProcessingAlgorithmTest *createSuite() {
    return new GroupProcessingAlgorithmTest();
  }
  static void destroySuite(GroupProcessingAlgorithmTest *suite) {
    delete suite;
  }

  GroupProcessingAlgorithmTest()
      : m_instruments{"INTER", "OFFSPEC", "POLREF", "SURF", "CRISP"},
        m_thetaTolerance(0.01), m_experiment(makeExperiment()),
        m_instrument(makeInstrument()),
        m_runsTable(m_instruments, m_thetaTolerance, ReductionJobs()),
        m_slicing() {}

  void testThrowsIfInputWorkspaceGroupHasSingleRow() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto group = makeGroupWithOneRow();
    TS_ASSERT_THROWS_ANYTHING(createAlgorithmRuntimeProps(model, group));
  }

  void testInputWorkspaceListForTwoRowGroup() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto group = makeGroupWithTwoRows();
    auto result = createAlgorithmRuntimeProps(model, group);
    TS_ASSERT_EQUALS(result["InputWorkspaces"], "IvsQ_1, IvsQ_2");
  }

  void testInputWorkspaceListForRowsWithNonStandardNames() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto group = makeGroupWithTwoRowsWithNonstandardNames();
    auto result = createAlgorithmRuntimeProps(model, group);
    TS_ASSERT_EQUALS(result["InputWorkspaces"], "testQ1, testQ2");
  }

  void testOutputNameForTwoRowGroup() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto group = makeGroupWithTwoRows();
    auto result = createAlgorithmRuntimeProps(model, group);
    // The standard IvsQ_ prefix is removed from the individual names so it
    // only appears once at the beginning.
    TS_ASSERT_EQUALS(result["OutputWorkspace"], "IvsQ_1_2");
  }

  void testOutputNameForRowsWithNonStandardNames() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto group = makeGroupWithTwoRowsWithNonstandardNames();
    auto result = createAlgorithmRuntimeProps(model, group);
    // The output is constructed from an IvsQ_ prefix and the original
    // output workspace names
    TS_ASSERT_EQUALS(result["OutputWorkspace"], "IvsQ_testQ1_testQ2");
  }

  void testStitchParamsSetFromStitchingOptions() {
    auto experiment = Experiment(
        AnalysisMode::PointDetector, ReductionType::Normal,
        SummationType::SumInLambda, false, false,
        PolarizationCorrections(PolarizationCorrectionType::None),
        FloodCorrections(FloodCorrectionType::Workspace), boost::none,
        makeStitchOptions(), std::vector<PerThetaDefaults>());
    auto model = Batch(experiment, m_instrument, m_runsTable, m_slicing);
    auto group = makeGroupWithTwoRows();
    auto result = createAlgorithmRuntimeProps(model, group);
    TS_ASSERT_EQUALS(result["key1"], "value1");
    TS_ASSERT_EQUALS(result["key2"], "value2");
    TS_ASSERT_EQUALS(result.find("Params"), result.cend());
  }

  void testPerThetaDefaultsQResolutionUsedForParamsIfStitchingOptionsEmpty() {
    auto experiment =
        Experiment(AnalysisMode::PointDetector, ReductionType::Normal,
                   SummationType::SumInLambda, false, false,
                   PolarizationCorrections(PolarizationCorrectionType::None),
                   FloodCorrections(FloodCorrectionType::Workspace),
                   boost::none, std::map<std::string, std::string>(),
                   makePerThetaDefaultsWithTwoAnglesAndWildcard());
    auto model = Batch(experiment, m_instrument, m_runsTable, m_slicing);
    auto group = makeGroupWithTwoRows();
    auto result = createAlgorithmRuntimeProps(model, group);
    TS_ASSERT_EQUALS(result["Params"], "-0.010000");
  }

  void testQResolutionForFirstValidRowUsedForParamsIfStitchingOptionsEmpty() {
    auto experiment =
        Experiment(AnalysisMode::PointDetector, ReductionType::Normal,
                   SummationType::SumInLambda, false, false,
                   PolarizationCorrections(PolarizationCorrectionType::None),
                   FloodCorrections(FloodCorrectionType::Workspace),
                   boost::none, std::map<std::string, std::string>(),
                   makePerThetaDefaultsWithTwoAnglesAndWildcard());
    auto model = Batch(experiment, m_instrument, m_runsTable, m_slicing);
    auto group = makeGroupWithTwoRowsWithMixedQResolutions();
    auto result = createAlgorithmRuntimeProps(model, group);
    TS_ASSERT_EQUALS(result["Params"], "-0.015000");
  }

  void
  testQOutputResolutionForFirstValidRowUsedForParamsIfStitchingOptionsEmpty() {
    auto experiment =
        Experiment(AnalysisMode::PointDetector, ReductionType::Normal,
                   SummationType::SumInLambda, false, false,
                   PolarizationCorrections(PolarizationCorrectionType::None),
                   FloodCorrections(FloodCorrectionType::Workspace),
                   boost::none, std::map<std::string, std::string>(),
                   makePerThetaDefaultsWithTwoAnglesAndWildcard());
    auto model = Batch(experiment, m_instrument, m_runsTable, m_slicing);
    auto group = makeGroupWithTwoRowsWithOutputQResolutions();
    auto result = createAlgorithmRuntimeProps(model, group);
    TS_ASSERT_EQUALS(result["Params"], "-0.016000");
  }

private:
  std::vector<std::string> m_instruments;
  double m_thetaTolerance;
  Experiment m_experiment;
  Instrument m_instrument;
  RunsTable m_runsTable;
  Slicing m_slicing;
};
#endif // MANTID_CUSTOMINTERFACES_GROUPPROCESSINGALGORITHMTEST_H_
