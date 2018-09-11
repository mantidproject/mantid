#ifndef MANTID_ALGORITHMS_DIFFRACTIONEVENTCALIBRATEDETECTORSTEST_H_
#define MANTID_ALGORITHMS_DIFFRACTIONEVENTCALIBRATEDETECTORSTEST_H_

#include "MantidAlgorithms/DiffractionEventCalibrateDetectors.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <Poco/File.h>
#include <cxxtest/TestSuite.h>
#include <fstream>

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class DiffractionEventCalibrateDetectorsTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    DiffractionEventCalibrateDetectors alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    EventWorkspace_sptr eventWS =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(1, 50);
    AnalysisDataService::Instance().addOrReplace("temp_event_ws", eventWS);

    // Name of the output workspace.
    std::string filename = "./DiffractionEventCalibrateDetectorsTest.DetCal";

    DiffractionEventCalibrateDetectors alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", "temp_event_ws"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Params", "1.9, 0.001, 2.2"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("MaxIterations", "1"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("LocationOfPeakToOptimize", "2.038"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("BankName", "bank1"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DetCalFilename", filename));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    filename = alg.getPropertyValue("DetCalFilename");

    // DetCal output:  5  1  50  50  40  40  0.2000  500  0 0 500  0.899688
    // 0.436438 0.00908776  0.00912458 0.894435 0.447104
    std::fstream outFile(filename.c_str());
    TS_ASSERT(outFile)
    int num, banknum, xpix, ypix;
    double xsize, ysize, zsize, cennorm, cenx, ceny, cenz, basex, basey, basez,
        upx, upy, upz;
    outFile >> num >> banknum >> xpix >> ypix >> xsize >> ysize >> zsize >>
        cennorm >> cenx >> ceny >> cenz >> basex >> basey >> basez >> upx >>
        upy >> upz;
    TS_ASSERT_DELTA(cennorm, 500., 0.0001)
    TS_ASSERT_DELTA(cenx, 0., 0.0001)
    TS_ASSERT_DELTA(ceny, 0., 0.0001)
    TS_ASSERT_DELTA(cenz, 500., 0.0001)
    TS_ASSERT_DELTA(basex, 0.899688, 0.0001)
    TS_ASSERT_DELTA(basey, 0.436438, 0.0001)
    TS_ASSERT_DELTA(basez, 0.00908776, 0.0001)
    TS_ASSERT_DELTA(upx, 0.00912458, 0.0001)
    TS_ASSERT_DELTA(upy, 0.894435, 0.0001)
    TS_ASSERT_DELTA(upz, 0.447104, 0.0001)
    outFile.close();
    Poco::File(filename).remove();

    AnalysisDataService::Instance().remove("temp_event_ws");
  }
};

#endif /* MANTID_ALGORITHMS_DIFFRACTIONEVENTCALIBRATEDETECTORSTEST_H_ */
