#ifndef SPECTRAAXISTEST_H_
#define SPECTRAAXISTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidTestHelpers/FakeObjects.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

// Now the unit test class itself
class SpectraAxisTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SpectraAxisTest *createSuite() { return new SpectraAxisTest(); }
  static void destroySuite( SpectraAxisTest *suite ) { delete suite; }

  SpectraAxisTest()
  {
    ws = new WorkspaceTester;
    ws->init(5,1,1);
    spectraAxis = new SpectraAxis(ws);
    spectraAxis->title() = "A spectra axis";
  }
  
  ~SpectraAxisTest()
  {
    delete spectraAxis;
  }
  
  void testConstructor()
  {
    TS_ASSERT_EQUALS( spectraAxis->title(), "A spectra axis" );
    TS_ASSERT( spectraAxis->unit().get() );
    for (int i=0; i<5; ++i)
    {
      TS_ASSERT_EQUALS( (*spectraAxis)(i), static_cast<double>(i+1) );
    }
  }

  void testClone()
  {
    Axis* newSpecAxis = spectraAxis->clone(ws);
    TS_ASSERT_DIFFERS( newSpecAxis, spectraAxis );
    delete newSpecAxis;
  }
  
  void testCloneDifferentLength()
  {
    Axis* newSpecAxis = spectraAxis->clone(2,ws);
    TS_ASSERT_DIFFERS( newSpecAxis, spectraAxis );
    TS_ASSERT( newSpecAxis->isSpectra() );
    TS_ASSERT_EQUALS( newSpecAxis->title(), "A spectra axis" );
    TS_ASSERT_EQUALS( newSpecAxis->unit()->unitID(), "Empty" );
    // Although the 'different length' constructor is still there (for now) it has no effect.
    TS_ASSERT_EQUALS( newSpecAxis->length(), 5 );
    TS_ASSERT_EQUALS( (*newSpecAxis)(1), 2.0 );
    delete newSpecAxis;
  }

  void testTitle()
  {
    spectraAxis->title() = "something";
    TS_ASSERT_EQUALS( spectraAxis->title(), "something" );
  }

  void testUnit()
  {
    spectraAxis->unit() = UnitFactory::Instance().create("TOF");
    TS_ASSERT_EQUALS( spectraAxis->unit()->unitID(), "TOF" );
  }

  void testIsSpectra()
  {
    TS_ASSERT( spectraAxis->isSpectra() );
  }

  void testIsNumeric()
  {
    TS_ASSERT( !spectraAxis->isNumeric() );
  }

  void testIsText()
  {
    TS_ASSERT( !spectraAxis->isText() );
  }

  void testOperatorBrackets()
  {
    TS_ASSERT_THROWS( (*spectraAxis)(-1), Exception::IndexError );
    TS_ASSERT_THROWS( (*spectraAxis)(5), Exception::IndexError );
  }

  void testSetValue()
  {
    TS_ASSERT_THROWS( spectraAxis->setValue(-1, 1.1), Exception::IndexError );
    TS_ASSERT_THROWS( spectraAxis->setValue(5, 1.1), Exception::IndexError );
    
    for (int i=0; i<5; ++i)
    {
      TS_ASSERT_THROWS_NOTHING( spectraAxis->setValue(i, i+0.1) );
      TS_ASSERT_EQUALS( (*spectraAxis)(i), i );
      TS_ASSERT_EQUALS( spectraAxis->spectraNo(i), i );
    }
  }

  void testSpectraNo()
  {
    TS_ASSERT_THROWS( spectraAxis->spectraNo(-1), Exception::IndexError );
    TS_ASSERT_THROWS( spectraAxis->spectraNo(5), Exception::IndexError );
    
    for (int i=0; i<5; ++i)
    {
      TS_ASSERT_THROWS_NOTHING( spectraAxis->setValue(i, 2*i) );
      TS_ASSERT_EQUALS( spectraAxis->spectraNo(i), 2*i );
      TS_ASSERT_EQUALS( (*spectraAxis)(i), 2*i );
    }    
  }

private:
  WorkspaceTester *ws;
  Axis *spectraAxis;
};

#endif /*SPECTRAAXISTEST_H_*/
