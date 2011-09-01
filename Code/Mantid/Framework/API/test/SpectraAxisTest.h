#ifndef SPECTRAAXISTEST_H_
#define SPECTRAAXISTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/Instrument/OneToOneSpectraDetectorMap.h"
#include "MantidAPI/SpectraAxis.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

// Small class for testing the protected copy constructor
class SpectraAxisTester : public SpectraAxis
{
public:
  SpectraAxisTester() : SpectraAxis(1) {}
  SpectraAxisTester(const SpectraAxisTester& right) : SpectraAxis(right) {}
};


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
    spectraAxis = new SpectraAxis(5);
  }
  
  ~SpectraAxisTest()
  {
    delete spectraAxis;
  }
  
  void testConstructorWithLengthAndDefaultInit()
  {
    TS_ASSERT_EQUALS( spectraAxis->title(), "" );
    TS_ASSERT( spectraAxis->unit().get() );
    for (int i=0; i<5; ++i)
    {
      TS_ASSERT_EQUALS( (*spectraAxis)(i), static_cast<double>(i+1) );
    }
  }

  void testConstructorWithLengthAndNoDefaultInit()
  {
    SpectraAxis local(5, false);
    TS_ASSERT_EQUALS( local.title(), "" );
    TS_ASSERT( local.unit().get() );
    for (int i=0; i<5; ++i)
    {
      TS_ASSERT_EQUALS( local(i), 0.0 );
    }
  }

  void testConstructorWithSpectraDetectorMap()
  {
    using Mantid::Geometry::ISpectraDetectorMap;
    using Mantid::Geometry::OneToOneSpectraDetectorMap;
    ISpectraDetectorMap *one2one = new OneToOneSpectraDetectorMap(5,9);
    TS_ASSERT_EQUALS(one2one->nElements(), 5);

    SpectraAxis local(one2one->nElements(),*one2one);
    TS_ASSERT_EQUALS(local.length(), 5);
    TS_ASSERT_EQUALS( local.title(), "" );
    TS_ASSERT( local.unit().get() );
    for (int i=0; i<5; ++i)
    {
      TS_ASSERT_EQUALS( local(i), static_cast<double>(i+5) );
    }
    
    // Now limit the axis length
    SpectraAxis localShort(2,*one2one);
    TS_ASSERT_EQUALS(localShort.length(), 2);
    TS_ASSERT_EQUALS( localShort.title(), "" );
    TS_ASSERT( localShort.unit().get() );
    for (int i=0; i<2; ++i)
    {
      TS_ASSERT_EQUALS( localShort(i), static_cast<double>(i+5) );
    }
    
    delete one2one;
  }

  void testPopulateOneToOne()
  {
    SpectraAxis ax(5);
    ax.populateOneToOne(1,100);
    TS_ASSERT_EQUALS( ax.length(), 100);
    TS_ASSERT_EQUALS( ax.spectraNo(23), 24);
  }

  void testCopyConstructor()
  {
    SpectraAxisTester axistester1;
    axistester1.title() = "tester1";
    axistester1.setValue(0,5);
    
    SpectraAxisTester copiedAxis1 = axistester1;
    TS_ASSERT_EQUALS( copiedAxis1.title(), "tester1" );
    TS_ASSERT( copiedAxis1.isSpectra() );
    TS_ASSERT_EQUALS( copiedAxis1(0), 5 );
    TS_ASSERT_THROWS( copiedAxis1(1), Exception::IndexError );
  }
  
  void testClone()
  {
    Axis* newSpecAxis = spectraAxis->clone();
    TS_ASSERT_DIFFERS( newSpecAxis, spectraAxis );
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
      TS_ASSERT_THROWS_NOTHING( spectraAxis->spectraNo(i) = 2*i );
      TS_ASSERT_EQUALS( spectraAxis->spectraNo(i), 2*i );
      TS_ASSERT_EQUALS( (*spectraAxis)(i), 2*i );
    }    
  }

private:
  Axis *spectraAxis;
};

#endif /*SPECTRAAXISTEST_H_*/
