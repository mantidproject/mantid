#ifndef INSTRUMENTRAYTRACERTEST_H_
#define INSTRUMENTRAYTRACERTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Objects/InstrumentRayTracer.h"
#include "MantidGeometry/V3D.h"
#include "MantidKernel/ConfigService.h"
#include "MantidTestHelpers/AlgorithmHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <iterator>

using namespace Mantid::Geometry;
using Mantid::DataObjects::Workspace2D_sptr;
using Mantid::API::AnalysisDataService;

//-------------------------------------------------------------
// Test suite
//-------------------------------------------------------------
class InstrumentRayTracerTest : public CxxTest::TestSuite
{
public:

  InstrumentRayTracerTest() : m_testInst()
  {
    // Start logging framework
    Mantid::Kernel::ConfigService::Instance();
  }
  
  void test_That_Constructor_Does_Not_Throw_On_Giving_A_Valid_Instrument()
  {
    boost::shared_ptr<Instrument> testInst(new Instrument("empty"));
    ObjComponent *source = new ObjComponent("moderator", NULL);
    testInst->add(source);
    testInst->markAsSource(source);
    InstrumentRayTracer *rayTracker(NULL);
    TS_ASSERT_THROWS_NOTHING(rayTracker = new InstrumentRayTracer(testInst));
    delete rayTracker;
  }
  
  void test_That_Constructor_Throws_Invalid_Argument_On_Giving_A_Null_Instrument()
  {
    InstrumentRayTracer *rayTracker(NULL);
    TS_ASSERT_THROWS(rayTracker = new InstrumentRayTracer(boost::shared_ptr<IInstrument>()), std::invalid_argument);
  }

  void test_That_Constructor_Throws_Invalid_Argument_On_Giving_An_Instrument_With_No_Source()
  {
    IInstrument_sptr testInst(new Instrument("empty"));
    InstrumentRayTracer *rayTracker(NULL);
    TS_ASSERT_THROWS(rayTracker = new InstrumentRayTracer(testInst), std::invalid_argument);
  }

