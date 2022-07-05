// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "../../../ISISReflectometry/GUI/Batch/GroupProcessingAlgorithm.h"
#include "../../../ISISReflectometry/Reduction/Batch.h"
#include "../../../ISISReflectometry/TestHelpers/ModelCreationHelper.h"

#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using namespace MantidQt::CustomInterfaces::ISISReflectometry::GroupProcessing;
using namespace MantidQt::CustomInterfaces::ISISReflectometry::ModelCreationHelper;
using MantidQt::API::IAlgorithmRuntimeProps;

namespace {
void assertProperty(IAlgorithmRuntimeProps const &props, std::string const &name, double expected) {
  TS_ASSERT_DELTA(static_cast<double>(props.getProperty(name)), expected, 1e-6);
}
} // namespace

class GroupProcessingAlgorithmTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GroupProcessingAlgorithmTest *createSuite() { return new GroupProcessingAlgorithmTest(); }
  static void destroySuite(GroupProcessingAlgorithmTest *suite) { delete suite; }

  GroupProcessingAlgorithmTest()
      : m_instruments{"INTER", "OFFSPEC", "POLREF", "SURF", "CRISP"}, m_thetaTolerance(0.01),
        m_experiment(makeExperiment()), m_instrument(makeInstrument()),
        m_runsTable(m_instruments, m_thetaTolerance, ReductionJobs()), m_slicing() {}

  void testThrowsIfInputWorkspaceGroupHasSingleRow() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto group = makeGroupWithOneRow();
    TS_ASSERT_THROWS_ANYTHING(createAlgorithmRuntimeProps(model, group));
  }

  void testInputWorkspaceListForTwoRowGroup() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto group = makeGroupWithTwoRows();
    auto result = createAlgorithmRuntimeProps(model, group);
    TS_ASSERT_EQUALS(result->getPropertyValue("InputWorkspaces"), "IvsQ_1, IvsQ_2");
  }

  void testInputWorkspaceListForRowsWithNonStandardNames() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto group = makeGroupWithTwoRowsWithNonstandardNames();
    auto result = createAlgorithmRuntimeProps(model, group);
    TS_ASSERT_EQUALS(result->getPropertyValue("InputWorkspaces"), "testQ1, testQ2");
  }

  void testOutputNameForTwoRowGroup() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto group = makeGroupWithTwoRows();
    auto result = createAlgorithmRuntimeProps(model, group);
    // The standard IvsQ_ prefix is removed from the individual names so it
    // only appears once at the beginning.
    TS_ASSERT_EQUALS(result->getPropertyValue("OutputWorkspace"), "IvsQ_1_2");
  }

  void testOutputNameForRowsWithNonStandardNames() {
    auto model = Batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    auto group = makeGroupWithTwoRowsWithNonstandardNames();
    auto result = createAlgorithmRuntimeProps(model, group);
    // The output is constructed from an IvsQ_ prefix and the original
    // output workspace names
    TS_ASSERT_EQUALS(result->getPropertyValue("OutputWorkspace"), "IvsQ_testQ1_testQ2");
  }

  void testStitchParamsSetFromStitchingOptions() {
    auto experiment =
        Experiment(AnalysisMode::PointDetector, ReductionType::Normal, SummationType::SumInLambda, false, false,
                   BackgroundSubtraction(), PolarizationCorrections(PolarizationCorrectionType::None),
                   FloodCorrections(FloodCorrectionType::Workspace), TransmissionStitchOptions(), makeStitchOptions(),
                   std::vector<LookupRow>());
    auto model = Batch(experiment, m_instrument, m_runsTable, m_slicing);
    auto group = makeGroupWithTwoRows();
    auto result = createAlgorithmRuntimeProps(model, group);
    TS_ASSERT_EQUALS(result->getPropertyValue("key1"), "value1");
    TS_ASSERT_EQUALS(result->getPropertyValue("key2"), "value2");
    TS_ASSERT(!result->existsProperty("Params"));
  }

  void testLookupRowQResolutionUsedForParamsIfStitchingOptionsEmpty() {
    auto lookupTable = makeLookupTableWithTwoAnglesAndWildcard();
    auto experiment =
        Experiment(AnalysisMode::PointDetector, ReductionType::Normal, SummationType::SumInLambda, false, false,
                   BackgroundSubtraction(), PolarizationCorrections(PolarizationCorrectionType::None),
                   FloodCorrections(FloodCorrectionType::Workspace), TransmissionStitchOptions(),
                   std::map<std::string, std::string>(), std::move(lookupTable));
    auto model = Batch(experiment, m_instrument, m_runsTable, m_slicing);
    auto group = makeGroupWithTwoRows();
    auto result = createAlgorithmRuntimeProps(model, group);
    assertProperty(*result, "Params", -0.01);
  }

  void testQResolutionForFirstValidRowUsedForParamsIfStitchingOptionsEmpty() {
    auto experiment =
        Experiment(AnalysisMode::PointDetector, ReductionType::Normal, SummationType::SumInLambda, false, false,
                   BackgroundSubtraction(), PolarizationCorrections(PolarizationCorrectionType::None),
                   FloodCorrections(FloodCorrectionType::Workspace), TransmissionStitchOptions(),
                   std::map<std::string, std::string>(), makeLookupTableWithTwoAnglesAndWildcard());
    auto model = Batch(experiment, m_instrument, m_runsTable, m_slicing);
    auto group = makeGroupWithTwoRowsWithMixedQResolutions();
    auto result = createAlgorithmRuntimeProps(model, group);
    assertProperty(*result, "Params", -0.015);
  }

  void testQOutputResolutionForFirstValidRowUsedForParamsIfStitchingOptionsEmpty() {
    auto experiment =
        Experiment(AnalysisMode::PointDetector, ReductionType::Normal, SummationType::SumInLambda, false, false,
                   BackgroundSubtraction(), PolarizationCorrections(PolarizationCorrectionType::None),
                   FloodCorrections(FloodCorrectionType::Workspace), TransmissionStitchOptions(),
                   std::map<std::string, std::string>(), makeLookupTableWithTwoAnglesAndWildcard());
    auto model = Batch(experiment, m_instrument, m_runsTable, m_slicing);
    auto group = makeGroupWithTwoRowsWithOutputQResolutions();
    auto result = createAlgorithmRuntimeProps(model, group);
    assertProperty(*result, "Params", -0.016);
  }

private:
  std::vector<std::string> m_instruments;
  double m_thetaTolerance;
  Experiment m_experiment;
  Instrument m_instrument;
  RunsTable m_runsTable;
  Slicing m_slicing;
};
