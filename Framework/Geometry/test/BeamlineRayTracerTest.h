#ifndef BEAMLINERAYTRACERTEST_H_
#define BEAMLINERAYTRACERTEST_H_

#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/ComponentInfoBankHelpers.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/InstrumentDefinitionParser.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Objects/BeamlineRayTracer.h"
#include "MantidGeometry/Objects/InstrumentRayTracer.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ProgressText.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

#include <boost/make_shared.hpp>
#include <cxxtest/TestSuite.h>

using namespace Mantid::Geometry;
using Mantid::Kernel::ConfigService;
using Mantid::Kernel::ProgressText;
using Mantid::Kernel::V3D;
using namespace Mantid::Kernel::Strings;
using namespace ComponentCreationHelper;
namespace RayTracer = Mantid::Geometry::BeamlineRayTracer;
// Define helper type alias for results of ray trace
using RayTraces = std::vector<Links>;

class BeamlineRayTracerTest : public CxxTest::TestSuite {

private:
  struct RayTraceTestSpec {
    std::string message; // name of the direction being tested
    V3D beamDirection;   // direction to test tracing towards
    double pixelX; // x pixel index within the detector bank, -1 if not using
    double pixelY; // y pixel index within the detector bank, -1 if not using
  };

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BeamlineRayTracerTest *createSuite() {
    return new BeamlineRayTracerTest();
  }
  static void destroySuite(BeamlineRayTracerTest *suite) { delete suite; }

  void
  test_that_a_trace_for_a_ray_that_intersects_many_components_gives_these_components_as_a_result() {
    // Create the test objects to use
    create_instrument_and_componentInfo();

    // Vector for a ray
    V3D testDir(0., 0., 1);

    // Do a trace and store the results
    Links results = RayTracer::traceFromSource(testDir, *m_compInfo);

    // Check size
    // This should be equal to 2 as the ray will first intersect with the sample
    // and then intersect with the detector
    TSM_ASSERT_EQUALS("Ray did not intersect with both sample and detector",
                      results.size(), 2);

    // Check they are actually what we expect: 1 with the sample and 1 with the
    // central detector
    const auto centralPixel = m_compInfo->indexOfAny("pixel-(0;0)");
    const auto sampleComp = m_compInfo->sample();

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

    // First intersection
    Link firstIntersect = *resultItr;

    // Based on our test ray, the first intersection should occur at these
    // distances.
    TS_ASSERT_DELTA(firstIntersect.distFromStart, 10.001, 1e-6);
    TS_ASSERT_DELTA(firstIntersect.distInsideObject, 0.002, 1e-6);

    // These should be the correct entry point coordinates
    TS_ASSERT_DELTA(firstIntersect.entryPoint.X(), 0.0, 1e-6);
    TS_ASSERT_DELTA(firstIntersect.entryPoint.Y(), 0.0, 1e-6);
    TS_ASSERT_DELTA(firstIntersect.entryPoint.Z(), -0.001, 1e-6);

    // These should be the correct exit point coordinates
    TS_ASSERT_DELTA(firstIntersect.exitPoint.X(), 0.0, 1e-6);
    TS_ASSERT_DELTA(firstIntersect.exitPoint.Y(), 0.0, 1e-6);
    TS_ASSERT_DELTA(firstIntersect.exitPoint.Z(), 0.001, 1e-6);

    // Component that should have been intersected
    TS_ASSERT_EQUALS(firstIntersect.componentID,
                     m_compInfo->componentID(sampleComp));

    // Second intersection
    ++resultItr;
    Link secondIntersect = *resultItr;

    // Based on our test ray, the second intersection should occur at these
    // distances.
    TS_ASSERT_DELTA(secondIntersect.distFromStart, 15.004, 1e-6);
    TS_ASSERT_DELTA(secondIntersect.distInsideObject, 0.008, 1e-6);

    // These should be the correct entry point coordinates
    TS_ASSERT_DELTA(secondIntersect.entryPoint.X(), 0.0, 1e-6);
    TS_ASSERT_DELTA(secondIntersect.entryPoint.Y(), 0.0, 1e-6);
    TS_ASSERT_DELTA(secondIntersect.entryPoint.Z(), 4.996, 1e-6);

    // These should be the correct exit point coordinates
    TS_ASSERT_DELTA(secondIntersect.exitPoint.X(), 0.0, 1e-6);
    TS_ASSERT_DELTA(secondIntersect.exitPoint.Y(), 0.0, 1e-6);
    TS_ASSERT_DELTA(secondIntersect.exitPoint.Z(), 5.004, 1e-6);

    // Component that should have been intersected
    TS_ASSERT_EQUALS(m_compInfo->indexOf(secondIntersect.componentID),
                     centralPixel);
  }

