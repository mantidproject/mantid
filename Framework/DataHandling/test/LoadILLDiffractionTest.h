#ifndef MANTID_DATAHANDLING_LOADILLDIFFRACTIONTEST_H_
#define MANTID_DATAHANDLING_LOADILLDIFFRACTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadILLDiffraction.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/DetectorInfo.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/Load.h"
#include "MantidKernel/ConfigService.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;

class LoadILLDiffractionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadILLDiffractionTest *createSuite() {
    return new LoadILLDiffractionTest();
  }
  static void destroySuite(LoadILLDiffractionTest *suite) { delete suite; }

  void setUp() override {
    ConfigService::Instance().appendDataSearchSubDir("ILL/D20/");
    ConfigService::Instance().appendDataSearchSubDir("ILL/D2B/");
    ConfigService::Instance().setFacility("ILL");
  }

  void test_Init() {
    LoadILLDiffraction alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_D20_no_scan() {
    // Tests the no-scan case for D20

    LoadILLDiffraction alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "967100.nxs"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 3073)
    TS_ASSERT_EQUALS(outputWS->blocksize(), 1)
  }

  void test_D20_scan() {
    // Tests the scanned case for D20

    LoadILLDiffraction alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    // Note that, this is the older type of file, and is modified manually
    // to match the final configuration having the custom NX_class attribute
    // So this will not run with generic Load
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "000017.nxs"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "_unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 3073)
    TS_ASSERT_EQUALS(outputWS->blocksize(), 21)
  }

  void test_D20_multifile() {
    // Tests 2 non-scanned files for D20 with the generic Load on ADS
    // This tests indirectly the confidence method
    // (and NexusDescriptor issue therein)

    Load alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("Filename", "967100-967101.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_outWS"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    MatrixWorkspace_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("_outWS");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 3073)
    TS_ASSERT_EQUALS(outputWS->blocksize(), 1)
  }

  void test_D2B_single_file() {
    // Test a D2B file with 25 detector positions

    const int NUMBER_OF_TUBES = 128;
    const int NUMBER_OF_PIXELS = 128;
    const int SCAN_COUNT = 25;
    const int NUMBER_OF_MONITORS = 1;

    LoadILLDiffraction alg;
    alg.setChild(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "508093.nxs"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_outWS"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    MatrixWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    const auto &detInfo = outputWS->detectorInfo();
    // Number of time indexes * (number of tubes * number of pixels + monitor)
    TS_ASSERT_EQUALS(
        outputWS->getNumberHistograms(),
        SCAN_COUNT * (NUMBER_OF_TUBES * NUMBER_OF_PIXELS + NUMBER_OF_MONITORS))
    TS_ASSERT_EQUALS(outputWS->blocksize(), 1)

    // Check time ranges
    const std::string EXPECTED_START_TIME = "2015-04-16T16:25:31";
    const std::string EXPECTED_SECOND_TIME = "2015-04-16T16:26:08.804000000";
    const std::string EXPECTED_SECOND_FROM_END_TIME =
        "2015-04-16T16:40:34.289000000";
    const std::string EXPECTED_END_TIME = "2015-04-16T16:41:11.956000000";

    for (size_t i = 0; i < detInfo.size(); ++i) {
      TS_ASSERT_EQUALS(detInfo.scanCount(i), SCAN_COUNT)

      const auto &startRange = detInfo.scanInterval({i, 0});
      const auto &secondRange = detInfo.scanInterval({i, 1});
      const auto &secondFromEndRange =
          detInfo.scanInterval({i, detInfo.scanCount(i) - 2});
      const auto &endRange =
          detInfo.scanInterval({i, detInfo.scanCount(i) - 1});

      TS_ASSERT_EQUALS(startRange.first.toISO8601String(), EXPECTED_START_TIME)
      TS_ASSERT_EQUALS(startRange.second.toISO8601String(),
                       EXPECTED_SECOND_TIME)
      TS_ASSERT_EQUALS(secondRange.first.toISO8601String(),
                       EXPECTED_SECOND_TIME)
      TS_ASSERT_EQUALS(secondFromEndRange.second.toISO8601String(),
                       EXPECTED_SECOND_FROM_END_TIME)
      TS_ASSERT_EQUALS(endRange.first.toISO8601String(),
                       EXPECTED_SECOND_FROM_END_TIME)
      TS_ASSERT_EQUALS(endRange.second.toISO8601String(), EXPECTED_END_TIME)
    }

    // Check monitor does not move
    for (size_t j = 0; j < detInfo.scanCount(0); ++j) {
      TS_ASSERT(detInfo.isMonitor({0, j}))
      TS_ASSERT_EQUALS(detInfo.position({0, j}), detInfo.position({0, 0}))
    }

    // Check detector tubes are moved as expected
    const double ANGULAR_DETECTOR_SPACING = 1.25;
    const double ANGULAR_SCAN_INCREMENT = 0.05;
    const double TUBE_1_START_ANGLE = 147.496;

    for (size_t i = 0; i < NUMBER_OF_TUBES; ++i) {
      for (size_t j = 0; j < detInfo.scanCount(i); ++j) {
        // Find two pixels just above and just below the centre, and take their
        // average position as the tube centre
        auto belowCentrePixel = i * NUMBER_OF_PIXELS + NUMBER_OF_PIXELS / 2;
        auto aboveCentrePixel = belowCentrePixel + 1;
        TS_ASSERT(!detInfo.isMonitor({belowCentrePixel, j}))
        TS_ASSERT(!detInfo.isMonitor({aboveCentrePixel, j}))
        auto tubeCentre = (detInfo.position({belowCentrePixel, j}) +
                           detInfo.position({aboveCentrePixel, j})) /
                          2;
        // Check the tube centre is 90 degrees from the y-axis
        TS_ASSERT_DELTA(tubeCentre.angle(V3D(0, 1, 0)) * RAD_2_DEG, 90.0, 1e-6)
        // Check the tube centre is at the expected angle from the z-axis
        // (incoming beam). The first angle for tube_1 is hard coded here, then
        // for each time index the angle increments by ANGULAR_SCAN_INCREMENT
        // (0.05 deg). Then detectors themselves are spaced apart by
        // ANGULARD_DETECTOR_SPACING (1.25 deg).
        //
        // A generous tolerance is required as the NeXus file contains the
        // actual hardware readings, which have a large tolerance.
        TS_ASSERT_DELTA(tubeCentre.angle(V3D(0, 0, 1)) * RAD_2_DEG,
                        std::abs(ANGULAR_SCAN_INCREMENT * double(j) +
                                 TUBE_1_START_ANGLE -
                                 ANGULAR_DETECTOR_SPACING * double(i)),
                        1e-2)
      }
    }
  }

private:
  const double RAD_2_DEG = 180.0 / M_PI;
};

#endif /* MANTID_DATAHANDLING_LOADILLDIFFRACTIONTEST_H_ */
