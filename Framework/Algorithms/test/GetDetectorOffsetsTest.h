#ifndef GETDETECTOROFFSETSTEST_H_
#define GETDETECTOROFFSETSTEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAlgorithms/GetDetectorOffsets.h"
#include "MantidDataObjects/OffsetsWorkspace.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using Mantid::Algorithms::GetDetectorOffsets;
using Mantid::DataObjects::OffsetsWorkspace_sptr;

class GetDetectorOffsetsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GetDetectorOffsetsTest *createSuite() {
    return new GetDetectorOffsetsTest();
  }
  static void destroySuite(GetDetectorOffsetsTest *suite) { delete suite; }

  GetDetectorOffsetsTest() { Mantid::API::FrameworkManager::Instance(); }

  void testTheBasics() {
    TS_ASSERT_EQUALS(offsets.name(), "GetDetectorOffsets");
    TS_ASSERT_EQUALS(offsets.version(), 1);
  }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(offsets.initialize());
    TS_ASSERT(offsets.isInitialized());
  }

  void testExec() {
    // ---- Create the simple workspace -------
    MatrixWorkspace_sptr WS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 200);
    WS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("dSpacing");

    auto xvals = WS->points(0);
    // loop through xvals, calculate and set to Y
    std::transform(
        xvals.cbegin(), xvals.cend(), WS->mutableY(0).begin(),
        [](const double x) { return exp(-0.5 * pow((x - 1) / 10.0, 2)); });

    auto &E = WS->mutableE(0);
    E.assign(E.size(), 0.001);

    // ---- Run algo -----
    if (!offsets.isInitialized())
      offsets.initialize();
    TS_ASSERT_THROWS_NOTHING(offsets.setProperty("InputWorkspace", WS));
    std::string outputWS("offsetsped");
    std::string maskWS("masksped");
    TS_ASSERT_THROWS_NOTHING(
        offsets.setPropertyValue("OutputWorkspace", outputWS));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("MaskWorkspace", maskWS));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("Step", "0.02"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("DReference", "1.00"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("XMin", "-20"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("XMax", "20"));
    TS_ASSERT_THROWS_NOTHING(offsets.execute());
    TS_ASSERT(offsets.isExecuted());

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputWS));
    if (!output)
      return;

    TS_ASSERT_DELTA(output->y(0)[0], -0.0196, 0.0001);

    AnalysisDataService::Instance().remove(outputWS);

    MatrixWorkspace_const_sptr mask;
    TS_ASSERT_THROWS_NOTHING(
        mask = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            maskWS));
    if (!mask)
      return;
    TS_ASSERT(!mask->detectorInfo().isMasked(0));
  }

  void testExecWithGroup() {
    // --------- Workspace with summed spectra -------
    MatrixWorkspace_sptr WS =
        WorkspaceCreationHelper::createGroupedWorkspace2D(3, 200, 1.0);
    WS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("dSpacing");

    auto xvals = WS->points(0);
    // loop through xvals, calculate and set to Y
    std::transform(
        xvals.cbegin(), xvals.cend(), WS->mutableY(0).begin(),
        [](const double x) { return exp(-0.5 * pow((x - 1) / 10.0, 2)); });

    auto &E = WS->mutableE(0);
    E.assign(E.size(), 0.001);

    // ---- Run algo -----
    if (!offsets.isInitialized())
      offsets.initialize();
    TS_ASSERT_THROWS_NOTHING(offsets.setProperty("InputWorkspace", WS));
    std::string outputWS("offsetsped");
    std::string maskWS("masksped");
    TS_ASSERT_THROWS_NOTHING(
        offsets.setPropertyValue("OutputWorkspace", outputWS));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("MaskWorkspace", maskWS));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("Step", "0.02"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("DReference", "1.00"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("XMin", "-20"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("XMax", "20"));
    TS_ASSERT_THROWS_NOTHING(offsets.execute());
    TS_ASSERT(offsets.isExecuted());

    OffsetsWorkspace_sptr output = offsets.getProperty("OutputWorkspace");
    if (!output)
      return;

    TS_ASSERT_DELTA(output->getValue(1), -0.0196, 0.0001);
    TS_ASSERT_EQUALS(output->getValue(1), output->getValue(2));
    TS_ASSERT_EQUALS(output->getValue(1), output->getValue(3));

    AnalysisDataService::Instance().remove(outputWS);

    MatrixWorkspace_const_sptr mask;
    TS_ASSERT_THROWS_NOTHING(
        mask = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            maskWS));
    if (!mask)
      return;
    TS_ASSERT(!mask->detectorInfo().isMasked(0));
  }

  void testExecAbsolute() {
    // ---- Create the simple workspace -------
    MatrixWorkspace_sptr WS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(1, 200);
    WS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("dSpacing");

    auto xvals = WS->points(0);
    // loop through xvals, calculate and set to Y
    std::transform(
        xvals.cbegin(), xvals.cend(), WS->mutableY(0).begin(),
        [](const double x) { return exp(-0.5 * pow((x - 1) / 10.0, 2)); });
    auto &E = WS->mutableE(0);
    E.assign(E.size(), 0.001);

    // ---- Run algo -----
    if (!offsets.isInitialized())
      offsets.initialize();
    TS_ASSERT_THROWS_NOTHING(offsets.setProperty("InputWorkspace", WS));
    std::string outputWS("offsetsped");
    std::string maskWS("masksped");
    TS_ASSERT_THROWS_NOTHING(
        offsets.setPropertyValue("OutputWorkspace", outputWS));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("MaskWorkspace", maskWS));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("Step", "0.02"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("DReference", "1.00"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("XMin", "-20"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("XMax", "20"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("MaxOffset", "10"));
    TS_ASSERT_THROWS_NOTHING(
        offsets.setPropertyValue("OffsetMode", "Absolute"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("DIdeal", "3.5"));
    TS_ASSERT_THROWS_NOTHING(offsets.execute());
    TS_ASSERT(offsets.isExecuted());

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            outputWS));
    if (!output)
      return;

    TS_ASSERT_DELTA(output->y(0)[0], 2.4803, 0.0001);

    AnalysisDataService::Instance().remove(outputWS);

    MatrixWorkspace_const_sptr mask;
    TS_ASSERT_THROWS_NOTHING(
        mask = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            maskWS));
    if (!mask)
      return;
    TS_ASSERT(!mask->detectorInfo().isMasked(0));
  }

