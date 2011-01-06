#ifndef INSTRUMENTRAYTRACERTEST_H_
#define INSTRUMENTRAYTRACERTEST_H_

//-------------------------------------------------------------
// Includes
//-------------------------------------------------------------
#include <cxxtest/TestSuite.h>
#include "MantidGeometry/Objects/InstrumentRayTracer.h"
#include "MantidGeometry/V3D.h"
#include "MantidKernel/ConfigService.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <iterator>

using namespace Mantid::Geometry;

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


#endif //InstrumentRayTracerTEST_H_
