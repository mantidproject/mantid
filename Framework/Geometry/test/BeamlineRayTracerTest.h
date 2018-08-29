#ifndef BEAMLINERAYTRACERTEST_H_
#define BEAMLINERAYTRACERTEST_H_

#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Objects/BeamlineRayTracer.h"
#include "MantidKernel/ConfigService.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

#include <boost/make_shared.hpp>
#include <cxxtest/TestSuite.h>

using namespace Mantid::Geometry;
using Mantid::Kernel::V3D;
using namespace ComponentCreationHelper;
namespace RayTracer = Mantid::Geometry::BeamlineRayTracer;

class BeamlineRayTracerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BeamlineRayTracerTest *createSuite() {
    return new BeamlineRayTracerTest();
  }
  static void destroySuite(BeamlineRayTracerTest *suite) { delete suite; }

  BeamlineRayTracerTest() {
    // Start logging framework
    Mantid::Kernel::ConfigService::Instance();
  }

  void
  test_that_a_trace_for_a_ray_that_intersects_many_components_gives_these_components_as_a_result() {
    // Create the test objects to use
    create_instrument_and_componentInfo();

    // Vector for a ray
    V3D testDir(0., 0., 1);

    // Do a trace and store the results
    Links results = RayTracer::traceFromSource(testDir, *m_compInfo);

    // Check size
    TS_ASSERT_EQUALS(results.size(), 2);

    // Check they are actually what we expect: 1 with the sample and 1 with the
    // central detector
    IComponent_const_sptr centralPixel =
        m_testInstrument->getComponentByName("pixel-(0;0)");
    IComponent_const_sptr sampleComp = m_testInstrument->getSample();

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

    // Iterate through the results
    Links::const_iterator resultItr = results.begin();
    Link firstIntersect = *resultItr;

    // Checks
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
  }

  void
  test_that_a_ray_which_just_intersects_one_component_gives_this_component_only() {
    // Create the test objects to use
    create_instrument_and_componentInfo();

    // Vector for a ray
    V3D testDir(0.010, 0.0, 15.004);

    // Do a trace and store the results
    Links results = RayTracer::traceFromSource(testDir, *m_compInfo);

    // Check size
    TS_ASSERT_EQUALS(results.size(), 1);

    // Check we have what we expect
    const IComponent *interceptedPixel =
        m_testInstrument->getComponentByName("pixel-(1;0)").get();

    // Checks
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
  }

  /**
   * Test ray tracing into a rectangular detector.
   *
   * @param message :: a string for debug information
   * @param testDir :: direction of track
   * @param expectX :: expected x index, -1 if off
   * @param expectY :: expected y index, -1 if off
   */
  void doTestRectangularDetector(std::string message, V3D testDir, int expectX,
                                 int expectY) {

    // Force to be unit vector
    testDir.normalize();

    // Do a trace and store the results
    Links results = RayTracer::traceFromSample(testDir, *m_compInfoRectangular);

    // Expect no intersection
    if (expectX == -1) {
      TSM_ASSERT_LESS_THAN(message, results.size(), 2);
      return;
    }

    // Check size is correct, otherwise quit
    TSM_ASSERT_EQUALS(message, results.size(), 2);
    if (results.size() < 2)
      return;

    // Get the first result
    Link res = *results.begin();

    // Get the detector
    IDetector_const_sptr det = boost::dynamic_pointer_cast<const IDetector>(
        m_testInstrumentRectangular->getComponentByID(res.componentID));

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
    // Create the test objects to use
    create_rectangular_instrument();

    // Towards the detector lower-left corner
    double w = 0.008;
    doTestRectangularDetector("Pixel (0,0)", V3D(0.0, 0.0, 5.0), 0, 0);

    // Move over some pixels
    doTestRectangularDetector("Pixel (1,0)", V3D(w * 1, w * 0, 5.0), 1, 0);
    doTestRectangularDetector("Pixel (1,2)", V3D(w * 1, w * 2, 5.0), 1, 2);
    doTestRectangularDetector("Pixel (0.95, 0.95)",
                              V3D(w * 0.45, w * 0.45, 5.0), 0, 0);
    doTestRectangularDetector("Pixel (1.05, 2.05)",
                              V3D(w * 0.55, w * 1.55, 5.0), 1, 2);
    doTestRectangularDetector("Pixel (99,99)", V3D(w * 99, w * 99, 5.0), 99,
                              99);
    doTestRectangularDetector("Off to left", V3D(-w, 0, 5.0), -1, -1);
    doTestRectangularDetector("Off to bottom", V3D(0, -w, 5.0), -1, -1);
    doTestRectangularDetector("Off to top", V3D(0, w * 100, 5.0), -1, -1);
    doTestRectangularDetector("Off to right", V3D(w * 100, w, 5.0), -1, -1);
    doTestRectangularDetector("Beam parallel to panel", V3D(1.0, 0.0, 0.0), -1,
                              -1);
    doTestRectangularDetector("Beam parallel to panel", V3D(0.0, 1.0, 0.0), -1,
                              -1);
    doTestRectangularDetector("Zero-beam", V3D(0.0, 0.0, 0.0), -1, -1);
  }

private:
  /**
   * Helper methods for tests
   */

  // Creates the objects to use in the tests
  void create_instrument_and_componentInfo() {
    // Create 9 cylindrical detectors and set the instrument
    m_testInstrument =
        ComponentCreationHelper::createTestInstrumentCylindrical(1);

    // Create an instrument visitor
    InstrumentVisitor visitor = (m_testInstrument);

    // Visit everything
    visitor.walkInstrument();

    // Get ComponentInfo and DetectorInfo objects and set them
    auto infos = InstrumentVisitor::makeWrappers(*m_testInstrument, nullptr);

    // Unpack the pair
    m_compInfo = std::move(infos.first);
    m_detInfo = std::move(infos.second);
  }

  void create_rectangular_instrument() {
    m_testInstrumentRectangular =
        ComponentCreationHelper::createTestInstrumentRectangular(1, 100);

    // Create an instrument visitor
    InstrumentVisitor visitor = (m_testInstrumentRectangular);

    // Visit everything
    visitor.walkInstrument();

    // Get ComponentInfo and DetectorInfo objects and set them
    auto infos =
        InstrumentVisitor::makeWrappers(*m_testInstrumentRectangular, nullptr);

    // Unpack the pair
    m_compInfoRectangular = std::move(infos.first);
    m_detInfoRectangular = std::move(infos.second);
  }

  /**
   * Getters
   */
  Instrument_sptr get_instrument() { return m_testInstrument; }

  Mantid::Geometry::ComponentInfo *get_componentInfo() {
    return m_compInfo.get();
  }

  /**
   * Member variables
   */
  // Holds the Instrument
  Instrument_sptr m_testInstrument;
  Instrument_sptr m_testInstrumentRectangular;

  // Holds the ComponentInfo
  std::unique_ptr<Mantid::Geometry::ComponentInfo> m_compInfo;
  std::unique_ptr<Mantid::Geometry::ComponentInfo> m_compInfoRectangular;

  // Holds the DetectorInfo
  std::unique_ptr<Mantid::Geometry::DetectorInfo> m_detInfo;
  std::unique_ptr<Mantid::Geometry::DetectorInfo> m_detInfoRectangular;
};

#endif /* BEAMLINERAYTRACERTEST_H_ */
