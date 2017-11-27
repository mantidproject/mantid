#ifndef GRAVITYCORRECTIONTEST_H
#define GRAVITYCORRECTIONTEST_H

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CompareWorkspaces.h"
#include "MantidAlgorithms/GravityCorrection.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
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

  void testName() {
    GravityCorrection gc0;
    TS_ASSERT_EQUALS(gc0.name(), "GravityCorrection");
  }

  void testCategory() {
    GravityCorrection gc1;
    TS_ASSERT_EQUALS(gc1.category(), "Reflectometry")
  }

  void testInit() {
    GravityCorrection gc2;
    TS_ASSERT_THROWS_NOTHING(gc2.initialize());
    gc2.setRethrows(true);
    TS_ASSERT(gc2.isInitialized());
  }

  void testInput() {
    GravityCorrection gc3;
    TS_ASSERT_THROWS_NOTHING(gc3.initialize());
    gc3.setRethrows(true);
  }

  void testBeamDirectionInvariant() {
    CompareWorkspaces instrumentNotModified;
    if (!instrumentNotModified.isInitialized())
      instrumentNotModified.initialize();
    GravityCorrection gc4;
    this->runGravityCorrection(gc4, inWS1, "outWSName1");
    TS_ASSERT_THROWS_NOTHING(
        instrumentNotModified.setProperty("Workspace1", "outWSName1"));
    GravityCorrection gc5;
    this->runGravityCorrection(gc5, inWS2, "outWSName2");
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

  void testInvalidSlitName() {
    GravityCorrection gc6;
    TS_ASSERT_THROWS_NOTHING(gc6.initialize());
    gc6.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(gc6.setProperty("InputWorkspace", inWS1));
    TS_ASSERT_THROWS_NOTHING(gc6.setProperty("OutputWorkspace", "out1"));
    TSM_ASSERT_THROWS_NOTHING("FirstSlitName slitt does not exist",
                              gc6.setProperty("FirstSlitName", "slitt"));
    TS_ASSERT_THROWS_ANYTHING(gc6.execute());
    TS_ASSERT(!gc6.isExecuted());
  }

  void testSlitInputInvariant() {
    // First algorithm run
    GravityCorrection gc7;
    this->runGravityCorrection(gc7, inWS1, "out1", "slit1", "slit2");
    // Second algorithm run
    GravityCorrection gc8;
    this->runGravityCorrection(gc8, inWS1, "out2", "slit1",
                               "slit2"); // change needed here
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
    GravityCorrection gc9;
    this->runGravityCorrection(gc9, inWS1, outWSName);
    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::AnalysisDataService::Instance().add(inWSName, inWS1));
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
    TSM_ASSERT_EQUALS("Do not modify the instrument!",
                      instrumentNotModified.getPropertyValue("Result"),
                      PROPERTY_VALUE_FALSE);
    // check explicitly that the messages are only for data!
    Mantid::API::ITableWorkspace_sptr table =
        Mantid::API::AnalysisDataService::Instance()
            .retrieveWS<Mantid::API::ITableWorkspace>("compare_msgs");
    TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "Data mismatch");
    if (Mantid::API::AnalysisDataService::Instance().doesExist(inWSName))
      TS_ASSERT_THROWS_NOTHING(
          Mantid::API::AnalysisDataService::Instance().clear());
  }

  void testBinMask() {
    Mantid::API::MatrixWorkspace_sptr ws1{
        WorkspaceCreationHelper::
            create2DWorkspaceWithReflectometryInstrument()};
    ws1->flagMasked(0, 4, 0.4);
    ws1->flagMasked(0, 52, 1.0); // fully masked
    ws1->flagMasked(0, 53, 0.1);

    GravityCorrection gc10;
    auto ws2 = this->runGravityCorrection(gc10, ws1, "ws2");
    Mantid::API::MatrixWorkspace::MaskList mList = ws2->maskedBins(0);
    auto iterator = mList.begin();
    TS_ASSERT_EQUALS(iterator->second, 0.4); // calculate new
    ++iterator;
    TS_ASSERT_EQUALS(iterator->second, 1.0); // calculate new
    ++iterator;
    TS_ASSERT_EQUALS(iterator->second, 0.1); // calculate new
  }

  void testHistoryCheck() {
    GravityCorrection gc11;
    auto ws = this->runGravityCorrection(gc11, inWS1, "out1");
    TS_ASSERT_THROWS_NOTHING(gc11.initialize());
    gc11.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(gc11.setProperty("InputWorkspace", ws));
    TS_ASSERT_THROWS_NOTHING(gc11.setProperty("OutputWorkspace", "out2"));
    TSM_ASSERT_THROWS_ANYTHING(
        "Running GravityCorrection again should not be possible",
        gc11.execute());
    TS_ASSERT(gc11.isExecuted());
  }

  void testInputWorkspace1D() {}

  void testInputWorkspace2D() {}

  void testDetectorMask() {}

  // Counts moved
  void testOutputThetaFinalCorrected() {}

  // TOF values modified
  void testOutputTOFCorrected() {}

  Mantid::API::MatrixWorkspace_sptr runGravityCorrection(
      GravityCorrection &gravityCorrection,
      Mantid::API::MatrixWorkspace_sptr &inWS, const std::string outName,
      std::string firstSlitName = "", std::string secondSlitName = "") {
    TS_ASSERT_THROWS_NOTHING(gravityCorrection.initialize());
    gravityCorrection.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(
        gravityCorrection.setProperty("InputWorkspace", inWS));
    TS_ASSERT_THROWS_NOTHING(
        gravityCorrection.setProperty("OutputWorkspace", outName));
    if (!firstSlitName.empty())
      TSM_ASSERT_THROWS_NOTHING(
          "FirstSlitName should be slit2",
          gravityCorrection.setProperty("FirstSlitName", firstSlitName));
    if (!secondSlitName.empty())
      TSM_ASSERT_THROWS_NOTHING(
          "SecondSlitName should be slit1",
          gravityCorrection.setProperty("SecondSlitName", secondSlitName));
    TS_ASSERT_THROWS_NOTHING(gravityCorrection.execute());
    TS_ASSERT(gravityCorrection.isExecuted());
    return Mantid::API::AnalysisDataService::Instance()
        .retrieveWS<Mantid::API::MatrixWorkspace>(outName);
  }

private:
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

// Performance testing
class GravityCorrectionTestPerformance : public CxxTest::TestSuite {
public:
  static GravityCorrectionTestPerformance *createSuite() {
    return new GravityCorrectionTestPerformance();
  }
  static void destroySuite(GravityCorrectionTestPerformance *suite) {
    delete suite;
  }

  GravityCorrectionTestPerformance() {
    Mantid::API::MatrixWorkspace_sptr ws{
        WorkspaceCreationHelper::
            create2DWorkspaceWithReflectometryInstrument()};
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    alg.setProperty("OutputWorkspace", "anon");
  }

  ~GravityCorrectionTestPerformance() override { alg.clear(); }

  void test_performace() { alg.execute(); }

private:
  GravityCorrection alg;
};

#endif // GRAVITYCORRECTIONTEST_H