  void
  test_that_a_ray_which_just_intersects_one_component_gives_this_component_only() {
    // Create the test objects to use
    create_instrument_and_componentInfo();

    // Test direction, offset in x to avoid intersection with the source
    V3D testDir(0.010, 0.0, 15.004);

    // Do a trace and store the results
    auto results = RayTracer::traceFromSource(testDir, *m_compInfo);

    // Check size
    TS_ASSERT_EQUALS(results.size(), 1);

    // Check we have what we expect
    const auto interceptedPixel = m_compInfo->indexOfAny("pixel-(1;0)");

    // First (and only) intersection
    auto intersect = results.front();

    // Based on our test ray, the second intersection should occur at these
    // distances.
    TS_ASSERT_DELTA(intersect.distFromStart, 15.003468, 1e-6);
    TS_ASSERT_DELTA(intersect.distInsideObject, 0.006931, 1e-6);

    // These should be the correct entry point coordinates
    TS_ASSERT_DELTA(intersect.entryPoint.X(), 0.009995, 1e-6);
    TS_ASSERT_DELTA(intersect.entryPoint.Y(), 0.0, 1e-6);
    TS_ASSERT_DELTA(intersect.entryPoint.Z(), 4.996533, 1e-6);

    // These should be the correct exit point coordinates
    TS_ASSERT_DELTA(intersect.exitPoint.X(), 0.01, 1e-6);
    TS_ASSERT_DELTA(intersect.exitPoint.Y(), 0.0, 1e-6);
    TS_ASSERT_DELTA(intersect.exitPoint.Z(), 5.003464, 1e-6);

    // Component that should have been intersected
    TS_ASSERT_EQUALS(m_compInfo->indexOf(intersect.componentID),
                     interceptedPixel);
  }

  /**
   * Test ray tracing into a outlined detector.
   *
   * @param spec :: Parameters for the ray to test
   */
  void doTestOutlinedDetector(const RayTraceTestSpec &spec) {

    // Force to be unit vector
    auto testDir = spec.beamDirection;
    testDir.normalize();

    // Do a trace and store the results
    auto results = RayTracer::traceFromSample(testDir, *m_compInfoOutlined);

    // Expect no intersection
    if (spec.pixelX == -1) {
      TSM_ASSERT_LESS_THAN(spec.message, results.size(), 2);
      return;
    }

    // Check size is correct, otherwise quit
    TSM_ASSERT_EQUALS(spec.message, results.size(), 2);
    if (results.size() < 2) {
      TS_FAIL("Did not hit a detector when we should have.");
      return;
    }

    // Get the first result
    auto res = *results.begin();

    // Check if what we found was a detector
    const auto detIndex = m_compInfoOutlined->indexOf(res.componentID);
    if (!m_compInfoOutlined->isDetector(detIndex)) {
      TS_FAIL("Expected a detector but found none");
      return;
    }

    const auto &det = m_detInfoOutlined->detector(detIndex);

    // Find the xy index from the detector ID
    TSM_ASSERT_EQUALS(spec.message, det.getPos().X(), spec.pixelX);
    TSM_ASSERT_EQUALS(spec.message, det.getPos().Y(), spec.pixelY);
  }

  /**
   * Test ray tracing into a rectangular detector.
   *
   * @param spec :: Parameters for the ray to test
   */
  void doTestRectangularDetector(const RayTraceTestSpec &params) {

    // Force to be unit vector
    auto testDir = params.beamDirection;
    testDir.normalize();

    // Do a trace and store the results
    auto results = RayTracer::traceFromSample(testDir, *m_compInfoRectangular);

    // Expect no intersection
    if (params.pixelX == -1) {
      TSM_ASSERT_LESS_THAN(params.message, results.size(), 2);
      return;
    }

    // Check size is correct, otherwise quit
    TSM_ASSERT_EQUALS(params.message, results.size(), 2);
    if (results.size() < 2)
      return;

    // Get the first result
    auto res = *results.begin();

    // Get the detector
    const auto detIndex = m_compInfoRectangular->indexOf(res.componentID);
    if (!m_compInfoRectangular->isDetector(detIndex)) {
      TS_FAIL("Expected a detector but found none");
      return;
    }

    const auto pixelIndex =
        ComponentInfoBankHelpers::findRowColIndexForRectangularBank(
            *m_compInfoRectangular, detIndex);

    // Find the xy index from the detector ID
    TSM_ASSERT_EQUALS(params.message, pixelIndex.first, params.pixelX);
    TSM_ASSERT_EQUALS(params.message, pixelIndex.second, params.pixelY);
  }

