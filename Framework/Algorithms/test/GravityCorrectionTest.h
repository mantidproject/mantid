#ifndef GRAVITYCORRECTIONTEST_H
#define GRAVITYCORRECTIONTEST_H

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/GravityCorrection.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/UnitFactory.h"

//#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::Algorithms;

class GravityCorrectionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GravityCorrectionTest *createSuite() {
    return new GravityCorrectionTest();
  }
  static void destroySuite(GravityCorrectionTest *suite) { delete suite; }

  void testName() { TS_ASSERT_EQUALS(gravCorr.name(), "GravityCorrection"); }

  void testCategory() { TS_ASSERT_EQUALS(gravCorr.category(), "Reflectometry") }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(gravCorr.initialize())
    TS_ASSERT(gravCorr.isInitialized())
  }

  // test of the input validation
  void testInputWorkspaceExists() {}

  // OutputWorkspace must have unit TOF
  void testOutputTOF() {}

  // OutputWorkspace must have corrected final angles
  void testOutputThetaFinalCorrected() {}

  // OutputWorkspace must have corrected TOF values
  void testOutputTOFCorrected() {}

  // test if detector can be modeled as a line in x-y plane
  void testDetectorLineModel() {}

  // test mandatory user input (component name of the two slits)
  void testSlits() {
    double startX = 0.005;
    Mantid::API::MatrixWorkspace_sptr inWS1 =
        WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument(
            startX);
    const std::string outWSName("GravityCorrectionTest");
    TS_ASSERT_THROWS_NOTHING(gravCorr.initialize());
    gravCorr.setRethrows(true);
    TS_ASSERT(gravCorr.isInitialized());
    TS_ASSERT_THROWS_NOTHING(gravCorr.setProperty("InputWorkspace", inWS1));
    TS_ASSERT_THROWS_NOTHING(
        gravCorr.setProperty("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(
        gravCorr.setPropertyValue("FirstSlitName", "slit1"));
    TS_ASSERT_THROWS_NOTHING(
        gravCorr.setPropertyValue("SecondSlitName", "slit2"));
    TS_ASSERT_THROWS_ANYTHING(gravCorr.execute());
  }

  void testInputWorkspace1D() {

    double startX = 0.005;
    Mantid::API::MatrixWorkspace_sptr inWS1 =
        WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument(
            startX);

    //Mantid::Geometry::Instrument_sptr instrument = boost::make_shared<Instrument>();
    auto s1 = new Mantid::Geometry::ObjComponent("slit1");
    //s1->setPos(s1Pos);
    //instrument->add(s1);

    //auto s2 = new Mantid::Geometry::ObjComponent("slit2");
    //s2->setPos(s2Pos);
    //instrument->add(s2);

    const std::string outWSName("GravityCorrectionTest");
    TS_ASSERT_THROWS_NOTHING(gravCorr.initialize());
    TS_ASSERT(gravCorr.isInitialized());
    TS_ASSERT_THROWS_NOTHING(gravCorr.setProperty("InputWorkspace", inWS1));
    TS_ASSERT_THROWS_NOTHING(
        gravCorr.setProperty("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(
        gravCorr.setPropertyValue("FirstSlitName", "slit1"));
    TS_ASSERT_THROWS_NOTHING(
        gravCorr.setPropertyValue("SecondSlitName", "slit2"));
    gravCorr.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(gravCorr.execute());

    // Mantid::DataObjects::Workspace2D_sptr outWS =
    // Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::DataObjects::Workspace2D_sptr>(outWSName);
    // TS_ASSERT_EQUALS(outWS->getAxis(0)->title(), "TOF");
    // Mantid::API::AnalysisDataService::Instance().remove("Input");
    // Mantid::API::AnalysisDataService::Instance().remove(outWSName);
  }

  // InputWorkspace of 2D results of an OutputWorkspace of same size, theta, TOF
  // modified
  void testInputWorkspace2D() {
    // const double detSize = 0.122;
    // Mantid::API::MatrixWorkspace_const_sptr inWS2 =
    // WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrumentMultiDetector(
    //   startX, detSize);
  }

private:
  GravityCorrection gravCorr;
};

// add PerformanceTest

#endif // GRAVITYCORRECTIONTEST_H
