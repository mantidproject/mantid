#ifndef INSTRUMENTRAYTRACER2TEST_H_
#define INSTRUMENTRAYTRACER2TEST_H_

#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Objects/InstrumentRayTracer2.h"
#include "MantidKernel/ConfigService.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"

#include <boost/make_shared.hpp>
#include <cxxtest/TestSuite.h>

using namespace Mantid::Geometry;
using Mantid::Kernel::V3D;
using namespace ComponentCreationHelper;
namespace IRT2 = Mantid::Geometry::InstrumentRayTracer2;
using Links = Track::LType;

class InstrumentRayTracer2Test : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InstrumentRayTracer2Test *createSuite() {
    return new InstrumentRayTracer2Test();
  }
  static void destroySuite(InstrumentRayTracer2Test *suite) { delete suite; }

  InstrumentRayTracer2Test() {
    // Start logging framework
    Mantid::Kernel::ConfigService::Instance();
  }

  void
    test_That_A_Trace_For_A_Ray_That_Intersects_Many_Components_Gives_These_Components_As_A_Result() {
    std::cout << "\n IN TEST 1\n " << std::endl;

    create_instrument_and_componentInfo();

    Links results = IRT2::traceFromSource(V3D(0., 0., 1), *m_compInfo);

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


    /*
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
    //results = tracker.getResults();
    //TS_ASSERT_EQUALS(results.size(), 0);

    // Results vector should be empty after first getResults call
    //results = tracker.getResults();
    //TS_ASSERT_EQUALS(results.size(), 0);
    */
    std::cout << "\n DONE TEST 1\n " << std::endl;
  }



  void
    test_That_A_Ray_Which_Just_Intersects_One_Component_Gives_This_Component_Only() {
    
    std::cout << "\n IN TEST 2 \n" << std::endl;

    create_instrument_and_componentInfo();

    V3D testDir(0.010, 0.0, 15.004);

    Links results = IRT2::traceFromSource(testDir, *m_compInfo);

    TS_ASSERT_EQUALS(results.size(), 1);

    const IComponent *interceptedPixel =
      m_testInstrument->getComponentByName("pixel-(1;0)").get();

    /*
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
    //results = getResults();
    //TS_ASSERT_EQUALS(results.size(), 0);

    // Results vector should be empty after first getResults call
    //results = tracker.getResults();
    //TS_ASSERT_EQUALS(results.size(), 0);
    */
    std::cout << "\n DONE TEST 2 \n" << std::endl;
  }


  /** Test ray tracing into a rectangular detector
  *
  * @param inst :: instrument with 1 rect
  * @param testDir :: direction of track
  * @param expectX :: expected x index, -1 if off
  * @param expectY :: expected y index, -1 if off
  */
  void doTestRectangularDetector(std::string message,
    V3D testDir, int expectX, int expectY) {

    testDir.normalize(); // Force to be unit vector
    Links results = IRT2::traceFromSample(testDir, *m_compInfoRectangular);

    if (expectX == -1) { // Expect no intersection
      TSM_ASSERT_LESS_THAN(message, results.size(), 2);
      return;
    }

    TSM_ASSERT_EQUALS(message, results.size(), 2);
    if (results.size() < 2)
      return;

    // Get the last result
    Link res = *results.begin();
    std::cout << "\n Number of results: " << results.size() << std::endl;
    for (auto r : results) {
      std::cout << "Component ID: " << r.componentID->getFullName() << std::endl;
    }

    IDetector_const_sptr det = boost::dynamic_pointer_cast<const IDetector>(
      m_testInstrumentRectangular->getComponentByID(res.componentID));

    std::cout << "\n COMPONENT FULL NAME \n " << m_testInstrumentRectangular->getComponentByID(res.componentID)->getFullName() << std::endl;

    if (!det) {
      std::cout << "\n DET IS NOT INITIALISED " << std::endl;
      std::cout << "\n COMPONENT FOUND IS: ";
      std::cout << m_testInstrumentRectangular->getComponentByID(res.componentID)->getFullName() << std::endl;
    }

    // Parent bank
    RectangularDetector_const_sptr rect = boost::dynamic_pointer_cast<const RectangularDetector>(
        det->getParent()->getParent());

    // Find the xy index from the detector ID
    std::pair<int, int> xy = rect->getXYForDetectorID(det->getID());
    TSM_ASSERT_EQUALS(message, xy.first, expectX);
    TSM_ASSERT_EQUALS(message, xy.second, expectY);

  }






  void test_RectangularDetector() {
    std::cout << "\n IN TEST 3 \n" << std::endl;

    create_rectangular_instrument();

    // Towards the detector lower-left corner
    double w = 0.008;
    doTestRectangularDetector("Pixel (0,0)", V3D(0.0, 0.0, 5.0), 0, 0);

    //// Move over some pixels
    //doTestRectangularDetector("Pixel (1,0)", V3D(w * 1, w * 0, 5.0), 1,0);
    //doTestRectangularDetector("Pixel (1,2)", V3D(w * 1, w * 2, 5.0), 1,2);
    //doTestRectangularDetector("Pixel (0.95, 0.95)",V3D(w * 0.45, w * 0.45, 5.0), 0, 0);
    //doTestRectangularDetector("Pixel (1.05, 2.05)",V3D(w * 0.55, w * 1.55, 5.0), 1, 2);
    //doTestRectangularDetector("Pixel (99,99)", V3D(w * 99, w * 99, 5.0),99, 99);
    //doTestRectangularDetector("Off to left", V3D(-w, 0, 5.0), -1, -1);
    //doTestRectangularDetector("Off to bottom", V3D(0, -w, 5.0), -1, -1);
    //doTestRectangularDetector("Off to top", V3D(0, w * 100, 5.0), -1, -1);
    //doTestRectangularDetector("Off to right", V3D(w * 100, w, 5.0), -1,-1);
    //doTestRectangularDetector("Beam parallel to panel",V3D(1.0, 0.0, 0.0), -1, -1);
    //doTestRectangularDetector("Beam parallel to panel",V3D(0.0, 1.0, 0.0), -1, -1);
    //doTestRectangularDetector("Zero-beam", V3D(0.0, 0.0, 0.0), -1, -1);

    std::cout << "\n DONE TEST 3 \n" << std::endl;
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
    m_testInstrumentRectangular = ComponentCreationHelper::createTestInstrumentRectangular(1, 100);

    // Create an instrument visitor
    InstrumentVisitor visitor = (m_testInstrumentRectangular);

    // Visit everything
    visitor.walkInstrument();

    // Get ComponentInfo and DetectorInfo objects and set them
    auto infos = InstrumentVisitor::makeWrappers(*m_testInstrumentRectangular, nullptr);

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

#endif /* INSTRUMENTRAYTRACER2TEST_H_ */
