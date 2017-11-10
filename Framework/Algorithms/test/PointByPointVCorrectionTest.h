#ifndef POINTBYPOINTVCORRECTIONTEST_H_
#define POINTBYPOINTVCORRECTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/PointByPointVCorrection.h"
#include "MantidGeometry/Instrument.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using Mantid::Algorithms::PointByPointVCorrection;

class PointByPointVCorrectionTest : public CxxTest::TestSuite {
public:
  void testName() { TS_ASSERT_EQUALS(pbpv.name(), "PointByPointVCorrection"); }

  void testVersion() { TS_ASSERT_EQUALS(pbpv.version(), 1); }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(pbpv.initialize());
    TS_ASSERT(pbpv.isInitialized());
  }

  void testExec() {
    if (!pbpv.isInitialized())
      pbpv.initialize();

    MatrixWorkspace_sptr testSample =
        WorkspaceCreationHelper::create2DWorkspaceBinned(2, 5, 0.5, 1.5);
    MatrixWorkspace_sptr testVanadium =
        WorkspaceCreationHelper::create2DWorkspaceBinned(2, 5, 0.5, 1.5);
    // Make the instruments match
    Mantid::Geometry::Instrument_sptr inst(new Mantid::Geometry::Instrument);
    testSample->setInstrument(inst);
    testVanadium->setInstrument(inst);
    // Change the Y values
    testSample->mutableY(1) = Mantid::HistogramData::HistogramY(5, 3.0);
    testVanadium->mutableY(1) = Mantid::HistogramData::HistogramY(5, 5.5);

    pbpv.setProperty<MatrixWorkspace_sptr>("InputW1", testSample);
    pbpv.setProperty<MatrixWorkspace_sptr>("InputW2", testVanadium);
    pbpv.setPropertyValue("OutputWorkspace", "out");
    TS_ASSERT_THROWS_NOTHING(pbpv.execute());
    TS_ASSERT(pbpv.isExecuted());

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output =
            AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("out"));

    // Check a few values
    TS_ASSERT_DELTA(output->x(1)[4], 6.5, 0.0001);
    TS_ASSERT_DELTA(output->x(1)[1], 2.0, 0.0001);
    TS_ASSERT_DELTA(output->x(0)[0], 0.5, 0.000001);
    TS_ASSERT_DELTA(output->y(1)[4], 2.9999, 0.0001);
    TS_ASSERT_DELTA(output->y(1)[1], 2.9999, 0.0001);
    TS_ASSERT_DELTA(output->y(0)[0], 2.0, 0.000001);
    TS_ASSERT_DELTA(output->e(1)[3], 1.8745, 0.0001);
    TS_ASSERT_DELTA(output->e(1)[2], 1.8745, 0.0001);
    TS_ASSERT_DELTA(output->e(0)[0], 2.2803, 0.0001);

    AnalysisDataService::Instance().remove("out");
  }

private:
  PointByPointVCorrection pbpv;
};

class PointByPointVCorrectionTestPerformance : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PointByPointVCorrectionTestPerformance *createSuite() {
    return new PointByPointVCorrectionTestPerformance();
  }

  static void destroySuite(PointByPointVCorrectionTestPerformance *suite) {
    delete suite;
  }

  void setUp() override {
    MatrixWorkspace_sptr testSample =
        WorkspaceCreationHelper::create2DWorkspaceBinned(20000, 5, 0.5, 1.5);
    MatrixWorkspace_sptr testVanadium =
        WorkspaceCreationHelper::create2DWorkspaceBinned(20000, 5, 0.5, 1.5);
    // Make the instruments match
    Mantid::Geometry::Instrument_sptr inst(new Mantid::Geometry::Instrument);
    testSample->setInstrument(inst);
    testVanadium->setInstrument(inst);
    // Change the Y values
    testSample->mutableY(1) = Mantid::HistogramData::HistogramY(5, 3.0);
    testVanadium->mutableY(1) = Mantid::HistogramData::HistogramY(5, 5.5);

    pbpv.initialize();
    pbpv.setProperty<MatrixWorkspace_sptr>("InputW1", testSample);
    pbpv.setProperty<MatrixWorkspace_sptr>("InputW2", testVanadium);
    pbpv.setPropertyValue("OutputWorkspace", "outputWS");
  }

  void tearDown() override {
    Mantid::API::AnalysisDataService::Instance().remove("outputWS");
  }

  void testPerformanceWS() { pbpv.execute(); }

private:
  PointByPointVCorrection pbpv;
};

#endif /*POINTBYPOINTVCORRECTIONTEST_H_*/
