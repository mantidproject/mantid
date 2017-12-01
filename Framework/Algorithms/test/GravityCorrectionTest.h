#ifndef GRAVITYCORRECTIONTEST_H
#define GRAVITYCORRECTIONTEST_H

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CompareWorkspaces.h"
#include "MantidAlgorithms/GravityCorrection.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/V3D.h"
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
    GravityCorrection gc4;
    this->runGravityCorrection(gc4, inWS1, "outWSName1");

    GravityCorrection gc5;
    this->runGravityCorrection(gc5, inWS1, "outWSName2");

    const std::string dataCheck = "1";
    const std::string instrumentCheck = "0";
    const std::string axesCheck = "1";
    CompareWorkspaces beamInvariant;
    comparer(beamInvariant, "outWSName1", "outWSName2", dataCheck,
             instrumentCheck, axesCheck);
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
    // A test workspace
    Mantid::Kernel::V3D slit1, slit2, sample, source, monitor, detector;

    source = Mantid::Kernel::V3D(0., 0., 0.);
    monitor = Mantid::Kernel::V3D(0.5, 0., 0.);
    slit1 = Mantid::Kernel::V3D(1., 0., 0.);
    slit2 = Mantid::Kernel::V3D(2., 0., 0.);
    sample = Mantid::Kernel::V3D(3., 0., 0.);
    detector = Mantid::Kernel::V3D(4., 0., 0.);
    const std::string noDataMismatch = PROPERTY_VALUE_TRUE;

    Mantid::API::MatrixWorkspace_sptr wsSlitA{
        WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument(
            0.5, slit1, slit2, 0.2, 0.2, source, monitor, sample, detector)};
    // First algorithm run
    GravityCorrection gc7;
    this->runGravityCorrection(gc7, wsSlitA, "out1", "slit1", "slit2");
    // Second algorithm run
    GravityCorrection gc8;
    this->runGravityCorrection(gc8, wsSlitA, "out2", "slit2", "slit1");
    // Output workspace comparison
    CompareWorkspaces slitInvariant1;
    comparer(slitInvariant1, "out1", "out2", noDataMismatch);

    Mantid::Kernel::V3D minus{-1., -1., -1.};
    Mantid::API::MatrixWorkspace_sptr wsSlitB{
        WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument(
            0.5, minus * slit1, minus * slit2, 0.2, 0.2, minus * source,
            minus * monitor, minus * sample, minus * detector)};
    // First algorithm run
    GravityCorrection gc14;
    this->runGravityCorrection(gc14, wsSlitB, "out3", "slit1", "slit2");
    // Second algorithm run
    GravityCorrection gc15;
    this->runGravityCorrection(gc15, wsSlitB, "out4", "slit2", "slit1");
    // Output workspace comparison
    CompareWorkspaces slitInvariant2;
    comparer(slitInvariant2, "out3", "out4", noDataMismatch);

    // Output workspace comparison
    const std::string detectorInfoMismatch = PROPERTY_VALUE_FALSE;
    CompareWorkspaces slitInvariant3;
    comparer(slitInvariant3, "out1", "out4", detectorInfoMismatch);
    if (slitInvariant3.getPropertyValue("Result") == detectorInfoMismatch) {
      // check explicitly that the messages are only for data!
      Mantid::API::ITableWorkspace_sptr table =
          Mantid::API::AnalysisDataService::Instance()
              .retrieveWS<Mantid::API::ITableWorkspace>("compare_msgs");
      TS_ASSERT_EQUALS(table->cell<std::string>(0, 0),
                       "DetectorInfo mismatch (position differences larger "
                       "than 1e-9 m or other difference found)");
      TS_ASSERT_THROWS_ANYTHING(table->cell<std::string>(1, 0));
    }
  }

  void testInstrumentUnchanged() {
    GravityCorrection gc9;
    this->runGravityCorrection(gc9, inWS1, outWSName);
    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::AnalysisDataService::Instance().add(inWSName, inWS1));
    CompareWorkspaces instrumentNotModified;
    const std::string dataMismatch = PROPERTY_VALUE_FALSE;
    comparer(instrumentNotModified, inWS1->getName(), outWSName, dataMismatch);
    if (instrumentNotModified.getPropertyValue("Result") == dataMismatch) {
      // check explicitly that the messages are only for data!
      Mantid::API::ITableWorkspace_sptr table =
          Mantid::API::AnalysisDataService::Instance()
              .retrieveWS<Mantid::API::ITableWorkspace>("compare_msgs");
      TS_ASSERT_EQUALS(table->cell<std::string>(0, 0), "Data mismatch");
      TS_ASSERT_THROWS_ANYTHING(table->cell<std::string>(1, 0));
    }
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

  void testMonitor() {
    GravityCorrection gc12;
    auto ws2 = this->runGravityCorrection(gc12, inWS1, "out1");
    // spectrum 1 is a monitor, compare input and output spectrum 1
    for (size_t i = 0; i < ws2->blocksize(); ++i) {
      TS_ASSERT_EQUALS(ws2->x(1)[i], inWS1->x(1)[i]);
      TS_ASSERT_EQUALS(ws2->y(1)[i], inWS1->y(1)[i]);
      TS_ASSERT_EQUALS(ws2->e(1)[i], inWS1->e(1)[i]);
    }
  }

  void testSizes() {
    GravityCorrection gc13;
    auto ws3 = this->runGravityCorrection(gc13, inWS1, "out1");
    TSM_ASSERT_EQUALS("Number indexable items", ws3->size(), inWS1->size());
    TSM_ASSERT_EQUALS("Number of bins", ws3->blocksize(), inWS1->blocksize());
    TSM_ASSERT_EQUALS("Number of spectra", ws3->getNumberHistograms(),
                      inWS1->getNumberHistograms());
  }

  void testInputWorkspace1D() {}

  void testInputWorkspace2D() {}

  void testDetectorMask() {}

  void testReflectionUp() {} // real data

  void testReflectionDown() {} // real data

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

  void comparer(CompareWorkspaces &compare, const std::string &in1,
                const std::string &in2, const std::string property_value,
                std::string property_instrument = "1",
                std::string property_axes = "0") {
    // Output workspace comparison
    compare.initialize();
    TS_ASSERT_THROWS_NOTHING(compare.setPropertyValue("Workspace1", in1));
    compare.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(compare.setProperty("Workspace2", in2));
    TS_ASSERT_THROWS_NOTHING(
        compare.setProperty("CheckInstrument", property_instrument));
    TS_ASSERT_THROWS_NOTHING(compare.setProperty("CheckAxes", property_axes));
    TS_ASSERT(compare.execute());
    TS_ASSERT(compare.isExecuted());
    TS_ASSERT_EQUALS(compare.getPropertyValue("Result"), property_value);
  }

private:
  const std::string outWSName{"GravityCorrectionTest_OutputWorkspace"};
  const std::string inWSName{"GravityCorrectionTest_InputWorkspace"};
  const std::string PROPERTY_VALUE_TRUE{"1"};
  const std::string PROPERTY_VALUE_FALSE{"0"};
  Mantid::API::MatrixWorkspace_sptr inWS1{
      WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument()};
  Mantid::API::MatrixWorkspace_sptr inWS3{
      WorkspaceCreationHelper::
          create2DWorkspaceWithReflectometryInstrumentMultiDetector(
              0.5, 0.25, 4, 50, 0.02, Mantid::Kernel::V3D(-3., 40., 0.),
              Mantid::Kernel::V3D(-2., 29.669, 0.),
              Mantid::Kernel::V3D(-5.94366667, 52.99776017, 0.),
              Mantid::Kernel::V3D(1., 0., 0.), Mantid::Kernel::V3D(0., 0., 0.),
              Mantid::Kernel::V3D(0.854, 35.73, 0.))};
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