  void test_RectangularDetector() {
    // Create the test objects to use
    create_rectangular_instrument();

    // width of a detector pixel
    double w = 0.008;

    doTestRectangularDetector([]() {
      RayTraceTestSpec p;
      p.message = "Pixel (0,0)";
      p.beamDirection = V3D(0.0, 0.0, 5.0);
      p.pixelX = 0;
      p.pixelY = 0;
      return p;
    }());

    doTestRectangularDetector([&w]() {
      RayTraceTestSpec p;
      p.message = "Pixel (1,0)";
      p.beamDirection = V3D(w * 1, w * 0, 5.0);
      p.pixelX = 1;
      p.pixelY = 0;
      return p;
    }());

    doTestRectangularDetector([&w]() {
      RayTraceTestSpec p;
      p.message = "Pixel (1,2)";
      p.beamDirection = V3D(w * 1, w * 2, 5.0);
      p.pixelX = 1;
      p.pixelY = 2;
      return p;
    }());

    doTestRectangularDetector([&w]() {
      RayTraceTestSpec p;
      p.message = "Pixel (0.95,0.95)";
      p.beamDirection = V3D(w * 0.45, w * 0.45, 5.0);
      p.pixelX = 0;
      p.pixelY = 0;
      return p;
    }());

    doTestRectangularDetector([&w]() {
      RayTraceTestSpec p;
      p.message = "Pixel (1.05,1.05)";
      p.beamDirection = V3D(w * 0.55, w * 1.55, 5.0);
      p.pixelX = 1;
      p.pixelY = 2;
      return p;
    }());

    doTestRectangularDetector([&w]() {
      RayTraceTestSpec p;
      p.message = "Pixel (99, 99)";
      p.beamDirection = V3D(w * 99, w * 99, 5.0);
      p.pixelX = 99;
      p.pixelY = 99;
      return p;
    }());

    doTestRectangularDetector([&w]() {
      RayTraceTestSpec p;
      p.message = "Off to left";
      p.beamDirection = V3D(-w, 0, 5.0);
      p.pixelX = -1;
      p.pixelY = -1;
      return p;
    }());

    doTestRectangularDetector([&w]() {
      RayTraceTestSpec p;
      p.message = "Off to bottom";
      p.beamDirection = V3D(0, -w, 5.0);
      p.pixelX = -1;
      p.pixelY = -1;
      return p;
    }());

    doTestRectangularDetector([&w]() {
      RayTraceTestSpec p;
      p.message = "Off to top";
      p.beamDirection = V3D(0, w * 100, 5.0);
      p.pixelX = -1;
      p.pixelY = -1;
      return p;
    }());

    doTestRectangularDetector([&w]() {
      RayTraceTestSpec p;
      p.message = "Off to right";
      p.beamDirection = V3D(w * 100, w, 5.0);
      p.pixelX = -1;
      p.pixelY = -1;
      return p;
    }());

    doTestRectangularDetector([]() {
      RayTraceTestSpec p;
      p.message = "Beam parallel to panel";
      p.beamDirection = V3D(1.0, 0.0, 0.0);
      p.pixelX = -1;
      p.pixelY = -1;
      return p;
    }());

    doTestRectangularDetector([]() {
      RayTraceTestSpec p;
      p.message = "Beam parallel to panel";
      p.beamDirection = V3D(0.0, 1.0, 0.0);
      p.pixelX = -1;
      p.pixelY = -1;
      return p;
    }());

    doTestRectangularDetector([]() {
      RayTraceTestSpec p;
      p.message = "Zero-beam";
      p.beamDirection = V3D(0.0, 0.0, 0.0);
      p.pixelX = -1;
      p.pixelY = -1;
      return p;
    }());
  }

