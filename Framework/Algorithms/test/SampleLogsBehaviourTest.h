// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_SAMPLELOGSBEHAVIOURTEST_H_
#define MANTID_ALGORITHMS_SAMPLELOGSBEHAVIOURTEST_H_

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/RunCombinationHelpers/SampleLogsBehaviour.h"
#include "MantidDataHandling/LoadParameterFile.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

#include "MantidKernel/TimeSeriesProperty.h"

using Mantid::Algorithms::SampleLogsBehaviour;
using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::Kernel;
using namespace WorkspaceCreationHelper;

class SampleLogsBehaviourTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SampleLogsBehaviourTest *createSuite() {
    return new SampleLogsBehaviourTest();
  }
  static void destroySuite(SampleLogsBehaviourTest *suite) { delete suite; }

  // Please note that many tests are currently present in MergeRunsTest.

  void testMergeRunsIPFNames() {
    // Using default values of the constructor
    Logger log("testLog");
    auto ws = createWorkspace(2.6, 1.5, 8.5);
    auto base = createWorkspace(4.5, 3.2, 7.9);
    testUnitsTestWorkspace(ws);
    testUnitsTestWorkspace(base);
    SampleLogsBehaviour sbh = SampleLogsBehaviour(base, log);
    TS_ASSERT_THROWS_NOTHING(sbh.mergeSampleLogs(ws, base));
    const std::string A = base->run().getLogData("A")->value();
    const std::string B = base->run().getLogData("B")->value();
    const std::string C = base->run().getLogData("C")->value();
    // A listed and B summed according to IPF
    TS_ASSERT_EQUALS(A, "4.5, 2.6000000000000001")
    TS_ASSERT_EQUALS(B, "4.7000000000000002")
    TS_ASSERT_EQUALS(C, "7.9000000000000004")
    testUnitsTestWorkspace(base);
  }

  void testMergeRunsUserNames() {
    // Using default values of the constructor
    Logger log("testLog");
    auto ws = createWorkspace(2.6, 1.5, 8.5);
    auto base = createWorkspace(4.5, 3.2, 7.9);
    testUnitsTestWorkspace(ws);
    testUnitsTestWorkspace(base);
    SampleLogsBehaviour::SampleLogNames sampleLogNames;
    sampleLogNames.sampleLogsSum = "A";
    SampleLogsBehaviour sbh = SampleLogsBehaviour(base, log, sampleLogNames);
    TS_ASSERT_THROWS_NOTHING(sbh.mergeSampleLogs(ws, base));
    const std::string A = base->run().getLogData("A")->value();
    const std::string B = base->run().getLogData("B")->value();
    const std::string C = base->run().getLogData("C")->value();
    // A summed according to user name and B summed according to IPF
    TS_ASSERT_EQUALS(A, "7.0999999999999996")
    TS_ASSERT_EQUALS(B, "4.7000000000000002")
    TS_ASSERT_EQUALS(C, "7.9000000000000004")
    testUnitsTestWorkspace(ws);
    testUnitsTestWorkspace(base);
  }

  void testConjoinXRunsIPFNames() {
    // Using prefix conjoin_ + default value names for constructing
    Logger log("testLog");
    auto ws = createWorkspace(2.6, 1.5, 8.5);
    auto base = createWorkspace(4.5, 3.2, 7.9);
    testUnitsTestWorkspace(ws);
    testUnitsTestWorkspace(base);
    SampleLogsBehaviour::SampleLogNames sampleLogNames;
    SampleLogsBehaviour::ParameterName parameterNames;
    parameterNames.SUM_MERGE = "conjoin_sample_logs_sum";
    SampleLogsBehaviour sbh =
        SampleLogsBehaviour(base, log, sampleLogNames, parameterNames);
    sbh.mergeSampleLogs(ws, base);
    const std::string A = base->run().getLogData("A")->value();
    const std::string B = base->run().getLogData("B")->value();
    const std::string C = base->run().getLogData("C")->value();
    // A and C summed according to IPF
    TS_ASSERT_EQUALS(A, "7.0999999999999996")
    TS_ASSERT_EQUALS(B, "3.2000000000000002")
    TS_ASSERT_EQUALS(C, "16.399999999999999")
    testUnitsTestWorkspace(ws);
    testUnitsTestWorkspace(base);
  }

  void testConjoinXRunsUserNames() {
    // Using prefix conjoin_ + default value names for constructing
    Logger log("testLog");
    auto ws = createWorkspace(2.6, 1.5, 8.5);
    auto base = createWorkspace(4.5, 3.2, 7.9);
    SampleLogsBehaviour::SampleLogNames sampleLogNames;
    sampleLogNames.sampleLogsSum = "B";
    SampleLogsBehaviour::ParameterName parameterNames;
    parameterNames.SUM_MERGE = "conjoin_sample_logs_sum";
    SampleLogsBehaviour sbh =
        SampleLogsBehaviour(base, log, sampleLogNames, parameterNames);
    sbh.mergeSampleLogs(ws, base);
    const std::string A = base->run().getLogData("A")->value();
    const std::string B = base->run().getLogData("B")->value();
    const std::string C = base->run().getLogData("C")->value();
    // B summed according to user name and A and C summed according to IPF
    TS_ASSERT_EQUALS(A, "7.0999999999999996")
    TS_ASSERT_EQUALS(B, "4.7000000000000002")
    TS_ASSERT_EQUALS(C, "16.399999999999999")
    testUnitsTestWorkspace(base);
    testUnitsTestWorkspace(ws);
  }

  void test_sum_unit() {
    // Using default values of the constructor
    Logger log("testLog");
    auto ws = createWorkspace(2.6, 1.5, 8.5);
    auto base = createWorkspace(4.5, 3.2, 7.9);
    TS_ASSERT_EQUALS(ws->getLog("A")->units(), "A_unit")
    TS_ASSERT_EQUALS(base->getLog("A")->units(), "A_unit")
    SampleLogsBehaviour::SampleLogNames sampleLogNames;
    sampleLogNames.sampleLogsSum = "A";
    SampleLogsBehaviour sbh = SampleLogsBehaviour(ws, log, sampleLogNames);
    TS_ASSERT_THROWS_NOTHING(sbh.mergeSampleLogs(ws, base));
    // A units must not have changed:
    TS_ASSERT_EQUALS(ws->getLog("A")->units(), "A_unit")
    TS_ASSERT_EQUALS(base->getLog("A")->units(), "A_unit")
  }

  void test_list_unit() {
    // Using default values of the constructor
    Logger log("testLog");
    auto ws = createWorkspace(2.6, 1.5, 8.5);
    auto base = createWorkspace(4.5, 3.2, 7.9);
    TS_ASSERT_EQUALS(ws->getLog("A")->units(), "A_unit")
    TS_ASSERT_EQUALS(base->getLog("A")->units(), "A_unit")
    SampleLogsBehaviour::SampleLogNames sampleLogNames;
    sampleLogNames.sampleLogsList = "A";
    SampleLogsBehaviour sbh = SampleLogsBehaviour(ws, log, sampleLogNames);
    TS_ASSERT_THROWS_NOTHING(sbh.mergeSampleLogs(ws, base));
    // A units must not have changed:
    TS_ASSERT_EQUALS(ws->getLog("A")->units(), "A_unit")
    TS_ASSERT_EQUALS(base->getLog("A")->units(), "A_unit")
  }

  void test_time_series_unit() {
    // Using default values of the constructor
    Logger log("testLog");
    auto ws = createWorkspace(2.65, 1.56, 8.55, "2018-11-30T16:17:01");
    auto base = createWorkspace(4.5, 3.2, 7.9, "2018-11-30T16:17:03");
    TS_ASSERT_EQUALS(ws->getLog("B")->units(), "B_unit")
    TS_ASSERT_EQUALS(base->getLog("B")->units(), "B_unit")
    SampleLogsBehaviour::SampleLogNames sampleLogNames;
    sampleLogNames.sampleLogsTimeSeries = "B";
    SampleLogsBehaviour sbh = SampleLogsBehaviour(base, log, sampleLogNames);
    TS_ASSERT_THROWS_NOTHING(sbh.mergeSampleLogs(ws, base));
    const std::string B = base->run().getLogData("B")->value();
    TS_ASSERT_EQUALS(B,
                     "2018-Nov-30 16:17:01  1.56\n2018-Nov-30 16:17:03  3.2\n")
    // B units must not have changed:
    TS_ASSERT_EQUALS(ws->getLog("B")->units(), "B_unit")
    TS_ASSERT_EQUALS(base->getLog("B")->units(), "B_unit")
  }

  MatrixWorkspace_sptr createWorkspace(double A, double B, double C,
                                       const std::string &time = "") {
    MatrixWorkspace_sptr ws = create2DWorkspaceWithFullInstrument(
        3, 3, true, false, true, m_instrName);
    // Add sample logs
    TS_ASSERT_THROWS_NOTHING(
        ws->mutableRun().addLogData(new PropertyWithValue<double>("A", A)))
    TS_ASSERT_THROWS_NOTHING(
        ws->mutableRun().addLogData(new PropertyWithValue<double>("B", B)))
    TS_ASSERT_THROWS_NOTHING(
        ws->mutableRun().addLogData(new PropertyWithValue<double>("C", C)))
    if (!time.empty()) {
      // add start times
      Property *tprop = new Mantid::Kernel::PropertyWithValue<std::string>(
          "start_time", time);
      ws->mutableRun().addLogData(tprop);
    }
    // Add units to the sample logs
    TS_ASSERT_THROWS_NOTHING(ws->getLog("A")->setUnits("A_unit"))
    TS_ASSERT_THROWS_NOTHING(ws->getLog("B")->setUnits("B_unit"))
    TS_ASSERT_THROWS_NOTHING(ws->getLog("C")->setUnits("C_unit"))
    testUnitsTestWorkspace(ws);
    // Load test parameter file
    LoadParameterFile addIPF;
    TS_ASSERT_THROWS_NOTHING(addIPF.initialize());
    TS_ASSERT_THROWS_NOTHING(addIPF.setProperty("ParameterXML", m_parameterXML))
    TS_ASSERT_THROWS_NOTHING(addIPF.setProperty("Workspace", ws))
    TS_ASSERT_THROWS_NOTHING(addIPF.execute())
    TS_ASSERT(addIPF.isExecuted())
    return ws;
  }

  void testUnitsTestWorkspace(MatrixWorkspace_sptr ws) {
    // All units must not have changed:
    TS_ASSERT_EQUALS(ws->getLog("A")->units(), "A_unit")
    TS_ASSERT_EQUALS(ws->getLog("B")->units(), "B_unit")
    TS_ASSERT_EQUALS(ws->getLog("C")->units(), "C_unit")
  }

private:
  // Test instrument name
  const std::string m_instrName = "INSTR";
  // Define parameter XML string
  std::string m_parameterXML =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"
      "<parameter-file instrument=\"INSTR\" valid-from=\"2018-11-07 "
      "12:00:00\">"
      "  <component-link name=\"INSTR\">"
      "    <!-- For MergeRuns.-->"
      "    <parameter name=\"sample_logs_sum\" type=\"string\">"
      "	      <value val=\"B\" />"
      "    </parameter>"
      "    <parameter name=\"sample_logs_list\" type=\"string\">"
      "	      <value val=\"A\" />"
      "    </parameter>"
      "    <parameter name=\"sample_logs_time_series\" type=\"string\">"
      "	      <value val=\"D\" />"
      "    </parameter>"
      "    <!-- For ConjoinXRuns. -->"
      "    <parameter name=\"conjoin_sample_logs_sum\" type=\"string\">"
      "       <value val=\"A, C\" />"
      "    </parameter>"
      "  </component-link>"
      "</parameter-file>";
};

#endif /* MANTID_ALGORITHMS_SAMPLELOGSBEHAVIOURTEST_H_ */
