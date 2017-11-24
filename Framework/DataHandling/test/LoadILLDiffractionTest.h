#ifndef MANTID_DATAHANDLING_LOADILLDIFFRACTIONTEST_H_
#define MANTID_DATAHANDLING_LOADILLDIFFRACTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/Load.h"
#include "MantidDataHandling/LoadILLDiffraction.h"
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
    // Temperature ramp is not a motor scan so produces a file per T

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
    TS_ASSERT(outputWS->detectorInfo().isMonitor(0))
    TS_ASSERT(!outputWS->isHistogramData())
    TS_ASSERT(!outputWS->isDistribution())

    TS_ASSERT_EQUALS(outputWS->x(0)[0], 0.)
    TS_ASSERT_EQUALS(outputWS->y(0)[0], 2685529)
    TS_ASSERT_DELTA(outputWS->e(0)[0], 1638.76, 0.01)

    TS_ASSERT_EQUALS(outputWS->x(1)[0], 0.)
    TS_ASSERT_EQUALS(outputWS->y(1)[0], 548)
    TS_ASSERT_DELTA(outputWS->e(1)[0], 23.40, 0.01)

    TS_ASSERT_EQUALS(outputWS->x(2)[0], 0.)
    TS_ASSERT_EQUALS(outputWS->y(2)[0], 991)
    TS_ASSERT_DELTA(outputWS->e(2)[0], 31.48, 0.01)

    TS_ASSERT_EQUALS(outputWS->x(1111)[0], 0.)
    TS_ASSERT_EQUALS(outputWS->y(1111)[0], 7080)
    TS_ASSERT_DELTA(outputWS->e(1111)[0], 84.14, 0.01)

    TS_ASSERT_EQUALS(outputWS->x(3072)[0], 0.)
    TS_ASSERT_EQUALS(outputWS->y(3072)[0], 0.)
    TS_ASSERT_EQUALS(outputWS->e(3072)[0], 0.)

    TS_ASSERT(outputWS->run().hasProperty("simulated_d20"))
    TS_ASSERT(outputWS->run().hasProperty("AcquisitionSpy"))
    TS_ASSERT(outputWS->run().hasProperty("SampleSettings"))

    const auto sim = outputWS->run().getLogData("simulated_d20");
    const auto spy = outputWS->run().getLogData("AcquisitionSpy");
    const auto sample = outputWS->run().getLogData("SampleSettings");

    TS_ASSERT_EQUALS(sim->size(), 1)
    TS_ASSERT_EQUALS(spy->size(), 1)
    TS_ASSERT_EQUALS(sample->size(), 1)

    TS_ASSERT_EQUALS(sim->value(), "2017-May-15 14:36:18  5.44174e+06\n")
    TS_ASSERT_EQUALS(spy->value(), "2017-May-15 14:36:18  240\n")
    TS_ASSERT_EQUALS(sample->value(), "2017-May-15 14:36:18  4.9681\n")
  }

  void test_D20_scan() {
    // Tests the omega scanned case for D20
    // Omega scan is a motor scan, so it is recorded in a single file
    // But it is not a detector scan within our context

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
    TS_ASSERT(outputWS->detectorInfo().isMonitor(0))
    TS_ASSERT(!outputWS->isHistogramData())
    TS_ASSERT(!outputWS->isDistribution())

    for (size_t row = 0; row < 10; ++row) {
      for (size_t col = 0; col < 21; ++col) {
        double val = static_cast<double>(col);
        TS_ASSERT_EQUALS(outputWS->y(row)[col], 3. * (val + 1))
        TS_ASSERT_EQUALS(outputWS->x(row)[col], 1. + 0.2 * val)
        TS_ASSERT_EQUALS(outputWS->e(row)[col], sqrt(3. * (val + 1)))
      }
    }

    TS_ASSERT(outputWS->run().hasProperty("Omega"))
    TS_ASSERT(outputWS->run().hasProperty("Detector"))
    TS_ASSERT(outputWS->run().hasProperty("AcquisitionSpy"))
    TS_ASSERT(outputWS->run().hasProperty("SampleSettings"))
    TS_ASSERT(outputWS->run().hasProperty("MagneticField"))

    const auto omega = outputWS->run().getLogData("Omega");

    TS_ASSERT_EQUALS(omega->size(), 21)

    const std::string omegaTimeSeriesValue =
        "2017-Feb-15 08:58:52  1\n2017-Feb-15 08:58:52.521547000  "
        "1.2\n2017-Feb-15 08:58:53.043086000  1.4\n2017-Feb-15 "
        "08:58:53.564674000  1.6\n2017-Feb-15 08:58:54.086244000  "
        "1.8\n2017-Feb-15 08:58:54.600926000  2\n2017-Feb-15 "
        "08:58:55.122357000  2.2\n2017-Feb-15 08:58:55.643809000  "
        "2.4\n2017-Feb-15 08:58:56.165310000  2.6\n2017-Feb-15 "
        "08:58:56.686815000  2.8\n2017-Feb-15 08:58:57.208370000  "
        "3\n2017-Feb-15 08:58:57.730012999  3.2\n2017-Feb-15 "
        "08:58:58.251527998  3.4\n2017-Feb-15 08:58:58.773040998  "
        "3.6\n2017-Feb-15 08:58:59.294480998  3.8\n2017-Feb-15 "
        "08:58:59.815922997  4\n2017-Feb-15 08:59:00.337767997  "
        "4.2\n2017-Feb-15 08:59:00.859268997  4.4\n2017-Feb-15 "
        "08:59:01.380606996  4.6\n2017-Feb-15 08:59:01.902055996  "
        "4.8\n2017-Feb-15 08:59:02.423509996  5\n";

    TS_ASSERT_EQUALS(omega->value(), omegaTimeSeriesValue)
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
    TS_ASSERT(outputWS->detectorInfo().isMonitor(0))
    TS_ASSERT(!outputWS->isHistogramData())
    TS_ASSERT(!outputWS->isDistribution())
  }

  void test_D2B_single_file() {
    // Test a D2B detector scan file with 25 detector positions
    // TODO: assert on values!

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
