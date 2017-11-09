#ifndef GRAVITYCORRECTIONTEST_H
#define GRAVITYCORRECTIONTEST_H

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CompareWorkspaces.h"
#include "MantidAlgorithms/GravityCorrection.h"
//#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

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
    TS_ASSERT_THROWS_NOTHING(gravCorr.initialize());
    gravCorr.setRethrows(true);
    TS_ASSERT(gravCorr.isInitialized());
  }

  void testInput() {
    TS_ASSERT_THROWS_NOTHING(gravCorr.initialize());
    gravCorr.setRethrows(true);

  }

  // Counts moved
  void testOutputThetaFinalCorrected() {}

  // TOF values modified
  void testOutputTOFCorrected() {}

  // Check different detectors (point, line, area, also invalid detectors)
  void testDetectorLineModel() {}

  void testBeamDirectionInvariant() {
    CompareWorkspaces instrumentNotModified;
    if (!instrumentNotModified.isInitialized())
      instrumentNotModified.initialize();
    this->runGravityCorrection(inWS1, "outWSName1");
    TS_ASSERT_THROWS_NOTHING(
        instrumentNotModified.setProperty("Workspace1", "outWSName1"));
    this->runGravityCorrection(inWS2, "outWSName2");
    TS_ASSERT_THROWS_NOTHING(
        instrumentNotModified.setProperty("Workspace2", "outWSName2"));
    TS_ASSERT_THROWS_NOTHING(instrumentNotModified.setProperty(
        "CheckInstrument", PROPERTY_VALUE_FALSE));
    TS_ASSERT_THROWS_NOTHING(
        instrumentNotModified.setProperty("CheckAxes", PROPERTY_VALUE_TRUE));
    TS_ASSERT(instrumentNotModified.execute());
    TS_ASSERT_EQUALS(instrumentNotModified.getPropertyValue("Result"),
                     PROPERTY_VALUE_TRUE);
  }

  void testSlitInputInvariant() {
    // First algorithm run
    this->runGravityCorrection(inWS1, "out1");
    // Second algorithm run
    std::string slit1Name = "slit2";
    std::string slit2Name = "slit1";
    this->runGravityCorrection(inWS1, "out2", slit1Name, slit2Name);
    // Output workspace comparison
    CompareWorkspaces compare;
    if (!compare.isInitialized())
      compare.initialize();
    TS_ASSERT_THROWS_NOTHING(compare.setPropertyValue("Workspace1", "out1"));
    compare.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(compare.setProperty("Workspace2", "out2"));
    TS_ASSERT(compare.execute());
    TS_ASSERT(compare.isExecuted());
    TSM_ASSERT_EQUALS(
        "Altering the names of the slit inputs changes the OutputWorkspace",
        compare.getPropertyValue("Result"), PROPERTY_VALUE_TRUE);
  }

  void testInstrumentUnchanged() {
    this->runGravityCorrection(inWS1, outWSName);
    TSM_ASSERT_THROWS_NOTHING(
        "Can't store workspace in ADS",
        WorkspaceCreationHelper::storeWS(inWSName, inWS1));
    //    Mantid::API::AnalysisDataService::Instance().addOrReplace(inWSName,
    //                                                              inWS1));
    TS_ASSERT_EQUALS(inWSName, inWS1->getName());
    CompareWorkspaces instrumentNotModified;
    if (!instrumentNotModified.isInitialized())
      instrumentNotModified.initialize();
    TS_ASSERT_THROWS_NOTHING(
        instrumentNotModified.setProperty("Workspace1", inWS1->getName()));
    TS_ASSERT_THROWS_NOTHING(
        instrumentNotModified.setProperty("Workspace2", outWSName));
    TS_ASSERT_THROWS_NOTHING(instrumentNotModified.setProperty(
        "CheckInstrument", PROPERTY_VALUE_TRUE));
    TS_ASSERT_THROWS_NOTHING(
        instrumentNotModified.setProperty("CheckAxes", PROPERTY_VALUE_FALSE));
    TS_ASSERT(instrumentNotModified.execute());
    TS_ASSERT_EQUALS(instrumentNotModified.getPropertyValue("Result"),
                     PROPERTY_VALUE_TRUE);
    TS_ASSERT_THROWS_NOTHING(WorkspaceCreationHelper::removeWS(inWSName));
    // if (Mantid::API::AnalysisDataService::Instance().doesExist(inWSName))
    //  TS_ASSERT_THROWS_NOTHING(
    //      Mantid::API::AnalysisDataService::Instance().clear());
  }

  void testInputWorkspace1D() {}

  void testInputWorkspace2D() {}

  Mantid::API::MatrixWorkspace_sptr runGravityCorrection(
      Mantid::API::MatrixWorkspace_sptr &inWS, const std::string outName,
      std::string firstSlitName = "", std::string secondSlitName = "") {
    TS_ASSERT_THROWS_NOTHING(gravCorr.initialize());
    gravCorr.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(gravCorr.setProperty("InputWorkspace", inWS));
    TS_ASSERT_THROWS_NOTHING(gravCorr.setProperty("OutputWorkspace", outName));
    if (!firstSlitName.empty())
      TSM_ASSERT_THROWS_NOTHING(
          "FirstSlitName should be slit2",
          gravCorr.setProperty("FirstSlitName", firstSlitName));
    if (!secondSlitName.empty())
      TSM_ASSERT_THROWS_NOTHING(
          "SecondSlitName should be slit1",
          gravCorr.setProperty("SecondSlitName", secondSlitName));
    TS_ASSERT_THROWS_NOTHING(gravCorr.execute());
    TS_ASSERT(gravCorr.isExecuted());
    return gravCorr.getProperty("OutputWorkspace");
  }

private:
  GravityCorrection gravCorr;
  const std::string outWSName{"GravityCorrectionTest_OutputWorkspace"};
  const std::string inWSName{"GravityCorrectionTest_InputWorkspace"};
  const std::string PROPERTY_VALUE_TRUE{"1"};
  const std::string PROPERTY_VALUE_FALSE{"0"};
  Mantid::API::MatrixWorkspace_sptr inWS1{
      WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument()};
  // change ReferenceFrame -> BeamDirection
  Mantid::API::MatrixWorkspace_sptr inWS2{
      WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument()};
  // need more workspaces
};

// add PerformanceTest

#endif // GRAVITYCORRECTIONTEST_H
