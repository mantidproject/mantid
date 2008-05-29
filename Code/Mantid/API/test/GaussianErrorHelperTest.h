#ifndef TESTGAUSSIANERRORHELPER_H_
#define TESTGAUSSIANERRORHELPER_H_

#include <cxxtest/TestSuite.h>
#include <cmath>
#include "MantidAPI/GaussianErrorHelper.h"
#include "MantidAPI/LocatedDataRef.h"
#include "MantidAPI/LocatedDataValue.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

class GaussianErrorHelperTest : public CxxTest::TestSuite
{
public:

  GaussianErrorHelperTest()
  {
    eh = GaussianErrorHelper::Instance();
    values[0] = 1;
    values[1] = 2;
    values[2] = 3;
    values[3] = 4;
    values[4] = 6;
    values[5] = 8;
  }

  void testInstance()
  {
    // Not really much to test
    GaussianErrorHelper *tester = GaussianErrorHelper::Instance();
    TS_ASSERT_EQUALS( eh, tester);
    TS_ASSERT_THROWS_NOTHING( IErrorHelper *tester = GaussianErrorHelper::Instance(); )
    //set lhs and rhs values
    lhs.xPointer=&values[0];
    lhs.yPointer=&values[1];
    lhs.ePointer=&values[2];
    rhs.xPointer=&values[3];
    rhs.yPointer=&values[4];
    rhs.ePointer=&values[5];
  }

  void testPlus()
  {
    eh->plus(lhs,rhs,result);
    TS_ASSERT_DELTA(result.Y(),values[1]+values[4],0.0001);
    TS_ASSERT_DELTA(result.E(),sqrt(pow(values[2],2)+pow(values[5],2)),0.0001);
    TS_ASSERT_EQUALS( result.ErrorHelper(), lhs.ErrorHelper());
  }

  void testMinus()
  {
    eh->minus(lhs,rhs,result);
    TS_ASSERT_DELTA(result.Y(),values[1]-values[4],0.0001);
    TS_ASSERT_DELTA(result.E(),sqrt(pow(values[2],2)+pow(values[5],2)),0.0001);
    TS_ASSERT_EQUALS( result.ErrorHelper(), lhs.ErrorHelper());
  }

  void testMultiply()
  {
    eh->multiply(lhs,rhs,result);
    TS_ASSERT_DELTA(result.Y(),values[1]*values[4],0.0001);
    TS_ASSERT_DELTA(result.E(),values[1]*values[4]*sqrt(pow(values[2]/values[1],2)+pow(values[5]/values[4],2)),0.0001);
    TS_ASSERT_EQUALS( result.ErrorHelper(), lhs.ErrorHelper());
  }

  void testDivision()
  {
    eh->divide(lhs,rhs,result);
    TS_ASSERT_DELTA(result.Y(),values[1]/values[4],0.0001);
    TS_ASSERT_DELTA(result.E(),values[1]/values[4]*sqrt(pow(values[2]/values[1],2)+pow(values[5]/values[4],2)),0.0001);
    TS_ASSERT_EQUALS( result.ErrorHelper(), lhs.ErrorHelper());
  }

private:
  GaussianErrorHelper* eh;
  LocatedDataRef lhs, rhs;
  LocatedDataValue result;
  double values[6];
};

#endif /*TESTGAUSSIANERRORHELPER_H_*/

