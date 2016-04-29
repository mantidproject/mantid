#ifndef MANTID_CUSTOMINTERFACES_DATAPROCESSORALGORITHMTEST_H
#define MANTID_CUSTOMINTERFACES_DATAPROCESSORALGORITHMTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidQtCustomInterfaces/Reflectometry/DataProcessorAlgorithm.h"

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class DataProcessorAlgorithmTest : public CxxTest::TestSuite {

private:
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DataProcessorAlgorithmTest *createSuite() {
    return new DataProcessorAlgorithmTest();
  }
  static void destroySuite(DataProcessorAlgorithmTest *suite) { delete suite; }
  DataProcessorAlgorithmTest() { FrameworkManager::Instance(); };

  void test_valid_algorithms() {
    // Any algorithm with at least one input ws property and one output ws
    // property is valid
    // Currently ws must be either MatrixWorkspace or Workspace but this can be
    // changed
    std::vector<std::string> prefix = {"run_"};
    TS_ASSERT_THROWS_NOTHING(DataProcessorAlgorithm("Rebin", prefix));
    TS_ASSERT_THROWS_NOTHING(DataProcessorAlgorithm("ExtractSpectra", prefix));
    TS_ASSERT_THROWS_NOTHING(DataProcessorAlgorithm("ConvertUnits", prefix));
  }

  void test_invalid_algorithms() {

    std::vector<std::string> prefix = {"IvsQ_"};

    // Algorithms with no input workspace properties
    TS_ASSERT_THROWS(DataProcessorAlgorithm("Stitch1DMany", prefix),
                     std::invalid_argument);
    // Algorithms with no output workspace properties
    TS_ASSERT_THROWS(DataProcessorAlgorithm("SaveAscii", prefix),
                     std::invalid_argument);
  }
  void test_ReflectometryReductionOneAuto() {

    std::string algName = "ReflectometryReductionOneAuto";

    // ReflectometryReductionOneAuto has two output ws properties
    // We should provide two prefixes, one for each ws
    std::vector<std::string> prefixes;
    prefixes.push_back("IvsQ_");
    // This should throw
    TS_ASSERT_THROWS(DataProcessorAlgorithm(algName, prefixes),
                     std::invalid_argument);
    // But this should be OK
    prefixes.push_back("IvsLam_");
    TS_ASSERT_THROWS_NOTHING(DataProcessorAlgorithm(algName, prefixes));

    auto alg = DataProcessorAlgorithm(algName, prefixes);
    TS_ASSERT_EQUALS(alg.name(), "ReflectometryReductionOneAuto");
    TS_ASSERT_EQUALS(alg.numberOfOutputProperties(), 2);
    TS_ASSERT_EQUALS(alg.prefix(0), "IvsQ_");
    TS_ASSERT_EQUALS(alg.prefix(1), "IvsLam_");
    TS_ASSERT_EQUALS(alg.inputPropertyName(0), "InputWorkspace");
    TS_ASSERT_EQUALS(alg.inputPropertyName(1), "FirstTransmissionRun");
    TS_ASSERT_EQUALS(alg.inputPropertyName(2), "SecondTransmissionRun");
    TS_ASSERT_EQUALS(alg.outputPropertyName(0), "OutputWorkspace");
    TS_ASSERT_EQUALS(alg.outputPropertyName(1), "OutputWorkspaceWavelength");
  }

  // Add more tests for specific algorithms here
};
#endif /* MANTID_CUSTOMINTERFACES_DATAPROCESSORALGORITHMTEST_H */
