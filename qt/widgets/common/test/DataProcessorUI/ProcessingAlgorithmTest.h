// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MANTIDWIDGETS_DATAPROCESSORPROCESSINGALGORITHMTEST_H
#define MANTID_MANTIDWIDGETS_DATAPROCESSORPROCESSINGALGORITHMTEST_H

#include <QStringList>
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidQtWidgets/Common/DataProcessorUI/ProcessingAlgorithm.h"

using namespace MantidQt::MantidWidgets;
using namespace MantidQt::MantidWidgets::DataProcessor;
using namespace Mantid::API;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class ProcessingAlgorithmTest : public CxxTest::TestSuite {

private:
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ProcessingAlgorithmTest *createSuite() {
    return new ProcessingAlgorithmTest();
  }
  static void destroySuite(ProcessingAlgorithmTest *suite) { delete suite; }
  ProcessingAlgorithmTest() { FrameworkManager::Instance(); };

  void test_valid_algorithms() {
    // Any algorithm with at least one input ws property and one output ws
    // property is valid
    // Currently ws must be either MatrixWorkspace or Workspace but this can be
    // changed
    std::vector<QString> prefix = {"run_"};
    TS_ASSERT_THROWS_NOTHING(ProcessingAlgorithm("Rebin", prefix, 0));
    TS_ASSERT_THROWS_NOTHING(ProcessingAlgorithm("ExtractSpectra", prefix, 0));
    TS_ASSERT_THROWS_NOTHING(ProcessingAlgorithm("ConvertUnits", prefix, 0));
  }

  void test_invalid_algorithms() {

    std::vector<QString> prefix = {"IvsQ_"};

    // Algorithms with no input workspace properties
    TS_ASSERT_THROWS(ProcessingAlgorithm("Stitch1DMany", prefix, 0),
                     const std::invalid_argument &);
    // Algorithms with no output workspace properties
    TS_ASSERT_THROWS(ProcessingAlgorithm("SaveAscii", prefix, 0),
                     const std::invalid_argument &);
  }
  void test_ReflectometryReductionOneAuto() {

    QString algName = "ReflectometryReductionOneAuto";

    // ReflectometryReductionOneAuto has three output ws properties
    // We should provide three prefixes, one for each ws
    std::vector<QString> prefixes;
    prefixes.emplace_back("IvsQ_binned_");
    prefixes.emplace_back("IvsQ_");
    // This should throw
    TS_ASSERT_THROWS(
        ProcessingAlgorithm(algName, prefixes, 0, std::set<QString>()),
        const std::invalid_argument &);

    // This should also throw
    TS_ASSERT_THROWS(
        ProcessingAlgorithm(algName, prefixes, 0, std::set<QString>()),
        const std::invalid_argument &);
    // But this should be OK
    prefixes.emplace_back("IvsLam_");
    TS_ASSERT_THROWS_NOTHING(
        ProcessingAlgorithm(algName, prefixes, 0, std::set<QString>()));

    auto const postprocessedOutputPrefixIndex = 1;
    auto alg = ProcessingAlgorithm(
        algName, prefixes, postprocessedOutputPrefixIndex, std::set<QString>());
    TS_ASSERT_EQUALS(alg.name(), "ReflectometryReductionOneAuto");
    TS_ASSERT_EQUALS(alg.numberOfOutputProperties(), 3);
    TS_ASSERT_EQUALS(alg.prefix(0), "IvsQ_binned_");
    TS_ASSERT_EQUALS(alg.prefix(1), "IvsQ_");
    TS_ASSERT_EQUALS(alg.prefix(2), "IvsLam_");
    TS_ASSERT_EQUALS(alg.postprocessedOutputPrefix(), "IvsQ_");
    TS_ASSERT_EQUALS(alg.inputPropertyName(0), "InputWorkspace");
    TS_ASSERT_EQUALS(alg.inputPropertyName(1), "FirstTransmissionRun");
    TS_ASSERT_EQUALS(alg.inputPropertyName(2), "SecondTransmissionRun");
    TS_ASSERT_EQUALS(alg.outputPropertyName(0), "OutputWorkspaceBinned");
    TS_ASSERT_EQUALS(alg.outputPropertyName(1), "OutputWorkspace");
    TS_ASSERT_EQUALS(alg.outputPropertyName(2), "OutputWorkspaceWavelength");
  }

  // Add more tests for specific algorithms here
};
#endif /* MANTID_MANTIDWIDGETS_DATAPROCESSORPROCESSINGALGORITHMTEST_H */