  void test_That_A_Trace_For_A_Ray_That_Intersects_Many_Components_Gives_These_Components_As_A_Result()
  {
    IInstrument_sptr testInst = setupInstrument(); 
    InstrumentRayTracer tracker(testInst);
    tracker.trace(V3D(0.,0.,1));
    Links results = tracker.getResults();
    TS_ASSERT_EQUALS(results.size(), 2);
    // Check they are actually what we expect: 1 with the sample and 1 with the central detector
    IComponent_sptr centralPixel = testInst->getComponentByName("pixel-(0,0)");
    IComponent_sptr sampleComp = testInst->getSample();

    if( !sampleComp )
    {
      TS_FAIL("Test instrument has been changed, the sample has been removed. Ray tracing tests need to be updated.");
      return;
    }
    if( !centralPixel )
    {
      TS_FAIL("Test instrument has been changed, the instrument config has changed. Ray tracing tests need to be updated.");
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
    TS_ASSERT_EQUALS(firstIntersect.componentID, sampleComp->getComponentID ());

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
    TS_ASSERT_EQUALS(secondIntersect.componentID, centralPixel->getComponentID ());

    // Results vector should be empty after first getResults call
    results = tracker.getResults();
    TS_ASSERT_EQUALS(results.size(), 0);

    // Results vector should be empty after first getResults call
    results = tracker.getResults();
    TS_ASSERT_EQUALS(results.size(), 0);
  }

  void test_That_A_Ray_Which_Just_Intersects_One_Component_Gives_This_Component_Only()
  {
    IInstrument_sptr testInst = setupInstrument(); 
    InstrumentRayTracer tracker(testInst);
    V3D testDir(0.010,0.0,15.004);
    tracker.trace(testDir);
    Links results = tracker.getResults();
    TS_ASSERT_EQUALS(results.size(), 1);

    IComponent * interceptedPixel = testInst->getComponentByName("pixel-(1,0)").get();

    Link intersect = results.front();
    TS_ASSERT_DELTA(intersect.distFromStart, 15.003468, 1e-6);
    TS_ASSERT_DELTA(intersect.distInsideObject, 0.006931, 1e-6);
    TS_ASSERT_DELTA(intersect.entryPoint.X(), 0.009995, 1e-6);
    TS_ASSERT_DELTA(intersect.entryPoint.Y(), 0.0, 1e-6);
    TS_ASSERT_DELTA(intersect.entryPoint.Z(), 4.996533, 1e-6);
    TS_ASSERT_DELTA(intersect.exitPoint.X(), 0.01, 1e-6);
    TS_ASSERT_DELTA(intersect.exitPoint.Y(), 0.0, 1e-6);
    TS_ASSERT_DELTA(intersect.exitPoint.Z(), 5.003464, 1e-6);
    TS_ASSERT_EQUALS(intersect.componentID, interceptedPixel->getComponentID ());

    // Results vector should be empty after first getResults call
    results = tracker.getResults();
    TS_ASSERT_EQUALS(results.size(), 0);

    // Results vector should be empty after first getResults call
    results = tracker.getResults();
    TS_ASSERT_EQUALS(results.size(), 0);
  }


  /** Test ray tracing into a rectangular detector
   *
   * @param inst :: instrument with 1 rect
   * @param testDir :: direction of track
   * @param expectX :: expected x index, -1 if off
   * @param expectY :: expected y index, -1 if off
   */
  void doTestRectangularDetector(std::string message, IInstrument_sptr inst, V3D testDir, int expectX, int expectY)
  {
//    std::cout << message << std::endl;
    InstrumentRayTracer tracker(inst);
    testDir.normalize(); // Force to be unit vector
    tracker.traceFromSample(testDir);

    Links results = tracker.getResults();
    if (expectX == -1)
    { // Expect no intersection
      TSM_ASSERT_LESS_THAN(message, results.size(), 2);
      return;
    }

    TSM_ASSERT_EQUALS(message, results.size(), 2);
    if (results.size() < 2)
      return;

    // Get the first result
    Link res = *results.begin();
    IDetector_sptr det = boost::dynamic_pointer_cast<IDetector>( inst->getComponentByID( res.componentID ) );
    // Parent bank
    RectangularDetector_const_sptr rect = boost::dynamic_pointer_cast<const RectangularDetector>( det->getParent()->getParent() );
    // Find the xy index from the detector ID
    std::pair<int,int> xy = rect->getXYForDetectorID( det->getID() );
    TSM_ASSERT_EQUALS( message, xy.first, expectX);
    TSM_ASSERT_EQUALS( message, xy.second, expectY);
  }


  void test_RectangularDetector()
  {
    IInstrument_sptr inst;
    inst = ComponentCreationHelper::createTestInstrumentRectangular(1, 100);

    // Towards the detector lower-left corner
    double w = 0.008;
    doTestRectangularDetector("Pixel (0,0)", inst, V3D(0.0, 0.0, 5.0), 0, 0);
    // Move over some pixels
    doTestRectangularDetector("Pixel (1,0)", inst, V3D(w*1, w*0, 5.0), 1, 0);
    doTestRectangularDetector("Pixel (1,2)", inst, V3D(w*1, w*2, 5.0), 1, 2);
    doTestRectangularDetector("Pixel (0.95, 0.95)", inst, V3D(w*0.45, w*0.45, 5.0), 0, 0);
    doTestRectangularDetector("Pixel (1.05, 2.05)", inst, V3D(w*0.55, w*1.55, 5.0), 1, 2);
    doTestRectangularDetector("Pixel (99,99)", inst, V3D(w*99, w*99, 5.0), 99, 99);

    doTestRectangularDetector("Off to left",   inst, V3D(-w, 0, 5.0), -1, -1);
    doTestRectangularDetector("Off to bottom", inst, V3D(0, -w, 5.0), -1, -1);
    doTestRectangularDetector("Off to top", inst, V3D(0, w*100, 5.0), -1, -1);
    doTestRectangularDetector("Off to right", inst, V3D(w*100, w, 5.0), -1, -1);

    doTestRectangularDetector("Beam parallel to panel", inst, V3D(1.0, 0.0, 0.0), -1, -1);
    doTestRectangularDetector("Beam parallel to panel", inst, V3D(0.0, 1.0, 0.0), -1, -1);
    doTestRectangularDetector("Zero-beam", inst, V3D(0.0, 0.0, 0.0), -1, -1);
  }


  static void showResults(Links & results, IInstrument_sptr inst)
  {
    Links::const_iterator resultItr = results.begin();
    for (; resultItr != results.end(); resultItr++)
    {
      IComponent_sptr component = inst->getComponentByID(resultItr->componentID);
      std::cout << component->getName() << ", ";
    }
    std::cout << "\n";
  }

private:
  /// Setup the shared test instrument
  IInstrument_sptr setupInstrument()
  {
    if( !m_testInst )
    {
      //9 cylindrical detectors
      m_testInst = ComponentCreationHelper::createTestInstrumentCylindrical(1);
    }
    return m_testInst;
  }

private:
  /// Test instrument
  IInstrument_sptr m_testInst;
};










/** Performance test for large rectangular detectors */
class InstrumentRayTracerTestPerformance : public CxxTest::TestSuite
{
public:
  /// Test instrument
  IInstrument_sptr m_inst;
  Workspace2D_sptr topazWS;

  void setUp()
  {
    m_inst = ComponentCreationHelper::createTestInstrumentRectangular(2, 100);

    topazWS = WorkspaceCreationHelper::Create2DWorkspace(1, 2);
    AnalysisDataService::Instance().add("TOPAZ_2010", topazWS);
    // Load a small test file
    AlgorithmHelper::runAlgorithm("LoadInstrument", 4,
        "Filename", "TOPAZ_Definition_2010.xml",
        "Workspace", "TOPAZ_2010");
  }

  void tearDown()
  {
    AnalysisDataService::Instance().remove("TOPAZ_2010");
  }

  void test_RectangularDetector()
  {
    // Directly in Z+ = towards the detector center
    V3D testDir(0.0, 0.0, 1.0);
    for (size_t i=0; i < 100; i++)
    {
      InstrumentRayTracer tracker(m_inst);
      tracker.traceFromSample(testDir);
      Links results = tracker.getResults();
      TS_ASSERT_EQUALS(results.size(), 3);
      //InstrumentRayTracerTest::showResults(results, m_inst);
    }
  }

  void test_TOPAZ()
  {
    bool verbose=false;
    IInstrument_sptr inst = topazWS->getInstrument();
    // Directly in Z+ = towards the detector center
    for (int azimuth=0; azimuth < 360; azimuth += 2)
      for (int elev=-89; elev < 89; elev += 2)
      {
        // Make a vector pointing in every direction
        V3D testDir;
        testDir.spherical(1, double(elev), double(azimuth));
        if (verbose) std::cout << testDir << " : ";
        // Track it
        InstrumentRayTracer tracker(inst);
        tracker.traceFromSample(testDir);
        Links results = tracker.getResults();

        if (verbose)
          InstrumentRayTracerTest::showResults(results, inst);
      }
  }



};


#endif //InstrumentRayTracerTEST_H_
