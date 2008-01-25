#ifndef TESTGAUSSIANERRORHELPER_H_
#define TESTGAUSSIANERRORHELPER_H_

#include <cxxtest/TestSuite.h>
#include <cmath>
#include "MantidAPI/GaussianErrorHelper.h"
#include "MantidAPI/TripleRef.h"

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
  }

  void testPlus()
  {
    TripleRef<double> lhs(values[0],values[1],values[2]);
    TripleRef<double> rhs(values[3],values[4],values[5]);
    TripleRef<double> result = eh->plus(lhs,rhs);
    TS_ASSERT_DELTA(result[0],1.0,0.0001);
    TS_ASSERT_DELTA(result[1],values[1]+values[4],0.0001);
    TS_ASSERT_DELTA(result[2],sqrt(pow(values[2],2)+pow(values[5],2)),0.0001);
    TS_ASSERT_EQUALS( result.ErrorHelper(), lhs.ErrorHelper());
    TS_ASSERT_EQUALS( result.Detector(), lhs.Detector());
  }

  void testMinus()
  {
    TripleRef<double> lhs(values[0],values[1],values[2]);
    TripleRef<double> rhs(values[3],values[4],values[5]);
    TripleRef<double> result = eh->minus(lhs,rhs);
    TS_ASSERT_DELTA(result[0],values[0],0.0001);
    TS_ASSERT_DELTA(result[1],values[1]-values[4],0.0001);
    TS_ASSERT_DELTA(result[2],sqrt(pow(values[2],2)+pow(values[5],2)),0.0001);
    TS_ASSERT_EQUALS( result.ErrorHelper(), lhs.ErrorHelper());
    TS_ASSERT_EQUALS( result.Detector(), lhs.Detector());
  }
  
  void testMultiply()
  {
    TripleRef<double> lhs(values[0],values[1],values[2]);
    TripleRef<double> rhs(values[3],values[4],values[5]);
    TripleRef<double> result = eh->multiply(lhs,rhs);
    TS_ASSERT_DELTA(result[0],values[0],0.0001);
    TS_ASSERT_DELTA(result[1],values[1]*values[4],0.0001);
    TS_ASSERT_DELTA(result[2],values[1]*values[4]*sqrt(pow(values[2]/values[1],2)+pow(values[5]/values[4],2)),0.0001);
    TS_ASSERT_EQUALS( result.ErrorHelper(), lhs.ErrorHelper());
    TS_ASSERT_EQUALS( result.Detector(), lhs.Detector());
  }
    
  void testDivision()
  {
    TripleRef<double> lhs(values[0],values[1],values[2]);
    TripleRef<double> rhs(values[3],values[4],values[5]);
    TripleRef<double> result = eh->divide(lhs,rhs);
    TS_ASSERT_DELTA(result[0],values[0],0.0001);
    TS_ASSERT_DELTA(result[1],values[1]/values[4],0.0001);
    TS_ASSERT_DELTA(result[2],values[1]/values[4]*sqrt(pow(values[2]/values[1],2)+pow(values[5]/values[4],2)),0.0001);
    TS_ASSERT_EQUALS( result.ErrorHelper(), lhs.ErrorHelper());
    TS_ASSERT_EQUALS( result.Detector(), lhs.Detector());
  }

private:
  GaussianErrorHelper* eh;
  double values[6];
};

#endif /*TESTGAUSSIANERRORHELPER_H_*/

