#ifndef MANTID_ALGORITHMS_PROCESSINDIRECTFITPARAMETERSTEST_H_
#define MANTID_ALGORITHMS_PROCESSINDIRECTFITPARAMETERSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ProcessIndirectFitParameters.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"

using Mantid::Algorithms::ProcessIndirectFitParameters;
using namespace Mantid::API;

class ProcessIndirectFitParametersTest : public CxxTest::TestSuite {

private:
  ITableWorkspace_sptr createTable() {
    auto tableWs = WorkspaceFactory::Instance().createTable();
    tableWs->addColumn("double", "axis-1");
    tableWs->addColumn("double", "f0.A0");
    tableWs->addColumn("double", "f0.A0_Err");
    tableWs->addColumn("double", "f1.f1.f0.Height");
    tableWs->addColumn("double", "f1.f1.f0.Height_Err");
    tableWs->addColumn("double", "f1.f1.f0.Amplitude");
    tableWs->addColumn("double", "f1.f1.f0.Amplitude_Err");
    tableWs->addColumn("double", "f1.f1.f0.PeakCentre");
    tableWs->addColumn("double", "f1.f1.f0.PeakCentre_Err");

    size_t n = 5;
    for (size_t i = 0; i < n; ++i) {
      TableRow row = tableWs->appendRow();
      double ax1 = int(i) * 1.0;
      double a0 = 0.0;
      double a0e = 0.0;
      double h = int(i) * 1.02;
      double he = sqrt(h);
      double am = int(i) * 2.43;
      double ame = sqrt(am);
      double pc = -0.0567;
      double pce = sqrt(pc);
      row << ax1 << a0 << a0e << h << he << am << ame << pc << pce;
    }
    return tableWs;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ProcessIndirectFitParametersTest *createSuite() {
    return new ProcessIndirectFitParametersTest();
  }
  static void destroySuite(ProcessIndirectFitParametersTest *suite) {
    delete suite;
  }

  void test_empty_input_is_not_allowed() {
    Mantid::Algorithms::ProcessIndirectFitParameters alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("InputWorkspace", ""),
                     std::invalid_argument);
  }

  void test_empty_x_column_is_not_allowed() {
    Mantid::Algorithms::ProcessIndirectFitParameters alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("X Column", ""),
                     std::invalid_argument);
  }

  void test_that_empty_param_names_is_not_allowed() {
    Mantid::Algorithms::ProcessIndirectFitParameters alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("Parameter Names", ""),
                     std::invalid_argument);
  }

  void test_empty_output_is_not_allowed() {
    Mantid::Algorithms::ProcessIndirectFitParameters alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT_THROWS(alg.setPropertyValue("OutputWorkspace Name", ""),
                     std::invalid_argument);
  }

  void test_property_input() {
    auto tableWs = createTable();
    std::string xColumn = "axis-1";
    std::string parameterValues = "Amplitude";
    std::string outputName = "outMatrix";

    Mantid::Algorithms::ProcessIndirectFitParameters alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    alg.setProperty("InputWorkspace", tableWs);
    alg.setPropertyValue("X Column", xColumn);
    alg.setPropertyValue("Parameter Names", parameterValues);
    alg.setProperty("OutputWorkspace Name", outputName);

    ITableWorkspace_sptr tableProp = alg.getProperty("InputWorkspace");

    TS_ASSERT_EQUALS(tableProp, tableWs);
    TS_ASSERT_EQUALS(std::string(alg.getProperty("X Column")), xColumn);
    TS_ASSERT_EQUALS(std::string(alg.getProperty("Parameter Names")),
                     parameterValues);
    TS_ASSERT_EQUALS(std::string(alg.getProperty("OutputWorkspace Name")),
                     outputName);
  }

  void test_output() {
    auto tableWs = createTable();
    std::string xColumn = "axis-1";
    std::string parameterValues = "Height,Amplitude";
    std::string outputName = "outMatrix";

    Mantid::Algorithms::ProcessIndirectFitParameters alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    alg.setProperty("InputWorkspace", tableWs);
    alg.setPropertyValue("X Column", xColumn);
    alg.setPropertyValue("Parameter Names", parameterValues);
    alg.setProperty("OutputWorkspace Name", outputName);

    alg.execute();

    MatrixWorkspace_sptr outws;
    TS_ASSERT_THROWS_NOTHING(
        outws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputName));
  }
};

#endif /* MANTID_ALGORITHMS_PROCESSINDIRECTFITPARAMETERSTEST_H_ */