#ifndef RUNTEST_H_
#define RUNTEST_H_

#include "MantidAPI/Run.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/V3D.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid;

// Helper class
namespace
{
  class ConcreteProperty : public Property
  {
  public:
    ConcreteProperty() : Property( "Test", typeid( int ) ) {}
    Property* clone() { return new ConcreteProperty(*this); }
    bool isDefault() const { return true; }
    std::string getDefault() const { return "getDefault() is not implemented in this class"; }
    std::string value() const { return "Nothing"; }
    std::string setValue( const std::string& ) { return ""; }
    Property& operator+=( Property const * ) { return *this; }
  };
}


class RunTest : public CxxTest::TestSuite
{
 
public:
  void testAddGetData()
  {
    Run runInfo;

    Property *p = new ConcreteProperty();
    TS_ASSERT_THROWS_NOTHING( runInfo.addProperty(p) );

    Property *pp = NULL;
    TS_ASSERT_THROWS_NOTHING( pp = runInfo.getProperty("Test") );
    TS_ASSERT_EQUALS( p, pp );
    TS_ASSERT( ! pp->name().compare("Test") );
    TS_ASSERT( dynamic_cast<ConcreteProperty*>(pp) );
    TS_ASSERT_THROWS( pp = runInfo.getProperty("NotThere"), Exception::NotFoundError );

    std::vector< Property* > props = runInfo.getProperties();
    TS_ASSERT( ! props.empty() );
    TS_ASSERT_EQUALS( props.size(), 1 );
    TS_ASSERT( ! props[0]->name().compare("Test") );
    TS_ASSERT( dynamic_cast<ConcreteProperty*>(props[0]) );
  }

  void testRemoveLogData()
  {
    Run runInfo;
    
    Property *p = new ConcreteProperty();
    TS_ASSERT_THROWS_NOTHING( runInfo.addProperty(p) );
    TS_ASSERT_THROWS_NOTHING( runInfo.removeProperty("Test") );
    TS_ASSERT_EQUALS( runInfo.getProperties().size(), 0 );
  }

  void testGetSetProtonCharge()
  {
    Run runInfo;
    TS_ASSERT_THROWS(runInfo.getProtonCharge(), Exception::NotFoundError);
    TS_ASSERT_THROWS_NOTHING( runInfo.setProtonCharge(10.0) );
    TS_ASSERT_EQUALS( runInfo.getProtonCharge(), 10.0 );
  }

  void testCopyAndAssignment()
  {
    Run runInfo;
    runInfo.setProtonCharge(10.0);
    Property *p = new ConcreteProperty();
    TS_ASSERT_THROWS_NOTHING( runInfo.addProperty(p) );
    TS_ASSERT_EQUALS( runInfo.getProperties().size(), 2);
    
    //Copy constructor
    Run runInfo_2(runInfo);
    TS_ASSERT_EQUALS( runInfo_2.getProperties().size(), 2);
    TS_ASSERT_DELTA( runInfo_2.getProtonCharge(), 10.0, 1e-8);
    TS_ASSERT_EQUALS( runInfo_2.getLogData("Test")->value(), "Nothing" );    


    // Now assignment
    runInfo.setProtonCharge(15.0);
    runInfo.removeProperty("Test");
    runInfo_2 = runInfo;
    TS_ASSERT_EQUALS( runInfo_2.getProperties().size(), 1);
    TS_ASSERT_DELTA( runInfo_2.getProtonCharge(), 15.0, 1e-8);
  }

  void testMemory()
  {
    Run runInfo;
    TS_ASSERT_EQUALS( runInfo.getMemorySize(), 0);
    
    Property *p = new ConcreteProperty();
    runInfo.addProperty(p);
    TS_ASSERT_EQUALS( runInfo.getMemorySize(), sizeof(ConcreteProperty) + sizeof( void *));
  }

  void test_getGoniometer()
  {
    Run runInfo;
    TS_ASSERT_THROWS_NOTHING( runInfo.getGoniometer() );
    // No axes by default
    TS_ASSERT_EQUALS( runInfo.getGoniometer().getNumberAxes(), 0 );
    // Now does copy work?
    runInfo.getGoniometer().makeUniversalGoniometer();
    TS_ASSERT_EQUALS( runInfo.getGoniometer().getNumberAxes(), 3 );
    Run runCopy(runInfo);
    TS_ASSERT_EQUALS( runCopy.getGoniometer().getNumberAxes(), 3 );
    runCopy = runInfo;
    TS_ASSERT_EQUALS( runCopy.getGoniometer().getNumberAxes(), 3 );
  }


  void AddTSPEntry(Run & runInfo, std::string name, double val)
  {
    TimeSeriesProperty<double> * tsp;
    tsp = new TimeSeriesProperty<double>(name);
    tsp->addValue("2011-05-24T00:00:00", val);
    runInfo.addProperty(tsp);
  }


  /** Setting up a goniometer and the angles to feed it
   * using sample logs, then getting the right rotation matrix out.
   */
  void test_getGoniometerMatrix()
  {
    Run runInfo;
    AddTSPEntry(runInfo, "phi", 90.0);
    AddTSPEntry(runInfo, "chi", 90.0);
    AddTSPEntry(runInfo, "omega", 90.0);
    runInfo.getGoniometer().makeUniversalGoniometer();
    DblMatrix r = runInfo.getGoniometerMatrix();
    V3D rot = r * V3D(-1,0,0);
    TS_ASSERT_EQUALS( rot, V3D(1, 0, 0));
    rot = r * V3D(0,0,1);
    TS_ASSERT_EQUALS( rot, V3D(0, 1, 0));
  }

  void test_getGoniometerMatrix2()
  {
    Run runInfo;
    AddTSPEntry(runInfo, "phi", 45.0);
    AddTSPEntry(runInfo, "chi", 90.0);
    AddTSPEntry(runInfo, "omega", 0.0);
    runInfo.getGoniometer().makeUniversalGoniometer();
    DblMatrix r = runInfo.getGoniometerMatrix();
    V3D rot = r * V3D(-1,0,0);
    TS_ASSERT_EQUALS( rot, V3D(0, -sqrt(0.5), sqrt(0.5)));
  }

};



#endif
