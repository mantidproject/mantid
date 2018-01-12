#ifndef GRAVITYCORRECTIONTEST_H
#define GRAVITYCORRECTIONTEST_H

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/CompareWorkspaces.h"
#include "MantidAlgorithms/GravityCorrection.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cmath>

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

  void testSlitPosDiffers() {
    Mantid::Kernel::V3D slit = Mantid::Kernel::V3D(2., 0., 0.);

    Mantid::API::MatrixWorkspace_sptr ws1{
        WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument(
            0.5, slit, slit, 0.2, 0.2, source, monitor, sample, detector)};
    GravityCorrection gc21;
    TS_ASSERT_THROWS_NOTHING(gc21.initialize());
    gc21.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(gc21.setProperty("InputWorkspace", ws1));
    TS_ASSERT_THROWS_NOTHING(gc21.setProperty("OutputWorkspace", "ws1out"));
    TSM_ASSERT_THROWS_NOTHING("Position of slits must differ",
                              gc21.setProperty("SecondSlitName", "slit2"));
    TS_ASSERT_THROWS_ANYTHING(gc21.execute());
    TS_ASSERT(!gc21.isExecuted());
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

  void testSlitInputInvariant() {
    // First algorithm run
    GravityCorrection gc7;
    this->runGravityCorrection(gc7, inWS1, "out1", "slit1", "slit2");
    // Second algorithm run
    GravityCorrection gc8;
    this->runGravityCorrection(gc8, inWS1, "out2", "slit2", "slit1");
    // Output workspace comparison
    CompareWorkspaces slitInvariant1;
    comparer(slitInvariant1, "out1", "out2", TRUE);

    Mantid::Kernel::V3D minus{-1., -1., -1.};
    Mantid::API::MatrixWorkspace_sptr wsSlitB{
        WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument(
            0.5, minus * s1, minus * s2, 0.2, 0.2, minus * source,
            minus * monitor, minus * sample, minus * detector)};
    // First algorithm run
    GravityCorrection gc14;
    this->runGravityCorrection(gc14, wsSlitB, "out3", "slit1", "slit2");
    // Second algorithm run
    GravityCorrection gc15;
    this->runGravityCorrection(gc15, wsSlitB, "out4", "slit2", "slit1");
    // Output workspace comparison
    CompareWorkspaces slitInvariant2;
    comparer(slitInvariant2, "out3", "out4", TRUE);

    // Output workspace comparison
    CompareWorkspaces slitInvariant3;
    comparer(slitInvariant3, "out1", "out4", FALSE);
    if (slitInvariant3.getPropertyValue("Result") == FALSE) {
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
    comparer(instrumentNotModified, inWS1->getName(), outWSName, FALSE);
    if (instrumentNotModified.getPropertyValue("Result") == FALSE) {
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

  void testInstrumentRotation() {
    // a rotation of the instrument should not vary the output of the gravity
    // correction. but only if the rotation angle is given as input.

    auto ws =
        WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument();

    std::vector<std::string> componentNames = {"source", "some-surface-holder",
                                               "slit1", "slit2"};

    for (const auto &component : componentNames) {
      const auto ID =
          ws->getInstrument()->getComponentByName(component)->getComponentID();
      double x =
          ws->getInstrument()->getComponentByName(component)->getPos().X();
      // new rotation
      Mantid::Kernel::Quat rot =
          Mantid::Kernel::Quat(30., Mantid::Kernel::V3D(0., 1., 0.)) *
          ws->getInstrument()->getComponentByName(component)->getRotation();
      ws->mutableComponentInfo().setRotation(
          ws->mutableComponentInfo().indexOf(ID), rot);
      // new position
      Mantid::Kernel::V3D pos{cos(30.) * x, sin(30.) * x, 0.};
      ws->mutableComponentInfo().setPosition(
          ws->mutableComponentInfo().indexOf(ID), pos);
    }

    // sample should not be at (15., 0., 0.) position test:
    TS_ASSERT_DIFFERS(ws->getInstrument()->getSample()->getPos(),
                      Mantid::Kernel::V3D(15., 0., 0.));

    GravityCorrection gc16;
    this->runGravityCorrection(gc16, ws, "out1", "slit1",
                               "slit2"); // angle not an input anymore

    GravityCorrection gc17;
    this->runGravityCorrection(gc17, inWS1, "out2");

    CompareWorkspaces rotatedWS;
    comparer(rotatedWS, "out1", "out2", "1", "0", "1");
    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::AnalysisDataService::Instance().clear());
  }

  void testInstrumentTranslation() {
    Mantid::Kernel::V3D translate = Mantid::Kernel::V3D(-5., 0., 0.);
    auto origin =
        WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument(
            0.0, s1, s2, 0.5, 1.0, source, monitor, sample, detector);
    auto translated =
        WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument(
            0.0, s1 - translate, s2 - translate, 0.5, 1.0, source - translate,
            monitor - translate, sample - translate, detector - translate);

    GravityCorrection gc18;
    this->runGravityCorrection(gc18, origin, "origin");

    GravityCorrection gc19;
    this->runGravityCorrection(gc19, translated, "translated");

    // ws4 and ws5 must be identical
    CompareWorkspaces translatedWS;
    comparer(translatedWS, "origin", "translated", "1", "0", "1");
    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::AnalysisDataService::Instance().clear());
  }

  void testInputWorkspace1D() {}

  void testInputWorkspace2D() {}

  void testDetectorMask() {}

  void testReflectionUp() {} // real data

  void testReflectionDown() {} // real data

  // Counts moved
  void testOutputThetaFinalCorrected() {

    // ReferenceFrame is up:Y along:X

    Mantid::Kernel::V3D source{-6., 0., 0.};
    Mantid::Kernel::V3D slit1{-1., 0., 0.};
    Mantid::Kernel::V3D slit2{-5., 0., 0.};
    Mantid::Kernel::V3D monitor{-1., 0., 0.};
    Mantid::Kernel::V3D sample{0., 0., 0.};
    double ys = 30.;
    double xs = 5.;
    Mantid::Kernel::V3D detector2{xs, ys, 0.};

    double tof = 0.5;

    double ySlit1 = -1. * tan(ys / xs);
    double ySlit2 = -.5 * tan(ys / xs);

    using Mantid::PhysicalConstants::g;
    // put length of parabola instead of direct path length
    double v0 = ySlit1 / tof;

    double k = g / (2 * pow(v0, 2));

    double x0 =
        (k * (1 - pow(0.5, 2)) + (ySlit1 - ySlit2)) / (2 * k * (-1 + .5));
    double y0 = ySlit1 + k * (-1 - x0);

    double finalAngle = atan(2 * k * sqrt(y0 / k));

    double v = sqrt(pow(v0, 2) * pow(cos(finalAngle), 2) +
                    (v0 * sin(finalAngle) - (g * tof)));
    double zDet = v * cos(finalAngle) * tof;

    double t = 0.;
    double yDet = v * sin(finalAngle) * tof - .5 * g * pow(t, 2);

    Mantid::Kernel::V3D detector1{0., yDet, zDet};

    Mantid::API::MatrixWorkspace_sptr ws{
        WorkspaceCreationHelper::
            create2DWorkspaceWithReflectometryInstrumentMultiDetector(
                tof, 0.25, 4, 50, 0.02, slit1, slit2, source, monitor, sample,
                detector1, detector2)};

    // input final angle
    TS_ASSERT_DELTA(ws->detectorInfo().signedTwoTheta(4), 2. * 1.0405648, 1e-6);
    // input counts
    // error: no match for ‘operator==’ (operand types are
    // ‘Mantid::HistogramData::HistogramY’ and ‘int’)
    // TS_ASSERT_EQUALS(ws->y(3), 2);

    GravityCorrection gc20;
    auto res = this->runGravityCorrection(gc20, ws, "ws");

    // resulting final angle
    TS_ASSERT_DELTA(ws->detectorInfo().signedTwoTheta(3), 2. * finalAngle,
                    1e-6);
    // resulting counts
    // TS_ASSERT_EQUALS(ws->y(4), 0); // counts removed
    // TS_ASSERT_EQUALS(ws->y(3), 2); // counts inserted
  }

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
  const std::string TRUE{"1"};
  const std::string FALSE{"0"};
  Mantid::Kernel::V3D source = Mantid::Kernel::V3D(0., 0., 0.);
  Mantid::Kernel::V3D monitor = Mantid::Kernel::V3D(0.5, 0., 0.);
  Mantid::Kernel::V3D s1 = Mantid::Kernel::V3D(1., 0., 0.);
  Mantid::Kernel::V3D s2 = Mantid::Kernel::V3D(2., 0., 0.);
  Mantid::Kernel::V3D sample = Mantid::Kernel::V3D(3., 0., 0.);
  Mantid::Kernel::V3D detector = Mantid::Kernel::V3D(4., 4., 0.);
  Mantid::API::MatrixWorkspace_sptr inWS1{
      WorkspaceCreationHelper::create2DWorkspaceWithReflectometryInstrument(
          0.0, s1, s2, 0.5, 1.0, source, monitor, sample, detector, 2, 100,
          2000.0)};
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