private:
  GetDetectorOffsets offsets;
};

class GetDetectorOffsetsTestPerformance : public CxxTest::TestSuite {
  MatrixWorkspace_sptr WS;
  int numpixels;

public:
  static GetDetectorOffsetsTestPerformance *createSuite() {
    return new GetDetectorOffsetsTestPerformance();
  }
  static void destroySuite(GetDetectorOffsetsTestPerformance *suite) {
    delete suite;
  }

  GetDetectorOffsetsTestPerformance() { FrameworkManager::Instance(); }

  void setUp() override {
    numpixels = 10000;
    WS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
        numpixels, 200, false);
    WS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("dSpacing");
    for (size_t wi = 0; wi < WS->getNumberHistograms(); wi++) {

      auto xvals = WS->points(wi);
      auto &Y = WS->mutableY(wi);

      std::transform(
          xvals.cbegin(), xvals.cend(), Y.begin(),
          [](const double x) { return exp(-0.5 * pow((x - 1) / 10.0, 2)); });
      auto &E = WS->mutableE(wi);
      E.assign(E.size(), 0.001);
    }
  }

  void test_performance() {
    AlgorithmManager::Instance(); // Initialize here to avoid an odd ABORT
    GetDetectorOffsets offsets;
    if (!offsets.isInitialized())
      offsets.initialize();
    TS_ASSERT_THROWS_NOTHING(offsets.setProperty("InputWorkspace", WS));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("Step", "0.02"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("DReference", "1.00"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("XMin", "-20"));
    TS_ASSERT_THROWS_NOTHING(offsets.setPropertyValue("XMax", "20"));
    TS_ASSERT_THROWS_NOTHING(
        offsets.setPropertyValue("OutputWorkspace", "dummyname"));
    TS_ASSERT_THROWS_NOTHING(offsets.execute());
    TS_ASSERT(offsets.isExecuted());
    OffsetsWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = offsets.getProperty("OutputWorkspace"));
    if (!output)
      return;
    TS_ASSERT_DELTA(output->mutableY(0)[0], -0.0196, 0.0001);
  }
};

#endif /*GETDETECTOROFFSETSTEST_H_*/
