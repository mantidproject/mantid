#ifndef AXISTEST_H_
#define AXISTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Exception.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

// Small class for testing the protected copy constructor
class SpectraAxisTester : public SpectraAxis
{
public:
  SpectraAxisTester() : SpectraAxis(1) {}
  SpectraAxisTester(const SpectraAxisTester& right) : SpectraAxis(right) {}
};

class NumericAxisTester : public NumericAxis
{
public:
  NumericAxisTester() : NumericAxis(1) {}
  NumericAxisTester(const NumericAxisTester& right) : NumericAxis(right) {}
};

// Now the unit test class itself
class AxisTest : public CxxTest::TestSuite
{
public:
  AxisTest()
  {
    spectraAxis = new SpectraAxis(5);
    numericAxis = new NumericAxis(5);
  }
  
  ~AxisTest()
  {
    delete spectraAxis, numericAxis;
  }
  
  void testConstructor()
  {
    TS_ASSERT_EQUALS( spectraAxis->title(), "" );
    TS_ASSERT_EQUALS( numericAxis->title(), "" );
    TS_ASSERT( spectraAxis->unit().get() );
    TS_ASSERT( numericAxis->unit().get() );
    for (int i=0; i<5; ++i)
    {
      TS_ASSERT_EQUALS( (*spectraAxis)(i), 0.0 );
      TS_ASSERT_EQUALS( (*numericAxis)(i), 0.0 );
    }
  }

  void testPopulateSimple()
  {
    SpectraAxis ax(5);
    ax.populateSimple(100);
    TS_ASSERT_EQUALS( ax.length(), 100);
    TS_ASSERT_EQUALS( ax.spectraNo(23), 23);
  }

  void testCopyConstructor()
  {
    NumericAxisTester axistester;
    axistester.title() = "tester";
    axistester.unit() = UnitFactory::Instance().create("Wavelength");
    axistester.setValue(0,5.5);
    
    NumericAxisTester copiedAxis = axistester;
    TS_ASSERT_EQUALS( copiedAxis.title(), "tester" );
    TS_ASSERT_EQUALS( copiedAxis.unit()->unitID(), "Wavelength" );
    TS_ASSERT( copiedAxis.isNumeric() );
    TS_ASSERT_EQUALS( copiedAxis(0), 5.5 );
    TS_ASSERT_THROWS( copiedAxis(1), Exception::IndexError );

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
    Axis* newNumAxis = numericAxis->clone();
    TS_ASSERT_DIFFERS( newNumAxis, numericAxis );
    delete newSpecAxis;
    delete newNumAxis;
  }
  
  void testTitle()
  {
    spectraAxis->title() = "something";
    TS_ASSERT_EQUALS( spectraAxis->title(), "something" );
    numericAxis->title() = "something else";
    TS_ASSERT_EQUALS( numericAxis->title(), "something else" );
  }

  void testUnit()
  {
    spectraAxis->unit() = UnitFactory::Instance().create("TOF");
    TS_ASSERT_EQUALS( spectraAxis->unit()->unitID(), "TOF" );
    numericAxis->unit() = UnitFactory::Instance().create("Energy");
    TS_ASSERT_EQUALS( numericAxis->unit()->unitID(), "Energy" );
  }

  void testIsSpectra()
  {
    TS_ASSERT( spectraAxis->isSpectra() );
    TS_ASSERT( ! numericAxis->isSpectra() );
  }

  void testIsNumeric()
  {
    TS_ASSERT( ! spectraAxis->isNumeric() );
    TS_ASSERT( numericAxis->isNumeric() );
  }

  void testOperatorBrackets()
  {
    TS_ASSERT_THROWS( (*spectraAxis)(-1), Exception::IndexError );
    TS_ASSERT_THROWS( (*spectraAxis)(5), Exception::IndexError );
    TS_ASSERT_THROWS( (*numericAxis)(-1), Exception::IndexError );
    TS_ASSERT_THROWS( (*numericAxis)(5), Exception::IndexError );
  }

  void testSetValue()
  {
    TS_ASSERT_THROWS( spectraAxis->setValue(-1, 1.1), Exception::IndexError );
    TS_ASSERT_THROWS( spectraAxis->setValue(5, 1.1), Exception::IndexError );
    TS_ASSERT_THROWS( numericAxis->setValue(-1, 1.1), Exception::IndexError );
    TS_ASSERT_THROWS( numericAxis->setValue(5, 1.1), Exception::IndexError );
    
    for (int i=0; i<5; ++i)
    {
      TS_ASSERT_THROWS_NOTHING( spectraAxis->setValue(i, i+0.1) );
      TS_ASSERT_THROWS_NOTHING( numericAxis->setValue(i, i+0.5) );
      TS_ASSERT_EQUALS( (*spectraAxis)(i), i );
      TS_ASSERT_EQUALS( (*numericAxis)(i), i+0.5 );
      TS_ASSERT_EQUALS( spectraAxis->spectraNo(i), i );
    }
  }

  void testSpectraNo()
  {
    TS_ASSERT_THROWS( spectraAxis->spectraNo(-1), Exception::IndexError );
    TS_ASSERT_THROWS( spectraAxis->spectraNo(5), Exception::IndexError );
    TS_ASSERT_THROWS( numericAxis->spectraNo(-1), std::domain_error );
    TS_ASSERT_THROWS( numericAxis->spectraNo(5), std::domain_error );
    
    NumericAxis nAxis(5);
    for (int i=0; i<5; ++i)
    {
      TS_ASSERT_THROWS_NOTHING( spectraAxis->spectraNo(i) = 2*i );
      TS_ASSERT_THROWS( nAxis.spectraNo(i) = 2*i, std::domain_error);
      TS_ASSERT_EQUALS( spectraAxis->spectraNo(i), 2*i );
      TS_ASSERT_EQUALS( (*spectraAxis)(i), 2*i );
      TS_ASSERT_EQUALS( nAxis(i), 0 );
    }    
  }

private:
  Axis *spectraAxis, *numericAxis;
};

#endif /*AXISTEST_H_*/
