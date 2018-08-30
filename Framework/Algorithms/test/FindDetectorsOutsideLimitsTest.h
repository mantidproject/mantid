#ifndef FINDDETECTORSOUTSIDELIMITSTEST_H_
#define FINDDETECTORSOUTSIDELIMITSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAlgorithms/FindDetectorsOutsideLimits.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using Mantid::HistogramData::CountStandardDeviations;
using Mantid::HistogramData::Counts;
using Mantid::Types::Core::DateAndTime;
using Mantid::Types::Event::TofEvent;

class FindDetectorsOutsideLimitsTest : public CxxTest::TestSuite {
public:
  void testInit() {
    FindDetectorsOutsideLimits alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void testExec() {
    const int sizex = 10, sizey = 20;
    // Register the workspace in the data service and initialise it with abitary
    // data
    Workspace2D_sptr work_in =
        // the x values look like this -1, 2, 5, 8, 11, 14, 17, 20, 23, 26
        WorkspaceCreationHelper::create2DWorkspaceBinned(sizey, sizex, -1, 3.0);

    Instrument_sptr instr(new Instrument);

    // yVeryDead is a detector with low counts
    Counts yVeryDead(sizex, 0.1);
    CountStandardDeviations eVeryDead(sizex, 0.1);
    // yTooDead gives some counts at the start but has a whole region full of
    // zeros
    double TD[sizex] = {2, 4, 5, 10, 0, 0, 0, 0, 0, 0};
    Counts yTooDead(TD, TD + 10);
    CountStandardDeviations eTooDead(TD, TD + 10);
    // yStrange dies after giving some counts but then comes back
    double S[sizex] = {0.2, 4, 50, 0.001, 0, 0, 0, 0, 1, 0};
    Counts yStrange(S, S + 10);
    CountStandardDeviations eStrange(S, S + 10);
    for (int i = 0; i < sizey; i++) {
      if (i % 3 == 0) { // the last column is set arbitrarily to have the same
                        // values as the second because the errors shouldn't
                        // make any difference
        work_in->setCounts(i, yTooDead);
        work_in->setCountStandardDeviations(i, eTooDead);
      }
      if (i % 2 == 0) {
        work_in->setCounts(i, yVeryDead);
        work_in->setCountStandardDeviations(i, eVeryDead);
      }
      if (i == 19) {
        work_in->setCounts(i, yStrange);
        work_in->setCountStandardDeviations(i, eTooDead);
      }
      work_in->getSpectrum(i).setSpectrumNo(i);

      Mantid::Geometry::Detector *det =
          new Mantid::Geometry::Detector("", i, nullptr);
      instr->add(det);
      instr->markAsDetector(det);
      work_in->getSpectrum(i).setDetectorID(i);
    }
    work_in->setInstrument(instr);

    FindDetectorsOutsideLimits alg;

    AnalysisDataService::Instance().add("testdead_in", work_in);
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", "testdead_in");
    alg.setPropertyValue("OutputWorkspace", "testdead_out");
    alg.setPropertyValue("LowThreshold", "1");
    alg.setPropertyValue("HighThreshold", "21.01");
    alg.setPropertyValue("RangeLower", "-1");

    // Testing behavour with Range_lower or Range_upper not set
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Get back the output workspace
    MatrixWorkspace_sptr work_out;
    TS_ASSERT_THROWS_NOTHING(
        work_out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "testdead_out"));

    const int numFailed = alg.getProperty("NumberOfFailures");
    TS_ASSERT_EQUALS(numFailed, 11);

    const auto &spectrumInfo = work_out->spectrumInfo();

    const double liveValue(0.0);
    const double maskValue(1.0);
    for (int i = 0; i < sizey; i++) {
      const double val = work_out->y(i)[0];
      double valExpected = liveValue;
      // Check masking
      IDetector_const_sptr det;
      TS_ASSERT_EQUALS(spectrumInfo.hasDetectors(i), true);
      // Spectra set up with yVeryDead fail low counts or yStrange fail on high
      if (i % 2 == 0 || i == 19) {
        valExpected = maskValue;
      }

      TS_ASSERT_DELTA(val, valExpected, 1e-9);
    }

    // Set cut off much of the range and yTooDead will stop failing on high
    // counts
    alg.setPropertyValue("RangeUpper", "4.9");
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    // retrieve the output workspace
    TS_ASSERT_THROWS_NOTHING(
        work_out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "testdead_out"));

    const int numFailed2 = alg.getProperty("NumberOfFailures");
    TS_ASSERT_EQUALS(numFailed2, 10);

    const auto &spectrumInfo2 = work_out->spectrumInfo();

    // Check the dead detectors found agrees with what was setup above
    for (int i = 0; i < sizey; i++) {
      const double val = work_out->y(i)[0];
      double valExpected = liveValue;
      // Check masking
      IDetector_const_sptr det;
      TS_ASSERT_EQUALS(spectrumInfo2.hasDetectors(i), true);
      // Spectra set up with yVeryDead fail low counts or yStrange fail on high
      if (i % 2 == 0) {
        valExpected = maskValue;
      }

      TS_ASSERT_DELTA(val, valExpected, 1e-9);
    }

    AnalysisDataService::Instance().remove("testdead_in");
    AnalysisDataService::Instance().remove("testdead_out");
  }

  void testExec_Event() {
    // Make a workspace with 50 pixels, 200 events per pixel.
    EventWorkspace_sptr work_in = WorkspaceCreationHelper::createEventWorkspace(
        50, 100, 100, 0.0, 1.0, 2, 1);
    Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentCylindrical(10);
    work_in->setInstrument(inst);
    DateAndTime run_start("2010-01-01T00:00:00");
    // Add ten more at #10 so that it fails
    for (int i = 0; i < 10; i++)
      work_in->getSpectrum(10).addEventQuickly(
          TofEvent((i + 0.5), run_start + double(i)));

    AnalysisDataService::Instance().add("testdead_in", work_in);

    FindDetectorsOutsideLimits alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", "testdead_in");
    alg.setPropertyValue("OutputWorkspace", "testdead_out");
    alg.setPropertyValue("LowThreshold", "1");
    alg.setPropertyValue("HighThreshold", "201");
    alg.setPropertyValue("RangeLower", "-1");
    alg.setPropertyValue("RangeUpper", "1000");
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr work_out;
    TS_ASSERT_THROWS_NOTHING(
        work_out = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "testdead_out"));

    TS_ASSERT_EQUALS(work_out->y(0)[0], 0.0);
    TS_ASSERT_EQUALS(work_out->y(9)[0], 0.0);
    TS_ASSERT_EQUALS(work_out->y(10)[0], 1.0);
    TS_ASSERT_EQUALS(work_out->y(11)[0], 0.0);

    AnalysisDataService::Instance().remove("testdead_in");
    AnalysisDataService::Instance().remove("testdead_out");
  }
};

#endif /*FINDDETECTORSOUTSIDELIMITSTEST_H_*/
