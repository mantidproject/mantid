#ifndef MANTID_WORKFLOWALGORITHMS_MUONLOADTEST_H_
#define MANTID_WORKFLOWALGORITHMS_MUONLOADTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidWorkflowAlgorithms/MuonLoad.h"
#include "MantidAPI/ScopedWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/TableWorkspace.h"

using Mantid::WorkflowAlgorithms::MuonLoad;

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class MuonLoadTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MuonLoadTest *createSuite() { return new MuonLoadTest(); }
  static void destroySuite(MuonLoadTest *suite) { delete suite; }

  void test_Init() {
    MuonLoad alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_simpleLoad() {
    ScopedWorkspace output;

    std::vector<int> group1, group2;

    for (int i = 1; i <= 16; ++i)
      group1.push_back(i);

    for (int i = 17; i <= 32; ++i)
      group2.push_back(i);

    TableWorkspace_sptr grouping = createGroupingTable(group1, group2);

    MuonLoad alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("Filename", "emu00006473.nxs"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("DetectorGroupingTable", grouping));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputType", "GroupCounts"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupIndex", 0));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", output.name()));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    MatrixWorkspace_sptr ws =
        boost::dynamic_pointer_cast<MatrixWorkspace>(output.retrieve());

    TS_ASSERT(ws);
    if (ws) {
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(ws->blocksize(), 2000);

      TS_ASSERT_EQUALS(ws->readY(0)[0], 461);
      TS_ASSERT_EQUALS(ws->readY(0)[1000], 192);
      TS_ASSERT_EQUALS(ws->readY(0)[1752], 5);

      TS_ASSERT_DELTA(ws->readE(0)[0], 21.471, 0.001);
      TS_ASSERT_DELTA(ws->readE(0)[1000], 13.856, 0.001);
      TS_ASSERT_DELTA(ws->readE(0)[1752], 2.236, 0.001);

      TS_ASSERT_DELTA(ws->readX(0)[0], -0.254, 0.001);
      TS_ASSERT_DELTA(ws->readX(0)[1000], 15.746, 0.001);
      TS_ASSERT_DELTA(ws->readX(0)[1752], 27.778, 0.001);
    }
  }

  void test_multiPeriod() {
    ScopedWorkspace output;

    std::vector<int> group1, group2;

    for (int i = 33; i <= 64; ++i)
      group1.push_back(i);
    for (int i = 1; i <= 32; ++i)
      group2.push_back(i);

    TableWorkspace_sptr grouping = createGroupingTable(group1, group2);

    MuonLoad alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("Filename", "MUSR00015189.nxs"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FirstPeriod", 0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SecondPeriod", 1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeriodOperation", "+"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("DetectorGroupingTable", grouping));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputType", "GroupCounts"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupIndex", 1));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", output.name()));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    MatrixWorkspace_sptr ws =
        boost::dynamic_pointer_cast<MatrixWorkspace>(output.retrieve());

    TS_ASSERT(ws);
    if (ws) {
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(ws->blocksize(), 2000);

      TS_ASSERT_EQUALS(ws->readY(0)[0], 23);
      TS_ASSERT_EQUALS(ws->readY(0)[1000], 3);
      TS_ASSERT_EQUALS(ws->readY(0)[1701], 1);

      TS_ASSERT_DELTA(ws->readE(0)[0], 4.796, 0.001);
      TS_ASSERT_DELTA(ws->readE(0)[1000], 1.732, 0.001);
      TS_ASSERT_DELTA(ws->readE(0)[1701], 1.000, 0.001);

      TS_ASSERT_DELTA(ws->readX(0)[0], -0.550, 0.001);
      TS_ASSERT_DELTA(ws->readX(0)[1000], 15.450, 0.001);
      TS_ASSERT_DELTA(ws->readX(0)[1701], 26.666, 0.001);
    }
  }

  void test_binCorrectionParams() {
    ScopedWorkspace output;

    std::vector<int> group1, group2;

    for (int i = 1; i <= 16; ++i)
      group1.push_back(i);
    for (int i = 17; i <= 32; ++i)
      group2.push_back(i);

    TableWorkspace_sptr grouping = createGroupingTable(group1, group2);

    MuonLoad alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("Filename", "emu00006473.nxs"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("DetectorGroupingTable", grouping));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputType", "GroupCounts"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupIndex", 0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeZero", 0.5));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Xmin", 0.1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Xmax", 16.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RebinParams", "0.08"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", output.name()));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    MatrixWorkspace_sptr ws =
        boost::dynamic_pointer_cast<MatrixWorkspace>(output.retrieve());

    TS_ASSERT(ws);
    if (ws) {
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(ws->blocksize(), 198);

      TS_ASSERT_DELTA(ws->readX(0)[0], 0.102, 0.001);
      TS_ASSERT_DELTA(ws->readX(0)[100], 8.102, 0.001);
      TS_ASSERT_DELTA(ws->readX(0)[198], 15.942, 0.001);

      TS_ASSERT_DELTA(ws->readY(0)[0], 1024372.2, 0.1);
      TS_ASSERT_DELTA(ws->readY(0)[100], 24589.0, 0.1);
      TS_ASSERT_DELTA(ws->readY(0)[197], 730.0, 0.1);

      TS_ASSERT_DELTA(ws->readE(0)[0], 1012.113, 0.001);
      TS_ASSERT_DELTA(ws->readE(0)[100], 156.809, 0.001);
      TS_ASSERT_DELTA(ws->readE(0)[197], 27.019, 0.001);
    }
  }

  void test_deadTimeCorrection() {
    ScopedWorkspace output;

    std::vector<int> group1, group2;

    for (int i = 1; i <= 16; ++i)
      group1.push_back(i);
    for (int i = 17; i <= 32; ++i)
      group2.push_back(i);

    TableWorkspace_sptr grouping = createGroupingTable(group1, group2);

    auto deadTimes = boost::make_shared<TableWorkspace>();
    deadTimes->addColumn("int", "spectrum");
    deadTimes->addColumn("double", "dead-time");

    for (int i = 0; i < 32; ++i) {
      TableRow newRow = deadTimes->appendRow();
      newRow << (i + 1) << 1.0;
    }

    MuonLoad alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("Filename", "emu00006473.nxs"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("DetectorGroupingTable", grouping));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ApplyDeadTimeCorrection", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("CustomDeadTimeTable", deadTimes));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputType", "GroupCounts"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupIndex", 0));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", output.name()));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    MatrixWorkspace_sptr ws =
        boost::dynamic_pointer_cast<MatrixWorkspace>(output.retrieve());

    TS_ASSERT(ws);
    if (ws) {
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);
      TS_ASSERT_EQUALS(ws->blocksize(), 2000);

      TS_ASSERT_DELTA(ws->readY(0)[0], 463.383, 0.001);
      TS_ASSERT_DELTA(ws->readY(0)[1000], 192.468, 0.001);
      TS_ASSERT_DELTA(ws->readY(0)[1752], 5.00075, 0.00001);

      TS_ASSERT_DELTA(ws->readE(0)[0], 21.471, 0.001);
      TS_ASSERT_DELTA(ws->readE(0)[1000], 13.856, 0.001);
      TS_ASSERT_DELTA(ws->readE(0)[1752], 2.236, 0.001);

      TS_ASSERT_DELTA(ws->readX(0)[0], -0.254, 0.001);
      TS_ASSERT_DELTA(ws->readX(0)[1000], 15.746, 0.001);
      TS_ASSERT_DELTA(ws->readX(0)[1752], 27.778, 0.001);
    }
  }

  void test_errorReporting() {
    ScopedWorkspace output;

    auto emptyGrouping =
        createGroupingTable(std::vector<int>(), std::vector<int>());

    MuonLoad alg;
    alg.setRethrows(true);

    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())

    TS_ASSERT_THROWS(alg.setPropertyValue("Filename", "non-existent-file.nxs"),
                     std::invalid_argument);

    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("Filename", "emu00006473.nxs"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("DetectorGroupingTable", emptyGrouping));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputType", "GroupCounts"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GroupIndex", 0));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", output.name()));

    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument);
    TS_ASSERT(!alg.isExecuted());
  }

  void test_autoGrouping() {
    ScopedWorkspace output;

    try {
      MuonLoad alg;
      alg.setRethrows(true);
      alg.initialize();
      alg.setPropertyValue("Filename", "emu00006473.nxs");
      alg.setProperty("OutputType", "GroupCounts");
      alg.setProperty("GroupIndex", 0);
      alg.setPropertyValue("OutputWorkspace", output.name());
      alg.execute();
    } catch (std::exception &e) {
      TS_FAIL(e.what());
      return;
    }

    MatrixWorkspace_sptr ws =
        boost::dynamic_pointer_cast<MatrixWorkspace>(output.retrieve());

    TS_ASSERT(ws);
    if (!ws)
      return; // Nothing to check

    TS_ASSERT_EQUALS(ws->getNumberHistograms(), 1);

    TS_ASSERT_EQUALS(ws->readY(0)[0], 461);
    TS_ASSERT_EQUALS(ws->readY(0)[1000], 192);
    TS_ASSERT_EQUALS(ws->readY(0)[1998], 1);
  }

private:
  TableWorkspace_sptr createGroupingTable(const std::vector<int> &group1,
                                          const std::vector<int> &group2) {
    auto t = boost::make_shared<TableWorkspace>();

    t->addColumn("vector_int", "Detectors");

    TableRow row1 = t->appendRow();
    row1 << group1;

    TableRow row2 = t->appendRow();
    row2 << group2;

    return t;
  }
};

#endif /* MANTID_WORKFLOWALGORITHMS_MUONLOADTEST_H_ */
