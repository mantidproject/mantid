#ifndef MANTID_ALGORITHMS_MUONGROUPDETECTORSTEST_H_
#define MANTID_ALGORITHMS_MUONGROUPDETECTORSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidMuon/MuonGroupDetectors.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::MuonGroupDetectors;

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class MuonGroupDetectorsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MuonGroupDetectorsTest *createSuite() {
    return new MuonGroupDetectorsTest();
  }
  static void destroySuite(MuonGroupDetectorsTest *suite) { delete suite; }

  void test_Init() {
    MuonGroupDetectors alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    // Name of the output workspace.
    const std::string outWSName("MuonGroupDetectorsTest_OutputWS");

    MatrixWorkspace_sptr inWS =
        WorkspaceCreationHelper::create2DWorkspace123(5, 3);

    for (size_t i = 0; i < inWS->getNumberHistograms(); ++i)
      inWS->getSpectrum(i).setDetectorID(static_cast<detid_t>(
          i + 1)); // To be consistent with how LoadMuonNexus works

    TableWorkspace_sptr grouping = createDetectorGroupingTable();

    MuonGroupDetectors alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inWS));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("DetectorGroupingTable", grouping));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outWSName));
    TS_ASSERT(ws);

    if (ws) {
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), 2);
      TS_ASSERT_EQUALS(ws->blocksize(), 3);

      TS_ASSERT_EQUALS(ws->y(0)[0], 4);
      TS_ASSERT_EQUALS(ws->y(1)[0], 6);

      TS_ASSERT_EQUALS(ws->x(0)[1], 2);
      TS_ASSERT_EQUALS(ws->x(1)[1], 2);

      TS_ASSERT_DELTA(ws->e(0)[2], 4.243, 0.001);
      TS_ASSERT_DELTA(ws->e(1)[2], 5.196, 0.001);

      TS_ASSERT_EQUALS(ws->getSpectrum(0).getSpectrumNo(), 1);
      TS_ASSERT_EQUALS(ws->getSpectrum(1).getSpectrumNo(), 2);

      std::set<detid_t> d1;
      d1.insert(1);
      d1.insert(2);
      TS_ASSERT_EQUALS(ws->getSpectrum(0).getDetectorIDs(), d1);

      std::set<detid_t> d2;
      d2.insert(3);
      d2.insert(4);
      d2.insert(5);
      TS_ASSERT_EQUALS(ws->getSpectrum(1).getDetectorIDs(), d2);
    }

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

private:
  TableWorkspace_sptr createDetectorGroupingTable() {
    auto t = boost::make_shared<TableWorkspace>();

    t->addColumn("vector_int", "Detectors");

    std::vector<int> group1;
    group1.push_back(1);
    group1.push_back(2);
    TableRow row1 = t->appendRow();
    row1 << group1;

    std::vector<int> group2;
    group2.push_back(3);
    group2.push_back(4);
    group2.push_back(5);
    TableRow row2 = t->appendRow();
    row2 << group2;

    return t;
  }
};

#endif /* MANTID_ALGORITHMS_MUONGROUPDETECTORSTEST_H_ */