  void test_OutlinedDetector() {
    // Create an instrument with some tubes
    create_outlined_instrument();

    const size_t numPixels = 50;

    // Test valid cases of test ray pointing into tube pixels
    const double h = 0.003;
    for (size_t i = 0; i < numPixels; ++i) {
      const auto index = static_cast<double>(i);
      // Iterate over all pixels in each of the three tubes
      doTestOutlinedDetector([&index, &h]() {
        RayTraceTestSpec s;
        s.message = "Tube 1, Pixel " + std::to_string(index);
        s.beamDirection = V3D(0.0, index * h, 1.0);
        s.pixelX = 0;
        s.pixelY = index * h;
        return s;
      }());

      doTestOutlinedDetector([&index, &h]() {
        const double tube2XZ = sin((M_PI / 2.0) * 0.5);
        RayTraceTestSpec s;
        s.message = "Tube 2, Pixel " + std::to_string(index);
        s.beamDirection = V3D(tube2XZ, index * h, tube2XZ);
        s.pixelX = tube2XZ;
        s.pixelY = index * h;
        return s;
      }());

      doTestOutlinedDetector([&index, &h]() {
        RayTraceTestSpec s;
        s.message = "Tube 3, Pixel " + std::to_string(index);
        s.beamDirection = V3D(1.0, index * h, 0.0);
        s.pixelX = 1.0;
        s.pixelY = index * h;
        return s;
      }());
    }

    // Test boundries of tube
    doTestOutlinedDetector([&h]() {
      RayTraceTestSpec s;
      s.message = "Just below tube detector";
      s.beamDirection = V3D(0.0, -h, 1.0);
      s.pixelX = -1;
      s.pixelY = -1;
      return s;
    }());

    GNU_DIAG_OFF("unused-lambda-capture")
    doTestOutlinedDetector([&h, &numPixels]() {
      RayTraceTestSpec s;
      s.message = "Just above tube detector";
      s.beamDirection = V3D(0.0, static_cast<double>(numPixels) * h, 1.0);
      s.pixelX = -1;
      s.pixelY = -1;
      return s;
    }());
    GNU_DIAG_ON("unused-lambda-capture")

    doTestOutlinedDetector([]() {
      RayTraceTestSpec p;
      p.message = "Beam parallel to panel";
      p.beamDirection = V3D(0.0, 1.0, 0.0);
      p.pixelX = -1;
      p.pixelY = -1;
      return p;
    }());

    doTestOutlinedDetector([]() {
      RayTraceTestSpec p;
      p.message = "Zero-beam";
      p.beamDirection = V3D(0.0, 0.0, 0.0);
      p.pixelX = -1;
      p.pixelY = -1;
      return p;
    }());
  }

  void test_rectangular_detector_multiple_rays() {
    using DetectorCoordinates = std::vector<std::pair<size_t, size_t>>;
    create_rectangular_instrument();

    // Towards the detector lower-left corner
    double w = 0.008;
    std::vector<V3D> testDirections = {V3D(0.0, 0.0, 5.0),
                                       V3D(w * 1, w * 0, 5.0),
                                       V3D(w * 1, w * 2, 5.0),
                                       V3D(w * 0.45, w * 0.45, 5.0),
                                       V3D(w * 0.55, w * 1.55, 5.0),
                                       V3D(w * 99, w * 99, 5.0),
                                       V3D(-w, 0, 5.0),
                                       V3D(0, -w, 5.0),
                                       V3D(0, w * 100, 5.0),
                                       V3D(w * 100, w, 5.0),
                                       V3D(1.0, 0.0, 0.0),
                                       V3D(0.0, 1.0, 0.0),
                                       V3D(0.0, 0.0, 0.0)};

    DetectorCoordinates expectedResults = {
        {0, 0},   {1, 0},   {1, 2},   {0, 0},   {1, 2},   {99, 99}, {-1, -1},
        {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1},
    };

    // Ray trace from V3D test directions to Links of intersections
    RayTraces traces;
    traces.reserve(testDirections.size());
    RayTracer::traceFromSample(testDirections.begin(), testDirections.end(),
                               std::back_inserter(traces),
                               *m_compInfoRectangular);

    // Transform from ray tracer Links to a pair of XY detector coordinates
    DetectorCoordinates actualResults;
    actualResults.reserve(traces.size());
    std::transform(traces.begin(), traces.end(),
                   std::back_inserter(actualResults),
                   [this](const auto &trace) {
                     return this->findRectangularDetectorXYFromTrace(trace);
                   });

    TS_ASSERT_EQUALS(expectedResults, actualResults);
  }

private:
  /**
   * Helper methods for tests
   */
  void create_outlined_instrument() {

    m_testInstrumentOutlined =
        ComponentCreationHelper::createInstrumentWithPSDTubes();
    InstrumentVisitor visitor{m_testInstrumentOutlined};

    // Visit everything
    visitor.walkInstrument();

    // Get ComponentInfo and DetectorInfo objects and set them
    auto infos =
        InstrumentVisitor::makeWrappers(*m_testInstrumentOutlined, nullptr);

    // Unpack the pair
    m_compInfoOutlined = std::move(infos.first);
    m_detInfoOutlined = std::move(infos.second);
  }

