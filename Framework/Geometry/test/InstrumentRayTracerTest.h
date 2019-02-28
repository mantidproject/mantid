// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef INSTRUMENTRAYTRACERTEST_H_
#define INSTRUMENTRAYTRACERTEST_H_

#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Objects/InstrumentRayTracer.h"
#include "MantidKernel/ConfigService.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <boost/make_shared.hpp>
#include <cxxtest/TestSuite.h>

using namespace Mantid::Geometry;
using Mantid::Kernel::V3D;

//-------------------------------------------------------------
// Test suite
//-------------------------------------------------------------
class InstrumentRayTracerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InstrumentRayTracerTest *createSuite() {
    return new InstrumentRayTracerTest();
  }
  static void destroySuite(InstrumentRayTracerTest *suite) { delete suite; }

  InstrumentRayTracerTest() : m_testInst() {
    // Start logging framework
    Mantid::Kernel::ConfigService::Instance();
  }

  void test_That_Constructor_Does_Not_Throw_On_Giving_A_Valid_Instrument() {
    boost::shared_ptr<Instrument> testInst =
        boost::make_shared<Instrument>("empty");
    ObjComponent *source = new ObjComponent("moderator", nullptr);
    testInst->add(source);
    testInst->markAsSource(source);
    InstrumentRayTracer *rayTracker(nullptr);
    TS_ASSERT_THROWS_NOTHING(rayTracker = new InstrumentRayTracer(testInst));
    delete rayTracker;
  }

  void
  test_That_Constructor_Throws_Invalid_Argument_On_Giving_A_Null_Instrument() {
    TS_ASSERT_THROWS(new InstrumentRayTracer(boost::shared_ptr<Instrument>()),
                     std::invalid_argument);
  }

  void
  test_That_Constructor_Throws_Invalid_Argument_On_Giving_An_Instrument_With_No_Source() {
    Instrument_sptr testInst(new Instrument("empty"));
    TS_ASSERT_THROWS(new InstrumentRayTracer(testInst), std::invalid_argument);
  }

  void
  test_That_A_Trace_For_A_Ray_That_Intersects_Many_Components_Gives_These_Components_As_A_Result() {
    Instrument_sptr testInst = setupInstrument();
    InstrumentRayTracer tracker(testInst);
    tracker.trace(V3D(0., 0., 1));
    Links results = tracker.getResults();
    TS_ASSERT_EQUALS(results.size(), 2);
    // Check they are actually what we expect: 1 with the sample and 1 with the
    // central detector
    IComponent_const_sptr centralPixel =
        testInst->getComponentByName("pixel-(0;0)");
    IComponent_const_sptr sampleComp = testInst->getSample();

    if (!sampleComp) {
      TS_FAIL("Test instrument has been changed, the sample has been removed. "
              "Ray tracing tests need to be updated.");
      return;
    }
    if (!centralPixel) {
      TS_FAIL("Test instrument has been changed, the instrument config has "
              "changed. Ray tracing tests need to be updated.");
      return;
    }
    Links::const_iterator resultItr = results.begin();
    Link firstIntersect = *resultItr;

    TS_ASSERT_DELTA(firstIntersect.distFromStart, 10.001, 1e-6);
    TS_ASSERT_DELTA(firstIntersect.distInsideObject, 0.002, 1e-6);
    TS_ASSERT_DELTA(firstIntersect.entryPoint.X(), 0.0, 1e-6);
    TS_ASSERT_DELTA(firstIntersect.entryPoint.Y(), 0.0, 1e-6);
    TS_ASSERT_DELTA(firstIntersect.entryPoint.Z(), -0.001, 1e-6);
    TS_ASSERT_DELTA(firstIntersect.exitPoint.X(), 0.0, 1e-6);
    TS_ASSERT_DELTA(firstIntersect.exitPoint.Y(), 0.0, 1e-6);
    TS_ASSERT_DELTA(firstIntersect.exitPoint.Z(), 0.001, 1e-6);
    TS_ASSERT_EQUALS(firstIntersect.componentID, sampleComp->getComponentID());

    ++resultItr;
    Link secondIntersect = *resultItr;
    TS_ASSERT_DELTA(secondIntersect.distFromStart, 15.004, 1e-6);
    TS_ASSERT_DELTA(secondIntersect.distInsideObject, 0.008, 1e-6);
    TS_ASSERT_DELTA(secondIntersect.entryPoint.X(), 0.0, 1e-6);
    TS_ASSERT_DELTA(secondIntersect.entryPoint.Y(), 0.0, 1e-6);
    TS_ASSERT_DELTA(secondIntersect.entryPoint.Z(), 4.996, 1e-6);
    TS_ASSERT_DELTA(secondIntersect.exitPoint.X(), 0.0, 1e-6);
    TS_ASSERT_DELTA(secondIntersect.exitPoint.Y(), 0.0, 1e-6);
    TS_ASSERT_DELTA(secondIntersect.exitPoint.Z(), 5.004, 1e-6);
    TS_ASSERT_EQUALS(secondIntersect.componentID,
                     centralPixel->getComponentID());

    // Results vector should be empty after first getResults call
    results = tracker.getResults();
    TS_ASSERT_EQUALS(results.size(), 0);

    // Results vector should be empty after first getResults call
    results = tracker.getResults();
    TS_ASSERT_EQUALS(results.size(), 0);
  }

  void
  test_That_A_Ray_Which_Just_Intersects_One_Component_Gives_This_Component_Only() {
    Instrument_sptr testInst = setupInstrument();
    InstrumentRayTracer tracker(testInst);
    V3D testDir(0.010, 0.0, 15.004);
    testDir.normalize();
    tracker.trace(testDir);
    Links results = tracker.getResults();
    TS_ASSERT_EQUALS(results.size(), 1);

    const IComponent *interceptedPixel =
        testInst->getComponentByName("pixel-(1;0)").get();

    Link intersect = results.front();
    TS_ASSERT_DELTA(intersect.distFromStart, 15.003468, 1e-6);
    TS_ASSERT_DELTA(intersect.distInsideObject, 0.006931, 1e-6);
    TS_ASSERT_DELTA(intersect.entryPoint.X(), 0.009995, 1e-6);
    TS_ASSERT_DELTA(intersect.entryPoint.Y(), 0.0, 1e-6);
    TS_ASSERT_DELTA(intersect.entryPoint.Z(), 4.996533, 1e-6);
    TS_ASSERT_DELTA(intersect.exitPoint.X(), 0.01, 1e-6);
    TS_ASSERT_DELTA(intersect.exitPoint.Y(), 0.0, 1e-6);
    TS_ASSERT_DELTA(intersect.exitPoint.Z(), 5.003464, 1e-6);
    TS_ASSERT_EQUALS(intersect.componentID, interceptedPixel->getComponentID());

    // Results vector should be empty after first getResults call
    results = tracker.getResults();
    TS_ASSERT_EQUALS(results.size(), 0);

    // Results vector should be empty after first getResults call
    results = tracker.getResults();
    TS_ASSERT_EQUALS(results.size(), 0);
  }

  void test_That_traceFromSample_throws_for_zero_dir() {
    Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentRectangular(1, 100);
    InstrumentRayTracer tracker(inst);
    constexpr V3D testDir(0., 0., 0.);
    TS_ASSERT_THROWS_ANYTHING(tracker.traceFromSample(testDir));
  }

  /** Test ray tracing into a rectangular detector
   *
   * @param inst :: instrument with 1 rect
   * @param testDir :: direction of track
   * @param expectX :: expected x index, -1 if off
   * @param expectY :: expected y index, -1 if off
   */
  void doTestRectangularDetector(std::string message, Instrument_sptr inst,
                                 V3D testDir, int expectX, int expectY) {
    InstrumentRayTracer tracker(inst);
    testDir.normalize();
    tracker.traceFromSample(testDir);

    Links results = tracker.getResults();
    if (expectX == -1) { // Expect no intersection
      TSM_ASSERT_LESS_THAN(message, results.size(), 2);
      return;
    }

    TSM_ASSERT_EQUALS(message, results.size(), 2);
    if (results.size() < 2)
      return;

    // Get the first result
    Link res = *results.begin();
    IDetector_const_sptr det = boost::dynamic_pointer_cast<const IDetector>(
        inst->getComponentByID(res.componentID));
    // Parent bank
    RectangularDetector_const_sptr rect =
        boost::dynamic_pointer_cast<const RectangularDetector>(
            det->getParent()->getParent());
    // Find the xy index from the detector ID
    std::pair<int, int> xy = rect->getXYForDetectorID(det->getID());
    TSM_ASSERT_EQUALS(message, xy.first, expectX);
    TSM_ASSERT_EQUALS(message, xy.second, expectY);
  }

  void test_RectangularDetector() {
    Instrument_sptr inst;
    inst = ComponentCreationHelper::createTestInstrumentRectangular(1, 100);

    // Towards the detector lower-left corner
    double w = 0.008;
    doTestRectangularDetector("Pixel (0,0)", inst, V3D(0.0, 0.0, 5.0), 0, 0);
    // Move over some pixels
    doTestRectangularDetector("Pixel (1,0)", inst, V3D(w * 1, w * 0, 5.0), 1,
                              0);
    doTestRectangularDetector("Pixel (1,2)", inst, V3D(w * 1, w * 2, 5.0), 1,
                              2);
    doTestRectangularDetector("Pixel (0.95, 0.95)", inst,
                              V3D(w * 0.45, w * 0.45, 5.0), 0, 0);
    doTestRectangularDetector("Pixel (1.05, 2.05)", inst,
                              V3D(w * 0.55, w * 1.55, 5.0), 1, 2);
    doTestRectangularDetector("Pixel (99,99)", inst, V3D(w * 99, w * 99, 5.0),
                              99, 99);

    doTestRectangularDetector("Off to left", inst, V3D(-w, 0, 5.0), -1, -1);
    doTestRectangularDetector("Off to bottom", inst, V3D(0, -w, 5.0), -1, -1);
    doTestRectangularDetector("Off to top", inst, V3D(0, w * 100, 5.0), -1, -1);
    doTestRectangularDetector("Off to right", inst, V3D(w * 100, w, 5.0), -1,
                              -1);

    doTestRectangularDetector("Beam parallel to panel", inst,
                              V3D(1.0, 0.0, 0.0), -1, -1);
    doTestRectangularDetector("Beam parallel to panel", inst,
                              V3D(0.0, 1.0, 0.0), -1, -1);
  }

private:
  /// Setup the shared test instrument
  Instrument_sptr setupInstrument() {
    if (!m_testInst) {
      // 9 cylindrical detectors
      m_testInst = ComponentCreationHelper::createTestInstrumentCylindrical(1);
    }
    return m_testInst;
  }

  /// Test instrument
  Instrument_sptr m_testInst;
};

//------------------------------------------------------------------------------------------------------
// PERFORMANCE TEST IS IN DataHandling/test/InstrumentRayTracerTest.h because it
// requires LoadInstrument
//------------------------------------------------------------------------------------------------------

#endif // InstrumentRayTracerTEST_H_
