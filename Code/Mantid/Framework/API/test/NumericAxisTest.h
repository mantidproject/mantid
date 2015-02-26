#ifndef NUMERICAXISTEST_H_
#define NUMERICAXISTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/NumericAxis.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Exception.h"
#include <cfloat>
#include "MantidTestHelpers/FakeObjects.h"

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
    WorkspaceTester ws; // Fake workspace to pass to clone
    Axis* newNumAxis = numericAxis->clone(&ws);
    TS_ASSERT_DIFFERS( newNumAxis, numericAxis );
    delete newNumAxis;
  }

  void testCloneDifferentLength()
  {
    numericAxis->setValue(0,9.9);
    WorkspaceTester ws; // Fake workspace to pass to clone
    Axis* newNumAxis = numericAxis->clone(1,&ws);
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

  void test_indexOfValue_Treats_Axis_Values_As_Bin_Centres()
  {
    double points[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    const size_t npoints(5);
    NumericAxis axis(std::vector<double>(points, points + npoints));

    TS_ASSERT_EQUALS(0, axis.indexOfValue(0.5));
    TS_ASSERT_EQUALS(0, axis.indexOfValue(1.4));
    TS_ASSERT_EQUALS(3, axis.indexOfValue(3.7));
    TS_ASSERT_EQUALS(3, axis.indexOfValue(4.0)); //exact value
    TS_ASSERT_EQUALS(4, axis.indexOfValue(5.4));
  }

  void test_equalWithinTolerance()
  {
    double points1[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double points2[] = {1.0, 2.0, 3.0, 4.0, 5.001};
    const size_t npoints(5);
    NumericAxis axis1(std::vector<double>(points1, points1 + npoints));
    NumericAxis axis2(std::vector<double>(points2, points2 + npoints));

    // Difference (0.001) < tolerance (0.01), should be equal
    TS_ASSERT( axis1.equalWithinTolerance(axis2, 0.01) );

    // Difference (0.001) > tolerance (0.0001), should not be equal
    TS_ASSERT( !axis1.equalWithinTolerance(axis2, 0.0001) );
  }

  //-------------------------------- Failure cases ----------------------------

  void test_indexOfValue_Throws_When_Input_Not_In_Axis_Range()
  {
    const size_t npoints(5);
    NumericAxis axis(npoints);
    for(size_t i = 0; i < npoints; ++i)
    {
      axis.setValue(i, static_cast<double>(i));
    }

    TS_ASSERT_THROWS(axis.indexOfValue(-0.6), std::out_of_range);
    TS_ASSERT_THROWS(axis.indexOfValue(4.6), std::out_of_range);
  }

private:
  Axis *numericAxis;
};

#endif /*NUMERICAXISTEST_H_*/