  // Creates the objects to use in the tests
  void create_instrument_and_componentInfo() {
    // Create 9 cylindrical detectors and set the instrument
    m_testInstrument =
        ComponentCreationHelper::createTestInstrumentCylindrical(1);

    // Create an instrument visitor
    InstrumentVisitor visitor{m_testInstrument};

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
    InstrumentVisitor visitor{m_testInstrumentRectangular};

    // Visit everything
    visitor.walkInstrument();

    // Get ComponentInfo and DetectorInfo objects and set them
    auto infos =
        InstrumentVisitor::makeWrappers(*m_testInstrumentRectangular, nullptr);

    // Unpack the pair
    m_compInfoRectangular = std::move(infos.first);
    m_detInfoRectangular = std::move(infos.second);
  }

  std::pair<size_t, size_t>
  findRectangularDetectorXYFromTrace(const Links &traces) const {
    Link res = *traces.begin();

    if (traces.size() < 2)
      return {-1, -1};

    // Get the detector
    const auto detIndex = m_compInfoRectangular->indexOf(res.componentID);

    if (!m_compInfoRectangular->isDetector(detIndex))
      return {-1, -1};

    return ComponentInfoBankHelpers::findRowColIndexForRectangularBank(
        *m_compInfoRectangular, detIndex);
  }

  /**
   * Member variables
   */
  // Holds the Instrument
  Instrument_sptr m_testInstrument;
  Instrument_sptr m_testInstrumentOutlined;
  Instrument_sptr m_testInstrumentRectangular;

  // Holds the ComponentInfo
  std::unique_ptr<Mantid::Geometry::ComponentInfo> m_compInfo;
  std::unique_ptr<Mantid::Geometry::ComponentInfo> m_compInfoOutlined;
  std::unique_ptr<Mantid::Geometry::ComponentInfo> m_compInfoRectangular;

  // Holds the DetectorInfo
  std::unique_ptr<Mantid::Geometry::DetectorInfo> m_detInfo;
  std::unique_ptr<Mantid::Geometry::DetectorInfo> m_detInfoOutlined;
  std::unique_ptr<Mantid::Geometry::DetectorInfo> m_detInfoRectangular;
};

class BeamlineRayTracerTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BeamlineRayTracerTestPerformance *createSuite() {
    return new BeamlineRayTracerTestPerformance();
  }
  static void destroySuite(BeamlineRayTracerTestPerformance *suite) {
    delete suite;
  }

  BeamlineRayTracerTestPerformance() : m_testDirections(makeTestDirections()) {
    // Instrument
    m_inst = ComponentCreationHelper::createTestInstrumentRectangular(2, 100);

    // Create an instrument visitor
    InstrumentVisitor visitor{m_inst};

    // Visit everything
    visitor.walkInstrument();

    // Get ComponentInfo and DetectorInfo objects and set them
    auto infos = InstrumentVisitor::makeWrappers(*m_inst, nullptr);

    // Unpack the pair
    m_compInfo = std::move(infos.first);
    m_detInfo = std::move(infos.second);

    // Get the instrument
    m_instTopaz = loadInstrumentDefinition("TOPAZ_Definition_2010.xml");

    // Create an instrument visitor
    InstrumentVisitor visitor2{m_instTopaz};

    // Visit everything
    visitor2.walkInstrument();

    // Get ComponentInfo and DetectorInfo objects and set them
    auto infos2 = InstrumentVisitor::makeWrappers(*m_instTopaz, nullptr);

    // Unpack the pair
    m_compInfoTopaz = std::move(infos2.first);
    m_detInfoTopaz = std::move(infos2.second);

    m_instWish = loadInstrumentDefinition("WISH_Definition_10Panels.xml");

    // Create an instrument visitor
    InstrumentVisitor visitor3{m_instWish};

    // Visit everything
    visitor3.walkInstrument();

    // Get ComponentInfo and DetectorInfo objects and set them
    auto infos3 = InstrumentVisitor::makeWrappers(*m_instWish, nullptr);

    // Unpack the pair
    m_compInfoWish = std::move(infos3.first);
    m_detInfoWish = std::move(infos3.second);
  }

  std::vector<V3D> makeTestDirections() {
    std::vector<V3D> testDirections;
    testDirections.reserve((360 / 3) * (180 / 3));

    // Directly in Z+ = towards the detector center
    for (int azimuth = 0; azimuth < 360; azimuth += 3) {
      for (int elev = -89; elev < 89; elev += 3) {
        // Make a vector pointing in every direction
        V3D testDir;
        testDir.spherical(1, double(elev), double(azimuth));
        testDirections.push_back(testDir);
      }
    }
    return testDirections;
  }

  Instrument_sptr loadInstrumentDefinition(const std::string &idfName) {
    // Parse test file
    std::string filename =
        ConfigService::Instance().getInstrumentDirectory() + idfName;
    std::string xmlText = Mantid::Kernel::Strings::loadFile(filename);
    InstrumentDefinitionParser IDP(filename, "UnitTesting", xmlText);

    // Get the instrument
    return IDP.parseXML(nullptr);
  }

  void test_RectangularDetector() {
    RayTraces traces;
    traces.reserve(m_testDirections.size());
    // Directly in Z+ = towards the detector center
    RayTracer::traceFromSample(m_testDirections.begin(), m_testDirections.end(),
                               std::back_inserter(traces), *m_compInfo);
  }

  void test_RectangularDetector_instrument_v1() {
    InstrumentRayTracer tracer(m_inst);
    // Directly in Z+ = towards the detector center
    for (const auto &testDir : m_testDirections) {
      tracer.traceFromSample(testDir);
      Links results = tracer.getResults();
    }
  }

  void test_TOPAZ() {
    RayTraces traces;
    traces.reserve(m_testDirections.size());
    // Directly in Z+ = towards the detector center
    RayTracer::traceFromSample(m_testDirections.begin(), m_testDirections.end(),
                               std::back_inserter(traces), *m_compInfoTopaz);
  }

  void test_TOPAZ_instrument_v1() {
    // Directly in Z+ = towards the detector center
    InstrumentRayTracer tracer(m_instTopaz);

    for (const auto &testDir : m_testDirections) {
      tracer.traceFromSample(testDir);
      Links results = tracer.getResults();
    }
  }

  void test_WISH() {
    RayTraces traces;
    traces.reserve(m_testDirections.size());
    // Directly in Z+ = towards the detector center
    RayTracer::traceFromSample(m_testDirections.begin(), m_testDirections.end(),
                               std::back_inserter(traces), *m_compInfoTopaz);
  }

  void test_WISH_instrument_v1() {
    // Directly in Z+ = towards the detector center
    InstrumentRayTracer tracer(m_instWish);
    for (const auto &testDir : m_testDirections) {
      // Track it
      tracer.traceFromSample(testDir);
      Links results = tracer.getResults();
    }
  }

private:
  // For Rectangular instrument
  Instrument_sptr m_inst;
  std::unique_ptr<Mantid::Geometry::ComponentInfo> m_compInfo;
  std::unique_ptr<Mantid::Geometry::DetectorInfo> m_detInfo;

  // For Topaz
  Instrument_sptr m_instTopaz;
  std::unique_ptr<Mantid::Geometry::ComponentInfo> m_compInfoTopaz;
  std::unique_ptr<Mantid::Geometry::DetectorInfo> m_detInfoTopaz;

  // For Wish
  Instrument_sptr m_instWish;
  std::unique_ptr<Mantid::Geometry::ComponentInfo> m_compInfoWish;
  std::unique_ptr<Mantid::Geometry::DetectorInfo> m_detInfoWish;

  const std::vector<V3D> m_testDirections;
};

#endif /* BEAMLINERAYTRACERTEST_H_ */
