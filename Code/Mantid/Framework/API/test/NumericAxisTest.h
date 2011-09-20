#ifndef NUMERICAXISTEST_H_
#define NUMERICAXISTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/NumericAxis.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Exception.h"
#include <cfloat>

using namespace Mantid::API;
using namespace Mantid::Kernel;

class NumericAxisTester : public NumericAxis
{
public:
  NumericAxisTester() : NumericAxis(1) {}
  NumericAxisTester(const NumericAxisTester& right) : NumericAxis(right) {}
};

// Now the unit test class itself
class NumericAxisTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NumericAxisTest *createSuite() { return new NumericAxisTest(); }
  static void destroySuite( NumericAxisTest *suite ) { delete suite; }

  NumericAxisTest()
  {
    numericAxis = new NumericAxis(5);
    numericAxis->title() = "A numeric axis";
  }
  
  ~NumericAxisTest()
  {
    delete numericAxis;
  }
  
  void testConstructor()
  {
    TS_ASSERT_EQUALS( numericAxis->title(), "A numeric axis" );
    TS_ASSERT( numericAxis->unit().get() );
    for (int i=0; i<5; ++i)
    {
      TS_ASSERT_EQUALS( (*numericAxis)(i), 0.0 );
    }
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
  }
  
  void testClone()
  {
    Axis* newNumAxis = numericAxis->clone();
    TS_ASSERT_DIFFERS( newNumAxis, numericAxis );
    delete newNumAxis;
  }
  
  void testCloneDifferentLength()
  {
    numericAxis->setValue(0,9.9);
    Axis* newNumAxis = numericAxis->clone(1);
    TS_ASSERT_DIFFERS( newNumAxis, numericAxis );
    TS_ASSERT( newNumAxis->isNumeric() );
    TS_ASSERT_EQUALS( newNumAxis->title(), "A numeric axis" );
    TS_ASSERT_EQUALS( newNumAxis->unit()->unitID(), "Empty" );
    TS_ASSERT_EQUALS( newNumAxis->length(), 1 );
    TS_ASSERT_EQUALS( (*newNumAxis)(0), 0.0 );
    delete newNumAxis;
  }

  void testTitle()
  {
    numericAxis->title() = "something else";
    TS_ASSERT_EQUALS( numericAxis->title(), "something else" );
  }

  void testUnit()
  {
    numericAxis->unit() = UnitFactory::Instance().create("Energy");
    TS_ASSERT_EQUALS( numericAxis->unit()->unitID(), "Energy" );
  }

  void testIsSpectra()
  {
    TS_ASSERT( ! numericAxis->isSpectra() );
  }

  void testIsNumeric()
  {
    TS_ASSERT( numericAxis->isNumeric() );
  }

  void testIsText()
  {
    TS_ASSERT( !numericAxis->isText() );
  }

  void testOperatorBrackets()
  {
    TS_ASSERT_THROWS( (*numericAxis)(-1), Exception::IndexError );
    TS_ASSERT_THROWS( (*numericAxis)(5), Exception::IndexError );
  }

  void testSetValue()
  {
    TS_ASSERT_THROWS( numericAxis->setValue(-1, 1.1), Exception::IndexError );
    TS_ASSERT_THROWS( numericAxis->setValue(5, 1.1), Exception::IndexError );
    
    for (int i=0; i<5; ++i)
    {
      TS_ASSERT_THROWS_NOTHING( numericAxis->setValue(i, i+0.5) );
      TS_ASSERT_EQUALS( (*numericAxis)(i), i+0.5 );
    }
  }

  void testSpectraNo()
  {
    TS_ASSERT_THROWS( numericAxis->spectraNo(-1), std::domain_error );
    TS_ASSERT_THROWS( numericAxis->spectraNo(5), std::domain_error );
    
    NumericAxis nAxis(5);
    for (int i=0; i<5; ++i)
    {
      TS_ASSERT_THROWS( nAxis.spectraNo(i) = 2*i, std::domain_error);
      TS_ASSERT_EQUALS( nAxis(i), 0 );
    }    
  }

  void testConversionToBins()
  {
    const size_t npoints(5);
    NumericAxis axis(npoints);
    for(size_t i = 0; i < npoints; ++i)
    {
      axis.setValue(i, static_cast<double>(i));
    }
    
    std::vector<double> boundaries = axis.createBinBoundaries();
    const size_t nvalues(boundaries.size());
    TS_ASSERT_EQUALS(nvalues, npoints + 1);
    // Easier to read the expected values with static sized array so
    // live with the hard-coded size
    double expectedValues[6] = {-0.5, 0.5, 1.5, 2.5, 3.5, 4.5};
    for( size_t i = 0; i < nvalues; ++i )
    {
      TS_ASSERT_DELTA(boundaries[i],expectedValues[i], DBL_EPSILON);
    }
  }

private:
  Axis *numericAxis;
};

#endif /*NUMERICAXISTEST_H_*/
